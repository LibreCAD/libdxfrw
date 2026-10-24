#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "drw_acis.h"
#include "drw_datastorage.h"
#include "drw_entities.h"
#include "libdwgr.h"
#include "intern/dwgreader27.h"
#include "intern/dwgutil.h"
#include "intern/proxygraphicdecoder.h"

namespace {

static_assert(DRW_AcisProjectionFailure::None
                  == DRW_AcisProjectionFailure::NoFailure,
              "the legacy ACIS failure spelling keeps its zero value");
static_assert(DRW_ProxyGraphicsOrigin::None
                  == DRW_ProxyGraphicsOrigin::Unspecified,
              "the legacy proxy-origin spelling keeps its zero value");

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

class ProxyGraphicCapture final : public DRW_ProxyGraphicSink {
public:
    bool addArc(const DRW_Arc&) override { return true; }
    bool addCircle(const DRW_Circle& value) override {
        circles.push_back(value);
        return true;
    }
    bool addEllipse(const DRW_Ellipse&) override { return true; }
    bool addLWPolyline(const DRW_LWPolyline&) override { return true; }
    bool addMesh(const DRW_Mesh& value) override {
        meshes.push_back(value);
        return true;
    }
    bool addPolyline(const DRW_Polyline&) override { return true; }
    bool addText(const DRW_Text&) override { return true; }

    std::vector<DRW_Circle> circles;
    std::vector<DRW_Mesh> meshes;
};

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

void appendSabRecordType(std::vector<std::uint8_t>& bytes,
                         const std::string& value,
                         std::int32_t attributePointer = -1,
                         std::int32_t entityId = 0,
                         bool with21800Header = true) {
    appendSabEntityType(bytes, value);
    if (!with21800Header)
        return;
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Pointer));
    appendU32(bytes, static_cast<std::uint32_t>(attributePointer));
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Int));
    appendU32(bytes, static_cast<std::uint32_t>(entityId));
}

void appendSabLocation(std::vector<std::uint8_t>& bytes,
                       double x, double y, double z) {
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::LocationVec));
    appendDouble(bytes, x);
    appendDouble(bytes, y);
    appendDouble(bytes, z);
}

std::vector<std::uint8_t> minimalSab(
    const std::string& signature = "ACIS BinaryFile") {
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

std::vector<std::uint8_t> unprojectedRecordsSab(std::uint32_t recordCount) {
    const std::string signature = "ACIS BinaryFile";
    const std::string terminal = "End-of-ASM-data";
    std::vector<std::uint8_t> bytes(signature.begin(), signature.end());
    appendU32(bytes, 21800u);
    appendU32(bytes, recordCount + 1u);
    appendU32(bytes, recordCount);
    appendU32(bytes, 0u);
    appendSabString(bytes, "LOCAL_PRODUCT");
    appendSabString(bytes, "LOCAL_ACIS");
    appendSabString(bytes, "LOCAL_DATE");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);

    for (std::uint32_t i = 0; i < recordCount; ++i) {
        appendSabRecordType(bytes, "unprojected-local-record");
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    }
    appendSabEntityType(bytes, terminal);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    return bytes;
}

std::vector<std::uint8_t> vertexSab(std::uint32_t version = 1u,
                                    std::uint32_t recordCount = 3u,
                                    std::uint32_t pointIndex = 0u,
                                    double unitsInMm = 1.0,
                                    std::int32_t pointAttribute = -1,
                                    std::int32_t vertexAttribute = -1) {
    const std::string signature = "ACIS BinaryFile";
    const std::string terminal = version >= 21800u
        ? "End-of-ASM-data" : "End-of-ACIS-data";
    std::vector<std::uint8_t> bytes(signature.begin(), signature.end());
    appendU32(bytes, version); // SAB version
    appendU32(bytes, recordCount); // point, vertex, and terminal records
    appendU32(bytes, 2); // entity count
    appendU32(bytes, 0); // flags
    appendSabString(bytes, "LOCAL_PRODUCT");
    appendSabString(bytes, "LOCAL_ACIS");
    appendSabString(bytes, "LOCAL_DATE");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, unitsInMm);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);
    appendSabRecordType(bytes, "point", pointAttribute, 101,
                        version == 21800u);
    appendSabLocation(bytes, 1.0, 2.0, 3.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    appendSabRecordType(bytes, "vertex", vertexAttribute, 202,
                        version == 21800u);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Pointer));
    appendU32(bytes, pointIndex);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    appendSabEntityType(bytes, terminal);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    return bytes;
}

std::vector<std::uint8_t> rootedVertexBodySab(
    std::uint32_t bodyCount = 1u, bool shareFirstPointWithPriorBodies = false) {
    const std::string signature = "ACIS BinaryFile";
    const std::string terminal = "End-of-ASM-data";
    const std::uint32_t recordCount = bodyCount * 14u + 1u;
    std::vector<std::uint8_t> bytes(signature.begin(), signature.end());
    appendU32(bytes, 21800u);
    appendU32(bytes, recordCount);
    appendU32(bytes, recordCount - 1u);
    appendU32(bytes, 0u);
    appendSabString(bytes, "LOCAL_PRODUCT");
    appendSabString(bytes, "LOCAL_ACIS");
    appendSabString(bytes, "LOCAL_DATE");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);

    std::int32_t entityId = 1;
    for (std::uint32_t bodyIndex = 0; bodyIndex < bodyCount; ++bodyIndex) {
        const std::int32_t recordBase =
            static_cast<std::int32_t>(bodyIndex * 14u);
        const double xOffset = static_cast<double>(bodyIndex) * 100.0;
        const auto startRecord = [&](const std::string& type,
                                     std::int32_t attributePointer = -1) {
            appendSabRecordType(bytes, type, attributePointer, entityId++);
        };
        const auto pointer = [&](std::int32_t value) {
            bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Pointer));
            appendU32(bytes, static_cast<std::uint32_t>(
                value < 0 ? value : value + recordBase));
        };
        const auto endRecord = [&]() {
            bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
        };

        // Each graph is a disjoint 14-record body/lump/shell/face/loop/
        // coedge/edge/vertex/point chain; pointer ordinals mirror the pinned
        // 21800 profile while record targets are local to that body.
        startRecord("body", 77);
        pointer(-1); pointer(1); pointer(-1); pointer(13); endRecord();
        startRecord("lump");
        pointer(-1); pointer(-1); pointer(2); pointer(0); endRecord();
        startRecord("shell");
        pointer(-1); pointer(-1); pointer(-1); pointer(3); pointer(-1);
        pointer(1); endRecord();
        startRecord("face");
        pointer(-1); pointer(-1); pointer(4); pointer(2); pointer(-1);
        pointer(5); endRecord();
        startRecord("loop");
        pointer(-1); pointer(-1); pointer(6); pointer(3); endRecord();
        startRecord("plane-surface");
        pointer(-1);
        appendSabLocation(bytes, xOffset, 0.0, 0.0);
        appendSabLocation(bytes, 0.0, 0.0, 1.0);
        appendSabLocation(bytes, 1.0, 0.0, 0.0);
        endRecord();
        startRecord("coedge");
        pointer(-1); pointer(6); pointer(6); pointer(-1); pointer(7);
        pointer(4); pointer(-1); endRecord();
        startRecord("edge");
        pointer(-1); pointer(8); pointer(9); pointer(6); pointer(10);
        endRecord();
        startRecord("vertex");
        pointer(-1); pointer(7);
        if (shareFirstPointWithPriorBodies && bodyIndex != 0u) {
            bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Pointer));
            appendU32(bytes, 11u); // first body's first point record
        } else {
            pointer(11);
        }
        endRecord();
        startRecord("vertex");
        pointer(-1); pointer(7); pointer(12); endRecord();
        startRecord("straight-curve");
        pointer(-1);
        appendSabLocation(bytes, 10.0 + xOffset, 20.0, 30.0);
        appendSabLocation(bytes, 2.0, 0.0, 0.0);
        endRecord();
        startRecord("point");
        pointer(-1);
        appendSabLocation(bytes, 1.0 + xOffset, 2.0, 3.0); endRecord();
        startRecord("point");
        pointer(-1);
        appendSabLocation(bytes, 4.0 + xOffset, 5.0, 6.0); endRecord();
        startRecord("transform");
        const std::string transform = bodyIndex == 0u
            ? "1 0 0 0 1 0 0 0 1 0 0 0"
            : "1 0 0 0 1 0 0 0 1 100 0 0";
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::LiteralStr));
        appendU32(bytes, static_cast<std::uint32_t>(transform.size()));
        bytes.insert(bytes.end(), transform.begin(), transform.end());
        endRecord();
    }
    appendSabEntityType(bytes, terminal);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    return bytes;
}

std::vector<std::uint8_t> sab21800FromGraph(const DRW_SabData& graph) {
    if (graph.header.signature != "ACIS BinaryFile"
        || graph.header.version != 21800
        || graph.records.size()
            >= static_cast<std::size_t>(
                std::numeric_limits<std::int32_t>::max())) {
        return {};
    }

    const std::uint32_t recordCount = static_cast<std::uint32_t>(
        graph.records.size() + 1u);
    std::vector<std::uint8_t> bytes(graph.header.signature.begin(),
                                    graph.header.signature.end());
    appendU32(bytes, 21800u);
    appendU32(bytes, recordCount);
    appendU32(bytes, static_cast<std::uint32_t>(graph.records.size()));
    appendU32(bytes, 0u);
    appendSabString(bytes, "LOCAL_PRODUCT");
    appendSabString(bytes, "LOCAL_ACIS");
    appendSabString(bytes, "LOCAL_DATE");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, graph.header.unitsInMm);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, graph.header.resTol);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, graph.header.norTol);

    const auto appendInt32 = [&bytes](long long value) {
        if (value < std::numeric_limits<std::int32_t>::min()
            || value > std::numeric_limits<std::int32_t>::max()) {
            return false;
        }
        appendU32(bytes, static_cast<std::uint32_t>(
            static_cast<std::int32_t>(value)));
        return true;
    };
    for (std::size_t recordIndex = 0;
         recordIndex < graph.records.size(); ++recordIndex) {
        const DRW_SabRecord& record = graph.records[recordIndex];
        if (record.tokens.empty()
            || record.tokens.front().tag != DRW_SabTag::EntityType
            || record.tokens.front().sval != record.type
            || record.type.size() > 255u) {
            return {};
        }
        appendSabRecordType(bytes, record.type, -1,
                            static_cast<std::int32_t>(recordIndex + 1u));
        for (std::size_t tokenIndex = 1u;
             tokenIndex < record.tokens.size(); ++tokenIndex) {
            const DRW_SabToken& token = record.tokens[tokenIndex];
            switch (token.tag) {
            case DRW_SabTag::Pointer:
            case DRW_SabTag::Int:
            case DRW_SabTag::Enum:
                bytes.push_back(static_cast<std::uint8_t>(token.tag));
                if (!appendInt32(token.ival))
                    return {};
                break;
            case DRW_SabTag::Double:
            case DRW_SabTag::Unknown0x17:
                bytes.push_back(static_cast<std::uint8_t>(token.tag));
                appendDouble(bytes, token.dval);
                break;
            case DRW_SabTag::Str:
                if (token.sval.size() > 255u)
                    return {};
                bytes.push_back(static_cast<std::uint8_t>(token.tag));
                bytes.push_back(static_cast<std::uint8_t>(token.sval.size()));
                bytes.insert(bytes.end(), token.sval.begin(), token.sval.end());
                break;
            case DRW_SabTag::EntityType:
                if (token.sval.size() > 255u)
                    return {};
                bytes.push_back(static_cast<std::uint8_t>(token.tag));
                bytes.push_back(static_cast<std::uint8_t>(token.sval.size()));
                bytes.insert(bytes.end(), token.sval.begin(), token.sval.end());
                break;
            case DRW_SabTag::LiteralStr:
                if (token.sval.size()
                    > static_cast<std::size_t>(
                        std::numeric_limits<std::uint32_t>::max())) {
                    return {};
                }
                bytes.push_back(static_cast<std::uint8_t>(token.tag));
                appendU32(bytes, static_cast<std::uint32_t>(token.sval.size()));
                bytes.insert(bytes.end(), token.sval.begin(), token.sval.end());
                break;
            case DRW_SabTag::LocationVec:
            case DRW_SabTag::DirectionVec:
                bytes.push_back(static_cast<std::uint8_t>(token.tag));
                appendDouble(bytes, token.vec.x);
                appendDouble(bytes, token.vec.y);
                appendDouble(bytes, token.vec.z);
                break;
            case DRW_SabTag::BoolTrue:
            case DRW_SabTag::BoolFalse:
            case DRW_SabTag::SubtypeStart:
            case DRW_SabTag::SubtypeEnd:
                bytes.push_back(static_cast<std::uint8_t>(token.tag));
                break;
            default:
                return {};
            }
        }
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    }

    appendSabEntityType(bytes, "End-of-ASM-data");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    return bytes;
}

std::vector<std::uint8_t> shiftSabBits(const std::vector<std::uint8_t>& bytes,
                                       std::uint8_t shift) {
    std::vector<std::uint8_t> shifted(bytes.size() + 1u, 0u);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        shifted[i] |= static_cast<std::uint8_t>(bytes[i] >> shift);
        shifted[i + 1u] |= static_cast<std::uint8_t>(
            bytes[i] << (8u - shift));
    }
    return shifted;
}

std::vector<std::uint8_t> extendedEntityTypeSab() {
    std::vector<std::uint8_t> bytes = vertexSab(21800u, 3u);
    const std::array<std::uint8_t, 7> original {{
        static_cast<std::uint8_t>(DRW_SabTag::EntityType), 5u,
        'p', 'o', 'i', 'n', 't'
    }};
    const auto found = std::search(bytes.begin(), bytes.end(),
                                   original.begin(), original.end());
    if (found == bytes.end())
        return {};
    const std::vector<std::uint8_t> replacement {
        static_cast<std::uint8_t>(DRW_SabTag::EntityTypeEx), 3u,
        'p', 'o', 'i', static_cast<std::uint8_t>(DRW_SabTag::EntityType),
        2u, 'n', 't'
    };
    const auto after = bytes.erase(found, found + original.size());
    bytes.insert(after, replacement.begin(), replacement.end());
    return bytes;
}

std::vector<std::uint8_t> repeatedLoopCycleSab(
    std::uint32_t faceCount, std::uint32_t loopCount = 1u,
    std::uint32_t coedgeCount = 1u) {
    const std::string signature = "ACIS BinaryFile";
    const std::string terminal = "End-of-ASM-data";
    const std::uint32_t loopStart = faceCount;
    const std::uint32_t coedgeStart = loopStart + loopCount;
    const std::uint32_t recordCount =
        faceCount + loopCount + coedgeCount + 1u;
    std::vector<std::uint8_t> bytes(signature.begin(), signature.end());
    appendU32(bytes, 21800u);
    appendU32(bytes, recordCount);
    appendU32(bytes, faceCount + loopCount + coedgeCount);
    appendU32(bytes, 0u);
    appendSabString(bytes, "LOCAL_PRODUCT");
    appendSabString(bytes, "LOCAL_ACIS");
    appendSabString(bytes, "LOCAL_DATE");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);

    for (std::uint32_t i = 0; i < faceCount; ++i) {
        appendSabRecordType(bytes, "face");
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Pointer));
        appendU32(bytes, loopStart);
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    }
    for (std::uint32_t i = 0; i < loopCount; ++i) {
        appendSabRecordType(bytes, "loop");
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Pointer));
        appendU32(bytes, coedgeStart);
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Pointer));
        appendU32(bytes, loopStart + ((i + 1u) % loopCount));
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    }
    for (std::uint32_t i = 0; i < coedgeCount; ++i) {
        appendSabRecordType(bytes, "coedge");
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Pointer));
        appendU32(bytes, coedgeStart + ((i + 1u) % coedgeCount));
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    }

    appendSabEntityType(bytes, terminal);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    return bytes;
}

std::vector<std::uint8_t> sharedVertexEdgesSab(std::uint32_t edgeCount) {
    const std::string signature = "ACIS BinaryFile";
    const std::string terminal = "End-of-ASM-data";
    const std::uint32_t recordCount = edgeCount + 3u;
    std::vector<std::uint8_t> bytes(signature.begin(), signature.end());
    appendU32(bytes, 21800u);
    appendU32(bytes, recordCount);
    appendU32(bytes, edgeCount + 2u);
    appendU32(bytes, 0u);
    appendSabString(bytes, "LOCAL_PRODUCT");
    appendSabString(bytes, "LOCAL_ACIS");
    appendSabString(bytes, "LOCAL_DATE");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);

    appendSabRecordType(bytes, "point");
    appendSabLocation(bytes, 1.0, 2.0, 3.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));

    appendSabRecordType(bytes, "vertex");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Pointer));
    appendU32(bytes, 0u);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));

    for (std::uint32_t i = 0; i < edgeCount; ++i) {
        appendSabRecordType(bytes, "edge");
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Pointer));
        appendU32(bytes, 1u);
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Pointer));
        appendU32(bytes, 1u);
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    }

    appendSabEntityType(bytes, terminal);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    return bytes;
}

std::vector<std::uint8_t> sharedSurfaceFacesSab(std::uint32_t faceCount) {
    const std::string signature = "ACIS BinaryFile";
    const std::string terminal = "End-of-ASM-data";
    const std::uint32_t recordCount = faceCount + 2u;
    std::vector<std::uint8_t> bytes(signature.begin(), signature.end());
    appendU32(bytes, 21800u);
    appendU32(bytes, recordCount);
    appendU32(bytes, faceCount + 1u);
    appendU32(bytes, 0u);
    appendSabString(bytes, "LOCAL_PRODUCT");
    appendSabString(bytes, "LOCAL_ACIS");
    appendSabString(bytes, "LOCAL_DATE");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);

    appendSabRecordType(bytes, "plane-surface");
    appendSabLocation(bytes, 1.0, 2.0, 3.0);
    appendSabLocation(bytes, 0.0, 0.0, 1.0);
    appendSabLocation(bytes, 1.0, 0.0, 0.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));

    for (std::uint32_t i = 0; i < faceCount; ++i) {
        appendSabRecordType(bytes, "face");
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Pointer));
        appendU32(bytes, 0u);
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    }

    appendSabEntityType(bytes, terminal);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    return bytes;
}

std::vector<std::uint8_t> orderedFaceLinksSab(std::uint32_t faceCount) {
    const std::string signature = "ACIS BinaryFile";
    const std::string terminal = "End-of-ASM-data";
    const std::uint32_t firstLoop = 2u;
    const std::uint32_t secondLoop = 3u;
    const std::uint32_t firstFiller = 4u;
    const std::uint32_t secondFiller = 5u;
    const std::uint32_t recordCount = faceCount + 7u;
    std::vector<std::uint8_t> bytes(signature.begin(), signature.end());
    appendU32(bytes, 21800u);
    appendU32(bytes, recordCount);
    appendU32(bytes, faceCount + 6u);
    appendU32(bytes, 0u);
    appendSabString(bytes, "LOCAL_PRODUCT");
    appendSabString(bytes, "LOCAL_ACIS");
    appendSabString(bytes, "LOCAL_DATE");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);

    appendSabRecordType(bytes, "plane-surface");
    appendSabLocation(bytes, 1.0, 2.0, 3.0);
    appendSabLocation(bytes, 0.0, 0.0, 1.0);
    appendSabLocation(bytes, 1.0, 0.0, 0.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));

    appendSabRecordType(bytes, "plane-surface");
    appendSabLocation(bytes, 9.0, 8.0, 7.0);
    appendSabLocation(bytes, 0.0, 1.0, 0.0);
    appendSabLocation(bytes, 1.0, 0.0, 0.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));

    appendSabRecordType(bytes, "loop");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    appendSabRecordType(bytes, "loop");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    appendSabRecordType(bytes, "unprojected-reference");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    appendSabRecordType(bytes, "unprojected-reference");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));

    for (std::uint32_t i = 0; i < faceCount; ++i) {
        appendSabRecordType(bytes, "face");
        for (std::uint32_t ref : {firstFiller, firstLoop, secondLoop,
                                  secondFiller, 0u, 1u}) {
            bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Pointer));
            appendU32(bytes, ref);
        }
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    }

    appendSabEntityType(bytes, terminal);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    return bytes;
}

std::vector<std::uint8_t> sharedCurveEdgesSab(std::uint32_t edgeCount) {
    const std::string signature = "ACIS BinaryFile";
    const std::string terminal = "End-of-ASM-data";
    const std::uint32_t recordCount = edgeCount + 2u;
    std::vector<std::uint8_t> bytes(signature.begin(), signature.end());
    appendU32(bytes, 21800u);
    appendU32(bytes, recordCount);
    appendU32(bytes, edgeCount + 1u);
    appendU32(bytes, 0u);
    appendSabString(bytes, "LOCAL_PRODUCT");
    appendSabString(bytes, "LOCAL_ACIS");
    appendSabString(bytes, "LOCAL_DATE");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);

    appendSabRecordType(bytes, "straight-curve");
    appendSabLocation(bytes, 10.0, 20.0, 30.0);
    appendSabLocation(bytes, 1.0, 0.0, 0.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));

    for (std::uint32_t i = 0; i < edgeCount; ++i) {
        appendSabRecordType(bytes, "edge");
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Pointer));
        appendU32(bytes, 0u);
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    }

    appendSabEntityType(bytes, terminal);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    return bytes;
}

std::vector<std::uint8_t> sharedEndpointCurveEdgesSab(
    std::uint32_t edgeCount) {
    const std::string signature = "ACIS BinaryFile";
    const std::string terminal = "End-of-ASM-data";
    const std::uint32_t recordCount = edgeCount + 6u;
    const std::uint32_t startVertex = 2u;
    const std::uint32_t endVertex = 3u;
    const std::uint32_t curveIndex = 4u;
    const auto appendPointer = [](std::vector<std::uint8_t>& target,
                                  std::uint32_t index) {
        target.push_back(static_cast<std::uint8_t>(DRW_SabTag::Pointer));
        appendU32(target, index);
    };
    const auto appendNullPointer = [&](std::vector<std::uint8_t>& target) {
        appendPointer(target, std::numeric_limits<std::uint32_t>::max());
    };

    std::vector<std::uint8_t> bytes(signature.begin(), signature.end());
    appendU32(bytes, 21800u);
    appendU32(bytes, recordCount);
    appendU32(bytes, edgeCount + 5u);
    appendU32(bytes, 0u);
    appendSabString(bytes, "LOCAL_PRODUCT");
    appendSabString(bytes, "LOCAL_ACIS");
    appendSabString(bytes, "LOCAL_DATE");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);

    appendSabRecordType(bytes, "point");
    appendSabLocation(bytes, 1.0, 2.0, 3.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    appendSabRecordType(bytes, "point");
    appendSabLocation(bytes, 4.0, 5.0, 6.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));

    appendSabRecordType(bytes, "vertex");
    appendPointer(bytes, 0u);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    appendSabRecordType(bytes, "vertex");
    appendPointer(bytes, 1u);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));

    appendSabRecordType(bytes, "straight-curve");
    appendSabLocation(bytes, 10.0, 20.0, 30.0);
    appendSabLocation(bytes, 1.0, 0.0, 0.0);
    appendNullPointer(bytes);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));

    for (std::uint32_t i = 0; i < edgeCount; ++i) {
        appendSabRecordType(bytes, "edge");
        appendNullPointer(bytes);
        appendPointer(bytes, startVertex);
        appendPointer(bytes, endVertex);
        appendNullPointer(bytes);
        appendPointer(bytes, curveIndex);
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    }

    appendSabEntityType(bytes, terminal);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    return bytes;
}

std::vector<std::uint8_t> sharedIntcurveEdgesSab(std::uint32_t edgeCount) {
    const std::string signature = "ACIS BinaryFile";
    const std::string terminal = "End-of-ASM-data";
    const std::uint32_t recordCount = edgeCount + 2u;
    std::vector<std::uint8_t> bytes(signature.begin(), signature.end());
    appendU32(bytes, 21800u);
    appendU32(bytes, recordCount);
    appendU32(bytes, edgeCount + 1u);
    appendU32(bytes, 0u);
    appendSabString(bytes, "LOCAL_PRODUCT");
    appendSabString(bytes, "LOCAL_ACIS");
    appendSabString(bytes, "LOCAL_DATE");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
    appendDouble(bytes, 1.0e-6);

    appendSabRecordType(bytes, "intcurve-curve");
    appendSabEntityType(bytes, "nubs");
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Int));
    appendU32(bytes, 16u);
    for (std::uint32_t i = 0; i < 16u; ++i) {
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
        appendDouble(bytes, static_cast<double>(i) / 15.0);
    }
    for (double coordinate : {7.0, 8.0, 9.0, 10.0, 11.0, 12.0}) {
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Double));
        appendDouble(bytes, coordinate);
    }
    bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));

    for (std::uint32_t i = 0; i < edgeCount; ++i) {
        appendSabRecordType(bytes, "edge");
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::Pointer));
        appendU32(bytes, 0u);
        bytes.push_back(static_cast<std::uint8_t>(DRW_SabTag::RecordEnd));
    }

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

DRW_SabToken acisLiteral(const std::string& value) {
    DRW_SabToken token;
    token.tag = DRW_SabTag::LiteralStr;
    token.sval = value;
    return token;
}

DRW_SabToken acisBoolean(bool value) {
    DRW_SabToken token;
    token.tag = value ? DRW_SabTag::BoolTrue : DRW_SabTag::BoolFalse;
    token.bval = value;
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

struct AcisGraphFixture {
    DRW_SabData data;

    explicit AcisGraphFixture(int version = 21800) {
        data.header.signature = "ACIS BinaryFile";
        data.header.version = version;
        data.header.unitsInMm = 25.4;
    }

    int addNode(const std::string& type, std::size_t pointerCount,
                std::vector<DRW_SabToken> payload = {}) {
        std::vector<DRW_SabToken> tokens;
        tokens.reserve(pointerCount + payload.size());
        for (std::size_t ordinal = 0; ordinal < pointerCount; ++ordinal)
            tokens.push_back(acisPointer(-1));
        tokens.insert(tokens.end(), payload.begin(), payload.end());
        data.records.push_back(acisRecord(type, std::move(tokens)));
        return static_cast<int>(data.records.size() - 1u);
    }

    void link(int source, std::size_t ordinal, int target) {
        std::vector<DRW_SabToken>& tokens =
            data.records[static_cast<std::size_t>(source)].tokens;
        std::size_t pointerOrdinal = 0u;
        for (DRW_SabToken& token : tokens) {
            if (token.tag != DRW_SabTag::Pointer)
                continue;
            if (pointerOrdinal++ == ordinal) {
                token.ival = target;
                return;
            }
        }
        throw std::out_of_range("ACIS fixture pointer ordinal out of range");
    }

    void setBooleanFieldsAfterPointerCount(
        int source, std::size_t pointerCount,
        const std::vector<bool>& values) {
        std::vector<DRW_SabToken>& tokens =
            data.records[static_cast<std::size_t>(source)].tokens;
        tokens.erase(std::remove_if(tokens.begin(), tokens.end(),
                                    [](const DRW_SabToken& token) {
                                        return token.tag == DRW_SabTag::BoolTrue
                                            || token.tag == DRW_SabTag::BoolFalse;
                                    }),
                     tokens.end());
        std::size_t pointerOrdinal = 0u;
        std::size_t insertionIndex = tokens.size();
        bool foundPointerPrefix = false;
        for (std::size_t index = 0u; index < tokens.size(); ++index) {
            if (tokens[index].tag != DRW_SabTag::Pointer)
                continue;
            if (++pointerOrdinal == pointerCount) {
                insertionIndex = index + 1u;
                foundPointerPrefix = true;
                break;
            }
        }
        if (!foundPointerPrefix)
            throw std::out_of_range("ACIS fixture boolean pointer prefix missing");
        std::vector<DRW_SabToken> flags;
        flags.reserve(values.size());
        for (bool value : values)
            flags.push_back(acisBoolean(value));
        tokens.insert(tokens.begin() + static_cast<std::ptrdiff_t>(insertionIndex),
                      flags.begin(), flags.end());
    }
};

struct SyntheticAcisBody {
    int body = -1;
    int lump = -1;
    int shell = -1;
    int face = -1;
    int loop = -1;
    int coedge = -1;
    int edge = -1;
    int vertex0 = -1;
    int vertex1 = -1;
    int point0 = -1;
    int point1 = -1;
    int surface = -1;
    int transform = -1;
};

SyntheticAcisBody addSyntheticAcisBody(AcisGraphFixture& fixture,
                                       double xOffset) {
    SyntheticAcisBody body;
    body.body = fixture.addNode("body", 4u);
    body.lump = fixture.addNode("lump", 4u);
    body.shell = fixture.addNode("shell", 6u);
    body.face = fixture.addNode("face", 6u);
    body.loop = fixture.addNode("loop", 4u);
    body.coedge = fixture.addNode("coedge", 7u);
    body.edge = fixture.addNode("edge", 5u);
    body.vertex0 = fixture.addNode("vertex", 3u);
    body.vertex1 = fixture.addNode("vertex", 3u);
    body.point0 = fixture.addNode(
        "point", 1u, {acisLocation(xOffset, 2.0, 3.0)});
    body.point1 = fixture.addNode(
        "point", 1u, {acisLocation(xOffset + 4.0, 5.0, 6.0)});
    body.surface = fixture.addNode(
        "plane-surface", 1u,
        {acisLocation(xOffset, 0.0, 0.0), acisDirection(0.0, 0.0, 1.0),
         acisDirection(1.0, 0.0, 0.0)});
    body.transform = fixture.addNode(
        "transform", 0u,
        {acisLiteral("11 22 33 44 55 66 77 88 99 111 122 133"),
         acisInteger(7), acisDouble(0.25), acisLiteral("vendor-hint")});

    fixture.link(body.body, 1u, body.lump);
    fixture.link(body.body, 3u, body.transform);
    fixture.link(body.lump, 2u, body.shell);
    fixture.link(body.lump, 3u, body.body);
    fixture.link(body.shell, 3u, body.face);
    fixture.link(body.shell, 5u, body.lump);
    fixture.link(body.face, 2u, body.loop);
    fixture.link(body.face, 3u, body.shell);
    fixture.link(body.face, 5u, body.surface);
    fixture.link(body.loop, 2u, body.coedge);
    fixture.link(body.loop, 3u, body.face);
    fixture.link(body.coedge, 1u, body.coedge);
    fixture.link(body.coedge, 2u, body.coedge);
    fixture.link(body.coedge, 3u, body.coedge);
    fixture.link(body.coedge, 4u, body.edge);
    fixture.link(body.coedge, 5u, body.loop);
    fixture.link(body.edge, 1u, body.vertex0);
    fixture.link(body.edge, 2u, body.vertex1);
    fixture.link(body.edge, 3u, body.coedge);
    fixture.link(body.vertex0, 1u, body.edge);
    fixture.link(body.vertex0, 2u, body.point0);
    fixture.link(body.vertex1, 1u, body.edge);
    fixture.link(body.vertex1, 2u, body.point1);
    fixture.setBooleanFieldsAfterPointerCount(body.face, 6u,
                                              {false, false});
    fixture.setBooleanFieldsAfterPointerCount(body.coedge, 5u, {false});
    return body;
}

void setModelPointer(DRW_AcisModel& model, int sourceIndex,
                     std::size_t pointerOrdinal, int targetIndex) {
    DRW_AcisNode& node = model.nodes[static_cast<std::size_t>(sourceIndex)];
    node.refs[pointerOrdinal] = targetIndex;
    std::size_t seen = 0;
    for (DRW_SabToken& token : node.record.tokens) {
        if (token.tag != DRW_SabTag::Pointer)
            continue;
        if (seen++ == pointerOrdinal) {
            token.ival = targetIndex;
            return;
        }
    }
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

    DRW_SabData diagnosticOrderData;
    diagnosticOrderData.header.version = 21200;
    diagnosticOrderData.records = {
        acisRecord("face", {acisPointer(-1), acisPointer(-1),
                              acisPointer(-1), acisPointer(-1),
                              acisPointer(-1)}),
        acisRecord("point", {acisLocation(1, 2, 3)}),
        acisRecord("vertex", {acisPointer(1)}),
        acisRecord("vertex", {acisPointer(99)}),
        acisRecord("edge", {acisPointer(2), acisPointer(2)}),
        acisRecord("End-of-ACIS-data", {})
    };
    const DRW_AcisModel diagnosticOrderModel =
        drw_buildAcisModel(diagnosticOrderData);
    DRW_AcisBrep diagnosticOrderBrep;
    t.expect(drw_extractAcisWireframe(diagnosticOrderModel,
                                       diagnosticOrderBrep)
                 && diagnosticOrderBrep.diagnostics
                        == std::vector<std::string> {
                            "vertex#3: no resolvable point",
                            "edge#4: degenerate (coincident endpoints)",
                            "face#0: no surface"},
             "ACIS single-pass projection preserves category-ordered diagnostics");

    DRW_AcisBrep reportedWireframe;
    DRW_AcisProjectionReport projectionReport;
    t.expect(drw_extractAcisWireframeWithReport(
                 model, reportedWireframe, projectionReport)
                 && projectionReport.coordinateSpace
                        == DRW_AcisCoordinateSpace::RawRecordValues
                 && projectionReport.modelNodeCount == model.nodes.size()
                 && projectionReport.vertexCount == 6u
                 && projectionReport.edgeCount == 3u
                 && projectionReport.faceCount == 4u
                 && projectionReport.verticesWithoutResolvedPoint == 0u
                 && projectionReport.edgesWithoutResolvedEndpointPair == 0u
                 && projectionReport.edgesWithoutCurveFlag == 0u
                 && projectionReport.facesWithoutSurfaceFlag == 1u
                 && !projectionReport.bodyRootSelected
                 && !projectionReport.bodyTransformsApplied
                 && !projectionReport.sourceUnitScaleApplied
                 && !projectionReport.brepValidityChecked,
             "ACIS projection report labels raw values and unresolved surface data");

    DRW_AcisModel wrongPointReference = model;
    wrongPointReference.nodes[3].refs[1] = 0;
    DRW_AcisBrep partialWireframe;
    DRW_AcisProjectionReport partialReport;
    t.expect(drw_extractAcisWireframeWithReport(
                 wrongPointReference, partialWireframe, partialReport)
                 && partialReport.verticesWithoutResolvedPoint == 1u
                 && partialReport.edgesWithoutResolvedEndpointPair == 1u
                 && partialReport.faceCount == 4u
                 && !partialReport.bodyRootSelected
                 && !partialReport.bodyTransformsApplied,
             "ACIS projection report exposes a wrong-type point reference");

    DRW_AcisModel nonFinitePoint = model;
    nonFinitePoint.nodes[1].record.tokens[1].vec.x =
        std::numeric_limits<double>::quiet_NaN();
    DRW_AcisBrep rejectedReportedWireframe;
    DRW_AcisProjectionReport rejectedProjectionReport;
    rejectedProjectionReport.modelNodeCount = 999u;
    rejectedProjectionReport.bodyRootSelected = true;
    t.expect(!drw_extractAcisWireframeWithReport(
                  nonFinitePoint, rejectedReportedWireframe,
                  rejectedProjectionReport)
                 && rejectedReportedWireframe.empty()
                 && rejectedProjectionReport.modelNodeCount == 0u
                 && !rejectedProjectionReport.bodyRootSelected,
             "ACIS projection report and output reset on non-finite geometry");

    const DRW_AcisEdge* straight = nullptr;
    const DRW_AcisEdge* ellipse = nullptr;
    const DRW_AcisEdge* intcurve = nullptr;
    for (const DRW_AcisEdge& edge : wireframe.edges) {
        if (edge.curveType == DRW_AcisCurve::Straight) straight = &edge;
        if (edge.curveType == DRW_AcisCurve::Ellipse) ellipse = &edge;
        if (edge.curveType == DRW_AcisCurve::Intcurve) intcurve = &edge;
    }
    const DRW_AcisNode* straightSource = straight && straight->nodeIndex >= 0
        && static_cast<std::size_t>(straight->nodeIndex) < model.nodes.size()
        ? &model.nodes[static_cast<std::size_t>(straight->nodeIndex)] : nullptr;
    t.expect(straight != nullptr && straight->hasStart && straight->hasEnd
                 && straightSource && straightSource->refs.size() >= 2u
                 && straight->startVertexNodeIndex == straightSource->refs[0]
                 && straight->endVertexNodeIndex == straightSource->refs[1]
                 && acisCoordNear(straight->start, 1, 2, 3)
                 && acisCoordNear(straight->end, 4, 5, 6)
                 && acisCoordNear(straight->p0, 10, 20, 30)
                 && acisCoordNear(straight->p1, 2, 0, 0),
             "ACIS straight curve skips leading pointer and resolves endpoints");
    DRW_Coord evaluatedStraight(90.0, 91.0, 92.0);
    const bool evaluatedStraightOk = straight
        && drw_evaluateAcisStraightCurve(*straight, 2.0, evaluatedStraight);
    t.expect(evaluatedStraightOk
                 && acisCoordNear(evaluatedStraight, 14.0, 20.0, 30.0),
             "ACIS straight-curve evaluation uses the raw unnormalized direction and parameter");
    double projectedParameter = 71.0;
    DRW_Coord projectedPoint(72.0, 73.0, 74.0);
    const bool projectedPointOk = straight
        && drw_projectPointToAcisStraightCurve(
            *straight, DRW_Coord(14.0, 25.0, 30.0), projectedParameter,
            projectedPoint);
    t.expect(projectedPointOk && std::fabs(projectedParameter - 2.0) < 1e-12
                 && acisCoordNear(projectedPoint, 14.0, 20.0, 30.0),
             "ACIS straight-curve projection returns the raw parameter and orthogonal point");
    double withinToleranceParameter = -81.0;
    const bool exactLinePoint = straight
        && drw_pointWithinToleranceOfAcisStraightCurve(
            *straight, DRW_Coord(14.0, 20.0, 30.0), 0.0,
            withinToleranceParameter);
    t.expect(exactLinePoint && withinToleranceParameter == 2.0,
             "ACIS straight-curve point check accepts an exact infinite-line point at zero tolerance");
    withinToleranceParameter = -82.0;
    const bool toleranceBoundaryPoint = straight
        && drw_pointWithinToleranceOfAcisStraightCurve(
            *straight, DRW_Coord(14.0, 20.25, 30.0), 0.25,
            withinToleranceParameter);
    t.expect(toleranceBoundaryPoint && withinToleranceParameter == 2.0,
             "ACIS straight-curve point check accepts the caller's exact tolerance boundary");
    withinToleranceParameter = -84.0;
    const bool multiAxisWithinTolerance = straight
        && drw_pointWithinToleranceOfAcisStraightCurve(
            *straight, DRW_Coord(14.0, 20.1, 30.2), 0.23,
            withinToleranceParameter);
    t.expect(multiAxisWithinTolerance && withinToleranceParameter == 2.0,
             "ACIS straight-curve point check compares the full Euclidean residual");

    DRW_AcisEdge finiteStraight;
    finiteStraight.hasCurve = true;
    finiteStraight.curveType = DRW_AcisCurve::Straight;
    finiteStraight.p0 = DRW_Coord(1.0, 2.0, 3.0);
    finiteStraight.p1 = DRW_Coord(2.0, 0.0, 0.0);
    finiteStraight.hasStart = true;
    finiteStraight.start = DRW_Coord(1.0, 2.0, 3.0);
    finiteStraight.hasEnd = true;
    finiteStraight.end = DRW_Coord(5.0, 2.0, 3.0);
    DRW_Coord evaluatedFiniteEdge(96.0, 97.0, 98.0);
    const bool finiteEdgeStartOk = drw_evaluateAcisStraightEdge(
        finiteStraight, 0.0, 0.0, evaluatedFiniteEdge);
    const bool finiteEdgeMidpointOk = drw_evaluateAcisStraightEdge(
        finiteStraight, 0.5, 0.0, evaluatedFiniteEdge);
    t.expect(finiteEdgeStartOk && finiteEdgeMidpointOk
                 && acisCoordNear(evaluatedFiniteEdge, 3.0, 2.0, 3.0),
             "ACIS straight-edge evaluation maps its normalized fraction to the raw endpoint parameter interval");
    const bool finiteEdgeEndOk = drw_evaluateAcisStraightEdge(
        finiteStraight, 1.0, 0.0, evaluatedFiniteEdge);
    t.expect(finiteEdgeEndOk
                 && acisCoordNear(evaluatedFiniteEdge, 5.0, 2.0, 3.0),
             "ACIS straight-edge evaluation reaches its second topological endpoint");

    DRW_Coord forwardCoedgeStart;
    DRW_Coord forwardCoedgeQuarter;
    const bool forwardCoedgeStartOk = drw_evaluateAcisStraightCoedge(
        finiteStraight, 0, 0.0, 0.0, forwardCoedgeStart);
    const bool forwardCoedgeQuarterOk = drw_evaluateAcisStraightCoedge(
        finiteStraight, 0, 0.25, 0.0, forwardCoedgeQuarter);
    t.expect(forwardCoedgeStartOk && forwardCoedgeQuarterOk
                 && acisCoordNear(forwardCoedgeStart, 1.0, 2.0, 3.0)
                 && acisCoordNear(forwardCoedgeQuarter, 2.0, 2.0, 3.0),
             "ACIS forward coedge evaluation follows raw edge endpoint order");
    DRW_Coord reverseCoedgeStart;
    DRW_Coord reverseCoedgeQuarter;
    DRW_Coord reverseCoedgeEnd;
    const bool reverseCoedgeStartOk = drw_evaluateAcisStraightCoedge(
        finiteStraight, 1, 0.0, 0.0, reverseCoedgeStart);
    const bool reverseCoedgeQuarterOk = drw_evaluateAcisStraightCoedge(
        finiteStraight, 1, 0.25, 0.0, reverseCoedgeQuarter);
    const bool reverseCoedgeEndOk = drw_evaluateAcisStraightCoedge(
        finiteStraight, 1, 1.0, 0.0, reverseCoedgeEnd);
    t.expect(reverseCoedgeStartOk && reverseCoedgeQuarterOk
                 && reverseCoedgeEndOk
                 && acisCoordNear(reverseCoedgeStart, 5.0, 2.0, 3.0)
                 && acisCoordNear(reverseCoedgeQuarter, 4.0, 2.0, 3.0)
                 && acisCoordNear(reverseCoedgeEnd, 1.0, 2.0, 3.0),
             "ACIS reversed coedge evaluation starts at the raw edge end and traverses backward");
    const DRW_Coord unchangedCoedgeEvaluation(101.0, 102.0, 103.0);
    evaluatedFiniteEdge = unchangedCoedgeEvaluation;
    const bool invalidCoedgeSenseRefused = !drw_evaluateAcisStraightCoedge(
        finiteStraight, -1, 0.5, 0.0, evaluatedFiniteEdge);
    const bool invalidCoedgeSenseAtomic =
        acisCoordNear(evaluatedFiniteEdge, unchangedCoedgeEvaluation.x,
                      unchangedCoedgeEvaluation.y,
                      unchangedCoedgeEvaluation.z);
    const bool invalidCoedgeFractionRefused = !drw_evaluateAcisStraightCoedge(
        finiteStraight, 1, 1.01, 0.0, evaluatedFiniteEdge);
    const bool invalidCoedgeFractionAtomic =
        acisCoordNear(evaluatedFiniteEdge, unchangedCoedgeEvaluation.x,
                      unchangedCoedgeEvaluation.y,
                      unchangedCoedgeEvaluation.z);
    t.expect(invalidCoedgeSenseRefused && invalidCoedgeSenseAtomic
                 && invalidCoedgeFractionRefused
                 && invalidCoedgeFractionAtomic,
             "ACIS coedge evaluation rejects unknown sense and out-of-range traversal atomically");
    const bool invalidCoedgeToleranceRefused =
        !drw_evaluateAcisStraightCoedge(
            finiteStraight, 0, 0.5, -1.0, evaluatedFiniteEdge);
    t.expect(invalidCoedgeToleranceRefused
                 && acisCoordNear(evaluatedFiniteEdge,
                                  unchangedCoedgeEvaluation.x,
                                  unchangedCoedgeEvaluation.y,
                                  unchangedCoedgeEvaluation.z),
             "ACIS coedge evaluation rejects invalid endpoint tolerance atomically");

    double finiteParameter = -91.0;
    DRW_Coord finiteProjection(92.0, 93.0, 94.0);
    const bool interiorFiniteProjection = drw_projectPointToAcisStraightEdge(
        finiteStraight, DRW_Coord(3.0, 4.0, 3.0), 0.0,
        finiteParameter, finiteProjection);
    t.expect(interiorFiniteProjection && finiteParameter == 1.0
                 && acisCoordNear(finiteProjection, 3.0, 2.0, 3.0),
             "ACIS straight-edge projection keeps an interior point on its finite parameter interval");
    const bool beforeFiniteProjection = drw_projectPointToAcisStraightEdge(
        finiteStraight, DRW_Coord(-5.0, 2.0, 3.0), 0.0,
        finiteParameter, finiteProjection);
    t.expect(beforeFiniteProjection && finiteParameter == 0.0
                 && acisCoordNear(finiteProjection, 1.0, 2.0, 3.0),
             "ACIS straight-edge projection clamps before the first topological endpoint");
    const bool afterFiniteProjection = drw_projectPointToAcisStraightEdge(
        finiteStraight, DRW_Coord(10.0, 2.0, 3.0), 0.0,
        finiteParameter, finiteProjection);
    t.expect(afterFiniteProjection && finiteParameter == 2.0
                 && acisCoordNear(finiteProjection, 5.0, 2.0, 3.0),
             "ACIS straight-edge projection clamps after the last topological endpoint");
    DRW_AcisEdge reversedFiniteStraight = finiteStraight;
    std::swap(reversedFiniteStraight.start, reversedFiniteStraight.end);
    const bool reversedFiniteProjection = drw_projectPointToAcisStraightEdge(
        reversedFiniteStraight, DRW_Coord(-5.0, 2.0, 3.0), 0.0,
        finiteParameter, finiteProjection);
    t.expect(reversedFiniteProjection && finiteParameter == 0.0
                 && acisCoordNear(finiteProjection, 1.0, 2.0, 3.0),
             "ACIS straight-edge interval is independent of raw endpoint ordering");
    const bool reversedFiniteEdgeStartOk = drw_evaluateAcisStraightEdge(
        reversedFiniteStraight, 0.0, 0.0, evaluatedFiniteEdge);
    t.expect(reversedFiniteEdgeStartOk
                 && acisCoordNear(evaluatedFiniteEdge, 5.0, 2.0, 3.0),
             "ACIS straight-edge evaluation follows reversed topological endpoint order");
    const DRW_Coord unchangedFiniteEvaluation(96.0, 97.0, 98.0);
    evaluatedFiniteEdge = unchangedFiniteEvaluation;
    t.expect(!drw_evaluateAcisStraightEdge(
                  finiteStraight, 1.0001, 0.0, evaluatedFiniteEdge)
                 && acisCoordNear(evaluatedFiniteEdge,
                                  unchangedFiniteEvaluation.x,
                                  unchangedFiniteEvaluation.y,
                                  unchangedFiniteEvaluation.z)
                 && !drw_evaluateAcisStraightEdge(
                        finiteStraight,
                        std::numeric_limits<double>::quiet_NaN(), 0.0,
                        evaluatedFiniteEdge)
                 && acisCoordNear(evaluatedFiniteEdge,
                                  unchangedFiniteEvaluation.x,
                                  unchangedFiniteEvaluation.y,
                                  unchangedFiniteEvaluation.z),
             "ACIS straight-edge evaluation rejects out-of-range/non-finite fractions atomically");
    DRW_AcisEdge wideParameterEdge;
    wideParameterEdge.hasCurve = true;
    wideParameterEdge.curveType = DRW_AcisCurve::Straight;
    wideParameterEdge.p0 = DRW_Coord(0.0, 0.0, 0.0);
    wideParameterEdge.p1 = DRW_Coord(1.0, 0.0, 0.0);
    wideParameterEdge.hasStart = true;
    wideParameterEdge.start = DRW_Coord(
        -std::numeric_limits<double>::max(), 0.0, 0.0);
    wideParameterEdge.hasEnd = true;
    wideParameterEdge.end = DRW_Coord(
        std::numeric_limits<double>::max(), 0.0, 0.0);
    const bool wideParameterMidpointOk = drw_evaluateAcisStraightEdge(
        wideParameterEdge, 0.5, 0.0, evaluatedFiniteEdge);
    t.expect(wideParameterMidpointOk
                 && acisCoordNear(evaluatedFiniteEdge, 0.0, 0.0, 0.0),
             "ACIS straight-edge evaluation interpolates opposite extreme endpoint parameters without overflowing their difference");

    double finiteToleranceParameter = -95.0;
    const bool finiteEdgeToleranceBoundary =
        drw_pointWithinToleranceOfAcisStraightEdge(
            finiteStraight, DRW_Coord(3.0, 2.25, 3.0), 0.0, 0.25,
            finiteToleranceParameter);
    t.expect(finiteEdgeToleranceBoundary && finiteToleranceParameter == 1.0,
             "ACIS straight-edge proximity accepts the exact Euclidean tolerance boundary");
    const double unchangedFiniteToleranceParameter = -95.0;
    finiteToleranceParameter = unchangedFiniteToleranceParameter;
    t.expect(!drw_pointWithinToleranceOfAcisStraightEdge(
                  finiteStraight, DRW_Coord(3.0, 2.25, 3.0), 0.0, 0.249,
                  finiteToleranceParameter)
                 && finiteToleranceParameter == unchangedFiniteToleranceParameter,
             "ACIS straight-edge proximity rejects a point just outside tolerance atomically");
    const bool nearFiniteEnd = drw_pointWithinToleranceOfAcisStraightEdge(
        finiteStraight, DRW_Coord(5.25, 2.0, 3.0), 0.0, 0.25,
        finiteToleranceParameter);
    t.expect(nearFiniteEnd && finiteToleranceParameter == 2.0,
             "ACIS straight-edge proximity measures to a clamped endpoint");
    finiteToleranceParameter = unchangedFiniteToleranceParameter;
    t.expect(!drw_pointWithinToleranceOfAcisStraightEdge(
                  finiteStraight, DRW_Coord(6.0, 2.0, 3.0), 0.0, 0.999,
                  finiteToleranceParameter)
                 && finiteToleranceParameter == unchangedFiniteToleranceParameter,
             "ACIS straight-edge proximity does not accept a point beyond the endpoint by supporting-line distance alone");
    t.expect(!drw_pointWithinToleranceOfAcisStraightEdge(
                  finiteStraight, DRW_Coord(3.0, 2.0, 3.0), 0.0,
                  std::numeric_limits<double>::infinity(),
                  finiteToleranceParameter)
                 && finiteToleranceParameter == unchangedFiniteToleranceParameter,
             "ACIS straight-edge proximity rejects non-finite query tolerance atomically");

    double coedgeTraversalFraction = -31.0;
    DRW_Coord coedgeProjectedPoint(32.0, 33.0, 34.0);
    const bool forwardCoedgeProjection =
        drw_projectPointToAcisStraightCoedge(
            finiteStraight, 0, DRW_Coord(2.0, 3.0, 3.0), 0.0,
            coedgeTraversalFraction, coedgeProjectedPoint);
    t.expect(forwardCoedgeProjection
                 && coedgeTraversalFraction == 0.25
                 && acisCoordNear(coedgeProjectedPoint, 2.0, 2.0, 3.0),
             "ACIS forward coedge projection returns its oriented traversal fraction");
    const bool reverseCoedgeProjection =
        drw_projectPointToAcisStraightCoedge(
            finiteStraight, 1, DRW_Coord(2.0, 3.0, 3.0), 0.0,
            coedgeTraversalFraction, coedgeProjectedPoint);
    t.expect(reverseCoedgeProjection
                 && coedgeTraversalFraction == 0.75
                 && acisCoordNear(coedgeProjectedPoint, 2.0, 2.0, 3.0),
             "ACIS reversed coedge projection maps the same point to reverse traversal order");
    const bool reverseClampedProjection =
        drw_projectPointToAcisStraightCoedge(
            finiteStraight, 1, DRW_Coord(-5.0, 2.0, 3.0), 0.0,
            coedgeTraversalFraction, coedgeProjectedPoint);
    t.expect(reverseClampedProjection
                 && coedgeTraversalFraction == 1.0
                 && acisCoordNear(coedgeProjectedPoint, 1.0, 2.0, 3.0),
             "ACIS reversed coedge projection clamps to traversal end before the raw edge start");
    double forwardCoedgeDistanceFraction = -32.0;
    double reverseCoedgeDistanceFraction = -33.0;
    const bool forwardCoedgeDistance =
        drw_pointWithinToleranceOfAcisStraightCoedge(
            finiteStraight, 0, DRW_Coord(2.0, 2.25, 3.0), 0.0, 0.25,
            forwardCoedgeDistanceFraction);
    const bool reverseCoedgeDistance =
        drw_pointWithinToleranceOfAcisStraightCoedge(
            finiteStraight, 1, DRW_Coord(2.0, 2.25, 3.0), 0.0, 0.25,
            reverseCoedgeDistanceFraction);
    t.expect(forwardCoedgeDistance && forwardCoedgeDistanceFraction == 0.25
                 && reverseCoedgeDistance
                 && reverseCoedgeDistanceFraction == 0.75,
             "ACIS coedge proximity accepts the exact distance boundary and returns oriented fractions");
    const double unchangedCoedgeFraction = -31.0;
    const DRW_Coord unchangedCoedgePoint(32.0, 33.0, 34.0);
    coedgeTraversalFraction = unchangedCoedgeFraction;
    coedgeProjectedPoint = unchangedCoedgePoint;
    const bool coedgeOutsideToleranceRefused =
        !drw_pointWithinToleranceOfAcisStraightCoedge(
            finiteStraight, 0, DRW_Coord(2.0, 2.25, 3.0), 0.0,
            std::nextafter(0.25, 0.0), coedgeTraversalFraction)
        && coedgeTraversalFraction == unchangedCoedgeFraction;
    const bool coedgeUnknownSenseRefused =
        !drw_projectPointToAcisStraightCoedge(
            finiteStraight, -1, DRW_Coord(2.0, 2.0, 3.0), 0.0,
            coedgeTraversalFraction, coedgeProjectedPoint)
        && coedgeTraversalFraction == unchangedCoedgeFraction
        && acisCoordNear(coedgeProjectedPoint, unchangedCoedgePoint.x,
                         unchangedCoedgePoint.y, unchangedCoedgePoint.z);
    t.expect(coedgeOutsideToleranceRefused && coedgeUnknownSenseRefused,
             "ACIS coedge projection and proximity preserve outputs on refused queries");
    DRW_AcisEdge degenerateStraight = finiteStraight;
    degenerateStraight.end = degenerateStraight.start;
    coedgeTraversalFraction = unchangedCoedgeFraction;
    t.expect(!drw_projectPointToAcisStraightCoedge(
                  degenerateStraight, 0, degenerateStraight.start, 0.0,
                  coedgeTraversalFraction, coedgeProjectedPoint)
                 && coedgeTraversalFraction == unchangedCoedgeFraction,
             "ACIS coedge projection refuses an undefined zero-length traversal fraction");
    const bool wideParameterCoedgeProjection =
        drw_projectPointToAcisStraightCoedge(
            wideParameterEdge, 0, DRW_Coord(0.0, 1.0, 0.0), 0.0,
            coedgeTraversalFraction, coedgeProjectedPoint);
    t.expect(wideParameterCoedgeProjection
                 && coedgeTraversalFraction == 0.5
                 && acisCoordNear(coedgeProjectedPoint, 0.0, 0.0, 0.0),
             "ACIS coedge projection computes a midpoint fraction when endpoint parameter subtraction overflows");

    AcisGraphFixture parserFedCoedgeFixture;
    const SyntheticAcisBody parserFedCoedgeBody =
        addSyntheticAcisBody(parserFedCoedgeFixture, 3.0);
    parserFedCoedgeFixture.data.records[
        static_cast<std::size_t>(parserFedCoedgeBody.point1)]
        .tokens[2u].vec = DRW_Coord(7.0, 2.0, 3.0);
    const int parserFedStraightCurve = parserFedCoedgeFixture.addNode(
        "straight-curve", 1u,
        {acisLocation(3.0, 2.0, 3.0), acisDirection(4.0, 0.0, 0.0)});
    parserFedCoedgeFixture.link(parserFedCoedgeBody.edge, 4u,
                                parserFedStraightCurve);
    parserFedCoedgeFixture.setBooleanFieldsAfterPointerCount(
        parserFedCoedgeBody.coedge, 5u, {true});
    const std::vector<std::uint8_t> parserFedCoedgeBytes =
        sab21800FromGraph(parserFedCoedgeFixture.data);
    DRW_SabData parserFedCoedgeData;
    const bool parserFedCoedgeParsed = !parserFedCoedgeBytes.empty()
        && drw_parseSab(parserFedCoedgeBytes.data(),
                        parserFedCoedgeBytes.size(), parserFedCoedgeData);
    const DRW_AcisModel parserFedCoedgeModel =
        drw_buildAcisModel(parserFedCoedgeData);
    std::vector<DRW_AcisBodyProjection> parserFedCoedgeProjections;
    DRW_AcisRootedProjectionReport parserFedCoedgeReport;
    const bool parserFedCoedgeProjected = parserFedCoedgeParsed
        && drw_extractAcisBodyWireframes(
            parserFedCoedgeModel, parserFedCoedgeProjections,
            parserFedCoedgeReport);
    const DRW_AcisEdge* parserFedCoedgeEdge = nullptr;
    const DRW_AcisFaceLoop* parserFedCoedgeLoop = nullptr;
    if (parserFedCoedgeProjections.size() == 1u) {
        for (const DRW_AcisEdge& edge : parserFedCoedgeProjections.front()
                                             .wireframe.edges) {
            if (edge.nodeIndex == parserFedCoedgeBody.edge) {
                parserFedCoedgeEdge = &edge;
                break;
            }
        }
        if (!parserFedCoedgeProjections.front().wireframe.faces.empty()
            && !parserFedCoedgeProjections.front().wireframe.faces.front()
                    .loops.empty())
            parserFedCoedgeLoop =
                &parserFedCoedgeProjections.front().wireframe.faces.front()
                     .loops.front();
    }
    DRW_Coord parserFedCoedgeStart;
    DRW_Coord parserFedCoedgeEnd;
    const bool parserFedCoedgeStartOk = parserFedCoedgeEdge
        && parserFedCoedgeLoop
        && parserFedCoedgeLoop->coedgeSenseFlags
                == std::vector<std::int8_t>{1}
        && drw_evaluateAcisStraightCoedge(
            *parserFedCoedgeEdge,
            parserFedCoedgeLoop->coedgeSenseFlags.front(), 0.0, 0.0,
            parserFedCoedgeStart);
    const bool parserFedCoedgeEndOk = parserFedCoedgeEdge
        && parserFedCoedgeLoop
        && drw_evaluateAcisStraightCoedge(
            *parserFedCoedgeEdge,
            parserFedCoedgeLoop->coedgeSenseFlags.front(), 1.0, 0.0,
            parserFedCoedgeEnd);
    double parserFedCoedgeFraction = -41.0;
    DRW_Coord parserFedCoedgeProjectedPoint;
    const DRW_Coord parserFedCoedgeQuery(4.0, 2.25, 3.0);
    const bool parserFedCoedgeProjectionOk = parserFedCoedgeEdge
        && parserFedCoedgeLoop
        && drw_projectPointToAcisStraightCoedge(
            *parserFedCoedgeEdge,
            parserFedCoedgeLoop->coedgeSenseFlags.front(),
            parserFedCoedgeQuery, 0.0, parserFedCoedgeFraction,
            parserFedCoedgeProjectedPoint);
    double parserFedCoedgeDistanceFraction = -42.0;
    const bool parserFedCoedgeDistanceOk = parserFedCoedgeEdge
        && parserFedCoedgeLoop
        && drw_pointWithinToleranceOfAcisStraightCoedge(
            *parserFedCoedgeEdge,
            parserFedCoedgeLoop->coedgeSenseFlags.front(),
            parserFedCoedgeQuery, 0.0, 0.25,
            parserFedCoedgeDistanceFraction);
    double unchangedParserFedCoedgeFraction = -43.0;
    const bool parserFedCoedgeOutsideToleranceRefused =
        parserFedCoedgeEdge && parserFedCoedgeLoop
        && !drw_pointWithinToleranceOfAcisStraightCoedge(
            *parserFedCoedgeEdge,
            parserFedCoedgeLoop->coedgeSenseFlags.front(),
            parserFedCoedgeQuery, 0.0, std::nextafter(0.25, 0.0),
            unchangedParserFedCoedgeFraction)
        && unchangedParserFedCoedgeFraction == -43.0;
    t.expect(parserFedCoedgeParsed && parserFedCoedgeProjected
                 && parserFedCoedgeEdge && parserFedCoedgeLoop
                 && parserFedCoedgeStartOk && parserFedCoedgeEndOk
                 && acisCoordNear(parserFedCoedgeStart, 7.0, 2.0, 3.0)
                 && acisCoordNear(parserFedCoedgeEnd, 3.0, 2.0, 3.0),
             "parser-fed SAB 21800 coedge sense drives straight-edge traversal endpoints");
    t.expect(parserFedCoedgeProjectionOk
                 && parserFedCoedgeFraction == 0.75
                 && acisCoordNear(parserFedCoedgeProjectedPoint,
                                  4.0, 2.0, 3.0),
             "parser-fed SAB 21800 reversed coedge projection returns traversal fraction and projected point");
    t.expect(parserFedCoedgeDistanceOk
                 && parserFedCoedgeDistanceFraction == 0.75
                 && parserFedCoedgeOutsideToleranceRefused,
             "parser-fed SAB 21800 reversed coedge proximity accepts exact tolerance and refuses just-outside");

    const double unchangedFiniteParameter = -91.0;
    const DRW_Coord unchangedFiniteProjection(92.0, 93.0, 94.0);
    finiteParameter = unchangedFiniteParameter;
    finiteProjection = unchangedFiniteProjection;
    DRW_AcisEdge offCurveStraight = finiteStraight;
    offCurveStraight.end.y += 0.125;
    t.expect(!drw_projectPointToAcisStraightEdge(
                  offCurveStraight, DRW_Coord(3.0, 4.0, 3.0), 0.124,
                  finiteParameter, finiteProjection)
                 && finiteParameter == unchangedFiniteParameter
                 && acisCoordNear(finiteProjection,
                                  unchangedFiniteProjection.x,
                                  unchangedFiniteProjection.y,
                                  unchangedFiniteProjection.z),
             "ACIS straight-edge projection rejects endpoints outside the declared curve tolerance atomically");
    offCurveStraight.end.y = finiteStraight.end.y + 0.125;
    const bool endpointToleranceBoundary = drw_projectPointToAcisStraightEdge(
        offCurveStraight, DRW_Coord(10.0, 2.0, 3.0), 0.125,
        finiteParameter, finiteProjection);
    t.expect(endpointToleranceBoundary && finiteParameter == 2.0
                 && acisCoordNear(finiteProjection, 5.0, 2.0, 3.0),
             "ACIS straight-edge projection accepts an endpoint exactly on its declared curve-tolerance boundary");
    finiteParameter = unchangedFiniteParameter;
    finiteProjection = unchangedFiniteProjection;
    t.expect(!drw_projectPointToAcisStraightEdge(
                  finiteStraight, DRW_Coord(3.0, 4.0, 3.0), -1.0,
                  finiteParameter, finiteProjection)
                 && finiteParameter == unchangedFiniteParameter
                 && acisCoordNear(finiteProjection,
                                  unchangedFiniteProjection.x,
                                  unchangedFiniteProjection.y,
                                  unchangedFiniteProjection.z),
             "ACIS straight-edge projection rejects an invalid endpoint tolerance atomically");
    DRW_AcisEdge noFiniteEndpoint = finiteStraight;
    noFiniteEndpoint.hasEnd = false;
    finiteParameter = unchangedFiniteParameter;
    finiteProjection = unchangedFiniteProjection;
    t.expect(!drw_projectPointToAcisStraightEdge(
                  noFiniteEndpoint, DRW_Coord(3.0, 4.0, 3.0), 0.0,
                  finiteParameter, finiteProjection)
                 && finiteParameter == unchangedFiniteParameter
                 && acisCoordNear(finiteProjection,
                                  unchangedFiniteProjection.x,
                                  unchangedFiniteProjection.y,
                                  unchangedFiniteProjection.z),
             "ACIS straight-edge projection refuses a missing topological endpoint atomically");

    const double unchangedToleranceParameter = -83.0;
    withinToleranceParameter = unchangedToleranceParameter;
    t.expect(straight && !drw_pointWithinToleranceOfAcisStraightCurve(
                  *straight, DRW_Coord(14.0, 20.25, 30.0), 0.249,
                  withinToleranceParameter)
                 && withinToleranceParameter == unchangedToleranceParameter,
             "ACIS straight-curve point check rejects an outside point atomically");
    withinToleranceParameter = unchangedToleranceParameter;
    t.expect(straight && !drw_pointWithinToleranceOfAcisStraightCurve(
                  *straight, DRW_Coord(14.0, 20.2, 30.2), 0.25,
                  withinToleranceParameter)
                 && withinToleranceParameter == unchangedToleranceParameter,
             "ACIS straight-curve point check rejects a multi-axis residual above Euclidean tolerance");
    t.expect(straight && !drw_pointWithinToleranceOfAcisStraightCurve(
                  *straight, DRW_Coord(14.0, 20.0, 30.0), -1.0,
                  withinToleranceParameter)
                 && withinToleranceParameter == unchangedToleranceParameter
                 && !drw_pointWithinToleranceOfAcisStraightCurve(
                        *straight, DRW_Coord(14.0, 20.0, 30.0),
                        std::numeric_limits<double>::quiet_NaN(),
                        withinToleranceParameter)
                 && withinToleranceParameter == unchangedToleranceParameter
                 && !drw_pointWithinToleranceOfAcisStraightCurve(
                        *straight, DRW_Coord(14.0, 20.0, 30.0),
                        std::numeric_limits<double>::infinity(),
                        withinToleranceParameter)
                 && withinToleranceParameter == unchangedToleranceParameter,
             "ACIS straight-curve point check rejects invalid caller tolerances atomically");
    DRW_AcisEdge scaledStraight;
    scaledStraight.hasCurve = true;
    scaledStraight.curveType = DRW_AcisCurve::Straight;
    scaledStraight.p1 = DRW_Coord(1.0, 1e-300, 0.0);
    double scaledParameter = -1.0;
    DRW_Coord scaledProjection;
    const bool scaledProjectionOk = drw_projectPointToAcisStraightCurve(
        scaledStraight, DRW_Coord(1e200, 1.0, 0.0), scaledParameter,
        scaledProjection);
    t.expect(scaledProjectionOk && scaledParameter == 1e200
                 && scaledProjection.x == 1e200
                 && scaledProjection.y > 0.0
                 && scaledProjection.y < 1e-90,
             "ACIS straight-curve projection preserves a representable component across extreme scales");
    const DRW_Coord unchangedEvaluation = evaluatedStraight;
    const DRW_Coord unchangedProjectedPoint = projectedPoint;
    const double unchangedProjectedParameter = projectedParameter;
    t.expect(ellipse && !drw_evaluateAcisStraightCurve(
                             *ellipse, 2.0, evaluatedStraight)
                 && acisCoordNear(evaluatedStraight,
                                  unchangedEvaluation.x,
                                  unchangedEvaluation.y,
                                  unchangedEvaluation.z),
             "ACIS straight-curve evaluator rejects other curve kinds without changing output");
    t.expect(ellipse && !drw_projectPointToAcisStraightCurve(
                             *ellipse, DRW_Coord(1.0, 2.0, 3.0),
                             projectedParameter, projectedPoint)
                 && projectedParameter == unchangedProjectedParameter
                 && acisCoordNear(projectedPoint,
                                  unchangedProjectedPoint.x,
                                  unchangedProjectedPoint.y,
                                  unchangedProjectedPoint.z),
             "ACIS straight-curve projection rejects other curve kinds atomically");
    DRW_AcisEdge zeroDirectionStraight;
    if (straight)
        zeroDirectionStraight = *straight;
    zeroDirectionStraight.hasCurve = true;
    zeroDirectionStraight.curveType = DRW_AcisCurve::Straight;
    zeroDirectionStraight.p1 = DRW_Coord(0.0, 0.0, 0.0);
    t.expect(!drw_evaluateAcisStraightCurve(
                  zeroDirectionStraight, 1.0, evaluatedStraight)
                 && acisCoordNear(evaluatedStraight,
                                  unchangedEvaluation.x,
                                  unchangedEvaluation.y,
                                  unchangedEvaluation.z),
             "ACIS straight-curve evaluator refuses a zero direction atomically");
    t.expect(!drw_projectPointToAcisStraightCurve(
                  zeroDirectionStraight, DRW_Coord(1.0, 2.0, 3.0),
                  projectedParameter, projectedPoint)
                 && projectedParameter == unchangedProjectedParameter
                 && acisCoordNear(projectedPoint,
                                  unchangedProjectedPoint.x,
                                  unchangedProjectedPoint.y,
                                  unchangedProjectedPoint.z),
             "ACIS straight-curve projection refuses a zero direction atomically");
    withinToleranceParameter = unchangedToleranceParameter;
    t.expect(!drw_pointWithinToleranceOfAcisStraightCurve(
                  zeroDirectionStraight, DRW_Coord(1.0, 2.0, 3.0), 1.0,
                  withinToleranceParameter)
                 && withinToleranceParameter == unchangedToleranceParameter,
             "ACIS straight-curve point check refuses a zero direction atomically");
    t.expect(straight && !drw_projectPointToAcisStraightCurve(
                  *straight,
                  DRW_Coord(std::numeric_limits<double>::quiet_NaN(),
                            2.0, 3.0),
                  projectedParameter, projectedPoint)
                 && projectedParameter == unchangedProjectedParameter
                 && acisCoordNear(projectedPoint,
                                  unchangedProjectedPoint.x,
                                  unchangedProjectedPoint.y,
                                  unchangedProjectedPoint.z),
             "ACIS straight-curve projection refuses non-finite input atomically");
    DRW_AcisEdge overflowingProjection;
    if (straight)
        overflowingProjection = *straight;
    overflowingProjection.hasCurve = true;
    overflowingProjection.curveType = DRW_AcisCurve::Straight;
    overflowingProjection.p0 = DRW_Coord(
        -std::numeric_limits<double>::max(), 0.0, 0.0);
    overflowingProjection.p1 = DRW_Coord(1.0, 0.0, 0.0);
    t.expect(!drw_projectPointToAcisStraightCurve(
                  overflowingProjection,
                  DRW_Coord(std::numeric_limits<double>::max(), 0.0, 0.0),
                  projectedParameter, projectedPoint)
                 && projectedParameter == unchangedProjectedParameter
                 && acisCoordNear(projectedPoint,
                                  unchangedProjectedPoint.x,
                                  unchangedProjectedPoint.y,
                                  unchangedProjectedPoint.z),
             "ACIS straight-curve projection refuses an unrepresentable offset atomically");
    t.expect(straight && !drw_evaluateAcisStraightCurve(
                  *straight, std::numeric_limits<double>::quiet_NaN(),
                  evaluatedStraight)
                 && acisCoordNear(evaluatedStraight,
                                  unchangedEvaluation.x,
                                  unchangedEvaluation.y,
                                  unchangedEvaluation.z),
             "ACIS straight-curve evaluator refuses a non-finite parameter atomically");
    DRW_AcisEdge overflowingStraight;
    if (straight)
        overflowingStraight = *straight;
    overflowingStraight.hasCurve = true;
    overflowingStraight.curveType = DRW_AcisCurve::Straight;
    overflowingStraight.p0 = DRW_Coord(
        std::numeric_limits<double>::max(), 0.0, 0.0);
    overflowingStraight.p1 = DRW_Coord(
        std::numeric_limits<double>::max(), 0.0, 0.0);
    t.expect(!drw_evaluateAcisStraightCurve(
                  overflowingStraight, 2.0, evaluatedStraight)
                 && acisCoordNear(evaluatedStraight,
                                  unchangedEvaluation.x,
                                  unchangedEvaluation.y,
                                  unchangedEvaluation.z),
             "ACIS straight-curve evaluator refuses non-finite results atomically");
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

    const auto rejectsNonFiniteVector = [&t, &wireframe](
        std::size_t nodeIndex, std::size_t vectorIndex, double x,
        const char* label) {
        DRW_AcisModel invalid =
            drw_buildAcisModel(syntheticAcisGeometry());
        if (nodeIndex >= invalid.nodes.size()) {
            t.expect(false, label);
            return;
        }
        std::size_t seenVectors = 0;
        bool changed = false;
        for (DRW_SabToken& token : invalid.nodes[nodeIndex].record.tokens) {
            if (token.tag != DRW_SabTag::LocationVec
                && token.tag != DRW_SabTag::DirectionVec)
                continue;
            if (seenVectors++ == vectorIndex) {
                token.vec.x = x;
                changed = true;
                break;
            }
        }
        DRW_AcisBrep rejected = wireframe;
        t.expect(changed && !drw_extractAcisWireframe(invalid, rejected)
                     && rejected.empty() && !rejected.hasBBox,
                 label);
    };
    const auto rejectsNonFiniteScalar = [&t, &wireframe](
        std::size_t nodeIndex, std::size_t scalarIndex, double value,
        const char* label) {
        DRW_AcisModel invalid =
            drw_buildAcisModel(syntheticAcisGeometry());
        if (nodeIndex >= invalid.nodes.size()) {
            t.expect(false, label);
            return;
        }
        std::size_t seenScalars = 0;
        bool changed = false;
        for (DRW_SabToken& token : invalid.nodes[nodeIndex].record.tokens) {
            if (token.tag != DRW_SabTag::Double)
                continue;
            if (seenScalars++ == scalarIndex) {
                token.dval = value;
                changed = true;
                break;
            }
        }
        DRW_AcisBrep rejected = wireframe;
        t.expect(changed && !drw_extractAcisWireframe(invalid, rejected)
                     && rejected.empty() && !rejected.hasBBox,
                 label);
    };
    rejectsNonFiniteVector(1u, 0u,
                           std::numeric_limits<double>::quiet_NaN(),
                           "ACIS projection rejects NaN vertex and clears output");
    rejectsNonFiniteVector(5u, 1u,
                           std::numeric_limits<double>::infinity(),
                           "ACIS projection rejects infinite curve parameters");
    rejectsNonFiniteVector(7u, 1u,
                           -std::numeric_limits<double>::infinity(),
                           "ACIS projection rejects infinite surface parameters");
    rejectsNonFiniteScalar(13u, 0u,
                           std::numeric_limits<double>::quiet_NaN(),
                           "ACIS projection rejects NaN ellipse ratio");
    rejectsNonFiniteScalar(17u, 1u,
                           -std::numeric_limits<double>::infinity(),
                           "ACIS projection rejects infinite cone parameter");
    rejectsNonFiniteScalar(19u, 0u,
                           std::numeric_limits<double>::infinity(),
                           "ACIS projection rejects infinite torus radius");
}

void testAcisRootedBodyProjection(TestContext& t) {
    AcisGraphFixture fixture;
    const SyntheticAcisBody first = addSyntheticAcisBody(fixture, 1.25);
    const SyntheticAcisBody second = addSyntheticAcisBody(fixture, 101.25);
    fixture.addNode("vertex", 3u);
    fixture.addNode("edge", 5u);
    fixture.addNode("face", 6u);
    const DRW_AcisModel model = drw_buildAcisModel(fixture.data);

    std::vector<DRW_AcisBodyProjection> projections;
    DRW_AcisRootedProjectionReport report;
    t.expect(drw_extractAcisBodyWireframes(model, projections, report)
                 && report.failure == DRW_AcisProjectionFailure::NoFailure
                 && report.bodyCount == 2u
                 && report.multiplyRootedNodeCount == 0u
                 && report.verticesOutsideBodyRoots == 1u
                 && report.edgesOutsideBodyRoots == 1u
                 && report.facesOutsideBodyRoots == 1u
                 && projections.size() == 2u,
             "ACIS rooted projection isolates two bodies and counts outside nodes");
    if (projections.size() != 2u)
        return;

    const DRW_AcisBodyProjection& firstProjection = projections[0];
    const DRW_AcisBodyProjection& secondProjection = projections[1];
    DRW_AcisModel nullCoedgeEdge = model;
    setModelPointer(nullCoedgeEdge, first.coedge, 4u, -1);
    std::vector<DRW_AcisBodyProjection> nullCoedgeEdgeProjections;
    DRW_AcisRootedProjectionReport nullCoedgeEdgeReport;
    t.expect(drw_extractAcisBodyWireframes(
                 nullCoedgeEdge, nullCoedgeEdgeProjections,
                 nullCoedgeEdgeReport)
                 && nullCoedgeEdgeProjections.size() == 2u
                 && !nullCoedgeEdgeProjections.front().wireframe.faces.empty()
                 && !nullCoedgeEdgeProjections.front().wireframe.faces.front()
                         .loops.empty()
                 && nullCoedgeEdgeProjections.front().wireframe.faces.front()
                         .loops.front().coedgeNodeIndices
                    == std::vector<int>{first.coedge}
                 && nullCoedgeEdgeProjections.front().wireframe.faces.front()
                         .loops.front().coedgeEdgeNodeIndices
                    == std::vector<int>{-1}
                 && drw_acisProjectionCoverage(
                        nullCoedgeEdgeProjections.front())
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "ACIS face-loop edge incidence preserves a null coedge edge slot");
    DRW_AcisModel nullCoedgeLoop = model;
    setModelPointer(nullCoedgeLoop, first.coedge, 5u, -1);
    std::vector<DRW_AcisBodyProjection> nullCoedgeLoopProjections;
    DRW_AcisRootedProjectionReport nullCoedgeLoopReport;
    t.expect(drw_extractAcisBodyWireframes(
                 nullCoedgeLoop, nullCoedgeLoopProjections,
                 nullCoedgeLoopReport)
                 && nullCoedgeLoopProjections.size() == 2u
                 && !nullCoedgeLoopProjections.front().wireframe.faces.empty()
                 && !nullCoedgeLoopProjections.front().wireframe.faces.front()
                         .loops.empty()
                 && nullCoedgeLoopProjections.front().wireframe.faces.front()
                         .loops.front().coedgeLoopNodeIndices
                    == std::vector<int>{-1}
                 && drw_acisProjectionCoverage(
                        nullCoedgeLoopProjections.front())
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "ACIS face-loop projection exposes a missing coedge owning-loop link");
    const std::array<double, 12> expectedTransformComponents = {{
        11.0, 22.0, 33.0, 44.0, 55.0, 66.0,
        77.0, 88.0, 99.0, 111.0, 122.0, 133.0
    }};
    t.expect(firstProjection.bodyNodeIndex == first.body
                 && secondProjection.bodyNodeIndex == second.body
                 && firstProjection.bodyGraphProjected
                 && secondProjection.bodyGraphProjected
                 && firstProjection.hasLumpChainObservation
                 && firstProjection.lumpChainNullTerminated
                 && firstProjection.lumpNodeIndices
                    == std::vector<int>{first.lump}
                 && firstProjection.lumpChainMatchesUseIndex
                 && firstProjection.wireframe.vertices.size() == 2u
                 && firstProjection.wireframe.edges.size() == 1u
                 && firstProjection.wireframe.faces.size() == 1u
                 && acisCoordNear(firstProjection.wireframe.vertices[0].point,
                                  1.25, 2.0, 3.0)
                 && acisCoordNear(secondProjection.wireframe.vertices[0].point,
                                  101.25, 2.0, 3.0),
             "ACIS rooted projections retain independent source identities and raw coordinates");
    t.expect(firstProjection.wireframe.edges.size() == 1u
                 && firstProjection.wireframe.edges.front().nodeIndex
                        == first.edge
                 && firstProjection.wireframe.edges.front()
                        .startVertexNodeIndex == first.vertex0
                 && firstProjection.wireframe.edges.front()
                        .endVertexNodeIndex == first.vertex1
                 && firstProjection.wireframe.edges.front().coedgeNodeIndex
                        == first.coedge
                 && firstProjection.wireframe.edges.front()
                        .hasReciprocalCoedgeLink,
             "ACIS rooted edge projection retains exact endpoint and reciprocal edge/coedge identities");
    DRW_AcisModel nullEdgeCoedge = model;
    setModelPointer(nullEdgeCoedge, first.edge, 3u, -1);
    std::vector<DRW_AcisBodyProjection> nullEdgeCoedgeProjections;
    DRW_AcisRootedProjectionReport nullEdgeCoedgeReport;
    t.expect(drw_extractAcisBodyWireframes(
                 nullEdgeCoedge, nullEdgeCoedgeProjections,
                 nullEdgeCoedgeReport)
                 && nullEdgeCoedgeProjections.size() == 2u
                 && !nullEdgeCoedgeProjections.front().wireframe.edges.empty()
                 && nullEdgeCoedgeProjections.front().wireframe.edges.front()
                         .coedgeNodeIndex == -1
                 && drw_acisProjectionCoverage(
                        nullEdgeCoedgeProjections.front())
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "ACIS rooted edge projection retains a null coedge pointer target as a known gap");
    t.expect(firstProjection.bodyTransformNodeIndex == first.transform
                 && firstProjection.report.bodyRootSelected
                 && firstProjection.report.bodyTransformPresent
                 && firstProjection.transform.sourceNodeIndex == first.transform
                 && firstProjection.transform.pointerResolved
                 && firstProjection.transform.state
                        == DRW_AcisTransformState::ParsedUnapplied
                 && firstProjection.transform.rawLiteralPresent
                 && firstProjection.transform.rawLiteral
                        == "11 22 33 44 55 66 77 88 99 111 122 133"
                 && firstProjection.transform.rawLiteralTokenIndex == 1u
                 && firstProjection.transform.rawTokens.size() == 5u
                 && firstProjection.transform.rawTokens[2].ival == 7
                 && std::fabs(
                        firstProjection.transform.rawTokens[3].dval - 0.25)
                        < 1e-9
                 && firstProjection.transform.rawTokens[4].sval
                        == "vendor-hint"
                 && firstProjection.transform.candidateComponentsPresent
                 && firstProjection.transform.candidateComponents
                        == expectedTransformComponents
                 && !firstProjection.report.bodyTransformsApplied
                 && !firstProjection.report.sourceUnitScaleApplied
                 && !firstProjection.report.brepValidityChecked
                 && firstProjection.report.coordinateSpace
                        == DRW_AcisCoordinateSpace::RawRecordValues,
             "ACIS rooted projection reports transform presence without applying placement or units");
    t.expect(firstProjection.coedgeNextCycleCount == 1u
                 && firstProjection.coedgePartnerCycleCount == 1u
                 && firstProjection.nextListCycleCount == 0u,
             "ACIS rooted projection reports but preserves coedge cycles");
    AcisGraphFixture sourceLinkFixture;
    const SyntheticAcisBody sourceLinkBody =
        addSyntheticAcisBody(sourceLinkFixture, 1.25);
    const int sourceLinkCurve = sourceLinkFixture.addNode(
        "straight-curve", 1u,
        {acisLocation(1.25, 2.0, 3.0), acisDirection(4.0, 3.0, 3.0)});
    sourceLinkFixture.link(sourceLinkBody.edge, 4u, sourceLinkCurve);
    const DRW_AcisModel sourceLinkModel =
        drw_buildAcisModel(sourceLinkFixture.data);
    std::vector<DRW_AcisBodyProjection> sourceLinkProjections;
    DRW_AcisRootedProjectionReport sourceLinkReport;
    const bool sourceLinkProjectionReady = drw_extractAcisBodyWireframes(
        sourceLinkModel, sourceLinkProjections, sourceLinkReport)
        && sourceLinkReport.failure == DRW_AcisProjectionFailure::NoFailure
        && sourceLinkProjections.size() == 1u;
    t.expect(sourceLinkProjectionReady,
             "ACIS coverage positive control retains graph-linked analytic geometry");
    const auto setProjectionGraphLinkTarget = [](
        DRW_AcisBodyProjection& projection, int sourceNodeIndex,
        std::size_t pointerOrdinal, int targetNodeIndex) {
        for (DRW_AcisBodyGraphLink& link : projection.graphLinks) {
            if (link.sourceNodeIndex == sourceNodeIndex
                && link.pointerOrdinal == pointerOrdinal) {
                link.targetNodeIndex = targetNodeIndex;
                return true;
            }
        }
        return false;
    };
    DRW_AcisBodyProjection noProjection;
    DRW_AcisBodyProjection gapFreeSubset = sourceLinkProjectionReady
        ? sourceLinkProjections.front() : DRW_AcisBodyProjection();
    gapFreeSubset.report.edgesWithoutCurveFlag = 0u;
    for (DRW_AcisFace& face : gapFreeSubset.wireframe.faces) {
        for (DRW_AcisFaceLoop& loop : face.loops) {
            for (std::size_t index = 0u;
                 index < loop.coedgeNodeIndices.size(); ++index) {
                loop.coedgeEdgeStartVertexNodeIndices[index] =
                    sourceLinkBody.vertex0;
                loop.coedgeEdgeEndVertexNodeIndices[index] =
                    sourceLinkBody.vertex0;
                loop.coedgeOrientedStartVertexNodeIndices[index] =
                    sourceLinkBody.vertex0;
                loop.coedgeOrientedEndVertexNodeIndices[index] =
                    sourceLinkBody.vertex0;
            }
            loop.hasCoedgeEndpointChainObservation =
                !loop.coedgeNodeIndices.empty()
                && (loop.coedgeRingClosed || loop.coedgeRingOpen)
                && loop.coedgeLinksReciprocal;
            loop.coedgeEndpointChainContinuous =
                loop.hasCoedgeEndpointChainObservation;
            loop.coedgeEndpointDiscontinuityCount = 0u;
            loop.firstCoedgeEndpointDiscontinuityNodeIndex = -1;
        }
    }
    gapFreeSubset.hasLumpChainObservation = true;
    gapFreeSubset.lumpChainNullTerminated = true;
    gapFreeSubset.lumpChainCycleDetected = false;
    gapFreeSubset.firstRepeatedLumpChainNodeIndex = -1;
    gapFreeSubset.lumpChainUnclassifiedLinkCount = 0u;
    gapFreeSubset.firstUnclassifiedLumpLinkSourceNodeIndex = -1;
    gapFreeSubset.hasLumpUseIndexObservation = true;
    gapFreeSubset.lumpUseIndexCoversParsedLumps = true;
    gapFreeSubset.lumpUseIndexUnclassifiedLumpCount = 0u;
    gapFreeSubset.firstUnclassifiedLumpOwnerNodeIndex = -1;
    gapFreeSubset.lumpChainMatchesUseIndex = true;
    for (DRW_AcisShell& shell : gapFreeSubset.wireframe.shells) {
        // The fixture intentionally includes an unrelated face without a
        // shell owner. Normalize that external-model census state here so the
        // isolated coverage mutations start from a coherent positive shell.
        shell.faceUseIndexCoversParsedFaces = true;
        shell.faceUseIndexUnclassifiedFaceCount = 0u;
        shell.firstUnclassifiedFaceOwnerNodeIndex = -1;
        shell.faceChainMatchesUseIndex = true;
    }
    for (DRW_AcisLump& lump : gapFreeSubset.wireframe.lumps) {
        lump.shellUseIndexCoversParsedShells = true;
        lump.shellUseIndexUnclassifiedShellCount = 0u;
        lump.firstUnclassifiedShellOwnerNodeIndex = -1;
        lump.shellChainMatchesUseIndex = true;
    }
    DRW_AcisBodyProjection reorderedGraphLinkGroups = gapFreeSubset;
    std::stable_sort(
        reorderedGraphLinkGroups.graphLinks.begin(),
        reorderedGraphLinkGroups.graphLinks.end(),
        [](const DRW_AcisBodyGraphLink& lhs,
           const DRW_AcisBodyGraphLink& rhs) {
            return lhs.sourceNodeIndex > rhs.sourceNodeIndex;
        });
    t.expect(sourceLinkProjectionReady
                 && drw_acisProjectionCoverage(reorderedGraphLinkGroups)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetNoKnownGaps,
             "ACIS coverage joins retained pointer rows by identity independent of source-group order");
    const std::array<std::size_t DRW_AcisProjectionReport::*, 11>
        knownGapCounters {{
            &DRW_AcisProjectionReport::unsupportedPatternNodeCount,
            &DRW_AcisProjectionReport::unsupportedWireNodeCount,
            &DRW_AcisProjectionReport::unsupportedSubshellNodeCount,
            &DRW_AcisProjectionReport::unsupportedPCurveNodeCount,
            &DRW_AcisProjectionReport::unsupportedCurveNodeCount,
            &DRW_AcisProjectionReport::unsupportedSurfaceNodeCount,
            &DRW_AcisProjectionReport::verticesWithoutResolvedPoint,
            &DRW_AcisProjectionReport::edgesWithoutResolvedEndpointPair,
            &DRW_AcisProjectionReport::edgesWithoutCurveFlag,
            &DRW_AcisProjectionReport::facesWithoutSurfaceFlag,
            &DRW_AcisProjectionReport::facesWithoutLoop
        }};
    bool everyKnownGapDetected = true;
    for (std::size_t DRW_AcisProjectionReport::*counter : knownGapCounters) {
        DRW_AcisBodyProjection candidate = gapFreeSubset;
        candidate.report.*counter = 1u;
        everyKnownGapDetected = everyKnownGapDetected
            && drw_acisProjectionCoverage(candidate)
                == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps;
    }
    t.expect(drw_acisProjectionCoverage(noProjection)
                     == DRW_AcisProjectionCoverage::NotProjected
                 && drw_acisProjectionCoverage(firstProjection)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(gapFreeSubset)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetNoKnownGaps
                 && everyKnownGapDetected
                 && gapFreeSubset.report.bodyTransformPresent
                 && !gapFreeSubset.report.bodyTransformsApplied
                 && !gapFreeSubset.report.sourceUnitScaleApplied
                 && !gapFreeSubset.report.brepValidityChecked,
             "ACIS coverage classifies only known projection gaps, not transform, unit, or B-rep validity");
    DRW_AcisBodyProjection missingBodyIdentity = gapFreeSubset;
    missingBodyIdentity.bodyNodeIndex = -1;
    DRW_AcisBodyProjection missingReachableBody = gapFreeSubset;
    for (int& nodeIndex : missingReachableBody.reachableNodeIndices) {
        if (nodeIndex == missingReachableBody.bodyNodeIndex)
            nodeIndex = -1;
    }
    DRW_AcisBodyProjection staleModelNodeCount = gapFreeSubset;
    ++staleModelNodeCount.report.modelNodeCount;
    const std::array<std::size_t DRW_AcisProjectionReport::*, 3>
        projectedGeometryCounts {{
            &DRW_AcisProjectionReport::vertexCount,
            &DRW_AcisProjectionReport::edgeCount,
            &DRW_AcisProjectionReport::faceCount
        }};
    bool everyGeometryCountMismatchDetected = true;
    for (std::size_t DRW_AcisProjectionReport::*count : projectedGeometryCounts) {
        DRW_AcisBodyProjection candidate = gapFreeSubset;
        ++(candidate.report.*count);
        everyGeometryCountMismatchDetected =
            everyGeometryCountMismatchDetected
            && drw_acisProjectionCoverage(candidate)
                == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps;
    }
    DRW_AcisBodyProjection duplicateLumpRow = gapFreeSubset;
    if (!duplicateLumpRow.wireframe.lumps.empty())
        duplicateLumpRow.wireframe.lumps.push_back(
            duplicateLumpRow.wireframe.lumps.front());
    DRW_AcisBodyProjection duplicateShellRow = gapFreeSubset;
    if (!duplicateShellRow.wireframe.shells.empty())
        duplicateShellRow.wireframe.shells.push_back(
            duplicateShellRow.wireframe.shells.front());
    DRW_AcisBodyProjection duplicateFaceRow = gapFreeSubset;
    if (!duplicateFaceRow.wireframe.faces.empty()) {
        duplicateFaceRow.wireframe.faces.push_back(
            duplicateFaceRow.wireframe.faces.front());
        ++duplicateFaceRow.report.faceCount;
    }
    const bool hasRootedParentRows =
        !gapFreeSubset.wireframe.lumps.empty()
        && !gapFreeSubset.wireframe.shells.empty()
        && !gapFreeSubset.wireframe.faces.empty();
    DRW_AcisBodyProjection mismatchedLumpBody = gapFreeSubset;
    DRW_AcisBodyProjection mismatchedShellLump = gapFreeSubset;
    DRW_AcisBodyProjection mismatchedFaceShell = gapFreeSubset;
    if (hasRootedParentRows) {
        mismatchedLumpBody.wireframe.lumps.front().bodyNodeIndex =
            std::numeric_limits<int>::max();
        mismatchedShellLump.wireframe.shells.front().lumpNodeIndex =
            std::numeric_limits<int>::max();
        mismatchedFaceShell.wireframe.faces.front().shellNodeIndex =
            std::numeric_limits<int>::max();
    }
    const bool hasRootedGeometryRows =
        !gapFreeSubset.wireframe.vertices.empty()
        && !gapFreeSubset.wireframe.edges.empty()
        && !gapFreeSubset.wireframe.faces.empty()
        && !gapFreeSubset.wireframe.faces.front().loops.empty();
    bool hasTypedGraphLinkRows = false;
    bool everyTypedGraphLinkMutationDetected = true;
    if (hasRootedGeometryRows
        && !gapFreeSubset.lumpNodeIndices.empty()
        && !gapFreeSubset.wireframe.lumps.empty()
        && !gapFreeSubset.wireframe.shells.empty()
        && gapFreeSubset.bodyTransformNodeIndex >= 0
        && !gapFreeSubset.wireframe.faces.front().loops.front()
                .coedgeNodeIndices.empty()) {
        const DRW_AcisFace& face = gapFreeSubset.wireframe.faces.front();
        const DRW_AcisFaceLoop& loop = face.loops.front();
        const DRW_AcisLump& lump = gapFreeSubset.wireframe.lumps.front();
        const DRW_AcisShell& shell = gapFreeSubset.wireframe.shells.front();
        const int bodyNodeIndex = gapFreeSubset.bodyNodeIndex;
        const int coedgeNodeIndex = loop.coedgeNodeIndices.front();
        const std::array<int, 21> linkSources {{
            bodyNodeIndex, bodyNodeIndex,
            lump.nodeIndex, lump.nodeIndex, lump.nodeIndex,
            shell.nodeIndex, shell.nodeIndex, shell.nodeIndex,
            face.nodeIndex, face.nodeIndex, face.nodeIndex, face.nodeIndex,
            loop.nodeIndex, loop.nodeIndex, loop.nodeIndex,
            coedgeNodeIndex, coedgeNodeIndex, coedgeNodeIndex,
            coedgeNodeIndex, coedgeNodeIndex, coedgeNodeIndex
        }};
        const std::array<std::size_t, 21> linkOrdinals {{
            1u, 3u,
            1u, 2u, 3u,
            1u, 3u, 5u,
            1u, 2u, 3u, 5u,
            1u, 2u, 3u,
            1u, 2u, 3u, 4u, 5u, 6u
        }};
        hasTypedGraphLinkRows = true;
        for (std::size_t index = 0u; index < linkSources.size(); ++index) {
            DRW_AcisBodyProjection candidate = gapFreeSubset;
            const bool changed = setProjectionGraphLinkTarget(
                candidate, linkSources[index], linkOrdinals[index],
                std::numeric_limits<int>::max());
            hasTypedGraphLinkRows = hasTypedGraphLinkRows && changed;
            everyTypedGraphLinkMutationDetected =
                everyTypedGraphLinkMutationDetected && changed
                && drw_acisProjectionCoverage(candidate)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps;
        }
    } else {
        everyTypedGraphLinkMutationDetected = false;
    }
    DRW_AcisBodyProjection mismatchedVertexEdgeIncidence = gapFreeSubset;
    bool vertexEdgeIncidenceControlDetected = false;
    if (gapFreeSubset.wireframe.vertices.size() >= 2u
        && !gapFreeSubset.wireframe.edges.empty()
        && !gapFreeSubset.reachableNodeIndices.empty()) {
        const int vertex0 =
            gapFreeSubset.wireframe.vertices[0].nodeIndex;
        const int vertex1 =
            gapFreeSubset.wireframe.vertices[1].nodeIndex;
        const int edgeSource =
            gapFreeSubset.wireframe.edges.front().nodeIndex;
        const int maximumReachable = *std::max_element(
            gapFreeSubset.reachableNodeIndices.begin(),
            gapFreeSubset.reachableNodeIndices.end());
        if (maximumReachable < std::numeric_limits<int>::max()) {
            const int detachedEdgeSource = maximumReachable + 1;
            std::vector<DRW_AcisBodyGraphLink> detachedEdgeLinks;
            for (const DRW_AcisBodyGraphLink& link :
                     mismatchedVertexEdgeIncidence.graphLinks) {
                if (link.sourceNodeIndex != edgeSource)
                    continue;
                DRW_AcisBodyGraphLink detachedLink = link;
                detachedLink.sourceNodeIndex = detachedEdgeSource;
                if (detachedLink.pointerOrdinal == 1u
                    || detachedLink.pointerOrdinal == 2u)
                    detachedLink.targetNodeIndex = vertex1;
                detachedEdgeLinks.push_back(detachedLink);
            }
            if (detachedEdgeLinks.size() == 5u) {
                mismatchedVertexEdgeIncidence.graphLinks.insert(
                    mismatchedVertexEdgeIncidence.graphLinks.end(),
                    detachedEdgeLinks.begin(), detachedEdgeLinks.end());
                mismatchedVertexEdgeIncidence.reachableNodeIndices.push_back(
                    detachedEdgeSource);
                mismatchedVertexEdgeIncidence.report.modelNodeCount =
                    mismatchedVertexEdgeIncidence.reachableNodeIndices.size();
                const bool detachedRowIsOtherwiseCoherent =
                    drw_acisProjectionCoverage(mismatchedVertexEdgeIncidence)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetNoKnownGaps;
                const bool redirectedVertex = setProjectionGraphLinkTarget(
                    mismatchedVertexEdgeIncidence, vertex0, 1u,
                    detachedEdgeSource);
                vertexEdgeIncidenceControlDetected =
                    detachedRowIsOtherwiseCoherent && redirectedVertex
                    && drw_acisProjectionCoverage(
                           mismatchedVertexEdgeIncidence)
                        == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps;
            }
        }
    }
    t.expect(vertexEdgeIncidenceControlDetected,
             "ACIS coverage rejects a vertex-to-edge link that is not incident to that vertex");
    DRW_AcisBodyProjection missingVertexSource = gapFreeSubset;
    DRW_AcisBodyProjection mismatchedVertexPoint = gapFreeSubset;
    DRW_AcisBodyProjection missingEdgeSource = gapFreeSubset;
    DRW_AcisBodyProjection mismatchedEdgeEndpoint = gapFreeSubset;
    DRW_AcisBodyProjection mismatchedEdgeCoedge = gapFreeSubset;
    DRW_AcisBodyProjection mismatchedEdgeCurve = gapFreeSubset;
    DRW_AcisBodyProjection mismatchedEdgeLinkOrdinal = gapFreeSubset;
    DRW_AcisBodyProjection mismatchedEdgeLinkTarget = gapFreeSubset;
    DRW_AcisBodyProjection negativeGraphLinkSource = gapFreeSubset;
    DRW_AcisBodyProjection malformedNonGeometryLinkOrdinal = gapFreeSubset;
    DRW_AcisBodyProjection duplicateGraphLinkSourceGroup = gapFreeSubset;
    DRW_AcisBodyProjection unreachableVertexSource = gapFreeSubset;
    DRW_AcisBodyProjection duplicateReachableNode = gapFreeSubset;
    DRW_AcisBodyProjection malformedNegativeGraphLinkTarget = gapFreeSubset;
    DRW_AcisBodyProjection missingFaceSource = gapFreeSubset;
    DRW_AcisBodyProjection mismatchedFaceLoop = gapFreeSubset;
    DRW_AcisBodyProjection mismatchedFaceSurface = gapFreeSubset;
    if (hasRootedGeometryRows) {
        missingVertexSource.wireframe.vertices.front().nodeIndex =
            std::numeric_limits<int>::max();
        mismatchedVertexPoint.wireframe.vertices.front().pointNodeIndex =
            std::numeric_limits<int>::max();
        missingEdgeSource.wireframe.edges.front().nodeIndex =
            std::numeric_limits<int>::max();
        mismatchedEdgeEndpoint.wireframe.edges.front().endVertexNodeIndex =
            std::numeric_limits<int>::max();
        mismatchedEdgeCoedge.wireframe.edges.front().coedgeNodeIndex =
            std::numeric_limits<int>::max();
        mismatchedEdgeCurve.wireframe.edges.front().curveNodeIndex =
            std::numeric_limits<int>::max();
        const int vertexSourceNode =
            unreachableVertexSource.wireframe.vertices.front().nodeIndex;
        const auto reachableVertex = std::find(
            unreachableVertexSource.reachableNodeIndices.begin(),
            unreachableVertexSource.reachableNodeIndices.end(),
            vertexSourceNode);
        if (reachableVertex != unreachableVertexSource.reachableNodeIndices.end()) {
            unreachableVertexSource.reachableNodeIndices.erase(reachableVertex);
            unreachableVertexSource.report.modelNodeCount =
                unreachableVertexSource.reachableNodeIndices.size();
        }
        duplicateReachableNode.reachableNodeIndices.push_back(
            duplicateReachableNode.bodyNodeIndex);
        duplicateReachableNode.report.modelNodeCount =
            duplicateReachableNode.reachableNodeIndices.size();
        for (DRW_AcisBodyGraphLink& link :
                 mismatchedEdgeLinkOrdinal.graphLinks) {
            if (link.sourceNodeIndex
                    == mismatchedEdgeLinkOrdinal.wireframe.edges.front().nodeIndex
                && link.pointerOrdinal == 2u) {
                link.pointerOrdinal = 1u;
                break;
            }
        }
        for (DRW_AcisBodyGraphLink& link :
                 mismatchedEdgeLinkTarget.graphLinks) {
            if (link.sourceNodeIndex
                    == mismatchedEdgeLinkTarget.wireframe.edges.front().nodeIndex
                && link.pointerOrdinal == 4u) {
                link.targetNodeIndex = std::numeric_limits<int>::max();
                break;
            }
        }
        if (!negativeGraphLinkSource.graphLinks.empty()) {
            negativeGraphLinkSource.graphLinks.front().sourceNodeIndex = -1;
            malformedNonGeometryLinkOrdinal.graphLinks.front()
                .pointerOrdinal = 1u;
            malformedNegativeGraphLinkTarget.graphLinks.front()
                .targetNodeIndex = -2;
            std::size_t firstGroupCount = 1u;
            while (firstGroupCount
                       < duplicateGraphLinkSourceGroup.graphLinks.size()
                   && duplicateGraphLinkSourceGroup
                              .graphLinks[firstGroupCount].sourceNodeIndex
                       == duplicateGraphLinkSourceGroup
                              .graphLinks.front().sourceNodeIndex) {
                ++firstGroupCount;
            }
            for (std::size_t index = 0u; index < firstGroupCount; ++index) {
                duplicateGraphLinkSourceGroup.graphLinks.push_back(
                    duplicateGraphLinkSourceGroup.graphLinks[index]);
            }
        }
        missingFaceSource.wireframe.faces.front().nodeIndex =
            std::numeric_limits<int>::max();
        mismatchedFaceLoop.wireframe.faces.front().loops.front().nodeIndex =
            std::numeric_limits<int>::max();
        mismatchedFaceSurface.wireframe.faces.front().surfaceNodeIndex =
            std::numeric_limits<int>::max();
    }
    t.expect(drw_acisProjectionCoverage(missingBodyIdentity)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(missingReachableBody)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(staleModelNodeCount)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && everyGeometryCountMismatchDetected
                 && drw_acisProjectionCoverage(duplicateLumpRow)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(duplicateShellRow)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(duplicateFaceRow)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && hasRootedParentRows
                 && drw_acisProjectionCoverage(mismatchedLumpBody)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(mismatchedShellLump)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(mismatchedFaceShell)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "ACIS coverage rejects stale counts, hierarchy row counts, and mismatched retained parent identities");
    t.expect(hasRootedGeometryRows
                 && drw_acisProjectionCoverage(missingVertexSource)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(mismatchedVertexPoint)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(missingEdgeSource)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(mismatchedEdgeEndpoint)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(mismatchedEdgeCoedge)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(mismatchedEdgeCurve)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(mismatchedEdgeLinkOrdinal)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(mismatchedEdgeLinkTarget)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(negativeGraphLinkSource)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(
                        malformedNonGeometryLinkOrdinal)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(
                        duplicateGraphLinkSourceGroup)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(unreachableVertexSource)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(duplicateReachableNode)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(
                        malformedNegativeGraphLinkTarget)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(missingFaceSource)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(mismatchedFaceLoop)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(mismatchedFaceSurface)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && hasTypedGraphLinkRows
                 && everyTypedGraphLinkMutationDetected,
             "ACIS coverage reconciles body, hierarchy, geometry, loop, and coedge identities against graph-link rows");
    DRW_AcisBodyProjection missingShellFaceRows = gapFreeSubset;
    missingShellFaceRows.wireframe.shells.clear();
    DRW_AcisBodyProjection missingFaceSense = gapFreeSubset;
    if (!missingFaceSense.wireframe.faces.empty())
        missingFaceSense.wireframe.faces.front().hasSense = false;
    DRW_AcisBodyProjection missingCoedgeSense = gapFreeSubset;
    if (!missingCoedgeSense.wireframe.faces.empty()
        && !missingCoedgeSense.wireframe.faces.front().loops.empty()
        && !missingCoedgeSense.wireframe.faces.front().loops.front()
                .coedgeSenseFlags.empty())
        missingCoedgeSense.wireframe.faces.front().loops.front()
            .coedgeSenseFlags.front() = -1;
    DRW_AcisBodyProjection inconsistentCoedgeOrientedEndpoint =
        gapFreeSubset;
    if (!inconsistentCoedgeOrientedEndpoint.wireframe.faces.empty()
        && !inconsistentCoedgeOrientedEndpoint.wireframe.faces.front()
                .loops.empty()
        && !inconsistentCoedgeOrientedEndpoint.wireframe.faces.front()
                .loops.front().coedgeOrientedStartVertexNodeIndices.empty()) {
        inconsistentCoedgeOrientedEndpoint.wireframe.faces.front()
            .loops.front().coedgeOrientedStartVertexNodeIndices.front() += 1;
    }
    DRW_AcisBodyProjection inconsistentCoedgeEndpointChainObservation =
        gapFreeSubset;
    if (!inconsistentCoedgeEndpointChainObservation.wireframe.faces.empty()
        && !inconsistentCoedgeEndpointChainObservation.wireframe.faces.front()
                .loops.empty()) {
        ++inconsistentCoedgeEndpointChainObservation.wireframe.faces.front()
              .loops.front().coedgeEndpointDiscontinuityCount;
    }
    DRW_AcisBodyProjection disconnectedCoedgeEndpointChain = gapFreeSubset;
    if (!disconnectedCoedgeEndpointChain.wireframe.faces.empty()
        && !disconnectedCoedgeEndpointChain.wireframe.faces.front()
                .loops.empty()
        && !disconnectedCoedgeEndpointChain.wireframe.edges.empty()) {
        DRW_AcisFaceLoop& loop = disconnectedCoedgeEndpointChain.wireframe
                                     .faces.front().loops.front();
        loop.coedgeEdgeEndVertexNodeIndices.front() = first.vertex1;
        loop.coedgeOrientedEndVertexNodeIndices.front() = first.vertex1;
        loop.coedgeEndpointChainContinuous = false;
        loop.coedgeEndpointDiscontinuityCount = 1u;
        loop.firstCoedgeEndpointDiscontinuityNodeIndex =
            loop.coedgeNodeIndices.front();
        disconnectedCoedgeEndpointChain.wireframe.edges.front()
            .endVertexNodeIndex = first.vertex1;
    }
    DRW_AcisBodyProjection misalignedCoedgeNext = gapFreeSubset;
    if (!misalignedCoedgeNext.wireframe.faces.empty()
        && !misalignedCoedgeNext.wireframe.faces.front().loops.empty()) {
        auto& next = misalignedCoedgeNext.wireframe.faces.front()
                         .loops.front().coedgeNextNodeIndices;
        if (!next.empty())
            next.pop_back();
    }
    DRW_AcisBodyProjection misalignedCoedgePrevious = gapFreeSubset;
    if (!misalignedCoedgePrevious.wireframe.faces.empty()
        && !misalignedCoedgePrevious.wireframe.faces.front().loops.empty()) {
        auto& previous = misalignedCoedgePrevious.wireframe.faces.front()
                             .loops.front().coedgePreviousNodeIndices;
        if (!previous.empty())
            previous.pop_back();
    }
    DRW_AcisBodyProjection misalignedCoedgePartner = gapFreeSubset;
    if (!misalignedCoedgePartner.wireframe.faces.empty()
        && !misalignedCoedgePartner.wireframe.faces.front().loops.empty()) {
        auto& partner = misalignedCoedgePartner.wireframe.faces.front()
                            .loops.front().coedgePartnerNodeIndices;
        if (!partner.empty())
            partner.pop_back();
    }
    DRW_AcisBodyProjection unclosedEdgePartnerRing = gapFreeSubset;
    if (!unclosedEdgePartnerRing.wireframe.edges.empty())
        unclosedEdgePartnerRing.wireframe.edges.front()
            .coedgePartnerRingClosed = false;
    DRW_AcisBodyProjection misalignedEdgePartnerRing = gapFreeSubset;
    if (!misalignedEdgePartnerRing.wireframe.edges.empty()) {
        auto& targets = misalignedEdgePartnerRing.wireframe.edges.front()
                            .coedgePartnerRingNextNodeIndices;
        if (!targets.empty())
            targets.pop_back();
    }
    DRW_AcisBodyProjection inconsistentEdgePartnerRing = gapFreeSubset;
    if (!inconsistentEdgePartnerRing.wireframe.edges.empty()) {
        auto& targets = inconsistentEdgePartnerRing.wireframe.edges.front()
                            .coedgePartnerRingNextNodeIndices;
        if (!targets.empty())
            targets.back() = -1;
    }
    DRW_AcisBodyProjection missingEdgeUseIndex = gapFreeSubset;
    if (!missingEdgeUseIndex.wireframe.edges.empty())
        missingEdgeUseIndex.wireframe.edges.front()
            .coedgeEdgeUseNodeIndices.clear();
    DRW_AcisBodyProjection falseEdgeUseIndexClaim = gapFreeSubset;
    if (!falseEdgeUseIndexClaim.wireframe.edges.empty()) {
        DRW_AcisEdge& edge = falseEdgeUseIndexClaim.wireframe.edges.front();
        if (!edge.coedgeEdgeUseNodeIndices.empty())
            edge.coedgeEdgeUseNodeIndices.front() += 100;
    }
    DRW_AcisBodyProjection edgeUseIndexWithoutObservation = gapFreeSubset;
    if (!edgeUseIndexWithoutObservation.wireframe.edges.empty())
        edgeUseIndexWithoutObservation.wireframe.edges.front()
            .hasCoedgeEdgeUseIndexObservation = false;
    DRW_AcisBodyProjection incompleteFaceLoopIndex = gapFreeSubset;
    if (!incompleteFaceLoopIndex.wireframe.faces.empty()) {
        DRW_AcisFace& face = incompleteFaceLoopIndex.wireframe.faces.front();
        face.loopUseIndexCoversParsedLoops = false;
        face.loopUseIndexUnclassifiedLoopCount = 1u;
        face.firstUnclassifiedLoopOwnerNodeIndex = 123;
    }
    DRW_AcisBodyProjection falseFaceLoopIndexClaim = gapFreeSubset;
    if (!falseFaceLoopIndexClaim.wireframe.faces.empty()) {
        DRW_AcisFace& face = falseFaceLoopIndexClaim.wireframe.faces.front();
        if (!face.loopUseNodeIndices.empty())
            face.loopUseNodeIndices.front() += 100;
    }
    DRW_AcisBodyProjection faceLoopIndexWithoutObservation = gapFreeSubset;
    if (!faceLoopIndexWithoutObservation.wireframe.faces.empty())
        faceLoopIndexWithoutObservation.wireframe.faces.front()
            .hasLoopUseIndexObservation = false;
    DRW_AcisBodyProjection unterminatedFaceLoopChain = gapFreeSubset;
    if (!unterminatedFaceLoopChain.wireframe.faces.empty())
        unterminatedFaceLoopChain.wireframe.faces.front()
            .loopChainNullTerminated = false;
    DRW_AcisBodyProjection cyclicFaceLoopChain = gapFreeSubset;
    if (!cyclicFaceLoopChain.wireframe.faces.empty()) {
        DRW_AcisFace& face = cyclicFaceLoopChain.wireframe.faces.front();
        face.loopChainNullTerminated = false;
        face.loopChainCycleDetected = true;
        face.firstRepeatedLoopChainNodeIndex = face.loops.front().nodeIndex;
        if (!face.loopNextNodeIndices.empty())
            face.loopNextNodeIndices.back() = face.loops.front().nodeIndex;
    }
    DRW_AcisBodyProjection falseFaceLoopNextTarget = gapFreeSubset;
    if (!falseFaceLoopNextTarget.wireframe.faces.empty()) {
        DRW_AcisFace& face = falseFaceLoopNextTarget.wireframe.faces.front();
        if (!face.loopNextNodeIndices.empty())
            face.loopNextNodeIndices.front() = 123;
    }
    DRW_AcisBodyProjection faceLoopChainWithoutObservation = gapFreeSubset;
    if (!faceLoopChainWithoutObservation.wireframe.faces.empty())
        faceLoopChainWithoutObservation.wireframe.faces.front()
            .hasLoopChainObservation = false;
    DRW_AcisBodyProjection incompleteShellFaceUseIndex = gapFreeSubset;
    if (!incompleteShellFaceUseIndex.wireframe.shells.empty()) {
        DRW_AcisShell& shell =
            incompleteShellFaceUseIndex.wireframe.shells.front();
        shell.faceUseIndexCoversParsedFaces = false;
        shell.faceUseIndexUnclassifiedFaceCount = 1u;
        shell.firstUnclassifiedFaceOwnerNodeIndex = 123;
        shell.faceChainMatchesUseIndex = false;
    }
    DRW_AcisBodyProjection falseShellFaceUseIndex = gapFreeSubset;
    if (!falseShellFaceUseIndex.wireframe.shells.empty()) {
        DRW_AcisShell& shell = falseShellFaceUseIndex.wireframe.shells.front();
        if (!shell.faceUseNodeIndices.empty())
            shell.faceUseNodeIndices.front() += 100;
    }
    DRW_AcisBodyProjection shellFaceChainWithoutObservation = gapFreeSubset;
    if (!shellFaceChainWithoutObservation.wireframe.shells.empty())
        shellFaceChainWithoutObservation.wireframe.shells.front()
            .hasFaceChainObservation = false;
    DRW_AcisBodyProjection misalignedShellFaceNext = gapFreeSubset;
    if (!misalignedShellFaceNext.wireframe.shells.empty()) {
        auto& next = misalignedShellFaceNext.wireframe.shells.front()
                         .faceNextNodeIndices;
        if (!next.empty())
            next.pop_back();
    }
    DRW_AcisBodyProjection incompleteLumpShellUseIndex = gapFreeSubset;
    if (!incompleteLumpShellUseIndex.wireframe.lumps.empty()) {
        DRW_AcisLump& lump =
            incompleteLumpShellUseIndex.wireframe.lumps.front();
        lump.shellUseIndexCoversParsedShells = false;
        lump.shellUseIndexUnclassifiedShellCount = 1u;
        lump.firstUnclassifiedShellOwnerNodeIndex = 123;
        lump.shellChainMatchesUseIndex = false;
    }
    DRW_AcisBodyProjection falseLumpShellUseIndex = gapFreeSubset;
    if (!falseLumpShellUseIndex.wireframe.lumps.empty()) {
        DRW_AcisLump& lump = falseLumpShellUseIndex.wireframe.lumps.front();
        if (!lump.shellUseNodeIndices.empty())
            lump.shellUseNodeIndices.front() += 100;
    }
    DRW_AcisBodyProjection lumpShellChainWithoutObservation = gapFreeSubset;
    if (!lumpShellChainWithoutObservation.wireframe.lumps.empty())
        lumpShellChainWithoutObservation.wireframe.lumps.front()
            .hasShellChainObservation = false;
    DRW_AcisBodyProjection misalignedLumpShellNext = gapFreeSubset;
    if (!misalignedLumpShellNext.wireframe.lumps.empty()) {
        auto& next = misalignedLumpShellNext.wireframe.lumps.front()
                         .shellNextNodeIndices;
        if (!next.empty())
            next.pop_back();
    }
    DRW_AcisBodyProjection missingBodyLumpRows = gapFreeSubset;
    missingBodyLumpRows.hasLumpChainObservation = false;
    DRW_AcisBodyProjection incompleteBodyLumpUseIndex = gapFreeSubset;
    incompleteBodyLumpUseIndex.lumpUseIndexCoversParsedLumps = false;
    incompleteBodyLumpUseIndex.lumpUseIndexUnclassifiedLumpCount = 1u;
    incompleteBodyLumpUseIndex.firstUnclassifiedLumpOwnerNodeIndex = 123;
    incompleteBodyLumpUseIndex.lumpChainMatchesUseIndex = false;
    DRW_AcisBodyProjection falseBodyLumpUseIndex = gapFreeSubset;
    if (!falseBodyLumpUseIndex.lumpUseNodeIndices.empty())
        falseBodyLumpUseIndex.lumpUseNodeIndices.front() += 100;
    DRW_AcisBodyProjection misalignedBodyLumpNext = gapFreeSubset;
    if (!misalignedBodyLumpNext.lumpNextNodeIndices.empty())
        misalignedBodyLumpNext.lumpNextNodeIndices.pop_back();
    DRW_AcisBodyProjection incompleteEdgeUseIndex = gapFreeSubset;
    if (!incompleteEdgeUseIndex.wireframe.edges.empty()) {
        DRW_AcisEdge& edge = incompleteEdgeUseIndex.wireframe.edges.front();
        edge.coedgeEdgeUseIndexCoversParsedCoedges = false;
        edge.coedgeEdgeUseIndexUnclassifiedCoedgeCount = 1u;
        edge.firstUnclassifiedCoedgeEdgeUseNodeIndex = 123;
    }
    DRW_AcisBodyProjection inconsistentEdgeUseIndexCompleteness = gapFreeSubset;
    if (!inconsistentEdgeUseIndexCompleteness.wireframe.edges.empty()) {
        DRW_AcisEdge& edge =
            inconsistentEdgeUseIndexCompleteness.wireframe.edges.front();
        edge.coedgeEdgeUseIndexCoversParsedCoedges = true;
        edge.coedgeEdgeUseIndexUnclassifiedCoedgeCount = 1u;
        edge.firstUnclassifiedCoedgeEdgeUseNodeIndex = 123;
    }
    DRW_AcisBodyProjection incompleteLoopCoedgeUseIndex = gapFreeSubset;
    if (!incompleteLoopCoedgeUseIndex.wireframe.faces.empty()
        && !incompleteLoopCoedgeUseIndex.wireframe.faces.front().loops.empty()) {
        DRW_AcisFaceLoop& loop =
            incompleteLoopCoedgeUseIndex.wireframe.faces.front().loops.front();
        loop.coedgeUseIndexCoversParsedCoedges = false;
        loop.coedgeUseIndexUnclassifiedCoedgeCount = 1u;
        loop.firstUnclassifiedCoedgeOwnerNodeIndex = 123;
        loop.coedgeChainMatchesUseIndex = false;
    }
    DRW_AcisBodyProjection missingVertexPointIdentity = gapFreeSubset;
    if (!missingVertexPointIdentity.wireframe.vertices.empty())
        missingVertexPointIdentity.wireframe.vertices.front().pointNodeIndex = -1;
    DRW_AcisBodyProjection missingCurveNodeIdentity = gapFreeSubset;
    if (!missingCurveNodeIdentity.wireframe.edges.empty())
        missingCurveNodeIdentity.wireframe.edges.front().curveNodeIndex = -1;
    DRW_AcisBodyProjection missingEdgeStartVertexIdentity = gapFreeSubset;
    if (!missingEdgeStartVertexIdentity.wireframe.edges.empty())
        missingEdgeStartVertexIdentity.wireframe.edges.front()
            .startVertexNodeIndex = -1;
    DRW_AcisBodyProjection missingEdgeEndVertexIdentity = gapFreeSubset;
    if (!missingEdgeEndVertexIdentity.wireframe.edges.empty())
        missingEdgeEndVertexIdentity.wireframe.edges.front()
            .endVertexNodeIndex = -1;
    DRW_AcisBodyProjection falseLoopCoedgeUseIndex = gapFreeSubset;
    if (!falseLoopCoedgeUseIndex.wireframe.faces.empty()
        && !falseLoopCoedgeUseIndex.wireframe.faces.front().loops.empty()) {
        DRW_AcisFaceLoop& loop =
            falseLoopCoedgeUseIndex.wireframe.faces.front().loops.front();
        if (!loop.coedgeUseNodeIndices.empty())
            loop.coedgeUseNodeIndices.front() += 100;
    }
    DRW_AcisBodyProjection misalignedLoopCoedgeUseIndex = gapFreeSubset;
    if (!misalignedLoopCoedgeUseIndex.wireframe.faces.empty()
        && !misalignedLoopCoedgeUseIndex
                .wireframe.faces.front().loops.empty()) {
        auto& uses = misalignedLoopCoedgeUseIndex.wireframe.faces.front()
                         .loops.front().coedgeUseNodeIndices;
        if (!uses.empty())
            uses.pop_back();
    }
    DRW_AcisBodyProjection loopCoedgeUseIndexWithoutObservation =
        gapFreeSubset;
    if (!loopCoedgeUseIndexWithoutObservation.wireframe.faces.empty()
        && !loopCoedgeUseIndexWithoutObservation
                .wireframe.faces.front().loops.empty())
        loopCoedgeUseIndexWithoutObservation.wireframe.faces.front()
            .loops.front().hasCoedgeUseIndexObservation = false;
    DRW_AcisBodyProjection observedOpenLoopSubset = gapFreeSubset;
    if (!observedOpenLoopSubset.wireframe.faces.empty()
        && !observedOpenLoopSubset.wireframe.faces.front().loops.empty()) {
        DRW_AcisFaceLoop& loop =
            observedOpenLoopSubset.wireframe.faces.front().loops.front();
        loop.coedgeRingClosed = false;
        loop.coedgeRingOpen = true;
        if (!loop.coedgeNextNodeIndices.empty())
            loop.coedgeNextNodeIndices.back() = -1;
        if (!loop.coedgePreviousNodeIndices.empty())
            loop.coedgePreviousNodeIndices.front() = -1;
        if (!loop.coedgeNodeIndices.empty()) {
            setProjectionGraphLinkTarget(
                observedOpenLoopSubset, loop.coedgeNodeIndices.back(), 1u, -1);
            setProjectionGraphLinkTarget(
                observedOpenLoopSubset, loop.coedgeNodeIndices.front(), 2u, -1);
        }
    }
    DRW_AcisBodyProjection inconsistentOpenLoopSubset = gapFreeSubset;
    if (!inconsistentOpenLoopSubset.wireframe.faces.empty()
        && !inconsistentOpenLoopSubset.wireframe.faces.front().loops.empty()) {
        DRW_AcisFaceLoop& loop =
            inconsistentOpenLoopSubset.wireframe.faces.front().loops.front();
        loop.coedgeRingClosed = false;
        loop.coedgeRingOpen = true;
    }
    DRW_AcisBodyProjection missingContainment = gapFreeSubset;
    if (!missingContainment.wireframe.faces.empty()) {
        DRW_AcisFace& face = missingContainment.wireframe.faces.front();
        face.doubleSided = true;
        face.hasContainment = false;
    }
    DRW_AcisBodyProjection zeroDirectionStraightCurve = gapFreeSubset;
    bool zeroDirectionStraightCurveFound = false;
    for (DRW_AcisEdge& edge : zeroDirectionStraightCurve.wireframe.edges) {
        if (edge.nodeIndex >= 0) {
            edge.hasCurve = true;
            edge.curveType = DRW_AcisCurve::Straight;
            edge.p1 = DRW_Coord(0.0, 0.0, 0.0);
            zeroDirectionStraightCurveFound = true;
            break;
        }
    }
    t.expect(drw_acisProjectionCoverage(missingFaceSense)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(missingVertexPointIdentity)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(missingCurveNodeIdentity)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(
                        missingEdgeStartVertexIdentity)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(
                        missingEdgeEndVertexIdentity)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(missingCoedgeSense)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(
                        inconsistentCoedgeOrientedEndpoint)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(
                        inconsistentCoedgeEndpointChainObservation)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(
                        disconnectedCoedgeEndpointChain)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(misalignedCoedgeNext)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(misalignedCoedgePrevious)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(misalignedCoedgePartner)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(unclosedEdgePartnerRing)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(misalignedEdgePartnerRing)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(inconsistentEdgePartnerRing)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(missingEdgeUseIndex)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(falseEdgeUseIndexClaim)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(
                        edgeUseIndexWithoutObservation)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(incompleteEdgeUseIndex)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(
                        inconsistentEdgeUseIndexCompleteness)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(
                        incompleteLoopCoedgeUseIndex)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(falseLoopCoedgeUseIndex)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(
                        misalignedLoopCoedgeUseIndex)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(
                        loopCoedgeUseIndexWithoutObservation)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(incompleteFaceLoopIndex)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(falseFaceLoopIndexClaim)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(
                        faceLoopIndexWithoutObservation)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(unterminatedFaceLoopChain)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(cyclicFaceLoopChain)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(falseFaceLoopNextTarget)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(
                        faceLoopChainWithoutObservation)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(incompleteShellFaceUseIndex)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(falseShellFaceUseIndex)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(
                        shellFaceChainWithoutObservation)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(missingShellFaceRows)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(misalignedShellFaceNext)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(
                        incompleteLumpShellUseIndex)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(falseLumpShellUseIndex)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(
                        lumpShellChainWithoutObservation)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(misalignedLumpShellNext)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(missingBodyLumpRows)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(incompleteBodyLumpUseIndex)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(falseBodyLumpUseIndex)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(misalignedBodyLumpNext)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(observedOpenLoopSubset)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetNoKnownGaps
                 && drw_acisProjectionCoverage(inconsistentOpenLoopSubset)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && drw_acisProjectionCoverage(missingContainment)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "ACIS coverage marks missing face/coedge orientation, misaligned incidence rows, and conditional containment as gaps");
    t.expect(zeroDirectionStraightCurveFound
                 && drw_acisProjectionCoverage(zeroDirectionStraightCurve)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "ACIS rooted coverage classifies a degenerate straight-curve direction as a known gap");
    DRW_AcisBodyProjection missingEdgeStartObservation = gapFreeSubset;
    if (!missingEdgeStartObservation.wireframe.edges.empty())
        missingEdgeStartObservation.wireframe.edges.front().hasStart = false;
    DRW_AcisBodyProjection missingEdgeEndObservation = gapFreeSubset;
    if (!missingEdgeEndObservation.wireframe.edges.empty())
        missingEdgeEndObservation.wireframe.edges.front().hasEnd = false;
    DRW_AcisBodyProjection missingEdgeCurveObservation = gapFreeSubset;
    if (!missingEdgeCurveObservation.wireframe.edges.empty())
        missingEdgeCurveObservation.wireframe.edges.front().hasCurve = false;
    DRW_AcisBodyProjection missingFaceSurfaceObservation = gapFreeSubset;
    if (!missingFaceSurfaceObservation.wireframe.faces.empty())
        missingFaceSurfaceObservation.wireframe.faces.front().hasSurface = false;
    DRW_AcisBodyProjection missingFaceLoopObservation = gapFreeSubset;
    if (!missingFaceLoopObservation.wireframe.faces.empty()) {
        DRW_AcisFace& face = missingFaceLoopObservation.wireframe.faces.front();
        face.loops.clear();
        face.loopNextNodeIndices.clear();
    }
    t.expect(!missingEdgeStartObservation.wireframe.edges.empty()
                 && drw_acisProjectionCoverage(missingEdgeStartObservation)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && !missingEdgeEndObservation.wireframe.edges.empty()
                 && drw_acisProjectionCoverage(missingEdgeEndObservation)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && !missingEdgeCurveObservation.wireframe.edges.empty()
                 && drw_acisProjectionCoverage(missingEdgeCurveObservation)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && !missingFaceSurfaceObservation.wireframe.faces.empty()
                 && drw_acisProjectionCoverage(missingFaceSurfaceObservation)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && !missingFaceLoopObservation.wireframe.faces.empty()
                 && drw_acisProjectionCoverage(missingFaceLoopObservation)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "ACIS rooted coverage detects missing geometry observations even when counters are zero");
    const auto ellipseCoverageFor = [&gapFreeSubset](
        const DRW_Coord& normal, const DRW_Coord& majorAxis,
        double ratio) {
        DRW_AcisBodyProjection candidate = gapFreeSubset;
        if (candidate.wireframe.edges.empty())
            return DRW_AcisProjectionCoverage::NotProjected;
        DRW_AcisEdge& edge = candidate.wireframe.edges.front();
        edge.hasCurve = true;
        edge.curveType = DRW_AcisCurve::Ellipse;
        edge.p1 = normal;
        edge.p2 = majorAxis;
        edge.ratio = ratio;
        return drw_acisProjectionCoverage(candidate);
    };
    t.expect(ellipseCoverageFor(DRW_Coord(0.0, 0.0, 1.0),
                                DRW_Coord(1.0, 0.0, 0.0), 0.5)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetNoKnownGaps
                 && ellipseCoverageFor(DRW_Coord(0.0, 0.0, 0.0),
                                       DRW_Coord(1.0, 0.0, 0.0), 0.5)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && ellipseCoverageFor(DRW_Coord(0.0, 0.0, 1.0),
                                       DRW_Coord(0.0, 0.0, 0.0), 0.5)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && ellipseCoverageFor(DRW_Coord(0.0, 0.0, 1.0),
                                       DRW_Coord(1.0, 0.0, 0.0), 0.0)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && ellipseCoverageFor(DRW_Coord(0.0, 0.0, 1.0),
                                       DRW_Coord(1.0, 0.0, 0.0), -0.5)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && ellipseCoverageFor(DRW_Coord(0.0, 0.0, 1.0),
                                       DRW_Coord(1.0, 0.0, 0.0),
                                       std::numeric_limits<double>::quiet_NaN())
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && ellipseCoverageFor(DRW_Coord(0.0, 0.0, 1.0),
                                       DRW_Coord(1.0, 0.0, 0.0),
                                       std::numeric_limits<double>::infinity())
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "ACIS rooted coverage requires a nondegenerate ellipse frame and a finite positive ratio without assuming its direction");
    const auto planeCoverageFor = [&gapFreeSubset](
        const DRW_Coord& normal) {
        DRW_AcisBodyProjection candidate = gapFreeSubset;
        if (candidate.wireframe.faces.empty())
            return DRW_AcisProjectionCoverage::NotProjected;
        DRW_AcisFace& face = candidate.wireframe.faces.front();
        face.hasSurface = true;
        face.surfaceType = DRW_AcisSurface::Plane;
        face.p1 = normal;
        return drw_acisProjectionCoverage(candidate);
    };
    t.expect(planeCoverageFor(DRW_Coord(0.0, 0.0, 2.0))
                     == DRW_AcisProjectionCoverage::ProjectedSubsetNoKnownGaps
                 && planeCoverageFor(DRW_Coord(0.0, 0.0, 0.0))
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && planeCoverageFor(DRW_Coord(
                        0.0, std::numeric_limits<double>::infinity(), 1.0))
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "ACIS rooted coverage requires a finite nonzero plane normal without requiring unit length");
    const auto sphereCoverageFor = [&gapFreeSubset](double radius) {
        DRW_AcisBodyProjection candidate = gapFreeSubset;
        if (candidate.wireframe.faces.empty())
            return DRW_AcisProjectionCoverage::NotProjected;
        DRW_AcisFace& face = candidate.wireframe.faces.front();
        face.hasSurface = true;
        face.surfaceType = DRW_AcisSurface::Sphere;
        face.majorRadius = radius;
        return drw_acisProjectionCoverage(candidate);
    };
    t.expect(sphereCoverageFor(-7.5)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetNoKnownGaps
                 && sphereCoverageFor(0.0)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && sphereCoverageFor(
                        std::numeric_limits<double>::infinity())
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "ACIS rooted coverage requires a finite nonzero signed sphere radius");
    const auto coneCoverageFor = [&gapFreeSubset](
        const DRW_Coord& axis, const DRW_Coord& majorAxis,
        double ratio, double sineAngle, double cosineAngle) {
        DRW_AcisBodyProjection candidate = gapFreeSubset;
        if (candidate.wireframe.faces.empty())
            return DRW_AcisProjectionCoverage::NotProjected;
        DRW_AcisFace& face = candidate.wireframe.faces.front();
        face.hasSurface = true;
        face.surfaceType = DRW_AcisSurface::Cone;
        face.p0 = DRW_Coord(1.0, 2.0, 3.0);
        face.p1 = axis;
        face.p2 = majorAxis;
        face.ratio = ratio;
        face.sineAngle = sineAngle;
        face.cosineAngle = cosineAngle;
        return drw_acisProjectionCoverage(candidate);
    };
    t.expect(coneCoverageFor(DRW_Coord(0.0, 0.0, 1.0),
                             DRW_Coord(2.0, 0.0, 0.0), 0.5, 0.6, 0.8)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetNoKnownGaps
                 && coneCoverageFor(DRW_Coord(0.0, 0.0, 0.0),
                                    DRW_Coord(2.0, 0.0, 0.0),
                                    0.5, 0.6, 0.8)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && coneCoverageFor(DRW_Coord(0.0, 0.0, 1.0),
                                    DRW_Coord(0.0, 0.0, 0.0),
                                    0.5, 0.6, 0.8)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && coneCoverageFor(DRW_Coord(0.0, 0.0, 1.0),
                                    DRW_Coord(2.0, 0.0, 0.0),
                                    0.0, 0.6, 0.8)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && coneCoverageFor(DRW_Coord(0.0, 0.0, 1.0),
                                    DRW_Coord(2.0, 0.0, 0.0),
                                    0.5, 0.0, 0.0)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && coneCoverageFor(DRW_Coord(0.0, 0.0, 1.0),
                                    DRW_Coord(2.0, 0.0, 0.0),
                                    0.5, 0.6,
                                    std::numeric_limits<double>::infinity())
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "ACIS rooted coverage requires finite nondegenerate cone fields");
    const auto torusCoverageFor = [&gapFreeSubset](
        const DRW_Coord& axis, double majorRadius, double minorRadius) {
        DRW_AcisBodyProjection candidate = gapFreeSubset;
        if (candidate.wireframe.faces.empty())
            return DRW_AcisProjectionCoverage::NotProjected;
        DRW_AcisFace& face = candidate.wireframe.faces.front();
        face.hasSurface = true;
        face.surfaceType = DRW_AcisSurface::Torus;
        face.p0 = DRW_Coord(1.0, 2.0, 3.0);
        face.p1 = axis;
        face.majorRadius = majorRadius;
        face.minorRadius = minorRadius;
        return drw_acisProjectionCoverage(candidate);
    };
    t.expect(torusCoverageFor(DRW_Coord(0.0, 0.0, 2.0), 12.0, 2.5)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetNoKnownGaps
                 && torusCoverageFor(DRW_Coord(0.0, 0.0, 0.0),
                                     12.0, 2.5)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && torusCoverageFor(DRW_Coord(0.0, 0.0, 2.0),
                                     0.0, 2.5)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && torusCoverageFor(DRW_Coord(0.0, 0.0, 2.0),
                                     12.0, -2.5)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && torusCoverageFor(DRW_Coord(0.0, 0.0, 2.0),
                                     12.0,
                                     std::numeric_limits<double>::quiet_NaN())
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "ACIS rooted coverage requires a finite nondegenerate torus axis and radii");
    DRW_AcisBodyProjection nonFiniteVertex = gapFreeSubset;
    const bool hasCoverageVertex = !nonFiniteVertex.wireframe.vertices.empty();
    if (hasCoverageVertex) {
        nonFiniteVertex.wireframe.vertices.front().valid = true;
        nonFiniteVertex.wireframe.vertices.front().point.x =
            std::numeric_limits<double>::quiet_NaN();
    }
    t.expect(hasCoverageVertex
                 && drw_acisProjectionCoverage(nonFiniteVertex)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "ACIS rooted coverage refuses a valid-marked vertex with a non-finite point");
    DRW_AcisBodyProjection nonFiniteEdgeEndpoint = gapFreeSubset;
    const bool hasCoverageEdge = !nonFiniteEdgeEndpoint.wireframe.edges.empty();
    if (hasCoverageEdge) {
        DRW_AcisEdge& edge = nonFiniteEdgeEndpoint.wireframe.edges.front();
        edge.hasStart = true;
        edge.start.y = std::numeric_limits<double>::infinity();
    }
    DRW_AcisBodyProjection nonFiniteStraightOrigin = gapFreeSubset;
    if (!nonFiniteStraightOrigin.wireframe.edges.empty()) {
        DRW_AcisEdge& edge = nonFiniteStraightOrigin.wireframe.edges.front();
        edge.hasCurve = true;
        edge.curveType = DRW_AcisCurve::Straight;
        edge.p0.x = std::numeric_limits<double>::quiet_NaN();
        edge.p1 = DRW_Coord(1.0, 0.0, 0.0);
    }
    DRW_AcisBodyProjection nonFiniteEllipseCenter = gapFreeSubset;
    if (!nonFiniteEllipseCenter.wireframe.edges.empty()) {
        DRW_AcisEdge& edge = nonFiniteEllipseCenter.wireframe.edges.front();
        edge.hasCurve = true;
        edge.curveType = DRW_AcisCurve::Ellipse;
        edge.p0.z = std::numeric_limits<double>::infinity();
        edge.p1 = DRW_Coord(0.0, 0.0, 1.0);
        edge.p2 = DRW_Coord(1.0, 0.0, 0.0);
        edge.ratio = 0.5;
    }
    DRW_AcisBodyProjection emptyIntcurveData = gapFreeSubset;
    if (!emptyIntcurveData.wireframe.edges.empty()) {
        DRW_AcisEdge& edge = emptyIntcurveData.wireframe.edges.front();
        edge.hasCurve = true;
        edge.curveType = DRW_AcisCurve::Intcurve;
        edge.controlPoints.clear();
    }
    DRW_AcisBodyProjection nonFinitePlaneOrigin = gapFreeSubset;
    if (!nonFinitePlaneOrigin.wireframe.faces.empty()) {
        DRW_AcisFace& face = nonFinitePlaneOrigin.wireframe.faces.front();
        face.hasSurface = true;
        face.surfaceType = DRW_AcisSurface::Plane;
        face.p0.x = std::numeric_limits<double>::quiet_NaN();
        face.p1 = DRW_Coord(0.0, 0.0, 1.0);
    }
    DRW_AcisBodyProjection nonFiniteSphereCenter = gapFreeSubset;
    if (!nonFiniteSphereCenter.wireframe.faces.empty()) {
        DRW_AcisFace& face = nonFiniteSphereCenter.wireframe.faces.front();
        face.hasSurface = true;
        face.surfaceType = DRW_AcisSurface::Sphere;
        face.p0.y = std::numeric_limits<double>::infinity();
        face.majorRadius = 2.0;
    }
    t.expect(hasCoverageEdge
                 && drw_acisProjectionCoverage(nonFiniteEdgeEndpoint)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && !nonFiniteStraightOrigin.wireframe.edges.empty()
                 && drw_acisProjectionCoverage(nonFiniteStraightOrigin)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && !nonFiniteEllipseCenter.wireframe.edges.empty()
                 && drw_acisProjectionCoverage(nonFiniteEllipseCenter)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && !emptyIntcurveData.wireframe.edges.empty()
                 && drw_acisProjectionCoverage(emptyIntcurveData)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && !nonFinitePlaneOrigin.wireframe.faces.empty()
                 && drw_acisProjectionCoverage(nonFinitePlaneOrigin)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps
                 && !nonFiniteSphereCenter.wireframe.faces.empty()
                 && drw_acisProjectionCoverage(nonFiniteSphereCenter)
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "ACIS rooted coverage classifies non-finite endpoint and analytic geometry fields");
    std::size_t bodyPointerLinks = 0;
    bool retainedNullPointer = false;
    bool retainedLumpPointer = false;
    bool retainedTransformPointer = false;
    for (const DRW_AcisBodyGraphLink& link : firstProjection.graphLinks) {
        if (link.sourceNodeIndex != first.body)
            continue;
        ++bodyPointerLinks;
        if (link.pointerOrdinal == 0u && link.targetNodeIndex == -1)
            retainedNullPointer = true;
        if (link.pointerOrdinal == 1u && link.targetNodeIndex == first.lump)
            retainedLumpPointer = true;
        if (link.pointerOrdinal == 3u
            && link.targetNodeIndex == first.transform)
            retainedTransformPointer = true;
    }
    t.expect(bodyPointerLinks == 4u && retainedNullPointer
                 && retainedLumpPointer && retainedTransformPointer,
             "ACIS rooted graph links preserve pointer ordinals and null slots");

    DRW_AcisModel noTransform = model;
    setModelPointer(noTransform, first.body, 3u, -1);
    std::vector<DRW_AcisBodyProjection> noTransformProjection;
    DRW_AcisRootedProjectionReport noTransformReport;
    t.expect(drw_extractAcisBodyWireframes(
                 noTransform, noTransformProjection, noTransformReport)
                 && noTransformProjection.size() == 2u
                 && noTransformProjection.front().transform.state
                        == DRW_AcisTransformState::Absent
                 && !noTransformProjection.front().transform.pointerResolved
                 && noTransformProjection.front().transform.rawTokens.empty()
                 && !noTransformProjection.front().report.bodyTransformPresent,
             "ACIS rooted projection distinguishes absent transform from resolved transform tokens");

    DRW_AcisModel transformWithoutLiteral = model;
    std::vector<DRW_SabToken>& transformTokens =
        transformWithoutLiteral.nodes[
            static_cast<std::size_t>(first.transform)].record.tokens;
    transformTokens.erase(
        std::remove_if(transformTokens.begin(), transformTokens.end(),
                       [](const DRW_SabToken& token) {
                           return token.tag == DRW_SabTag::LiteralStr;
                       }),
        transformTokens.end());
    std::vector<DRW_AcisBodyProjection> rawOnlyTransformProjection;
    DRW_AcisRootedProjectionReport rawOnlyTransformReport;
    t.expect(drw_extractAcisBodyWireframes(
                 transformWithoutLiteral, rawOnlyTransformProjection,
                 rawOnlyTransformReport)
                 && rawOnlyTransformProjection.size() == 2u
                 && rawOnlyTransformProjection.front().transform.state
                        == DRW_AcisTransformState::UnsupportedVariant
                 && !rawOnlyTransformProjection.front()
                        .transform.rawLiteralPresent
                 && !rawOnlyTransformProjection.front()
                        .transform.candidateComponentsPresent
                 && rawOnlyTransformProjection.front()
                        .transform.rawTokens.size() == 3u,
             "ACIS rooted projection preserves bounded non-literal transform tokens as unsupported");

    DRW_AcisModel emptyTransform = model;
    emptyTransform.nodes[static_cast<std::size_t>(first.transform)]
        .record.tokens.clear();
    std::vector<DRW_AcisBodyProjection> emptyTransformProjection;
    DRW_AcisRootedProjectionReport emptyTransformReport;
    t.expect(drw_extractAcisBodyWireframes(
                 emptyTransform, emptyTransformProjection,
                 emptyTransformReport)
                 && emptyTransformProjection.size() == 2u
                 && emptyTransformProjection.front().transform.state
                        == DRW_AcisTransformState::Invalid
                 && emptyTransformProjection.front()
                        .transform.rawTokens.empty(),
             "ACIS rooted projection distinguishes an empty transform record from a bounded alternate grammar");

    const auto rejectsTransformLiteral = [&t, &model, &first](
        const std::string& literal, const std::string& label) {
        DRW_AcisModel invalidTransform = model;
        bool replaced = false;
        for (DRW_SabToken& token : invalidTransform.nodes[
                 static_cast<std::size_t>(first.transform)].record.tokens) {
            if (token.tag == DRW_SabTag::LiteralStr) {
                token.sval = literal;
                replaced = true;
                break;
            }
        }
        std::vector<DRW_AcisBodyProjection> invalidProjection;
        DRW_AcisRootedProjectionReport invalidReport;
        const bool extracted = drw_extractAcisBodyWireframes(
            invalidTransform, invalidProjection, invalidReport);
        t.expect(replaced && extracted && invalidProjection.size() == 2u
                     && invalidProjection.front().transform.state
                            == DRW_AcisTransformState::Invalid
                     && invalidProjection.front().transform.rawLiteralPresent
                     && invalidProjection.front().transform.rawLiteral == literal
                     && !invalidProjection.front().transform
                            .candidateComponentsPresent
                     && !invalidProjection.front().report.bodyTransformsApplied,
                 label);
    };
    rejectsTransformLiteral(
        "11 22 33",
        "ACIS transform parser rejects truncated candidate but preserves it");
    const std::string thirteenValueTransform =
        "1 2 3 4 5 6 7 8 9 10 11 12 13";
    DRW_AcisModel alternateLiteralTransform = model;
    bool replacedAlternateLiteral = false;
    for (DRW_SabToken& token : alternateLiteralTransform.nodes[
             static_cast<std::size_t>(first.transform)].record.tokens) {
        if (token.tag == DRW_SabTag::LiteralStr) {
            token.sval = thirteenValueTransform;
            replacedAlternateLiteral = true;
            break;
        }
    }
    std::vector<DRW_AcisBodyProjection> alternateLiteralProjection;
    DRW_AcisRootedProjectionReport alternateLiteralReport;
    t.expect(replacedAlternateLiteral && drw_extractAcisBodyWireframes(
                 alternateLiteralTransform, alternateLiteralProjection,
                 alternateLiteralReport)
                 && alternateLiteralProjection.size() == 2u
                 && alternateLiteralProjection.front().transform.state
                        == DRW_AcisTransformState::UnsupportedVariant
                 && alternateLiteralProjection.front().transform.rawLiteral
                        == thirteenValueTransform
                 && !alternateLiteralProjection.front().transform
                        .candidateComponentsPresent
                 && alternateLiteralProjection.front().transform.rawTokens.size()
                        == 5u,
             "ACIS transform parser preserves a bounded thirteen-value profile without treating it as malformed or applying it");
    rejectsTransformLiteral("1 2 3 4 5 6 7 8 9 10 11 inf",
                            "ACIS transform parser rejects non-finite candidate values");
    rejectsTransformLiteral("1 2 3 4 5 6 7 8 9 10 x 12",
                            "ACIS transform parser rejects malformed candidate values");
    rejectsTransformLiteral("1 2 3 4 5 6 7 8 9 10 11 12 13 14",
                            "ACIS transform parser rejects values beyond the characterized alternate profile");

    DRW_AcisModel tripletTransform = model;
    std::vector<DRW_SabToken>& tripletTokens = tripletTransform.nodes[
        static_cast<std::size_t>(first.transform)].record.tokens;
    tripletTokens = {acisLocation(1.0, 0.0, 0.0),
                     acisLocation(0.0, 1.0, 0.0),
                     acisLocation(0.0, 0.0, 1.0),
                     acisLocation(11.0, 12.0, 13.0), acisDouble(0.5),
                     acisLiteral("rotation-hint")};
    std::vector<DRW_AcisBodyProjection> tripletProjection;
    DRW_AcisRootedProjectionReport tripletReport;
    t.expect(drw_extractAcisBodyWireframes(
                 tripletTransform, tripletProjection, tripletReport)
                 && tripletProjection.size() == 2u
                 && tripletProjection.front().transform.state
                        == DRW_AcisTransformState::UnsupportedVariant
                 && tripletProjection.front().transform.rawLiteralPresent
                 && tripletProjection.front().transform.rawLiteral
                        == "rotation-hint"
                 && tripletProjection.front().transform.rawLiteralTokenIndex
                        == 5u
                 && tripletProjection.front().transform.rawTokens.size() == 6u
                 && tripletProjection.front().transform.rawTokens[0].tag
                        == DRW_SabTag::LocationVec
                 && acisCoordNear(
                        tripletProjection.front().transform.rawTokens[3].vec,
                        11.0, 12.0, 13.0)
                 && tripletProjection.front().transform.rawTokens[4].tag
                        == DRW_SabTag::Double
                 && std::fabs(
                        tripletProjection.front().transform.rawTokens[4].dval
                        - 0.5) < 1e-9
                 && tripletProjection.front().transform.rawTokens[5].sval
                        == "rotation-hint"
                 && !tripletProjection.front().transform
                        .candidateComponentsPresent
                 && !tripletProjection.front().report.bodyTransformsApplied,
             "ACIS rooted projection preserves triplet-plus-scalar transform tokens as an unapplied alternate grammar");

    DRW_AcisModel unresolvedTransform = model;
    unresolvedTransform.nodes[static_cast<std::size_t>(first.body)]
        .refs[3] = -1;
    std::vector<DRW_AcisBodyProjection> unresolvedTransformProjection;
    DRW_AcisRootedProjectionReport unresolvedTransformReport;
    t.expect(!drw_extractAcisBodyWireframes(
                 unresolvedTransform, unresolvedTransformProjection,
                 unresolvedTransformReport)
                 && unresolvedTransformProjection.empty()
                 && unresolvedTransformReport.failure
                        == DRW_AcisProjectionFailure::InvalidReference
                 && unresolvedTransformReport.failedNodeIndex == first.body
                 && unresolvedTransformReport.failedPointerOrdinal == 3u,
             "ACIS rooted projection rejects an unresolved transform pointer");

    DRW_AcisModel wrongTypeTransform = model;
    setModelPointer(wrongTypeTransform, first.body, 3u, first.point0);
    std::vector<DRW_AcisBodyProjection> wrongTypeTransformProjection;
    DRW_AcisRootedProjectionReport wrongTypeTransformReport;
    t.expect(!drw_extractAcisBodyWireframes(
                 wrongTypeTransform, wrongTypeTransformProjection,
                 wrongTypeTransformReport)
                 && wrongTypeTransformProjection.empty()
                 && wrongTypeTransformReport.failure
                        == DRW_AcisProjectionFailure::InvalidReference
                 && wrongTypeTransformReport.failedNodeIndex == first.body
                 && wrongTypeTransformReport.failedPointerOrdinal == 3u,
             "ACIS rooted projection rejects a wrong-type transform pointer");

    DRW_AcisModel whitespaceTransform = model;
    const std::string whitespaceLiteral =
        "11 22 33 44 55 66 77 88 99 111 122 133 \t\n";
    bool replacedTransformLiteral = false;
    for (DRW_SabToken& token : whitespaceTransform.nodes[
             static_cast<std::size_t>(first.transform)].record.tokens) {
        if (token.tag == DRW_SabTag::LiteralStr) {
            token.sval = whitespaceLiteral;
            replacedTransformLiteral = true;
            break;
        }
    }
    std::vector<DRW_AcisBodyProjection> whitespaceProjection;
    DRW_AcisRootedProjectionReport whitespaceReport;
    const bool whitespaceExtracted = drw_extractAcisBodyWireframes(
        whitespaceTransform, whitespaceProjection, whitespaceReport);
    t.expect(replacedTransformLiteral && whitespaceExtracted
                 && whitespaceProjection.size() == 2u
                 && whitespaceProjection.front().transform.state
                        == DRW_AcisTransformState::ParsedUnapplied
                 && whitespaceProjection.front().transform.rawLiteral
                        == whitespaceLiteral
                 && whitespaceProjection.front().transform
                        .candidateComponentsPresent
                 && whitespaceProjection.front().transform.candidateComponents
                        == expectedTransformComponents
                 && !whitespaceProjection.front().report.bodyTransformsApplied,
             "ACIS transform parser accepts exact finite tuple with trailing whitespace without applying it");

    DRW_AcisModel oversizedTransform = model;
    const std::string oversizedLiteral = "0123456789abcdefg";
    bool replacedOversizedLiteral = false;
    for (DRW_SabToken& token : oversizedTransform.nodes[
             static_cast<std::size_t>(first.transform)].record.tokens) {
        if (token.tag == DRW_SabTag::LiteralStr) {
            token.sval = oversizedLiteral;
            replacedOversizedLiteral = true;
            break;
        }
    }
    DRW_SabParseLimits transformStringLimit;
    transformStringLimit.maxStringBytes = oversizedLiteral.size() - 1u;
    std::vector<DRW_AcisBodyProjection> oversizedTransformProjection(1u);
    DRW_AcisRootedProjectionReport oversizedTransformReport;
    oversizedTransformReport.bodyCount = 999u;
    const bool oversizedTransformExtracted = drw_extractAcisBodyWireframes(
        oversizedTransform, oversizedTransformProjection,
        oversizedTransformReport, transformStringLimit);
    t.expect(replacedOversizedLiteral && !oversizedTransformExtracted
                 && oversizedTransformProjection.empty()
                 && oversizedTransformReport.failure
                        == DRW_AcisProjectionFailure::ResourceLimit,
             "ACIS rooted projection rejects and clears an over-limit transform token string");

    DRW_SabParseLimits transformCumulativeStringLimit;
    transformCumulativeStringLimit.maxStringBytes = oversizedLiteral.size();
    transformCumulativeStringLimit.maxCumulativeStringBytes =
        oversizedLiteral.size();
    std::vector<DRW_AcisBodyProjection> cumulativeStringProjection(1u);
    DRW_AcisRootedProjectionReport cumulativeStringReport;
    const bool cumulativeStringExtracted = drw_extractAcisBodyWireframes(
        oversizedTransform, cumulativeStringProjection,
        cumulativeStringReport, transformCumulativeStringLimit);
    t.expect(!cumulativeStringExtracted
                 && cumulativeStringProjection.empty()
                 && cumulativeStringReport.failure
                        == DRW_AcisProjectionFailure::ResourceLimit,
             "ACIS rooted projection charges duplicated transform literal copies to the cumulative string budget");

    AcisGraphFixture complexFixture;
    const SyntheticAcisBody root = addSyntheticAcisBody(complexFixture, 3.0);
    const int secondLump = complexFixture.addNode("lump", 4u);
    const int secondShell = complexFixture.addNode("shell", 6u);
    const int thirdShell = complexFixture.addNode("shell", 6u);
    const int secondLoop = complexFixture.addNode("loop", 4u);
    const int secondCoedge = complexFixture.addNode("coedge", 7u);
    const int thirdCoedge = complexFixture.addNode("coedge", 7u);
    const int fourthCoedge = complexFixture.addNode("coedge", 7u);
    const int secondEdge = complexFixture.addNode("edge", 5u);
    const int pattern = complexFixture.addNode("pattern", 0u);
    const int wire = complexFixture.addNode("wire", 0u);
    const int subshell = complexFixture.addNode("subshell", 0u);
    const int pcurve = complexFixture.addNode("pcurve", 0u);
    const int unsupportedCurve = complexFixture.addNode("parabola-curve", 1u);
    const int unsupportedSurface = complexFixture.addNode("spline-surface", 1u);
    complexFixture.addNode("vertex", 3u);
    complexFixture.addNode("edge", 5u);
    const int outsideFace = complexFixture.addNode("face", 6u);

    complexFixture.link(root.body, 0u, pattern);
    complexFixture.link(root.body, 2u, wire);
    complexFixture.link(root.lump, 1u, secondLump);
    complexFixture.link(secondLump, 2u, thirdShell);
    complexFixture.link(secondLump, 3u, root.body);
    complexFixture.link(root.shell, 1u, secondShell);
    complexFixture.link(root.shell, 2u, subshell);
    complexFixture.link(root.shell, 4u, wire);
    complexFixture.link(secondShell, 5u, root.lump);
    complexFixture.link(thirdShell, 5u, secondLump);
    complexFixture.link(root.face, 4u, subshell);
    complexFixture.link(root.face, 5u, unsupportedSurface);
    // The face's first loop points to a higher-indexed node; its next-loop
    // link then reaches the lower-indexed base loop. Projection must preserve
    // graph order, not sort loops by SAB record index.
    complexFixture.link(root.face, 2u, secondLoop);
    complexFixture.link(secondLoop, 1u, root.loop);
    complexFixture.link(secondLoop, 2u, secondCoedge);
    complexFixture.link(secondLoop, 3u, root.face);

    complexFixture.link(root.coedge, 1u, thirdCoedge);
    complexFixture.link(root.coedge, 2u, fourthCoedge);
    complexFixture.link(root.coedge, 3u, secondCoedge);
    complexFixture.link(secondCoedge, 1u, secondCoedge);
    complexFixture.link(secondCoedge, 2u, secondCoedge);
    complexFixture.link(secondCoedge, 3u, thirdCoedge);
    complexFixture.link(secondCoedge, 4u, root.edge);
    complexFixture.link(secondCoedge, 5u, secondLoop);
    complexFixture.link(secondCoedge, 6u, pcurve);
    complexFixture.link(thirdCoedge, 1u, fourthCoedge);
    complexFixture.link(thirdCoedge, 2u, root.coedge);
    complexFixture.link(thirdCoedge, 3u, fourthCoedge);
    complexFixture.link(thirdCoedge, 4u, secondEdge);
    complexFixture.link(thirdCoedge, 5u, root.loop);
    complexFixture.link(fourthCoedge, 1u, root.coedge);
    complexFixture.link(fourthCoedge, 2u, thirdCoedge);
    complexFixture.link(fourthCoedge, 3u, root.coedge);
    complexFixture.link(fourthCoedge, 4u, root.edge);
    complexFixture.link(fourthCoedge, 5u, root.loop);
    complexFixture.link(secondEdge, 1u, root.vertex0);
    complexFixture.link(secondEdge, 2u, root.vertex1);
    complexFixture.link(secondEdge, 3u, thirdCoedge);
    complexFixture.link(secondEdge, 4u, unsupportedCurve);
    complexFixture.setBooleanFieldsAfterPointerCount(
        root.face, 6u, {true, true, false});
    complexFixture.setBooleanFieldsAfterPointerCount(
        root.coedge, 5u, {true});
    complexFixture.setBooleanFieldsAfterPointerCount(
        secondCoedge, 5u, {false});
    complexFixture.setBooleanFieldsAfterPointerCount(
        thirdCoedge, 5u, {true});
    complexFixture.setBooleanFieldsAfterPointerCount(
        fourthCoedge, 5u, {false});

    DRW_AcisRootedProjectionReport complexReport;
    std::vector<DRW_AcisBodyProjection> complexProjections;
    const bool complexSucceeded = drw_extractAcisBodyWireframes(
        drw_buildAcisModel(complexFixture.data), complexProjections,
        complexReport);
    t.expect(complexSucceeded && complexReport.bodyCount == 1u
                 && complexProjections.size() == 1u
                 && complexReport.verticesOutsideBodyRoots == 1u
                 && complexReport.edgesOutsideBodyRoots == 1u
                 && complexReport.facesOutsideBodyRoots == 1u,
             "ACIS rooted projection traverses multiple lumps, shells, and loops");
    if (complexProjections.size() == 1u) {
        const DRW_AcisBodyProjection& projected = complexProjections.front();
        t.expect(projected.wireframe.vertices.size() == 2u
                     && projected.wireframe.vertices[0].nodeIndex
                        == root.vertex0
                     && projected.wireframe.vertices[0].pointNodeIndex
                        == root.point0
                     && projected.wireframe.vertices[0].valid
                     && projected.wireframe.vertices[1].nodeIndex
                        == root.vertex1
                     && projected.wireframe.vertices[1].pointNodeIndex
                        == root.point1
                     && projected.wireframe.vertices[1].valid,
                 "ACIS rooted vertices retain their exact ordinal-2 point-record identities");
        const auto includes = [&projected](int index) {
            return std::find(projected.reachableNodeIndices.begin(),
                             projected.reachableNodeIndices.end(), index)
                != projected.reachableNodeIndices.end();
        };
        t.expect(includes(secondLump) && includes(secondShell)
                     && includes(thirdShell) && includes(secondLoop)
                     && projected.wireframe.faces.size() == 1u
                     && projected.wireframe.faces.front().loops.size() == 2u
                     && projected.wireframe.faces.front().loops[0].nodeIndex
                            == secondLoop
                     && projected.wireframe.faces.front().loops[1].nodeIndex
                            == root.loop
                     && projected.wireframe.faces.front().loops[0].coedgeCount
                            == 1
                     && projected.wireframe.faces.front().loops[1].coedgeCount
                            == 3,
                 "ACIS rooted projection preserves multi-lump/shell and unsorted face-loop/coedge order");
        t.expect(projected.wireframe.faces.front().loops[0]
                         .coedgeNodeIndices == std::vector<int>{secondCoedge}
                     && projected.wireframe.faces.front().loops[1]
                         .coedgeNodeIndices
                            == std::vector<int>{root.coedge, thirdCoedge,
                                                fourthCoedge},
                 "ACIS face loops retain exact coedge identities in next-link order");
        t.expect(projected.wireframe.faces.front().loops[0]
                         .coedgeNextNodeIndices == std::vector<int>{secondCoedge}
                     && projected.wireframe.faces.front().loops[1]
                         .coedgeNextNodeIndices
                            == std::vector<int>{thirdCoedge, fourthCoedge,
                                                root.coedge},
                 "ACIS face loops retain exact next-link targets in coedge order");
        t.expect(projected.wireframe.faces.front()
                         .hasLoopChainObservation
                     && projected.wireframe.faces.front()
                            .loopChainNullTerminated
                     && !projected.wireframe.faces.front()
                            .loopChainCycleDetected
                     && projected.wireframe.faces.front()
                            .loopNextNodeIndices
                        == std::vector<int>{root.loop, -1},
                 "ACIS face projection retains exact next-loop targets and null termination");
        t.expect(projected.wireframe.faces.front().loops[0]
                         .coedgePreviousNodeIndices
                         == std::vector<int>{secondCoedge}
                     && projected.wireframe.faces.front().loops[1]
                         .coedgePreviousNodeIndices
                            == std::vector<int>{fourthCoedge, root.coedge,
                                                thirdCoedge}
                     && projected.wireframe.faces.front().loops[0]
                         .coedgePartnerNodeIndices
                         == std::vector<int>{thirdCoedge}
                     && projected.wireframe.faces.front().loops[1]
                         .coedgePartnerNodeIndices
                            == std::vector<int>{secondCoedge, fourthCoedge,
                                                root.coedge},
                 "ACIS face loops retain exact previous and partner pointer targets in coedge order");
        t.expect(projected.wireframe.faces.front().loops[0]
                         .hasCoedgeRingClosure
                     && projected.wireframe.faces.front().loops[0]
                         .coedgeRingClosed
                     && projected.wireframe.faces.front().loops[0]
                         .coedgeLinksReciprocal
                     && projected.wireframe.faces.front().loops[1]
                         .hasCoedgeRingClosure
                     && projected.wireframe.faces.front().loops[1]
                         .coedgeRingClosed
                     && projected.wireframe.faces.front().loops[1]
                         .coedgeLinksReciprocal,
                 "ACIS 21800 face-loop projection confirms closed rings have reciprocal next/previous links");
        t.expect(projected.wireframe.faces.front().loops[0]
                         .coedgeEdgeNodeIndices == std::vector<int>{root.edge}
                     && projected.wireframe.faces.front().loops[1]
                         .coedgeEdgeNodeIndices
                            == std::vector<int>{root.edge, secondEdge,
                                                root.edge},
                 "ACIS face-loop coedges retain exact edge-node incidence");
        const auto edgeCoedgeTarget = [&projected](int edgeIndex) {
            const auto found = std::find_if(
                projected.wireframe.edges.begin(),
                projected.wireframe.edges.end(),
                [edgeIndex](const DRW_AcisEdge& edge) {
                    return edge.nodeIndex == edgeIndex;
                });
            return found == projected.wireframe.edges.end()
                ? -1 : found->coedgeNodeIndex;
        };
        const auto edgeCoedgeReciprocal = [&projected](int edgeIndex) {
            const auto found = std::find_if(
                projected.wireframe.edges.begin(),
                projected.wireframe.edges.end(),
                [edgeIndex](const DRW_AcisEdge& edge) {
                    return edge.nodeIndex == edgeIndex;
                });
            return found != projected.wireframe.edges.end()
                && found->hasReciprocalCoedgeLink;
        };
        const auto edgeByIndex = [&projected](int edgeIndex)
            -> const DRW_AcisEdge* {
            const auto found = std::find_if(
                projected.wireframe.edges.begin(),
                projected.wireframe.edges.end(),
                [edgeIndex](const DRW_AcisEdge& edge) {
                    return edge.nodeIndex == edgeIndex;
                });
            return found == projected.wireframe.edges.end()
                ? nullptr : &*found;
        };
        t.expect(edgeCoedgeTarget(root.edge) == root.coedge
                     && edgeCoedgeTarget(secondEdge) == thirdCoedge
                     && edgeCoedgeReciprocal(root.edge)
                     && edgeCoedgeReciprocal(secondEdge),
                 "ACIS rooted edges retain ordinal-3 targets with reciprocal coedge edge links");
        const DRW_AcisEdge* rootEdge = edgeByIndex(root.edge);
        const DRW_AcisEdge* secondRootEdge = edgeByIndex(secondEdge);
        t.expect(rootEdge && rootEdge->hasCoedgePartnerRingObservation
                     && rootEdge->coedgePartnerRingClosed
                     && rootEdge->coedgePartnerRingNodeIndices
                        == std::vector<int>{root.coedge, secondCoedge,
                                            thirdCoedge, fourthCoedge}
                     && rootEdge->coedgePartnerRingNextNodeIndices
                        == std::vector<int>{secondCoedge, thirdCoedge,
                                            fourthCoedge, root.coedge}
                     && rootEdge->hasCoedgeEdgeUseIndexObservation
                     && !rootEdge->coedgePartnerRingMatchesEdgeUseIndex
                     && rootEdge->coedgeEdgeUseNodeIndices
                        == std::vector<int>{root.coedge, secondCoedge,
                                            fourthCoedge}
                     && secondRootEdge
                     && secondRootEdge->coedgePartnerRingClosed
                     && secondRootEdge->coedgePartnerRingNodeIndices
                        == std::vector<int>{thirdCoedge, fourthCoedge,
                                            root.coedge, secondCoedge}
                     && secondRootEdge->coedgePartnerRingNextNodeIndices
                        == std::vector<int>{fourthCoedge, root.coedge,
                                            secondCoedge, thirdCoedge}
                     && !secondRootEdge->coedgePartnerRingMatchesEdgeUseIndex
                     && secondRootEdge->coedgeEdgeUseNodeIndices
                        == std::vector<int>{thirdCoedge},
                 "ACIS edges retain partner cycles and classify disagreement with exact edge-use back references");
        t.expect(rootEdge
                     && rootEdge->startVertexNodeIndex == root.vertex0
                     && rootEdge->endVertexNodeIndex == root.vertex1
                     && secondRootEdge
                     && secondRootEdge->startVertexNodeIndex == root.vertex0
                     && secondRootEdge->endVertexNodeIndex == root.vertex1,
                 "ACIS rooted edges retain exact ordinal-1/2 endpoint vertex identities");
        t.expect(rootEdge && rootEdge->curveNodeIndex == -1
                     && secondRootEdge
                     && secondRootEdge->curveNodeIndex == unsupportedCurve,
                 "ACIS rooted edges retain the exact ordinal-4 curve identity, including unsupported curves and null slots");
        t.expect(projected.wireframe.faces.front().loops[0]
                         .coedgeEdgeStartVertexNodeIndices
                        == std::vector<int>{root.vertex0}
                     && projected.wireframe.faces.front().loops[0]
                         .coedgeEdgeEndVertexNodeIndices
                        == std::vector<int>{root.vertex1}
                     && projected.wireframe.faces.front().loops[1]
                         .coedgeEdgeStartVertexNodeIndices
                        == std::vector<int>{root.vertex0, root.vertex0,
                                            root.vertex0}
                     && projected.wireframe.faces.front().loops[1]
                         .coedgeEdgeEndVertexNodeIndices
                        == std::vector<int>{root.vertex1, root.vertex1,
                                            root.vertex1},
                 "ACIS face-loop coedges retain edge endpoint identities in raw edge order");
        t.expect(projected.wireframe.faces.front().loops[0]
                         .coedgeOrientedStartVertexNodeIndices
                        == std::vector<int>{root.vertex0}
                     && projected.wireframe.faces.front().loops[0]
                         .coedgeOrientedEndVertexNodeIndices
                        == std::vector<int>{root.vertex1}
                     && projected.wireframe.faces.front().loops[1]
                         .coedgeOrientedStartVertexNodeIndices
                        == std::vector<int>{root.vertex1, root.vertex1,
                                            root.vertex0}
                     && projected.wireframe.faces.front().loops[1]
                         .coedgeOrientedEndVertexNodeIndices
                        == std::vector<int>{root.vertex0, root.vertex0,
                                            root.vertex1},
                 "ACIS face-loop coedge sense derives traversal endpoint identities without changing raw edge order");
        t.expect(projected.wireframe.faces.front().loops[0]
                         .coedgeLoopNodeIndices == std::vector<int>{secondLoop}
                     && projected.wireframe.faces.front().loops[1]
                         .coedgeLoopNodeIndices
                            == std::vector<int>{root.loop, root.loop, root.loop},
                 "ACIS face-loop coedges retain exact owning-loop links");
        t.expect(projected.wireframe.faces.front().loops[0]
                         .hasCoedgeUseIndexObservation
                     && projected.wireframe.faces.front().loops[0]
                         .coedgeUseIndexCoversParsedCoedges
                     && projected.wireframe.faces.front().loops[0]
                         .coedgeChainMatchesUseIndex
                     && projected.wireframe.faces.front().loops[0]
                         .coedgeUseNodeIndices
                            == std::vector<int>{secondCoedge}
                     && projected.wireframe.faces.front().loops[1]
                         .coedgeUseIndexCoversParsedCoedges
                     && projected.wireframe.faces.front().loops[1]
                         .coedgeChainMatchesUseIndex
                     && projected.wireframe.faces.front().loops[1]
                         .coedgeUseNodeIndices
                            == std::vector<int>{root.coedge, thirdCoedge,
                                                fourthCoedge},
                 "ACIS face loops compare their coedge chains with the source-ordered owner census");
        t.expect(projected.wireframe.faces.front().loops[0]
                         .coedgePCurveNodeIndices == std::vector<int>{pcurve}
                     && projected.wireframe.faces.front().loops[1]
                         .coedgePCurveNodeIndices
                            == std::vector<int>{-1, -1, -1},
                 "ACIS face-loop coedges retain exact optional pcurve links");
        const DRW_AcisFace& projectedFace =
            projected.wireframe.faces.front();
        t.expect(projectedFace.shellNodeIndex == root.shell,
                 "ACIS 21800 face projection retains exact parent-shell identity");
        t.expect(projectedFace.surfaceNodeIndex == unsupportedSurface,
                 "ACIS 21800 face projection retains exact source-surface identity");
        t.expect(projectedFace.hasSense && projectedFace.senseReversed
                     && projectedFace.hasDoubleSided
                     && projectedFace.doubleSided
                     && projectedFace.hasContainment
                     && !projectedFace.containmentIn,
                 "ACIS 21800 projection retains face sense and containment flags");
        DRW_Coord orientedFaceNormal(7.0, 8.0, 9.0);
        const bool reversedFaceNormalOk = drw_orientAcisFaceNormal(
            projectedFace, DRW_Coord(0.0, 0.0, 8.0), orientedFaceNormal);
        DRW_AcisFace forwardSenseFace = projectedFace;
        forwardSenseFace.senseReversed = false;
        DRW_Coord forwardFaceNormal(7.0, 8.0, 9.0);
        const bool forwardFaceNormalOk = drw_orientAcisFaceNormal(
            forwardSenseFace, DRW_Coord(3.0, 4.0, 0.0), forwardFaceNormal);
        DRW_Coord scaledFaceNormal(7.0, 8.0, 9.0);
        const bool scaledFaceNormalOk = drw_orientAcisFaceNormal(
            forwardSenseFace,
            DRW_Coord(std::numeric_limits<double>::max(),
                      std::numeric_limits<double>::max(),
                      std::numeric_limits<double>::max()),
            scaledFaceNormal);
        DRW_Coord tinyFaceNormal(7.0, 8.0, 9.0);
        const bool tinyFaceNormalOk = drw_orientAcisFaceNormal(
            forwardSenseFace,
            DRW_Coord(0.0, std::numeric_limits<double>::denorm_min(), 0.0),
            tinyFaceNormal);
        DRW_Coord unchangedFaceNormal(7.0, 8.0, 9.0);
        DRW_AcisFace unknownFaceSense = forwardSenseFace;
        unknownFaceSense.hasSense = false;
        t.expect(reversedFaceNormalOk && orientedFaceNormal.x == 0.0
                     && orientedFaceNormal.y == 0.0
                     && orientedFaceNormal.z == -1.0
                     && forwardFaceNormalOk
                     && std::fabs(forwardFaceNormal.x - 0.6) < 1e-15
                     && std::fabs(forwardFaceNormal.y - 0.8) < 1e-15
                     && forwardFaceNormal.z == 0.0
                     && scaledFaceNormalOk
                     && std::fabs(scaledFaceNormal.x
                                  - 1.0 / std::sqrt(3.0)) < 1e-15
                     && std::fabs(scaledFaceNormal.y
                                  - 1.0 / std::sqrt(3.0)) < 1e-15
                     && std::fabs(scaledFaceNormal.z
                                  - 1.0 / std::sqrt(3.0)) < 1e-15
                     && tinyFaceNormalOk && tinyFaceNormal.x == 0.0
                     && tinyFaceNormal.y == 1.0 && tinyFaceNormal.z == 0.0,
                 "ACIS face sense orients caller-provided normals with scale-safe unit normalization");
        t.expect(!drw_orientAcisFaceNormal(
                         unknownFaceSense, DRW_Coord(0.0, 0.0, 1.0),
                         unchangedFaceNormal)
                     && !drw_orientAcisFaceNormal(
                         forwardSenseFace, DRW_Coord(0.0, 0.0, 0.0),
                         unchangedFaceNormal)
                     && !drw_orientAcisFaceNormal(
                         forwardSenseFace,
                         DRW_Coord(std::numeric_limits<double>::quiet_NaN(),
                                   0.0, 1.0),
                         unchangedFaceNormal)
                     && unchangedFaceNormal.x == 7.0
                     && unchangedFaceNormal.y == 8.0
                     && unchangedFaceNormal.z == 9.0,
                 "ACIS face-normal orientation rejects unknown, zero, or non-finite input atomically");
        t.expect(projectedFace.loops[0].coedgeSenseFlags
                     == std::vector<std::int8_t>{0},
                 "ACIS 21800 projection retains first-loop coedge sense");
        t.expect(projectedFace.loops[1].coedgeSenseFlags
                     == std::vector<std::int8_t>{1, 1, 0},
                 "ACIS 21800 projection retains multi-coedge loop sense order");
        t.expect(projectedFace.loops[0]
                         .hasCoedgeEndpointChainObservation
                     && !projectedFace.loops[0]
                            .coedgeEndpointChainContinuous
                     && projectedFace.loops[0]
                            .coedgeEndpointDiscontinuityCount == 1u
                     && projectedFace.loops[0]
                            .firstCoedgeEndpointDiscontinuityNodeIndex
                        == secondCoedge
                     && projectedFace.loops[1]
                            .hasCoedgeEndpointChainObservation
                     && !projectedFace.loops[1]
                            .coedgeEndpointChainContinuous
                     && projectedFace.loops[1]
                            .coedgeEndpointDiscontinuityCount == 1u
                     && projectedFace.loops[1]
                            .firstCoedgeEndpointDiscontinuityNodeIndex
                        == root.coedge,
                 "ACIS face-loop projection reports exact oriented endpoint discontinuities");
        t.expect(projected.coedgeNextCycleCount == 2u
                     && projected.coedgePartnerCycleCount == 1u,
                 "ACIS rooted projection reports cyclic next and non-manifold partner rings");
        t.expect(projected.report.unsupportedPatternNodeCount == 1u
                     && projected.report.unsupportedWireNodeCount == 1u
                     && projected.report.unsupportedSubshellNodeCount == 1u
                     && projected.report.unsupportedPCurveNodeCount == 1u
                     && projected.report.unsupportedCurveNodeCount == 1u
                     && projected.report.unsupportedSurfaceNodeCount == 1u,
                 "ACIS rooted projection counts unsupported branches by family");
        t.expect(drw_acisProjectionCoverage(projected)
                     == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
                 "ACIS coverage reports a structurally projected body with unsupported branches as partial");
        const auto parentEdgeMismatch = std::find_if(
            projected.wireframe.edges.begin(), projected.wireframe.edges.end(),
            [secondEdge](const DRW_AcisEdge& edge) {
                return edge.nodeIndex == secondEdge;
            });
        t.expect(parentEdgeMismatch != projected.wireframe.edges.end()
                     && parentEdgeMismatch->hasStart
                     && acisCoordNear(parentEdgeMismatch->start, 3.0, 2.0, 3.0)
                     && !projected.report.brepValidityChecked,
                 "ACIS rooted projection accepts endpoint vertex parented to another edge without claiming validity");
        t.expect(projected.report.facesWithoutSurfaceFlag == 1u,
                 "ACIS rooted projection marks unsupported surface parameters unresolved");
    }

    const std::vector<std::uint8_t> complexSabBytes =
        sab21800FromGraph(complexFixture.data);
    const auto parseAndProject21800 = [](
        const AcisGraphFixture& input, DRW_AcisModel& model,
        std::vector<DRW_AcisBodyProjection>& projections,
        DRW_AcisRootedProjectionReport& report) {
        const std::vector<std::uint8_t> bytes =
            sab21800FromGraph(input.data);
        DRW_SabData parsed;
        DRW_SabParseOutcome outcome;
        return !bytes.empty()
            && drw_parseSab(bytes.data(), bytes.size(), parsed)
            && drw_buildAcisModelWithLimits(
                parsed, model, outcome, DRW_SabParseLimits())
            && drw_extractAcisBodyWireframes(model, projections, report);
    };
    AcisGraphFixture continuousEndpointFixture;
    const SyntheticAcisBody continuousEndpointBody =
        addSyntheticAcisBody(continuousEndpointFixture, 9.0);
    continuousEndpointFixture.link(continuousEndpointBody.edge, 2u,
                                   continuousEndpointBody.vertex0);
    DRW_AcisModel continuousEndpointModel;
    DRW_AcisRootedProjectionReport continuousEndpointReport;
    std::vector<DRW_AcisBodyProjection> continuousEndpointProjections;
    const bool continuousEndpointParsedAndProjected = parseAndProject21800(
        continuousEndpointFixture, continuousEndpointModel,
        continuousEndpointProjections, continuousEndpointReport);
    const DRW_AcisFaceLoop* continuousEndpointLoop = nullptr;
    if (continuousEndpointProjections.size() == 1u
        && !continuousEndpointProjections.front().wireframe.faces.empty()
        && !continuousEndpointProjections.front().wireframe.faces.front()
                .loops.empty()) {
        continuousEndpointLoop =
            &continuousEndpointProjections.front().wireframe.faces.front()
                 .loops.front();
    }
    t.expect(continuousEndpointParsedAndProjected
                 && continuousEndpointLoop
                 && continuousEndpointLoop
                        ->hasCoedgeEndpointChainObservation
                 && continuousEndpointLoop->coedgeEndpointChainContinuous
                 && continuousEndpointLoop
                        ->coedgeEndpointDiscontinuityCount == 0u
                 && continuousEndpointLoop
                        ->firstCoedgeEndpointDiscontinuityNodeIndex == -1,
             "SAB 21800 rooted endpoint-chain observation recognizes a connected closed coedge cycle");
    const auto findProjectedEdge = [](
        const std::vector<DRW_AcisBodyProjection>& projections,
        int edgeIndex) -> const DRW_AcisEdge* {
        if (projections.size() != 1u)
            return nullptr;
        const auto& edges = projections.front().wireframe.edges;
        const auto found = std::find_if(
            edges.begin(), edges.end(), [edgeIndex](const DRW_AcisEdge& edge) {
                return edge.nodeIndex == edgeIndex;
            });
        return found == edges.end() ? nullptr : &*found;
    };
    const auto findProjectedFace = [](
        const std::vector<DRW_AcisBodyProjection>& projections,
        int faceIndex) -> const DRW_AcisFace* {
        if (projections.size() != 1u)
            return nullptr;
        const auto& faces = projections.front().wireframe.faces;
        const auto found = std::find_if(
            faces.begin(), faces.end(), [faceIndex](const DRW_AcisFace& face) {
                return face.nodeIndex == faceIndex;
            });
        return found == faces.end() ? nullptr : &*found;
    };
    const auto findProjectedLoop = [](
        const std::vector<DRW_AcisBodyProjection>& projections,
        int loopIndex) -> const DRW_AcisFaceLoop* {
        if (projections.size() != 1u)
            return nullptr;
        for (const DRW_AcisFace& face : projections.front().wireframe.faces) {
            const auto found = std::find_if(
                face.loops.begin(), face.loops.end(),
                [loopIndex](const DRW_AcisFaceLoop& loop) {
                    return loop.nodeIndex == loopIndex;
                });
            if (found != face.loops.end())
                return &*found;
        }
        return nullptr;
    };
    const auto findProjectedShell = [](
        const std::vector<DRW_AcisBodyProjection>& projections,
        int shellIndex) -> const DRW_AcisShell* {
        if (projections.size() != 1u)
            return nullptr;
        const auto& shells = projections.front().wireframe.shells;
        const auto found = std::find_if(
            shells.begin(), shells.end(), [shellIndex](const DRW_AcisShell& shell) {
                return shell.nodeIndex == shellIndex;
            });
        return found == shells.end() ? nullptr : &*found;
    };
    const auto findProjectedLump = [](
        const std::vector<DRW_AcisBodyProjection>& projections,
        int lumpIndex) -> const DRW_AcisLump* {
        if (projections.size() != 1u)
            return nullptr;
        const auto& lumps = projections.front().wireframe.lumps;
        const auto found = std::find_if(
            lumps.begin(), lumps.end(), [lumpIndex](const DRW_AcisLump& lump) {
                return lump.nodeIndex == lumpIndex;
            });
        return found == lumps.end() ? nullptr : &*found;
    };
    AcisGraphFixture loopCoedgeSourceOrderFixture;
    const int earlierLoopCoedge =
        loopCoedgeSourceOrderFixture.addNode("coedge", 7u);
    const SyntheticAcisBody loopCoedgeOrderRoot =
        addSyntheticAcisBody(loopCoedgeSourceOrderFixture, 29.0);
    loopCoedgeSourceOrderFixture.link(
        loopCoedgeOrderRoot.coedge, 1u, earlierLoopCoedge);
    loopCoedgeSourceOrderFixture.link(
        loopCoedgeOrderRoot.coedge, 2u, earlierLoopCoedge);
    loopCoedgeSourceOrderFixture.link(
        loopCoedgeOrderRoot.coedge, 3u, earlierLoopCoedge);
    loopCoedgeSourceOrderFixture.link(
        loopCoedgeOrderRoot.coedge, 4u, loopCoedgeOrderRoot.edge);
    loopCoedgeSourceOrderFixture.link(
        loopCoedgeOrderRoot.coedge, 5u, loopCoedgeOrderRoot.loop);
    loopCoedgeSourceOrderFixture.link(earlierLoopCoedge, 1u,
                                      loopCoedgeOrderRoot.coedge);
    loopCoedgeSourceOrderFixture.link(earlierLoopCoedge, 2u,
                                      loopCoedgeOrderRoot.coedge);
    loopCoedgeSourceOrderFixture.link(earlierLoopCoedge, 3u,
                                      loopCoedgeOrderRoot.coedge);
    loopCoedgeSourceOrderFixture.link(earlierLoopCoedge, 4u,
                                      loopCoedgeOrderRoot.edge);
    loopCoedgeSourceOrderFixture.link(earlierLoopCoedge, 5u,
                                      loopCoedgeOrderRoot.loop);
    loopCoedgeSourceOrderFixture.setBooleanFieldsAfterPointerCount(
        earlierLoopCoedge, 5u, {false});
    DRW_AcisModel loopCoedgeSourceOrderModel;
    DRW_AcisRootedProjectionReport loopCoedgeSourceOrderReport;
    std::vector<DRW_AcisBodyProjection> loopCoedgeSourceOrderProjections;
    const bool loopCoedgeSourceOrderParsedAndProjected =
        parseAndProject21800(
            loopCoedgeSourceOrderFixture, loopCoedgeSourceOrderModel,
            loopCoedgeSourceOrderProjections, loopCoedgeSourceOrderReport);
    const DRW_AcisFace* loopCoedgeSourceOrderFace = findProjectedFace(
        loopCoedgeSourceOrderProjections, loopCoedgeOrderRoot.face);
    const DRW_AcisFaceLoop* loopCoedgeSourceOrderProjection =
        loopCoedgeSourceOrderFace
        && !loopCoedgeSourceOrderFace->loops.empty()
        ? &loopCoedgeSourceOrderFace->loops.front() : nullptr;
    t.expect(loopCoedgeSourceOrderParsedAndProjected
                 && loopCoedgeSourceOrderProjection
                 && loopCoedgeSourceOrderProjection
                        ->hasCoedgeUseIndexObservation
                 && loopCoedgeSourceOrderProjection
                        ->coedgeUseIndexCoversParsedCoedges
                 && loopCoedgeSourceOrderProjection
                        ->coedgeUseIndexUnclassifiedCoedgeCount == 0u
                 && loopCoedgeSourceOrderProjection
                        ->firstUnclassifiedCoedgeOwnerNodeIndex == -1
                 && loopCoedgeSourceOrderProjection->coedgeRingClosed
                 && loopCoedgeSourceOrderProjection->coedgeLinksReciprocal
                 && loopCoedgeSourceOrderProjection->coedgeNodeIndices
                    == std::vector<int>{loopCoedgeOrderRoot.coedge,
                                        earlierLoopCoedge}
                 && loopCoedgeSourceOrderProjection->coedgeUseNodeIndices
                    == std::vector<int>{earlierLoopCoedge,
                                        loopCoedgeOrderRoot.coedge}
                 && loopCoedgeSourceOrderProjection
                        ->coedgeChainMatchesUseIndex,
             "SAB 21800 loop coedge census matches identities when source and next-link order differ");

    AcisGraphFixture matchingPartnerEdgeFixture = complexFixture;
    matchingPartnerEdgeFixture.link(thirdCoedge, 4u, root.edge);
    DRW_AcisModel matchingPartnerEdgeModel;
    DRW_AcisRootedProjectionReport matchingPartnerEdgeReport;
    std::vector<DRW_AcisBodyProjection> matchingPartnerEdgeProjections;
    const bool matchingPartnerEdgeParsedAndProjected = parseAndProject21800(
        matchingPartnerEdgeFixture, matchingPartnerEdgeModel,
        matchingPartnerEdgeProjections, matchingPartnerEdgeReport);
    const DRW_AcisEdge* matchingPartnerEdge = findProjectedEdge(
        matchingPartnerEdgeProjections, root.edge);
    const DRW_AcisFace* matchingFaceLoopIndex = findProjectedFace(
        matchingPartnerEdgeProjections, root.face);
    t.expect(matchingPartnerEdgeParsedAndProjected
                 && matchingPartnerEdge
                 && matchingPartnerEdge->hasCoedgeEdgeUseIndexObservation
                 && matchingPartnerEdge->coedgeEdgeUseIndexCoversParsedCoedges
                 && matchingPartnerEdge
                        ->coedgeEdgeUseIndexUnclassifiedCoedgeCount == 0u
                 && matchingPartnerEdge
                        ->firstUnclassifiedCoedgeEdgeUseNodeIndex == -1
                 && matchingPartnerEdge->coedgePartnerRingClosed
                 && matchingPartnerEdge->coedgePartnerRingMatchesEdgeUseIndex
                 && matchingPartnerEdge->coedgePartnerRingNodeIndices
                    == std::vector<int>{root.coedge, secondCoedge,
                                        thirdCoedge, fourthCoedge}
                 && matchingPartnerEdge->coedgeEdgeUseNodeIndices
                    == std::vector<int>{root.coedge, secondCoedge,
                                        thirdCoedge, fourthCoedge}
                 && matchingFaceLoopIndex
                 && matchingFaceLoopIndex->hasLoopUseIndexObservation
                 && matchingFaceLoopIndex->loopUseIndexCoversParsedLoops
                 && matchingFaceLoopIndex
                        ->loopUseIndexUnclassifiedLoopCount == 0u
                 && matchingFaceLoopIndex
                        ->firstUnclassifiedLoopOwnerNodeIndex == -1
                 && matchingFaceLoopIndex->faceLoopChainMatchesUseIndex
                 && matchingFaceLoopIndex->loopUseNodeIndices
                    == std::vector<int>{root.loop, secondLoop}
                 && matchingFaceLoopIndex->hasLoopChainObservation
                 && matchingFaceLoopIndex->loopChainNullTerminated
                 && !matchingFaceLoopIndex->loopChainCycleDetected
                 && matchingFaceLoopIndex
                        ->loopChainUnclassifiedNextLinkCount == 0u
                 && matchingFaceLoopIndex
                        ->firstUnclassifiedLoopNextNodeIndex == -1
                 && matchingFaceLoopIndex
                        ->firstRepeatedLoopChainNodeIndex == -1
                 && matchingFaceLoopIndex->loopNextNodeIndices
                    == std::vector<int>{root.loop, -1}
                 && matchingFaceLoopIndex->loops.size() == 2u
                 && matchingFaceLoopIndex->loops[0].nodeIndex == secondLoop
                 && matchingFaceLoopIndex->loops[1].nodeIndex == root.loop,
             "SAB 21800 parser-fed partner ring matches the complete explicit edge-use index");

    AcisGraphFixture cyclicFaceLoopFixture = matchingPartnerEdgeFixture;
    cyclicFaceLoopFixture.link(root.loop, 1u, secondLoop);
    DRW_AcisModel cyclicFaceLoopModel;
    DRW_AcisRootedProjectionReport cyclicFaceLoopReport;
    std::vector<DRW_AcisBodyProjection> cyclicFaceLoopProjections;
    const bool cyclicFaceLoopParsedAndProjected = parseAndProject21800(
        cyclicFaceLoopFixture, cyclicFaceLoopModel,
        cyclicFaceLoopProjections, cyclicFaceLoopReport);
    const DRW_AcisFace* cyclicFaceLoopProjection = findProjectedFace(
        cyclicFaceLoopProjections, root.face);
    t.expect(cyclicFaceLoopParsedAndProjected
                 && cyclicFaceLoopProjection
                 && cyclicFaceLoopProjection->loopUseIndexCoversParsedLoops
                 && cyclicFaceLoopProjection->loopChainCycleDetected
                 && !cyclicFaceLoopProjection->loopChainNullTerminated
                 && cyclicFaceLoopProjection
                        ->firstRepeatedLoopChainNodeIndex == secondLoop
                 && cyclicFaceLoopProjection->loopNextNodeIndices
                    == std::vector<int>{root.loop, secondLoop}
                 && !cyclicFaceLoopProjection
                        ->faceLoopChainMatchesUseIndex
                 && drw_acisProjectionCoverage(
                        cyclicFaceLoopProjections.front())
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "SAB 21800 parser-fed face-loop cycle cannot masquerade as a complete null-terminated chain");

    AcisGraphFixture wrongTypeFaceLoopNextFixture =
        matchingPartnerEdgeFixture;
    wrongTypeFaceLoopNextFixture.link(root.loop, 1u, root.point0);
    const std::vector<std::uint8_t> wrongTypeFaceLoopNextBytes =
        sab21800FromGraph(wrongTypeFaceLoopNextFixture.data);
    DRW_SabData wrongTypeFaceLoopNextSab;
    DRW_AcisModel wrongTypeFaceLoopNextModel;
    DRW_SabParseOutcome wrongTypeFaceLoopNextBuildOutcome;
    DRW_AcisRootedProjectionReport wrongTypeFaceLoopNextReport;
    std::vector<DRW_AcisBodyProjection> wrongTypeFaceLoopNextProjections;
    const bool wrongTypeFaceLoopNextParsed =
        !wrongTypeFaceLoopNextBytes.empty()
        && drw_parseSab(wrongTypeFaceLoopNextBytes.data(),
                        wrongTypeFaceLoopNextBytes.size(),
                        wrongTypeFaceLoopNextSab)
        && drw_buildAcisModelWithLimits(
            wrongTypeFaceLoopNextSab, wrongTypeFaceLoopNextModel,
            wrongTypeFaceLoopNextBuildOutcome, DRW_SabParseLimits());
    const bool wrongTypeFaceLoopNextProjected = wrongTypeFaceLoopNextParsed
        && drw_extractAcisBodyWireframes(
            wrongTypeFaceLoopNextModel, wrongTypeFaceLoopNextProjections,
            wrongTypeFaceLoopNextReport);
    t.expect(wrongTypeFaceLoopNextParsed,
             "SAB 21800 parser-fed wrong-type loop-next stream builds a model");
    t.expect(!wrongTypeFaceLoopNextProjected
                 && wrongTypeFaceLoopNextProjections.empty()
                 && wrongTypeFaceLoopNextReport.failure
                        == DRW_AcisProjectionFailure::InvalidReference
                 && wrongTypeFaceLoopNextReport.failedNodeIndex == root.loop
                 && wrongTypeFaceLoopNextReport.failedPointerOrdinal == 1u,
             "SAB 21800 wrong-type loop-next target is refused transactionally at its exact pointer slot");

    const std::vector<std::uint8_t> missingFaceLoopNextBytes =
        sab21800FromGraph(matchingPartnerEdgeFixture.data);
    DRW_SabData missingFaceLoopNextData;
    DRW_SabParseOutcome missingFaceLoopNextParseOutcome;
    DRW_AcisModel missingFaceLoopNextModel;
    DRW_SabParseOutcome missingFaceLoopNextBuildOutcome;
    bool missingFaceLoopNextParsed =
        !missingFaceLoopNextBytes.empty()
        && drw_parseSab(missingFaceLoopNextBytes.data(),
                        missingFaceLoopNextBytes.size(),
                        missingFaceLoopNextData)
        && drw_buildAcisModelWithLimits(
            missingFaceLoopNextData, missingFaceLoopNextModel,
            missingFaceLoopNextBuildOutcome, DRW_SabParseLimits());
    if (missingFaceLoopNextParsed
        && static_cast<std::size_t>(root.loop)
            < missingFaceLoopNextModel.nodes.size()) {
        missingFaceLoopNextModel.nodes[
            static_cast<std::size_t>(root.loop)].refs.resize(1u);
    } else {
        missingFaceLoopNextParsed = false;
    }
    DRW_AcisRootedProjectionReport missingFaceLoopNextReport;
    std::vector<DRW_AcisBodyProjection> missingFaceLoopNextProjections;
    const bool missingFaceLoopNextProjected = missingFaceLoopNextParsed
        && drw_extractAcisBodyWireframes(
            missingFaceLoopNextModel, missingFaceLoopNextProjections,
            missingFaceLoopNextReport);
    t.expect(!missingFaceLoopNextProjected
                 && missingFaceLoopNextProjections.empty()
                 && missingFaceLoopNextReport.failure
                        == DRW_AcisProjectionFailure::InvalidReference
                 && missingFaceLoopNextReport.failedNodeIndex == root.loop
                 && missingFaceLoopNextReport.failedPointerOrdinal == 1u,
             "SAB 21800 truncated resolved loop pointer row is refused at its exact missing slot");

    AcisGraphFixture bodyLumpChainFixture;
    const SyntheticAcisBody bodyLumpRoot =
        addSyntheticAcisBody(bodyLumpChainFixture, 39.0);
    const int laterBodyLump = bodyLumpChainFixture.addNode("lump", 4u);
    const int laterBodyLumpShell = bodyLumpChainFixture.addNode("shell", 6u);
    bodyLumpChainFixture.link(bodyLumpRoot.body, 1u, laterBodyLump);
    bodyLumpChainFixture.link(laterBodyLump, 1u, bodyLumpRoot.lump);
    bodyLumpChainFixture.link(laterBodyLump, 2u, laterBodyLumpShell);
    bodyLumpChainFixture.link(laterBodyLump, 3u, bodyLumpRoot.body);
    bodyLumpChainFixture.link(laterBodyLumpShell, 5u, laterBodyLump);
    DRW_AcisModel bodyLumpChainModel;
    DRW_AcisRootedProjectionReport bodyLumpChainReport;
    std::vector<DRW_AcisBodyProjection> bodyLumpChainProjections;
    const bool bodyLumpChainParsedAndProjected = parseAndProject21800(
        bodyLumpChainFixture, bodyLumpChainModel,
        bodyLumpChainProjections, bodyLumpChainReport);
    const DRW_AcisBodyProjection* bodyLumpChain =
        bodyLumpChainProjections.size() == 1u
            ? &bodyLumpChainProjections.front() : nullptr;
    t.expect(bodyLumpChainParsedAndProjected && bodyLumpChain
                 && bodyLumpChain->hasLumpChainObservation
                 && bodyLumpChain->lumpChainNullTerminated
                 && !bodyLumpChain->lumpChainCycleDetected
                 && bodyLumpChain->lumpNodeIndices
                    == std::vector<int>{laterBodyLump,
                                        bodyLumpRoot.lump}
                 && bodyLumpChain->lumpNextNodeIndices
                    == std::vector<int>{bodyLumpRoot.lump, -1}
                 && bodyLumpChain->hasLumpUseIndexObservation
                 && bodyLumpChain->lumpUseIndexCoversParsedLumps
                 && bodyLumpChain->lumpUseIndexUnclassifiedLumpCount == 0u
                 && bodyLumpChain->firstUnclassifiedLumpOwnerNodeIndex == -1
                 && bodyLumpChain->lumpUseNodeIndices
                    == std::vector<int>{bodyLumpRoot.lump, laterBodyLump}
                 && bodyLumpChain->lumpNodeIndices
                    != bodyLumpChain->lumpUseNodeIndices
                 && bodyLumpChain->lumpChainMatchesUseIndex,
             "SAB 21800 body lump chain matches source-ordered lump-owner census despite different traversal order");

    AcisGraphFixture detachedBodyLumpFixture = bodyLumpChainFixture;
    const int detachedBodyLump =
        detachedBodyLumpFixture.addNode("lump", 4u);
    detachedBodyLumpFixture.link(detachedBodyLump, 3u, bodyLumpRoot.body);
    DRW_AcisModel detachedBodyLumpModel;
    DRW_AcisRootedProjectionReport detachedBodyLumpReport;
    std::vector<DRW_AcisBodyProjection> detachedBodyLumpProjections;
    const bool detachedBodyLumpParsedAndProjected = parseAndProject21800(
        detachedBodyLumpFixture, detachedBodyLumpModel,
        detachedBodyLumpProjections, detachedBodyLumpReport);
    t.expect(detachedBodyLumpParsedAndProjected
                 && detachedBodyLumpProjections.size() == 1u
                 && detachedBodyLumpProjections.front()
                        .lumpUseIndexCoversParsedLumps
                 && detachedBodyLumpProjections.front().lumpUseNodeIndices
                    == std::vector<int>{bodyLumpRoot.lump,
                                        laterBodyLump, detachedBodyLump}
                 && !detachedBodyLumpProjections.front()
                        .lumpChainMatchesUseIndex,
             "SAB 21800 detached parsed lump naming a body prevents a false body-chain census match");

    AcisGraphFixture missingBodyLumpOwnerFixture = bodyLumpChainFixture;
    const int missingBodyLumpOwner =
        missingBodyLumpOwnerFixture.addNode("lump", 4u);
    DRW_AcisModel missingBodyLumpOwnerModel;
    DRW_AcisRootedProjectionReport missingBodyLumpOwnerReport;
    std::vector<DRW_AcisBodyProjection> missingBodyLumpOwnerProjections;
    const bool missingBodyLumpOwnerParsedAndProjected = parseAndProject21800(
        missingBodyLumpOwnerFixture, missingBodyLumpOwnerModel,
        missingBodyLumpOwnerProjections, missingBodyLumpOwnerReport);
    t.expect(missingBodyLumpOwnerParsedAndProjected
                 && missingBodyLumpOwnerProjections.size() == 1u
                 && !missingBodyLumpOwnerProjections.front()
                        .lumpUseIndexCoversParsedLumps
                 && missingBodyLumpOwnerProjections.front()
                        .lumpUseIndexUnclassifiedLumpCount == 1u
                 && missingBodyLumpOwnerProjections.front()
                        .firstUnclassifiedLumpOwnerNodeIndex
                    == missingBodyLumpOwner
                 && !missingBodyLumpOwnerProjections.front()
                        .lumpChainMatchesUseIndex,
             "SAB 21800 missing parsed lump owner marks body census incomplete at the exact lump");

    AcisGraphFixture wrongTypeBodyLumpOwnerFixture = bodyLumpChainFixture;
    const int wrongTypeBodyLumpOwner =
        wrongTypeBodyLumpOwnerFixture.addNode("lump", 4u);
    wrongTypeBodyLumpOwnerFixture.link(
        wrongTypeBodyLumpOwner, 3u, bodyLumpRoot.shell);
    DRW_AcisModel wrongTypeBodyLumpOwnerModel;
    DRW_AcisRootedProjectionReport wrongTypeBodyLumpOwnerReport;
    std::vector<DRW_AcisBodyProjection> wrongTypeBodyLumpOwnerProjections;
    const bool wrongTypeBodyLumpOwnerParsedAndProjected = parseAndProject21800(
        wrongTypeBodyLumpOwnerFixture, wrongTypeBodyLumpOwnerModel,
        wrongTypeBodyLumpOwnerProjections, wrongTypeBodyLumpOwnerReport);
    t.expect(wrongTypeBodyLumpOwnerParsedAndProjected
                 && wrongTypeBodyLumpOwnerProjections.size() == 1u
                 && !wrongTypeBodyLumpOwnerProjections.front()
                        .lumpUseIndexCoversParsedLumps
                 && wrongTypeBodyLumpOwnerProjections.front()
                        .lumpUseIndexUnclassifiedLumpCount == 1u
                 && wrongTypeBodyLumpOwnerProjections.front()
                        .firstUnclassifiedLumpOwnerNodeIndex
                    == wrongTypeBodyLumpOwner
                 && !wrongTypeBodyLumpOwnerProjections.front()
                        .lumpChainMatchesUseIndex,
             "SAB 21800 wrong-type parsed lump owner prevents a complete body census");

    AcisGraphFixture cyclicBodyLumpFixture = bodyLumpChainFixture;
    cyclicBodyLumpFixture.link(bodyLumpRoot.lump, 1u, laterBodyLump);
    DRW_AcisModel cyclicBodyLumpModel;
    DRW_AcisRootedProjectionReport cyclicBodyLumpReport;
    std::vector<DRW_AcisBodyProjection> cyclicBodyLumpProjections;
    const bool cyclicBodyLumpParsedAndProjected = parseAndProject21800(
        cyclicBodyLumpFixture, cyclicBodyLumpModel,
        cyclicBodyLumpProjections, cyclicBodyLumpReport);
    t.expect(cyclicBodyLumpParsedAndProjected
                 && cyclicBodyLumpProjections.size() == 1u
                 && cyclicBodyLumpProjections.front().lumpChainCycleDetected
                 && !cyclicBodyLumpProjections.front().lumpChainNullTerminated
                 && cyclicBodyLumpProjections.front()
                        .firstRepeatedLumpChainNodeIndex == laterBodyLump
                 && cyclicBodyLumpProjections.front().lumpNextNodeIndices
                    == std::vector<int>{bodyLumpRoot.lump, laterBodyLump}
                 && !cyclicBodyLumpProjections.front()
                        .lumpChainMatchesUseIndex,
             "SAB 21800 body lump cycle remains distinct from a complete null-terminated chain");

    AcisGraphFixture wrongTypeBodyLumpNextFixture = bodyLumpChainFixture;
    wrongTypeBodyLumpNextFixture.link(laterBodyLump, 1u,
                                      bodyLumpRoot.point0);
    DRW_AcisModel wrongTypeBodyLumpNextModel;
    DRW_AcisRootedProjectionReport wrongTypeBodyLumpNextReport;
    std::vector<DRW_AcisBodyProjection> wrongTypeBodyLumpNextProjections;
    const bool wrongTypeBodyLumpNextProjected = parseAndProject21800(
        wrongTypeBodyLumpNextFixture, wrongTypeBodyLumpNextModel,
        wrongTypeBodyLumpNextProjections, wrongTypeBodyLumpNextReport);
    t.expect(!wrongTypeBodyLumpNextProjected
                 && wrongTypeBodyLumpNextProjections.empty()
                 && wrongTypeBodyLumpNextReport.failure
                    == DRW_AcisProjectionFailure::InvalidReference
                 && wrongTypeBodyLumpNextReport.failedNodeIndex == laterBodyLump
                 && wrongTypeBodyLumpNextReport.failedPointerOrdinal == 1u,
             "SAB 21800 wrong-type traversed body lump-next target is refused transactionally at its exact slot");

    const auto exactBodyLumpBudgetBoundary = [&](bool outputBudget) {
        std::size_t low = 0u;
        std::size_t high = 1u;
        bool highFits = false;
        while (high <= 65536u) {
            DRW_SabParseLimits limits;
            if (outputBudget)
                limits.maxOutputItems = high;
            else
                limits.maxExtractionWorkUnits = high;
            std::vector<DRW_AcisBodyProjection> trial;
            DRW_AcisRootedProjectionReport trialReport;
            if (drw_extractAcisBodyWireframes(
                    bodyLumpChainModel, trial, trialReport, limits)) {
                highFits = true;
                break;
            }
            low = high;
            high *= 2u;
        }
        while (highFits && low + 1u < high) {
            const std::size_t middle = low + (high - low) / 2u;
            DRW_SabParseLimits limits;
            if (outputBudget)
                limits.maxOutputItems = middle;
            else
                limits.maxExtractionWorkUnits = middle;
            std::vector<DRW_AcisBodyProjection> trial;
            DRW_AcisRootedProjectionReport trialReport;
            if (drw_extractAcisBodyWireframes(
                    bodyLumpChainModel, trial, trialReport, limits))
                high = middle;
            else
                low = middle;
        }
        DRW_SabParseLimits exactLimits;
        if (outputBudget)
            exactLimits.maxOutputItems = high;
        else
            exactLimits.maxExtractionWorkUnits = high;
        std::vector<DRW_AcisBodyProjection> exact;
        DRW_AcisRootedProjectionReport exactReport;
        const bool exactFits = highFits
            && drw_extractAcisBodyWireframes(
                bodyLumpChainModel, exact, exactReport, exactLimits);
        if (outputBudget)
            --exactLimits.maxOutputItems;
        else
            --exactLimits.maxExtractionWorkUnits;
        std::vector<DRW_AcisBodyProjection> under(1u);
        DRW_AcisRootedProjectionReport underReport;
        const bool underFits = drw_extractAcisBodyWireframes(
            bodyLumpChainModel, under, underReport, exactLimits);
        return exactFits && !underFits && under.empty()
            && underReport.failure == DRW_AcisProjectionFailure::ResourceLimit;
    };
    t.expect(exactBodyLumpBudgetBoundary(true),
             "SAB 21800 body lump census admits exact output budget and refuses one item under transactionally");
    t.expect(exactBodyLumpBudgetBoundary(false),
             "SAB 21800 body lump census admits exact extraction-work budget and refuses one unit under transactionally");

    AcisGraphFixture shellFaceChainFixture;
    const int earlierFaceBySource =
        shellFaceChainFixture.addNode("face", 6u);
    const SyntheticAcisBody shellFaceRoot =
        addSyntheticAcisBody(shellFaceChainFixture, 41.0);
    shellFaceChainFixture.link(shellFaceRoot.face, 1u, earlierFaceBySource);
    shellFaceChainFixture.link(earlierFaceBySource, 3u, shellFaceRoot.shell);
    DRW_AcisModel shellFaceChainModel;
    DRW_AcisRootedProjectionReport shellFaceChainReport;
    std::vector<DRW_AcisBodyProjection> shellFaceChainProjections;
    const bool shellFaceChainParsedAndProjected = parseAndProject21800(
        shellFaceChainFixture, shellFaceChainModel,
        shellFaceChainProjections, shellFaceChainReport);
    const DRW_AcisShell* shellFaceChain = findProjectedShell(
        shellFaceChainProjections, shellFaceRoot.shell);
    t.expect(shellFaceChainParsedAndProjected && shellFaceChain
                 && shellFaceChain->lumpNodeIndex == shellFaceRoot.lump
                 && shellFaceChain->hasFaceChainObservation
                 && shellFaceChain->faceChainNullTerminated
                 && !shellFaceChain->faceChainCycleDetected
                 && shellFaceChain->faceNodeIndices
                    == std::vector<int>{shellFaceRoot.face,
                                         earlierFaceBySource}
                 && shellFaceChain->faceNextNodeIndices
                    == std::vector<int>{earlierFaceBySource, -1}
                 && shellFaceChain->hasFaceUseIndexObservation
                 && shellFaceChain->faceUseIndexCoversParsedFaces
                 && shellFaceChain->faceUseIndexUnclassifiedFaceCount == 0u
                 && shellFaceChain->firstUnclassifiedFaceOwnerNodeIndex == -1
                 && shellFaceChain->faceUseNodeIndices
                    == std::vector<int>{earlierFaceBySource,
                                         shellFaceRoot.face}
                 && shellFaceChain->faceNodeIndices
                    != shellFaceChain->faceUseNodeIndices
                 && shellFaceChain->faceChainMatchesUseIndex,
             "SAB 21800 shell face chain exactly matches source-ordered face-owner census despite different traversal order");

    AcisGraphFixture lumpShellChainFixture;
    const int earlierShellBySource =
        lumpShellChainFixture.addNode("shell", 6u);
    const SyntheticAcisBody lumpShellRoot =
        addSyntheticAcisBody(lumpShellChainFixture, 47.0);
    lumpShellChainFixture.link(lumpShellRoot.shell, 1u,
                               earlierShellBySource);
    lumpShellChainFixture.link(earlierShellBySource, 5u,
                               lumpShellRoot.lump);
    DRW_AcisModel lumpShellChainModel;
    DRW_AcisRootedProjectionReport lumpShellChainReport;
    std::vector<DRW_AcisBodyProjection> lumpShellChainProjections;
    const bool lumpShellChainParsedAndProjected = parseAndProject21800(
        lumpShellChainFixture, lumpShellChainModel,
        lumpShellChainProjections, lumpShellChainReport);
    const DRW_AcisLump* lumpShellChain = findProjectedLump(
        lumpShellChainProjections, lumpShellRoot.lump);
    t.expect(lumpShellChainParsedAndProjected && lumpShellChain
                 && lumpShellChain->bodyNodeIndex == lumpShellRoot.body
                 && lumpShellChain->hasShellChainObservation
                 && lumpShellChain->shellChainNullTerminated
                 && !lumpShellChain->shellChainCycleDetected
                 && lumpShellChain->shellNodeIndices
                    == std::vector<int>{lumpShellRoot.shell,
                                        earlierShellBySource}
                 && lumpShellChain->shellNextNodeIndices
                    == std::vector<int>{earlierShellBySource, -1}
                 && lumpShellChain->hasShellUseIndexObservation
                 && lumpShellChain->shellUseIndexCoversParsedShells
                 && lumpShellChain->shellUseIndexUnclassifiedShellCount == 0u
                 && lumpShellChain->firstUnclassifiedShellOwnerNodeIndex == -1
                 && lumpShellChain->shellUseNodeIndices
                    == std::vector<int>{earlierShellBySource,
                                        lumpShellRoot.shell}
                 && lumpShellChain->shellNodeIndices
                    != lumpShellChain->shellUseNodeIndices
                 && lumpShellChain->shellChainMatchesUseIndex,
             "SAB 21800 lump shell chain matches source-ordered shell-owner census despite different traversal order");

    AcisGraphFixture detachedLumpShellFixture = lumpShellChainFixture;
    const int detachedLumpShell =
        detachedLumpShellFixture.addNode("shell", 6u);
    detachedLumpShellFixture.link(detachedLumpShell, 5u,
                                  lumpShellRoot.lump);
    DRW_AcisModel detachedLumpShellModel;
    DRW_AcisRootedProjectionReport detachedLumpShellReport;
    std::vector<DRW_AcisBodyProjection> detachedLumpShellProjections;
    const bool detachedLumpShellParsedAndProjected = parseAndProject21800(
        detachedLumpShellFixture, detachedLumpShellModel,
        detachedLumpShellProjections, detachedLumpShellReport);
    const DRW_AcisLump* detachedLumpShellProjection = findProjectedLump(
        detachedLumpShellProjections, lumpShellRoot.lump);
    t.expect(detachedLumpShellParsedAndProjected
                 && detachedLumpShellProjection
                 && detachedLumpShellProjection->shellUseIndexCoversParsedShells
                 && detachedLumpShellProjection->shellUseNodeIndices
                    == std::vector<int>{earlierShellBySource,
                                        lumpShellRoot.shell,
                                        detachedLumpShell}
                 && !detachedLumpShellProjection->shellChainMatchesUseIndex
                 && drw_acisProjectionCoverage(
                        detachedLumpShellProjections.front())
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "SAB 21800 detached parsed shell naming a lump prevents a false shell-chain census match");

    AcisGraphFixture missingLumpShellOwnerFixture = lumpShellChainFixture;
    const int missingLumpShellOwner =
        missingLumpShellOwnerFixture.addNode("shell", 6u);
    DRW_AcisModel missingLumpShellOwnerModel;
    DRW_AcisRootedProjectionReport missingLumpShellOwnerReport;
    std::vector<DRW_AcisBodyProjection> missingLumpShellOwnerProjections;
    const bool missingLumpShellOwnerParsedAndProjected = parseAndProject21800(
        missingLumpShellOwnerFixture, missingLumpShellOwnerModel,
        missingLumpShellOwnerProjections, missingLumpShellOwnerReport);
    const DRW_AcisLump* missingLumpShellOwnerProjection = findProjectedLump(
        missingLumpShellOwnerProjections, lumpShellRoot.lump);
    t.expect(missingLumpShellOwnerParsedAndProjected
                 && missingLumpShellOwnerProjection
                 && !missingLumpShellOwnerProjection
                        ->shellUseIndexCoversParsedShells
                 && missingLumpShellOwnerProjection
                        ->shellUseIndexUnclassifiedShellCount == 1u
                 && missingLumpShellOwnerProjection
                        ->firstUnclassifiedShellOwnerNodeIndex
                    == missingLumpShellOwner
                 && !missingLumpShellOwnerProjection->shellChainMatchesUseIndex,
             "SAB 21800 missing parsed shell owner marks the lump census incomplete at the exact shell");

    AcisGraphFixture wrongTypeLumpShellOwnerFixture = lumpShellChainFixture;
    const int wrongTypeLumpShellOwner =
        wrongTypeLumpShellOwnerFixture.addNode("shell", 6u);
    wrongTypeLumpShellOwnerFixture.link(
        wrongTypeLumpShellOwner, 5u, lumpShellRoot.body);
    DRW_AcisModel wrongTypeLumpShellOwnerModel;
    DRW_AcisRootedProjectionReport wrongTypeLumpShellOwnerReport;
    std::vector<DRW_AcisBodyProjection> wrongTypeLumpShellOwnerProjections;
    const bool wrongTypeLumpShellOwnerParsedAndProjected = parseAndProject21800(
        wrongTypeLumpShellOwnerFixture, wrongTypeLumpShellOwnerModel,
        wrongTypeLumpShellOwnerProjections, wrongTypeLumpShellOwnerReport);
    const DRW_AcisLump* wrongTypeLumpShellOwnerProjection = findProjectedLump(
        wrongTypeLumpShellOwnerProjections, lumpShellRoot.lump);
    t.expect(wrongTypeLumpShellOwnerParsedAndProjected
                 && wrongTypeLumpShellOwnerProjection
                 && !wrongTypeLumpShellOwnerProjection
                        ->shellUseIndexCoversParsedShells
                 && wrongTypeLumpShellOwnerProjection
                        ->shellUseIndexUnclassifiedShellCount == 1u
                 && wrongTypeLumpShellOwnerProjection
                        ->firstUnclassifiedShellOwnerNodeIndex
                    == wrongTypeLumpShellOwner
                 && !wrongTypeLumpShellOwnerProjection->shellChainMatchesUseIndex,
             "SAB 21800 wrong-type parsed shell owner prevents a complete lump shell census");

    AcisGraphFixture cyclicLumpShellFixture = lumpShellChainFixture;
    cyclicLumpShellFixture.link(earlierShellBySource, 1u,
                                lumpShellRoot.shell);
    DRW_AcisModel cyclicLumpShellModel;
    DRW_AcisRootedProjectionReport cyclicLumpShellReport;
    std::vector<DRW_AcisBodyProjection> cyclicLumpShellProjections;
    const bool cyclicLumpShellParsedAndProjected = parseAndProject21800(
        cyclicLumpShellFixture, cyclicLumpShellModel,
        cyclicLumpShellProjections, cyclicLumpShellReport);
    const DRW_AcisLump* cyclicLumpShellProjection = findProjectedLump(
        cyclicLumpShellProjections, lumpShellRoot.lump);
    t.expect(cyclicLumpShellParsedAndProjected && cyclicLumpShellProjection
                 && cyclicLumpShellProjection->shellChainCycleDetected
                 && !cyclicLumpShellProjection->shellChainNullTerminated
                 && cyclicLumpShellProjection
                        ->firstRepeatedShellChainNodeIndex
                    == lumpShellRoot.shell
                 && cyclicLumpShellProjection->shellNextNodeIndices
                    == std::vector<int>{earlierShellBySource,
                                        lumpShellRoot.shell}
                 && !cyclicLumpShellProjection->shellChainMatchesUseIndex,
             "SAB 21800 lump shell cycle remains distinct from a complete null-terminated chain");

    AcisGraphFixture wrongTypeLumpShellNextFixture = lumpShellChainFixture;
    wrongTypeLumpShellNextFixture.link(lumpShellRoot.shell, 1u,
                                       lumpShellRoot.point0);
    DRW_AcisModel wrongTypeLumpShellNextModel;
    DRW_AcisRootedProjectionReport wrongTypeLumpShellNextReport;
    std::vector<DRW_AcisBodyProjection> wrongTypeLumpShellNextProjections;
    const bool wrongTypeLumpShellNextProjected = parseAndProject21800(
        wrongTypeLumpShellNextFixture, wrongTypeLumpShellNextModel,
        wrongTypeLumpShellNextProjections, wrongTypeLumpShellNextReport);
    t.expect(!wrongTypeLumpShellNextProjected
                 && wrongTypeLumpShellNextProjections.empty()
                 && wrongTypeLumpShellNextReport.failure
                    == DRW_AcisProjectionFailure::InvalidReference
                 && wrongTypeLumpShellNextReport.failedNodeIndex
                    == lumpShellRoot.shell
                 && wrongTypeLumpShellNextReport.failedPointerOrdinal == 1u,
             "SAB 21800 wrong-type traversed lump shell-next target is refused transactionally at its exact slot");

    const auto exactBudgetBoundary = [&](bool outputBudget) {
        std::size_t low = 0u;
        std::size_t high = 1u;
        bool highFits = false;
        while (high <= 65536u) {
            DRW_SabParseLimits limits;
            if (outputBudget)
                limits.maxOutputItems = high;
            else
                limits.maxExtractionWorkUnits = high;
            std::vector<DRW_AcisBodyProjection> trial;
            DRW_AcisRootedProjectionReport trialReport;
            if (drw_extractAcisBodyWireframes(
                    lumpShellChainModel, trial, trialReport, limits)) {
                highFits = true;
                break;
            }
            low = high;
            high *= 2u;
        }
        while (highFits && low + 1u < high) {
            const std::size_t middle = low + (high - low) / 2u;
            DRW_SabParseLimits limits;
            if (outputBudget)
                limits.maxOutputItems = middle;
            else
                limits.maxExtractionWorkUnits = middle;
            std::vector<DRW_AcisBodyProjection> trial;
            DRW_AcisRootedProjectionReport trialReport;
            if (drw_extractAcisBodyWireframes(
                    lumpShellChainModel, trial, trialReport, limits))
                high = middle;
            else
                low = middle;
        }
        DRW_SabParseLimits exactLimits;
        if (outputBudget)
            exactLimits.maxOutputItems = high;
        else
            exactLimits.maxExtractionWorkUnits = high;
        std::vector<DRW_AcisBodyProjection> exact;
        DRW_AcisRootedProjectionReport exactReport;
        const bool exactFits = highFits
            && drw_extractAcisBodyWireframes(
                lumpShellChainModel, exact, exactReport, exactLimits);
        if (outputBudget)
            --exactLimits.maxOutputItems;
        else
            --exactLimits.maxExtractionWorkUnits;
        std::vector<DRW_AcisBodyProjection> under(1u);
        DRW_AcisRootedProjectionReport underReport;
        const bool underFits = drw_extractAcisBodyWireframes(
            lumpShellChainModel, under, underReport, exactLimits);
        return exactFits && !underFits && under.empty()
            && underReport.failure == DRW_AcisProjectionFailure::ResourceLimit;
    };
    t.expect(exactBudgetBoundary(true),
             "SAB 21800 lump shell census admits exact output budget and refuses one item under transactionally");
    t.expect(exactBudgetBoundary(false),
             "SAB 21800 lump shell census admits exact extraction-work budget and refuses one unit under transactionally");

    DRW_AcisBrep unrootedShellFaceWireframe;
    t.expect(drw_extractAcisWireframe(
                 shellFaceChainModel, unrootedShellFaceWireframe)
                 && unrootedShellFaceWireframe.lumps.empty()
                 && unrootedShellFaceWireframe.shells.empty(),
             "generic unrooted SAB 21800 wireframe leaves rooted lump/shell census fields empty");

    AcisGraphFixture detachedShellFaceFixture = shellFaceChainFixture;
    const int detachedShellFace =
        detachedShellFaceFixture.addNode("face", 6u);
    detachedShellFaceFixture.link(
        detachedShellFace, 3u, shellFaceRoot.shell);
    DRW_AcisModel detachedShellFaceModel;
    DRW_AcisRootedProjectionReport detachedShellFaceReport;
    std::vector<DRW_AcisBodyProjection> detachedShellFaceProjections;
    const bool detachedShellFaceParsedAndProjected = parseAndProject21800(
        detachedShellFaceFixture, detachedShellFaceModel,
        detachedShellFaceProjections, detachedShellFaceReport);
    const DRW_AcisShell* detachedShellFaceProjection = findProjectedShell(
        detachedShellFaceProjections, shellFaceRoot.shell);
    t.expect(detachedShellFaceParsedAndProjected
                 && detachedShellFaceProjection
                 && detachedShellFaceProjection->faceUseIndexCoversParsedFaces
                 && detachedShellFaceProjection->faceUseNodeIndices
                    == std::vector<int>{earlierFaceBySource,
                                         shellFaceRoot.face,
                                         detachedShellFace}
                 && !detachedShellFaceProjection->faceChainMatchesUseIndex
                 && drw_acisProjectionCoverage(
                        detachedShellFaceProjections.front())
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "SAB 21800 detached parsed face naming a shell prevents a false face-chain census match");

    AcisGraphFixture missingShellFaceOwnerFixture = shellFaceChainFixture;
    const int missingShellFaceOwner =
        missingShellFaceOwnerFixture.addNode("face", 6u);
    DRW_AcisModel missingShellFaceOwnerModel;
    DRW_AcisRootedProjectionReport missingShellFaceOwnerReport;
    std::vector<DRW_AcisBodyProjection> missingShellFaceOwnerProjections;
    const bool missingShellFaceOwnerParsedAndProjected = parseAndProject21800(
        missingShellFaceOwnerFixture, missingShellFaceOwnerModel,
        missingShellFaceOwnerProjections, missingShellFaceOwnerReport);
    const DRW_AcisShell* missingShellFaceOwnerProjection = findProjectedShell(
        missingShellFaceOwnerProjections, shellFaceRoot.shell);
    t.expect(missingShellFaceOwnerParsedAndProjected
                 && missingShellFaceOwnerProjection
                 && !missingShellFaceOwnerProjection
                        ->faceUseIndexCoversParsedFaces
                 && missingShellFaceOwnerProjection
                        ->faceUseIndexUnclassifiedFaceCount == 1u
                 && missingShellFaceOwnerProjection
                        ->firstUnclassifiedFaceOwnerNodeIndex
                    == missingShellFaceOwner
                 && !missingShellFaceOwnerProjection
                        ->faceChainMatchesUseIndex,
             "SAB 21800 missing parsed face owner marks the shell face census incomplete at the exact face");

    AcisGraphFixture wrongTypeShellFaceOwnerFixture = shellFaceChainFixture;
    const int wrongTypeShellFaceOwner =
        wrongTypeShellFaceOwnerFixture.addNode("face", 6u);
    wrongTypeShellFaceOwnerFixture.link(
        wrongTypeShellFaceOwner, 3u, shellFaceRoot.point0);
    DRW_AcisModel wrongTypeShellFaceOwnerModel;
    DRW_AcisRootedProjectionReport wrongTypeShellFaceOwnerReport;
    std::vector<DRW_AcisBodyProjection> wrongTypeShellFaceOwnerProjections;
    const bool wrongTypeShellFaceOwnerParsedAndProjected = parseAndProject21800(
        wrongTypeShellFaceOwnerFixture, wrongTypeShellFaceOwnerModel,
        wrongTypeShellFaceOwnerProjections, wrongTypeShellFaceOwnerReport);
    const DRW_AcisShell* wrongTypeShellFaceOwnerProjection = findProjectedShell(
        wrongTypeShellFaceOwnerProjections, shellFaceRoot.shell);
    t.expect(wrongTypeShellFaceOwnerParsedAndProjected
                 && wrongTypeShellFaceOwnerProjection
                 && !wrongTypeShellFaceOwnerProjection
                        ->faceUseIndexCoversParsedFaces
                 && wrongTypeShellFaceOwnerProjection
                        ->faceUseIndexUnclassifiedFaceCount == 1u
                 && wrongTypeShellFaceOwnerProjection
                        ->firstUnclassifiedFaceOwnerNodeIndex
                    == wrongTypeShellFaceOwner
                 && !wrongTypeShellFaceOwnerProjection
                        ->faceChainMatchesUseIndex,
             "SAB 21800 wrong-type parsed face owner prevents a complete shell face census");

    AcisGraphFixture cyclicShellFaceFixture = shellFaceChainFixture;
    cyclicShellFaceFixture.link(earlierFaceBySource, 1u,
                                shellFaceRoot.face);
    DRW_AcisModel cyclicShellFaceModel;
    DRW_AcisRootedProjectionReport cyclicShellFaceReport;
    std::vector<DRW_AcisBodyProjection> cyclicShellFaceProjections;
    const bool cyclicShellFaceParsedAndProjected = parseAndProject21800(
        cyclicShellFaceFixture, cyclicShellFaceModel,
        cyclicShellFaceProjections, cyclicShellFaceReport);
    const DRW_AcisShell* cyclicShellFaceProjection = findProjectedShell(
        cyclicShellFaceProjections, shellFaceRoot.shell);
    t.expect(cyclicShellFaceParsedAndProjected && cyclicShellFaceProjection
                 && cyclicShellFaceProjection->faceChainCycleDetected
                 && !cyclicShellFaceProjection->faceChainNullTerminated
                 && cyclicShellFaceProjection
                        ->firstRepeatedFaceChainNodeIndex
                    == shellFaceRoot.face
                 && cyclicShellFaceProjection->faceNextNodeIndices
                    == std::vector<int>{earlierFaceBySource,
                                         shellFaceRoot.face}
                 && !cyclicShellFaceProjection->faceChainMatchesUseIndex,
             "SAB 21800 shell face cycle remains distinct from a complete null-terminated chain");

    AcisGraphFixture wrongTypeShellFaceNextFixture = shellFaceChainFixture;
    wrongTypeShellFaceNextFixture.link(
        shellFaceRoot.face, 1u, shellFaceRoot.point0);
    DRW_AcisModel wrongTypeShellFaceNextModel;
    DRW_AcisRootedProjectionReport wrongTypeShellFaceNextReport;
    std::vector<DRW_AcisBodyProjection> wrongTypeShellFaceNextProjections;
    const bool wrongTypeShellFaceNextProjected = parseAndProject21800(
        wrongTypeShellFaceNextFixture, wrongTypeShellFaceNextModel,
        wrongTypeShellFaceNextProjections, wrongTypeShellFaceNextReport);
    t.expect(!wrongTypeShellFaceNextProjected
                 && wrongTypeShellFaceNextProjections.empty()
                 && wrongTypeShellFaceNextReport.failure
                        == DRW_AcisProjectionFailure::InvalidReference
                 && wrongTypeShellFaceNextReport.failedNodeIndex
                    == shellFaceRoot.face
                 && wrongTypeShellFaceNextReport.failedPointerOrdinal == 1u,
             "SAB 21800 wrong-type traversed shell face-next target is refused transactionally at its exact slot");

    DRW_SabParseLimits shellFaceBudget;
    std::size_t lowShellFaceBudget = 0u;
    std::size_t highShellFaceBudget = 1u;
    bool highShellFaceBudgetFits = false;
    while (highShellFaceBudget <= 8192u) {
        shellFaceBudget.maxOutputItems = highShellFaceBudget;
        std::vector<DRW_AcisBodyProjection> trialProjections;
        DRW_AcisRootedProjectionReport trialReport;
        if (drw_extractAcisBodyWireframes(
                shellFaceChainModel, trialProjections, trialReport,
                shellFaceBudget)) {
            highShellFaceBudgetFits = true;
            break;
        }
        lowShellFaceBudget = highShellFaceBudget;
        highShellFaceBudget *= 2u;
    }
    while (highShellFaceBudgetFits
           && lowShellFaceBudget + 1u < highShellFaceBudget) {
        const std::size_t trialBudget = lowShellFaceBudget
            + (highShellFaceBudget - lowShellFaceBudget) / 2u;
        DRW_SabParseLimits trialLimits;
        trialLimits.maxOutputItems = trialBudget;
        std::vector<DRW_AcisBodyProjection> trialProjections;
        DRW_AcisRootedProjectionReport trialReport;
        if (drw_extractAcisBodyWireframes(
                shellFaceChainModel, trialProjections, trialReport,
                trialLimits))
            highShellFaceBudget = trialBudget;
        else
            lowShellFaceBudget = trialBudget;
    }
    shellFaceBudget.maxOutputItems = highShellFaceBudget;
    std::vector<DRW_AcisBodyProjection> exactShellFaceProjections;
    DRW_AcisRootedProjectionReport exactShellFaceReport;
    const bool exactShellFaceBudgetFits = highShellFaceBudgetFits
        && drw_extractAcisBodyWireframes(
            shellFaceChainModel, exactShellFaceProjections,
            exactShellFaceReport, shellFaceBudget);
    DRW_SabParseLimits underShellFaceBudget = shellFaceBudget;
    if (underShellFaceBudget.maxOutputItems > 0u)
        --underShellFaceBudget.maxOutputItems;
    std::vector<DRW_AcisBodyProjection> rejectedShellFaceProjections(1u);
    DRW_AcisRootedProjectionReport rejectedShellFaceReport;
    const bool underShellFaceBudgetFits = underShellFaceBudget.maxOutputItems > 0u
        && drw_extractAcisBodyWireframes(
            shellFaceChainModel, rejectedShellFaceProjections,
            rejectedShellFaceReport, underShellFaceBudget);
    const DRW_AcisShell* exactShellFace = findProjectedShell(
        exactShellFaceProjections, shellFaceRoot.shell);
    t.expect(exactShellFaceBudgetFits && exactShellFace
                 && exactShellFace->faceChainMatchesUseIndex
                 && !underShellFaceBudgetFits
                 && rejectedShellFaceProjections.empty()
                 && rejectedShellFaceReport.failure
                    == DRW_AcisProjectionFailure::ResourceLimit,
             "SAB 21800 shell face chain census admits its exact output budget and refuses one item under transactionally");

    DRW_SabParseLimits shellFaceWorkLimits;
    std::size_t lowShellFaceWork = 0u;
    std::size_t highShellFaceWork = 1u;
    bool highShellFaceWorkFits = false;
    while (highShellFaceWork <= 65536u) {
        shellFaceWorkLimits.maxExtractionWorkUnits = highShellFaceWork;
        std::vector<DRW_AcisBodyProjection> trialProjections;
        DRW_AcisRootedProjectionReport trialReport;
        if (drw_extractAcisBodyWireframes(
                shellFaceChainModel, trialProjections, trialReport,
                shellFaceWorkLimits)) {
            highShellFaceWorkFits = true;
            break;
        }
        lowShellFaceWork = highShellFaceWork;
        highShellFaceWork *= 2u;
    }
    while (highShellFaceWorkFits
           && lowShellFaceWork + 1u < highShellFaceWork) {
        const std::size_t trialWork = lowShellFaceWork
            + (highShellFaceWork - lowShellFaceWork) / 2u;
        DRW_SabParseLimits trialLimits;
        trialLimits.maxExtractionWorkUnits = trialWork;
        std::vector<DRW_AcisBodyProjection> trialProjections;
        DRW_AcisRootedProjectionReport trialReport;
        if (drw_extractAcisBodyWireframes(
                shellFaceChainModel, trialProjections, trialReport,
                trialLimits))
            highShellFaceWork = trialWork;
        else
            lowShellFaceWork = trialWork;
    }
    shellFaceWorkLimits.maxExtractionWorkUnits = highShellFaceWork;
    std::vector<DRW_AcisBodyProjection> exactShellFaceWorkProjections;
    DRW_AcisRootedProjectionReport exactShellFaceWorkReport;
    const bool exactShellFaceWorkFits = highShellFaceWorkFits
        && drw_extractAcisBodyWireframes(
            shellFaceChainModel, exactShellFaceWorkProjections,
            exactShellFaceWorkReport, shellFaceWorkLimits);
    DRW_SabParseLimits underShellFaceWorkLimits = shellFaceWorkLimits;
    if (underShellFaceWorkLimits.maxExtractionWorkUnits > 0u)
        --underShellFaceWorkLimits.maxExtractionWorkUnits;
    std::vector<DRW_AcisBodyProjection> rejectedShellFaceWorkProjections(1u);
    DRW_AcisRootedProjectionReport rejectedShellFaceWorkReport;
    const bool underShellFaceWorkFits =
        underShellFaceWorkLimits.maxExtractionWorkUnits > 0u
        && drw_extractAcisBodyWireframes(
            shellFaceChainModel, rejectedShellFaceWorkProjections,
            rejectedShellFaceWorkReport, underShellFaceWorkLimits);
    t.expect(exactShellFaceWorkFits
                 && !underShellFaceWorkFits
                 && rejectedShellFaceWorkProjections.empty()
                 && rejectedShellFaceWorkReport.failure
                    == DRW_AcisProjectionFailure::ResourceLimit,
             "SAB 21800 shell face census admits exact extraction work and refuses one unit under transactionally");

    DRW_SabParseLimits exactFaceUseBudget;
    std::size_t lowFaceUseBudget = 0u;
    std::size_t highFaceUseBudget = 1u;
    bool highFaceUseBudgetFits = false;
    while (highFaceUseBudget <= 4096u) {
        exactFaceUseBudget.maxOutputItems = highFaceUseBudget;
        std::vector<DRW_AcisBodyProjection> trialFaceUseProjections;
        DRW_AcisRootedProjectionReport trialFaceUseReport;
        if (drw_extractAcisBodyWireframes(
                matchingPartnerEdgeModel, trialFaceUseProjections,
                trialFaceUseReport, exactFaceUseBudget)) {
            highFaceUseBudgetFits = true;
            break;
        }
        lowFaceUseBudget = highFaceUseBudget;
        highFaceUseBudget *= 2u;
    }
    while (highFaceUseBudgetFits
           && lowFaceUseBudget + 1u < highFaceUseBudget) {
        const std::size_t trialBudget = lowFaceUseBudget
            + (highFaceUseBudget - lowFaceUseBudget) / 2u;
        DRW_SabParseLimits trialLimits;
        trialLimits.maxOutputItems = trialBudget;
        std::vector<DRW_AcisBodyProjection> trialFaceUseProjections;
        DRW_AcisRootedProjectionReport trialFaceUseReport;
        if (drw_extractAcisBodyWireframes(
                matchingPartnerEdgeModel, trialFaceUseProjections,
                trialFaceUseReport, trialLimits))
            highFaceUseBudget = trialBudget;
        else
            lowFaceUseBudget = trialBudget;
    }
    exactFaceUseBudget.maxOutputItems = highFaceUseBudget;
    std::vector<DRW_AcisBodyProjection> exactFaceUseProjections;
    DRW_AcisRootedProjectionReport exactFaceUseReport;
    const bool exactFaceUseFits = highFaceUseBudgetFits
        && drw_extractAcisBodyWireframes(
            matchingPartnerEdgeModel, exactFaceUseProjections,
            exactFaceUseReport, exactFaceUseBudget);
    DRW_SabParseLimits underFaceUseBudget = exactFaceUseBudget;
    if (underFaceUseBudget.maxOutputItems > 0u)
        --underFaceUseBudget.maxOutputItems;
    std::vector<DRW_AcisBodyProjection> rejectedFaceUseProjections(1u);
    DRW_AcisRootedProjectionReport rejectedFaceUseReport;
    const bool underFaceUseFits = underFaceUseBudget.maxOutputItems > 0u
        && drw_extractAcisBodyWireframes(
            matchingPartnerEdgeModel, rejectedFaceUseProjections,
            rejectedFaceUseReport, underFaceUseBudget);
    const DRW_AcisFace* exactFaceUseProjection = findProjectedFace(
        exactFaceUseProjections, root.face);
    t.expect(exactFaceUseFits && exactFaceUseProjection
                 && exactFaceUseProjection->faceLoopChainMatchesUseIndex
                 && exactFaceUseProjection->loopUseNodeIndices
                    == std::vector<int>{root.loop, secondLoop},
             "SAB 21800 face-loop use census fits its measured exact output budget");
    t.expect(!underFaceUseFits && rejectedFaceUseProjections.empty()
                 && rejectedFaceUseReport.failure
                        == DRW_AcisProjectionFailure::ResourceLimit,
             "SAB 21800 face-loop use census refuses one item below its exact output budget transactionally");

    const auto minimumLoopCoedgeBudget = [&loopCoedgeSourceOrderModel](
        bool outputBudget) {
        std::size_t low = 0u;
        std::size_t high = 1u;
        bool highFits = false;
        while (high <= 65536u) {
            DRW_SabParseLimits limits;
            if (outputBudget)
                limits.maxOutputItems = high;
            else
                limits.maxExtractionWorkUnits = high;
            std::vector<DRW_AcisBodyProjection> trialProjections;
            DRW_AcisRootedProjectionReport trialReport;
            if (drw_extractAcisBodyWireframes(
                    loopCoedgeSourceOrderModel, trialProjections,
                    trialReport, limits)) {
                highFits = true;
                break;
            }
            low = high;
            high *= 2u;
        }
        if (!highFits)
            return std::size_t{0u};
        while (low + 1u < high) {
            const std::size_t trialBudget =
                low + (high - low) / 2u;
            DRW_SabParseLimits limits;
            if (outputBudget)
                limits.maxOutputItems = trialBudget;
            else
                limits.maxExtractionWorkUnits = trialBudget;
            std::vector<DRW_AcisBodyProjection> trialProjections;
            DRW_AcisRootedProjectionReport trialReport;
            if (drw_extractAcisBodyWireframes(
                    loopCoedgeSourceOrderModel, trialProjections,
                    trialReport, limits))
                high = trialBudget;
            else
                low = trialBudget;
        }
        return high;
    };
    const std::size_t exactLoopCoedgeOutputBudget =
        minimumLoopCoedgeBudget(true);
    DRW_SabParseLimits exactLoopCoedgeOutputLimits;
    exactLoopCoedgeOutputLimits.maxOutputItems =
        exactLoopCoedgeOutputBudget;
    std::vector<DRW_AcisBodyProjection> exactLoopCoedgeOutputProjections;
    DRW_AcisRootedProjectionReport exactLoopCoedgeOutputReport;
    const bool exactLoopCoedgeOutputFits = exactLoopCoedgeOutputBudget > 0u
        && drw_extractAcisBodyWireframes(
            loopCoedgeSourceOrderModel, exactLoopCoedgeOutputProjections,
            exactLoopCoedgeOutputReport, exactLoopCoedgeOutputLimits);
    DRW_SabParseLimits underLoopCoedgeOutputLimits =
        exactLoopCoedgeOutputLimits;
    if (underLoopCoedgeOutputLimits.maxOutputItems > 0u)
        --underLoopCoedgeOutputLimits.maxOutputItems;
    std::vector<DRW_AcisBodyProjection> rejectedLoopCoedgeOutputProjections(1u);
    DRW_AcisRootedProjectionReport rejectedLoopCoedgeOutputReport;
    const bool underLoopCoedgeOutputFits =
        underLoopCoedgeOutputLimits.maxOutputItems > 0u
        && drw_extractAcisBodyWireframes(
            loopCoedgeSourceOrderModel, rejectedLoopCoedgeOutputProjections,
            rejectedLoopCoedgeOutputReport, underLoopCoedgeOutputLimits);
    t.expect(exactLoopCoedgeOutputFits
                 && !underLoopCoedgeOutputFits
                 && rejectedLoopCoedgeOutputProjections.empty()
                 && rejectedLoopCoedgeOutputReport.failure
                    == DRW_AcisProjectionFailure::ResourceLimit,
             "SAB 21800 loop coedge census admits its exact output budget and refuses one item under transactionally");

    const std::size_t exactLoopCoedgeWorkBudget =
        minimumLoopCoedgeBudget(false);
    DRW_SabParseLimits exactLoopCoedgeWorkLimits;
    exactLoopCoedgeWorkLimits.maxExtractionWorkUnits =
        exactLoopCoedgeWorkBudget;
    std::vector<DRW_AcisBodyProjection> exactLoopCoedgeWorkProjections;
    DRW_AcisRootedProjectionReport exactLoopCoedgeWorkReport;
    const bool exactLoopCoedgeWorkFits = exactLoopCoedgeWorkBudget > 0u
        && drw_extractAcisBodyWireframes(
            loopCoedgeSourceOrderModel, exactLoopCoedgeWorkProjections,
            exactLoopCoedgeWorkReport, exactLoopCoedgeWorkLimits);
    DRW_SabParseLimits underLoopCoedgeWorkLimits = exactLoopCoedgeWorkLimits;
    if (underLoopCoedgeWorkLimits.maxExtractionWorkUnits > 0u)
        --underLoopCoedgeWorkLimits.maxExtractionWorkUnits;
    std::vector<DRW_AcisBodyProjection> rejectedLoopCoedgeWorkProjections(1u);
    DRW_AcisRootedProjectionReport rejectedLoopCoedgeWorkReport;
    const bool underLoopCoedgeWorkFits =
        underLoopCoedgeWorkLimits.maxExtractionWorkUnits > 0u
        && drw_extractAcisBodyWireframes(
            loopCoedgeSourceOrderModel, rejectedLoopCoedgeWorkProjections,
            rejectedLoopCoedgeWorkReport, underLoopCoedgeWorkLimits);
    t.expect(exactLoopCoedgeWorkFits && !underLoopCoedgeWorkFits
                 && rejectedLoopCoedgeWorkProjections.empty()
                 && rejectedLoopCoedgeWorkReport.failure
                    == DRW_AcisProjectionFailure::ResourceLimit,
             "SAB 21800 loop coedge census admits exact extraction work and refuses one unit under transactionally");

    AcisGraphFixture detachedLoopCoedgeUseFixture =
        matchingPartnerEdgeFixture;
    const int detachedLoopCoedgeUse =
        detachedLoopCoedgeUseFixture.addNode("coedge", 7u);
    detachedLoopCoedgeUseFixture.link(detachedLoopCoedgeUse, 4u, root.edge);
    detachedLoopCoedgeUseFixture.link(detachedLoopCoedgeUse, 5u, root.loop);
    DRW_AcisModel detachedLoopCoedgeUseModel;
    DRW_AcisRootedProjectionReport detachedLoopCoedgeUseReport;
    std::vector<DRW_AcisBodyProjection> detachedLoopCoedgeUseProjections;
    const bool detachedLoopCoedgeUseParsedAndProjected =
        parseAndProject21800(
            detachedLoopCoedgeUseFixture, detachedLoopCoedgeUseModel,
            detachedLoopCoedgeUseProjections,
            detachedLoopCoedgeUseReport);
    const DRW_AcisFaceLoop* detachedLoopCoedgeUseProjection =
        findProjectedLoop(detachedLoopCoedgeUseProjections, root.loop);
    t.expect(detachedLoopCoedgeUseParsedAndProjected
                 && detachedLoopCoedgeUseProjection
                 && detachedLoopCoedgeUseProjection
                        ->coedgeUseIndexCoversParsedCoedges
                 && detachedLoopCoedgeUseProjection
                        ->coedgeUseIndexUnclassifiedCoedgeCount == 0u
                 && detachedLoopCoedgeUseProjection->coedgeUseNodeIndices
                    == std::vector<int>{root.coedge, thirdCoedge, fourthCoedge,
                                        detachedLoopCoedgeUse}
                 && !detachedLoopCoedgeUseProjection
                        ->coedgeChainMatchesUseIndex
                 && drw_acisProjectionCoverage(
                        detachedLoopCoedgeUseProjections.front())
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "SAB 21800 parser-fed loop coedge census exposes a detached coedge that names a loop but is absent from its next-chain");

    const auto checkUnclassifiedLoopCoedgeOwner =
        [&](AcisGraphFixture fixture, int coedgeNodeIndex,
            const char* description) {
            DRW_AcisModel model;
            DRW_AcisRootedProjectionReport report;
            std::vector<DRW_AcisBodyProjection> projections;
            const bool parsedAndProjected = parseAndProject21800(
                fixture, model, projections, report);
            const DRW_AcisFaceLoop* loop =
                findProjectedLoop(projections, root.loop);
            t.expect(parsedAndProjected && loop
                         && !loop->coedgeUseIndexCoversParsedCoedges
                         && loop->coedgeUseIndexUnclassifiedCoedgeCount == 1u
                         && loop->firstUnclassifiedCoedgeOwnerNodeIndex
                            == coedgeNodeIndex
                         && !loop->coedgeChainMatchesUseIndex
                         && drw_acisProjectionCoverage(projections.front())
                            == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
                     description);
        };

    AcisGraphFixture missingLoopOwnerFixture = matchingPartnerEdgeFixture;
    const int missingLoopOwnerCoedge =
        missingLoopOwnerFixture.addNode("coedge", 5u);
    missingLoopOwnerFixture.link(missingLoopOwnerCoedge, 4u, root.edge);
    checkUnclassifiedLoopCoedgeOwner(
        missingLoopOwnerFixture, missingLoopOwnerCoedge,
        "SAB 21800 parser-fed missing ordinal-5 coedge owner prevents a complete loop coedge census");

    AcisGraphFixture nullLoopOwnerFixture = matchingPartnerEdgeFixture;
    const int nullLoopOwnerCoedge = nullLoopOwnerFixture.addNode("coedge", 6u);
    nullLoopOwnerFixture.link(nullLoopOwnerCoedge, 4u, root.edge);
    checkUnclassifiedLoopCoedgeOwner(
        nullLoopOwnerFixture, nullLoopOwnerCoedge,
        "SAB 21800 parser-fed null ordinal-5 coedge owner prevents a complete loop coedge census");

    AcisGraphFixture wrongTypeLoopOwnerFixture = matchingPartnerEdgeFixture;
    const int wrongTypeLoopOwnerCoedge =
        wrongTypeLoopOwnerFixture.addNode("coedge", 6u);
    wrongTypeLoopOwnerFixture.link(wrongTypeLoopOwnerCoedge, 4u, root.edge);
    wrongTypeLoopOwnerFixture.link(wrongTypeLoopOwnerCoedge, 5u, root.point0);
    checkUnclassifiedLoopCoedgeOwner(
        wrongTypeLoopOwnerFixture, wrongTypeLoopOwnerCoedge,
        "SAB 21800 parser-fed wrong-type ordinal-5 coedge owner prevents a complete loop coedge census");

    AcisGraphFixture detachedFaceLoopFixture = matchingPartnerEdgeFixture;
    const int detachedFaceLoop =
        detachedFaceLoopFixture.addNode("loop", 4u);
    detachedFaceLoopFixture.link(detachedFaceLoop, 3u, root.face);
    DRW_AcisModel detachedFaceLoopModel;
    DRW_AcisRootedProjectionReport detachedFaceLoopReport;
    std::vector<DRW_AcisBodyProjection> detachedFaceLoopProjections;
    const bool detachedFaceLoopParsedAndProjected = parseAndProject21800(
        detachedFaceLoopFixture, detachedFaceLoopModel,
        detachedFaceLoopProjections, detachedFaceLoopReport);
    const DRW_AcisFace* detachedFaceLoopProjection = findProjectedFace(
        detachedFaceLoopProjections, root.face);
    t.expect(detachedFaceLoopParsedAndProjected
                 && detachedFaceLoopProjection
                 && detachedFaceLoopProjection->loopUseIndexCoversParsedLoops
                 && detachedFaceLoopProjection->loopUseNodeIndices
                    == std::vector<int>{root.loop, secondLoop,
                                        detachedFaceLoop}
                 && !detachedFaceLoopProjection
                        ->faceLoopChainMatchesUseIndex
                 && detachedFaceLoopProjection->loops.size() == 2u
                 && drw_acisProjectionCoverage(
                        detachedFaceLoopProjections.front())
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "SAB 21800 parser-fed face loop census exposes a detached loop that names the face but is absent from its next-chain");

    AcisGraphFixture missingFaceLoopOwnerFixture =
        matchingPartnerEdgeFixture;
    const int missingFaceLoopOwner =
        missingFaceLoopOwnerFixture.addNode("loop", 3u);
    DRW_AcisModel missingFaceLoopOwnerModel;
    DRW_AcisRootedProjectionReport missingFaceLoopOwnerReport;
    std::vector<DRW_AcisBodyProjection> missingFaceLoopOwnerProjections;
    const bool missingFaceLoopOwnerParsedAndProjected = parseAndProject21800(
        missingFaceLoopOwnerFixture, missingFaceLoopOwnerModel,
        missingFaceLoopOwnerProjections, missingFaceLoopOwnerReport);
    const DRW_AcisFace* missingFaceLoopOwnerProjection = findProjectedFace(
        missingFaceLoopOwnerProjections, root.face);
    t.expect(missingFaceLoopOwnerParsedAndProjected
                 && missingFaceLoopOwnerProjection
                 && missingFaceLoopOwnerProjection
                        ->loopUseIndexUnclassifiedLoopCount == 1u
                 && !missingFaceLoopOwnerProjection
                        ->loopUseIndexCoversParsedLoops
                 && missingFaceLoopOwnerProjection
                        ->firstUnclassifiedLoopOwnerNodeIndex
                    == missingFaceLoopOwner
                 && !missingFaceLoopOwnerProjection
                        ->faceLoopChainMatchesUseIndex
                 && drw_acisProjectionCoverage(
                        missingFaceLoopOwnerProjections.front())
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "SAB 21800 parser-fed missing ordinal-3 loop owner prevents a complete face loop census");

    AcisGraphFixture wrongTypeFaceLoopOwnerFixture =
        matchingPartnerEdgeFixture;
    const int wrongTypeFaceLoopOwner =
        wrongTypeFaceLoopOwnerFixture.addNode("loop", 4u);
    wrongTypeFaceLoopOwnerFixture.link(wrongTypeFaceLoopOwner, 3u,
                                       root.point0);
    DRW_AcisModel wrongTypeFaceLoopOwnerModel;
    DRW_AcisRootedProjectionReport wrongTypeFaceLoopOwnerReport;
    std::vector<DRW_AcisBodyProjection> wrongTypeFaceLoopOwnerProjections;
    const bool wrongTypeFaceLoopOwnerParsedAndProjected =
        parseAndProject21800(
            wrongTypeFaceLoopOwnerFixture, wrongTypeFaceLoopOwnerModel,
            wrongTypeFaceLoopOwnerProjections,
            wrongTypeFaceLoopOwnerReport);
    const DRW_AcisFace* wrongTypeFaceLoopOwnerProjection =
        findProjectedFace(wrongTypeFaceLoopOwnerProjections, root.face);
    t.expect(wrongTypeFaceLoopOwnerParsedAndProjected
                 && wrongTypeFaceLoopOwnerProjection
                 && wrongTypeFaceLoopOwnerProjection
                        ->loopUseIndexUnclassifiedLoopCount == 1u
                 && !wrongTypeFaceLoopOwnerProjection
                        ->loopUseIndexCoversParsedLoops
                 && wrongTypeFaceLoopOwnerProjection
                        ->firstUnclassifiedLoopOwnerNodeIndex
                    == wrongTypeFaceLoopOwner
                 && !wrongTypeFaceLoopOwnerProjection
                        ->faceLoopChainMatchesUseIndex
                 && drw_acisProjectionCoverage(
                        wrongTypeFaceLoopOwnerProjections.front())
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "SAB 21800 parser-fed wrong-type ordinal-3 loop owner prevents a complete face loop census");

    AcisGraphFixture missingStartVertexFixture = complexFixture;
    missingStartVertexFixture.link(root.edge, 1u, -1);
    DRW_AcisModel missingStartVertexModel;
    DRW_AcisRootedProjectionReport missingStartVertexReport;
    std::vector<DRW_AcisBodyProjection> missingStartVertexProjections;
    const bool missingStartVertexParsedAndProjected = parseAndProject21800(
        missingStartVertexFixture, missingStartVertexModel,
        missingStartVertexProjections, missingStartVertexReport);
    const DRW_AcisEdge* missingStartVertexEdge = findProjectedEdge(
        missingStartVertexProjections, root.edge);
    t.expect(missingStartVertexParsedAndProjected && missingStartVertexEdge
                 && missingStartVertexEdge->startVertexNodeIndex == -1
                 && missingStartVertexEdge->endVertexNodeIndex
                    == root.vertex1
                 && !missingStartVertexEdge->hasStart
                 && missingStartVertexEdge->hasEnd
                 && acisCoordNear(missingStartVertexEdge->end, 7.0, 5.0, 6.0)
                 && drw_acisProjectionCoverage(
                        missingStartVertexProjections.front())
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "parser-fed null edge ordinal-1 preserves the ordinal-2 endpoint without shifting it to the start");

    AcisGraphFixture missingEndVertexFixture = complexFixture;
    missingEndVertexFixture.link(root.edge, 2u, -1);
    DRW_AcisModel missingEndVertexModel;
    DRW_AcisRootedProjectionReport missingEndVertexReport;
    std::vector<DRW_AcisBodyProjection> missingEndVertexProjections;
    const bool missingEndVertexParsedAndProjected = parseAndProject21800(
        missingEndVertexFixture, missingEndVertexModel,
        missingEndVertexProjections, missingEndVertexReport);
    const DRW_AcisEdge* missingEndVertexEdge = findProjectedEdge(
        missingEndVertexProjections, root.edge);
    const bool missingEndVertexPreserved =
        missingEndVertexParsedAndProjected && missingEndVertexEdge
        && missingEndVertexEdge->startVertexNodeIndex == root.vertex0
        && missingEndVertexEdge->endVertexNodeIndex == -1
        && missingEndVertexEdge->hasStart
        && !missingEndVertexEdge->hasEnd
        && acisCoordNear(missingEndVertexEdge->start, 3.0, 2.0, 3.0)
        && !missingEndVertexProjections.empty()
        && !missingEndVertexProjections.front().wireframe.faces.empty()
        && !missingEndVertexProjections.front().wireframe.faces.front()
                .loops.empty()
        && missingEndVertexProjections.front().wireframe.faces.front()
                .loops.front().coedgeOrientedStartVertexNodeIndices
            == std::vector<int>{root.vertex0}
        && missingEndVertexProjections.front().wireframe.faces.front()
                .loops.front().coedgeOrientedEndVertexNodeIndices
            == std::vector<int>{-1}
        && !missingEndVertexProjections.front().wireframe.faces.front()
                .loops.front().hasCoedgeEndpointChainObservation
        && drw_acisProjectionCoverage(missingEndVertexProjections.front())
            == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps;
    if (!missingEndVertexPreserved) {
        std::cerr << "missing end vertex detail: parsed/projected="
                  << missingEndVertexParsedAndProjected
                  << " edge=" << (missingEndVertexEdge != nullptr);
        if (missingEndVertexEdge != nullptr) {
            std::cerr << " startNode="
                      << missingEndVertexEdge->startVertexNodeIndex
                      << " expectedStart=" << root.vertex0
                      << " endNode="
                      << missingEndVertexEdge->endVertexNodeIndex
                      << " hasStart=" << missingEndVertexEdge->hasStart
                      << " hasEnd=" << missingEndVertexEdge->hasEnd
                      << " start=" << missingEndVertexEdge->start.x << ','
                      << missingEndVertexEdge->start.y << ','
                      << missingEndVertexEdge->start.z;
        }
        if (!missingEndVertexProjections.empty())
            std::cerr << " coverage=" << static_cast<int>(
                drw_acisProjectionCoverage(
                    missingEndVertexProjections.front()));
        std::cerr << '\n';
    }
    t.expect(missingEndVertexPreserved,
             "parser-fed null edge ordinal-2 preserves the ordinal-1 endpoint and marks only the end unresolved");

    AcisGraphFixture missingOrdinalFourFixture = matchingPartnerEdgeFixture;
    const int missingOrdinalFourCoedge =
        missingOrdinalFourFixture.addNode("coedge", 4u);
    DRW_AcisModel missingOrdinalFourModel;
    DRW_AcisRootedProjectionReport missingOrdinalFourReport;
    std::vector<DRW_AcisBodyProjection> missingOrdinalFourProjections;
    const bool missingOrdinalFourParsedAndProjected = parseAndProject21800(
        missingOrdinalFourFixture, missingOrdinalFourModel,
        missingOrdinalFourProjections, missingOrdinalFourReport);
    const DRW_AcisEdge* missingOrdinalFourEdge = findProjectedEdge(
        missingOrdinalFourProjections, root.edge);
    t.expect(missingOrdinalFourParsedAndProjected
                 && missingOrdinalFourEdge
                 && missingOrdinalFourEdge->coedgeEdgeUseNodeIndices
                    == std::vector<int>{root.coedge, secondCoedge,
                                        thirdCoedge, fourthCoedge}
                 && !missingOrdinalFourEdge
                        ->coedgeEdgeUseIndexCoversParsedCoedges
                 && missingOrdinalFourEdge
                        ->coedgeEdgeUseIndexUnclassifiedCoedgeCount == 1u
                 && missingOrdinalFourEdge
                        ->firstUnclassifiedCoedgeEdgeUseNodeIndex
                    == missingOrdinalFourCoedge
                 && !missingOrdinalFourEdge
                        ->coedgePartnerRingMatchesEdgeUseIndex
                 && drw_acisProjectionCoverage(
                        missingOrdinalFourProjections.front())
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "SAB 21800 parser-fed missing ordinal-4 coedge target prevents a complete edge-use census");

    AcisGraphFixture wrongTypeOrdinalFourFixture = matchingPartnerEdgeFixture;
    const int wrongTypeOrdinalFourCoedge =
        wrongTypeOrdinalFourFixture.addNode("coedge", 5u);
    wrongTypeOrdinalFourFixture.link(wrongTypeOrdinalFourCoedge, 4u,
                                     root.vertex0);
    DRW_AcisModel wrongTypeOrdinalFourModel;
    DRW_AcisRootedProjectionReport wrongTypeOrdinalFourReport;
    std::vector<DRW_AcisBodyProjection> wrongTypeOrdinalFourProjections;
    const bool wrongTypeOrdinalFourParsedAndProjected = parseAndProject21800(
        wrongTypeOrdinalFourFixture, wrongTypeOrdinalFourModel,
        wrongTypeOrdinalFourProjections, wrongTypeOrdinalFourReport);
    const DRW_AcisEdge* wrongTypeOrdinalFourEdge = findProjectedEdge(
        wrongTypeOrdinalFourProjections, root.edge);
    t.expect(wrongTypeOrdinalFourParsedAndProjected
                 && wrongTypeOrdinalFourEdge
                 && wrongTypeOrdinalFourEdge->coedgeEdgeUseNodeIndices
                    == std::vector<int>{root.coedge, secondCoedge,
                                        thirdCoedge, fourthCoedge}
                 && !wrongTypeOrdinalFourEdge
                        ->coedgeEdgeUseIndexCoversParsedCoedges
                 && wrongTypeOrdinalFourEdge
                        ->coedgeEdgeUseIndexUnclassifiedCoedgeCount == 1u
                 && wrongTypeOrdinalFourEdge
                        ->firstUnclassifiedCoedgeEdgeUseNodeIndex
                    == wrongTypeOrdinalFourCoedge
                 && !wrongTypeOrdinalFourEdge
                        ->coedgePartnerRingMatchesEdgeUseIndex
                 && drw_acisProjectionCoverage(
                        wrongTypeOrdinalFourProjections.front())
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "SAB 21800 parser-fed wrong-type ordinal-4 target prevents a complete edge-use census");

    AcisGraphFixture detachedEdgeUseFixture = matchingPartnerEdgeFixture;
    const int detachedEdgeUseCoedge =
        detachedEdgeUseFixture.addNode("coedge", 7u);
    detachedEdgeUseFixture.link(detachedEdgeUseCoedge, 4u, root.edge);
    DRW_AcisModel detachedEdgeUseModel;
    DRW_AcisRootedProjectionReport detachedEdgeUseReport;
    std::vector<DRW_AcisBodyProjection> detachedEdgeUseProjections;
    const bool detachedEdgeUseParsedAndProjected = parseAndProject21800(
        detachedEdgeUseFixture, detachedEdgeUseModel,
        detachedEdgeUseProjections, detachedEdgeUseReport);
    const DRW_AcisEdge* detachedEdgeUse = findProjectedEdge(
        detachedEdgeUseProjections, root.edge);
    t.expect(detachedEdgeUseParsedAndProjected && detachedEdgeUse
                 && detachedEdgeUse->coedgePartnerRingClosed
                 && detachedEdgeUse->coedgeEdgeUseIndexCoversParsedCoedges
                 && !detachedEdgeUse->coedgePartnerRingMatchesEdgeUseIndex
                 && detachedEdgeUse->coedgeEdgeUseNodeIndices
                    == std::vector<int>{root.coedge, secondCoedge,
                                        thirdCoedge, fourthCoedge,
                                        detachedEdgeUseCoedge}
                 && drw_acisProjectionCoverage(
                        detachedEdgeUseProjections.front())
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "SAB 21800 parser-fed edge-use indexing exposes a detached coedge omitted from the partner ring");

    AcisGraphFixture missingEdgeCoedgeBacklink = complexFixture;
    missingEdgeCoedgeBacklink.link(root.coedge, 4u, -1);
    const std::vector<std::uint8_t> missingEdgeCoedgeBacklinkBytes =
        sab21800FromGraph(missingEdgeCoedgeBacklink.data);
    DRW_SabData missingEdgeCoedgeBacklinkSab;
    const bool missingEdgeCoedgeBacklinkParsed =
        !missingEdgeCoedgeBacklinkBytes.empty()
        && drw_parseSab(missingEdgeCoedgeBacklinkBytes.data(),
                        missingEdgeCoedgeBacklinkBytes.size(),
                        missingEdgeCoedgeBacklinkSab);
    const DRW_AcisModel missingEdgeCoedgeBacklinkModel =
        drw_buildAcisModel(missingEdgeCoedgeBacklinkSab);
    std::vector<DRW_AcisBodyProjection> missingEdgeCoedgeBacklinkProjections;
    DRW_AcisRootedProjectionReport missingEdgeCoedgeBacklinkReport;
    const bool missingEdgeCoedgeBacklinkProjected =
        drw_extractAcisBodyWireframes(
            missingEdgeCoedgeBacklinkModel,
            missingEdgeCoedgeBacklinkProjections,
            missingEdgeCoedgeBacklinkReport);
    bool missingEdgeCoedgeBacklinkObserved = false;
    if (missingEdgeCoedgeBacklinkProjections.size() == 1u) {
        const DRW_AcisBrep& wireframe =
            missingEdgeCoedgeBacklinkProjections.front().wireframe;
        const auto found = std::find_if(
            wireframe.edges.begin(), wireframe.edges.end(),
            [root](const DRW_AcisEdge& edge) {
                return edge.nodeIndex == root.edge;
            });
        missingEdgeCoedgeBacklinkObserved =
            found != wireframe.edges.end()
            && found->coedgeNodeIndex == root.coedge
            && !found->hasReciprocalCoedgeLink
            && drw_acisProjectionCoverage(
                   missingEdgeCoedgeBacklinkProjections.front())
                == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps;
    }
    t.expect(missingEdgeCoedgeBacklinkParsed
                 && missingEdgeCoedgeBacklinkProjected
                 && missingEdgeCoedgeBacklinkObserved,
             "SAB 21800 parser-fed missing coedge back-link remains a known projection gap");
    AcisGraphFixture missingFaceShellFixture = complexFixture;
    missingFaceShellFixture.link(root.face, 3u, -1);
    const std::vector<std::uint8_t> missingFaceShellBytes =
        sab21800FromGraph(missingFaceShellFixture.data);
    DRW_SabData missingFaceShellSab;
    DRW_AcisModel missingFaceShellModel;
    DRW_SabParseOutcome missingFaceShellBuildOutcome;
    DRW_AcisRootedProjectionReport missingFaceShellReport;
    std::vector<DRW_AcisBodyProjection> missingFaceShellProjections;
    const bool missingFaceShellParsed = !missingFaceShellBytes.empty()
        && drw_parseSab(missingFaceShellBytes.data(),
                        missingFaceShellBytes.size(), missingFaceShellSab)
        && drw_buildAcisModelWithLimits(
            missingFaceShellSab, missingFaceShellModel,
            missingFaceShellBuildOutcome, DRW_SabParseLimits());
    const bool missingFaceShellProjected = missingFaceShellParsed
        && drw_extractAcisBodyWireframes(
            missingFaceShellModel, missingFaceShellProjections,
            missingFaceShellReport);
    const bool missingFaceShellObserved = missingFaceShellProjected
        && missingFaceShellProjections.size() == 1u
        && missingFaceShellProjections.front().wireframe.faces.size() == 1u
        && missingFaceShellProjections.front().wireframe.faces.front()
                .shellNodeIndex == -1
        && drw_acisProjectionCoverage(missingFaceShellProjections.front())
            == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps;
    t.expect(missingFaceShellObserved,
             "ACIS parser-fed rooted projection retains a missing face-shell association as a known gap");
    AcisGraphFixture missingFaceSurfaceFixture = complexFixture;
    missingFaceSurfaceFixture.link(root.face, 5u, -1);
    const std::vector<std::uint8_t> missingFaceSurfaceBytes =
        sab21800FromGraph(missingFaceSurfaceFixture.data);
    DRW_SabData missingFaceSurfaceSab;
    DRW_AcisModel missingFaceSurfaceModel;
    DRW_SabParseOutcome missingFaceSurfaceBuildOutcome;
    DRW_AcisRootedProjectionReport missingFaceSurfaceReport;
    std::vector<DRW_AcisBodyProjection> missingFaceSurfaceProjections;
    const bool missingFaceSurfaceParsed = !missingFaceSurfaceBytes.empty()
        && drw_parseSab(missingFaceSurfaceBytes.data(),
                        missingFaceSurfaceBytes.size(), missingFaceSurfaceSab)
        && drw_buildAcisModelWithLimits(
            missingFaceSurfaceSab, missingFaceSurfaceModel,
            missingFaceSurfaceBuildOutcome, DRW_SabParseLimits());
    const bool missingFaceSurfaceProjected = missingFaceSurfaceParsed
        && drw_extractAcisBodyWireframes(
            missingFaceSurfaceModel, missingFaceSurfaceProjections,
            missingFaceSurfaceReport);
    const bool missingFaceSurfaceObserved = missingFaceSurfaceProjected
        && missingFaceSurfaceProjections.size() == 1u
        && missingFaceSurfaceProjections.front().wireframe.faces.size() == 1u
        && missingFaceSurfaceProjections.front().wireframe.faces.front()
                .surfaceNodeIndex == -1
        && drw_acisProjectionCoverage(missingFaceSurfaceProjections.front())
            == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps;
    t.expect(missingFaceSurfaceObserved,
             "ACIS parser-fed rooted projection preserves a missing exact face-surface slot as a known gap");
    AcisGraphFixture missingEdgeEndpointFixture = complexFixture;
    missingEdgeEndpointFixture.link(root.edge, 1u, -1);
    const std::vector<std::uint8_t> missingEdgeEndpointBytes =
        sab21800FromGraph(missingEdgeEndpointFixture.data);
    DRW_SabData missingEdgeEndpointSab;
    DRW_AcisModel missingEdgeEndpointModel;
    DRW_SabParseOutcome missingEdgeEndpointBuildOutcome;
    DRW_AcisRootedProjectionReport missingEdgeEndpointReport;
    std::vector<DRW_AcisBodyProjection> missingEdgeEndpointProjections;
    const bool missingEdgeEndpointParsed = !missingEdgeEndpointBytes.empty()
        && drw_parseSab(missingEdgeEndpointBytes.data(),
                        missingEdgeEndpointBytes.size(),
                        missingEdgeEndpointSab)
        && drw_buildAcisModelWithLimits(
            missingEdgeEndpointSab, missingEdgeEndpointModel,
            missingEdgeEndpointBuildOutcome, DRW_SabParseLimits());
    const bool missingEdgeEndpointProjected = missingEdgeEndpointParsed
        && drw_extractAcisBodyWireframes(
            missingEdgeEndpointModel, missingEdgeEndpointProjections,
            missingEdgeEndpointReport);
    const bool missingEdgeEndpointObserved = missingEdgeEndpointProjected
        && missingEdgeEndpointProjections.size() == 1u
        && missingEdgeEndpointProjections.front().wireframe.faces.size() == 1u
        && missingEdgeEndpointProjections.front().wireframe.faces.front()
                .loops.size() == 2u
        && missingEdgeEndpointProjections.front().wireframe.faces.front()
                .loops[1u].coedgeEdgeStartVertexNodeIndices
            == std::vector<int>{-1, root.vertex0, -1}
        && missingEdgeEndpointProjections.front().wireframe.faces.front()
                .loops[1u].coedgeEdgeEndVertexNodeIndices
            == std::vector<int>{root.vertex1, root.vertex1, root.vertex1}
        && missingEdgeEndpointProjections.front().wireframe.faces.front()
                .loops[1u].coedgeOrientedStartVertexNodeIndices
            == std::vector<int>{root.vertex1, root.vertex1, -1}
        && missingEdgeEndpointProjections.front().wireframe.faces.front()
                .loops[1u].coedgeOrientedEndVertexNodeIndices
            == std::vector<int>{-1, root.vertex0, root.vertex1}
        && !missingEdgeEndpointProjections.front().wireframe.faces.front()
                .loops[1u].hasCoedgeEndpointChainObservation
        && drw_acisProjectionCoverage(
               missingEdgeEndpointProjections.front())
            == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps;
    t.expect(missingEdgeEndpointObserved,
             "ACIS parser-fed face-loop projection preserves an unresolved edge endpoint as a known gap");
    AcisGraphFixture missingCoedgePreviousFixture = complexFixture;
    missingCoedgePreviousFixture.link(root.coedge, 2u, -1);
    const std::vector<std::uint8_t> missingCoedgePreviousBytes =
        sab21800FromGraph(missingCoedgePreviousFixture.data);
    DRW_SabData missingCoedgePreviousSab;
    DRW_AcisModel missingCoedgePreviousModel;
    DRW_SabParseOutcome missingCoedgePreviousBuildOutcome;
    DRW_AcisRootedProjectionReport missingCoedgePreviousReport;
    std::vector<DRW_AcisBodyProjection> missingCoedgePreviousProjections;
    const bool missingCoedgePreviousParsed =
        !missingCoedgePreviousBytes.empty()
        && drw_parseSab(missingCoedgePreviousBytes.data(),
                        missingCoedgePreviousBytes.size(),
                        missingCoedgePreviousSab)
        && drw_buildAcisModelWithLimits(
            missingCoedgePreviousSab, missingCoedgePreviousModel,
            missingCoedgePreviousBuildOutcome, DRW_SabParseLimits());
    const bool missingCoedgePreviousProjected = missingCoedgePreviousParsed
        && drw_extractAcisBodyWireframes(
            missingCoedgePreviousModel, missingCoedgePreviousProjections,
            missingCoedgePreviousReport);
    const bool missingCoedgePreviousObserved = missingCoedgePreviousProjected
        && missingCoedgePreviousProjections.size() == 1u
        && missingCoedgePreviousProjections.front().wireframe.faces.size() == 1u
        && missingCoedgePreviousProjections.front().wireframe.faces.front()
                .loops[1u].coedgePreviousNodeIndices
            == std::vector<int>{-1, root.coedge, thirdCoedge}
        && drw_acisProjectionCoverage(
               missingCoedgePreviousProjections.front())
            == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps;
    t.expect(missingCoedgePreviousObserved,
             "ACIS parser-fed projection preserves a null previous-link slot as an aligned known gap");

    AcisGraphFixture missingCoedgePartnerFixture = complexFixture;
    missingCoedgePartnerFixture.link(thirdCoedge, 3u, -1);
    const std::vector<std::uint8_t> missingCoedgePartnerBytes =
        sab21800FromGraph(missingCoedgePartnerFixture.data);
    DRW_SabData missingCoedgePartnerSab;
    DRW_AcisModel missingCoedgePartnerModel;
    DRW_SabParseOutcome missingCoedgePartnerBuildOutcome;
    DRW_AcisRootedProjectionReport missingCoedgePartnerReport;
    std::vector<DRW_AcisBodyProjection> missingCoedgePartnerProjections;
    const bool missingCoedgePartnerParsed =
        !missingCoedgePartnerBytes.empty()
        && drw_parseSab(missingCoedgePartnerBytes.data(),
                        missingCoedgePartnerBytes.size(),
                        missingCoedgePartnerSab)
        && drw_buildAcisModelWithLimits(
            missingCoedgePartnerSab, missingCoedgePartnerModel,
            missingCoedgePartnerBuildOutcome, DRW_SabParseLimits());
    const bool missingCoedgePartnerProjected = missingCoedgePartnerParsed
        && drw_extractAcisBodyWireframes(
            missingCoedgePartnerModel, missingCoedgePartnerProjections,
            missingCoedgePartnerReport);
    const bool missingCoedgePartnerObserved = missingCoedgePartnerProjected
        && missingCoedgePartnerProjections.size() == 1u
        && missingCoedgePartnerProjections.front().wireframe.faces.size() == 1u
        && missingCoedgePartnerProjections.front().wireframe.faces.front()
                .loops[1u].coedgePartnerNodeIndices
            == std::vector<int>{secondCoedge, -1, root.coedge}
        && !missingCoedgePartnerProjections.front().wireframe.edges.empty()
        && missingCoedgePartnerProjections.front().wireframe.edges.front()
                .coedgePartnerRingNodeIndices
            == std::vector<int>{root.coedge, secondCoedge, thirdCoedge}
        && missingCoedgePartnerProjections.front().wireframe.edges.front()
                .coedgePartnerRingNextNodeIndices
            == std::vector<int>{secondCoedge, thirdCoedge, -1}
        && !missingCoedgePartnerProjections.front().wireframe.edges.front()
                .coedgePartnerRingClosed
        && drw_acisProjectionCoverage(
               missingCoedgePartnerProjections.front())
            == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps;
    t.expect(missingCoedgePartnerObserved,
             "ACIS parser-fed projection preserves the exact open partner walk as a known gap");

    AcisGraphFixture partnerCycleEntersOtherCoedgeFixture = complexFixture;
    partnerCycleEntersOtherCoedgeFixture.link(secondCoedge, 3u, root.coedge);
    const std::vector<std::uint8_t> partnerCycleEntersOtherCoedgeBytes =
        sab21800FromGraph(partnerCycleEntersOtherCoedgeFixture.data);
    DRW_SabData partnerCycleEntersOtherCoedgeSab;
    DRW_AcisModel partnerCycleEntersOtherCoedgeModel;
    DRW_SabParseOutcome partnerCycleEntersOtherCoedgeBuildOutcome;
    DRW_AcisRootedProjectionReport partnerCycleEntersOtherCoedgeReport;
    std::vector<DRW_AcisBodyProjection>
        partnerCycleEntersOtherCoedgeProjections;
    const bool partnerCycleEntersOtherCoedgeProjected =
        !partnerCycleEntersOtherCoedgeBytes.empty()
        && drw_parseSab(partnerCycleEntersOtherCoedgeBytes.data(),
                        partnerCycleEntersOtherCoedgeBytes.size(),
                        partnerCycleEntersOtherCoedgeSab)
        && drw_buildAcisModelWithLimits(
            partnerCycleEntersOtherCoedgeSab,
            partnerCycleEntersOtherCoedgeModel,
            partnerCycleEntersOtherCoedgeBuildOutcome,
            DRW_SabParseLimits())
        && drw_extractAcisBodyWireframes(
            partnerCycleEntersOtherCoedgeModel,
            partnerCycleEntersOtherCoedgeProjections,
            partnerCycleEntersOtherCoedgeReport);
    const DRW_AcisEdge* partnerCycleRootEdge = nullptr;
    const DRW_AcisEdge* partnerCycleSecondEdge = nullptr;
    if (partnerCycleEntersOtherCoedgeProjected
        && partnerCycleEntersOtherCoedgeProjections.size() == 1u) {
        const auto& edges = partnerCycleEntersOtherCoedgeProjections.front()
                                .wireframe.edges;
        const auto findEdge = [&edges](int nodeIndex) {
            return std::find_if(
                edges.begin(), edges.end(), [nodeIndex](const DRW_AcisEdge& edge) {
                    return edge.nodeIndex == nodeIndex;
                });
        };
        const auto rootFound = findEdge(root.edge);
        const auto secondFound = findEdge(secondEdge);
        if (rootFound != edges.end())
            partnerCycleRootEdge = &*rootFound;
        if (secondFound != edges.end())
            partnerCycleSecondEdge = &*secondFound;
    }
    t.expect(partnerCycleEntersOtherCoedgeProjected
                 && partnerCycleRootEdge
                 && partnerCycleRootEdge->coedgePartnerRingClosed
                 && partnerCycleRootEdge->coedgePartnerRingNodeIndices
                    == std::vector<int>{root.coedge, secondCoedge}
                 && partnerCycleRootEdge->coedgePartnerRingNextNodeIndices
                    == std::vector<int>{secondCoedge, root.coedge}
                 && partnerCycleSecondEdge
                 && !partnerCycleSecondEdge->coedgePartnerRingClosed
                 && partnerCycleSecondEdge->coedgePartnerRingNodeIndices
                    == std::vector<int>{thirdCoedge, fourthCoedge, root.coedge,
                                        secondCoedge}
                 && partnerCycleSecondEdge->coedgePartnerRingNextNodeIndices
                    == std::vector<int>{fourthCoedge, root.coedge,
                                        secondCoedge, root.coedge}
                 && drw_acisProjectionCoverage(
                        partnerCycleEntersOtherCoedgeProjections.front())
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "ACIS parser-fed partner walk distinguishes a cycle returning to its own root from one entering another coedge");

    AcisGraphFixture wrongTypeCoedgePartnerFixture = complexFixture;
    wrongTypeCoedgePartnerFixture.link(thirdCoedge, 3u, root.edge);
    const std::vector<std::uint8_t> wrongTypeCoedgePartnerBytes =
        sab21800FromGraph(wrongTypeCoedgePartnerFixture.data);
    DRW_SabData wrongTypeCoedgePartnerSab;
    DRW_AcisModel wrongTypeCoedgePartnerModel;
    DRW_SabParseOutcome wrongTypeCoedgePartnerBuildOutcome;
    DRW_AcisRootedProjectionReport wrongTypeCoedgePartnerReport;
    std::vector<DRW_AcisBodyProjection> wrongTypeCoedgePartnerProjections;
    const bool wrongTypeCoedgePartnerParsed =
        !wrongTypeCoedgePartnerBytes.empty()
        && drw_parseSab(wrongTypeCoedgePartnerBytes.data(),
                        wrongTypeCoedgePartnerBytes.size(),
                        wrongTypeCoedgePartnerSab)
        && drw_buildAcisModelWithLimits(
            wrongTypeCoedgePartnerSab, wrongTypeCoedgePartnerModel,
            wrongTypeCoedgePartnerBuildOutcome, DRW_SabParseLimits());
    const bool wrongTypeCoedgePartnerProjected = wrongTypeCoedgePartnerParsed
        && drw_extractAcisBodyWireframes(
            wrongTypeCoedgePartnerModel, wrongTypeCoedgePartnerProjections,
            wrongTypeCoedgePartnerReport);
    t.expect(wrongTypeCoedgePartnerParsed,
             "ACIS wrong-type partner SAB parses and builds a graph");
    t.expect(!wrongTypeCoedgePartnerProjected
                 && wrongTypeCoedgePartnerProjections.empty()
                 && wrongTypeCoedgePartnerReport.failure
                        == DRW_AcisProjectionFailure::InvalidReference
                 && wrongTypeCoedgePartnerReport.failedNodeIndex == thirdCoedge
                 && wrongTypeCoedgePartnerReport.failedPointerOrdinal == 3u,
             "ACIS rooted projection refuses a wrong-type partner reference transactionally at its exact pointer slot");

    AcisGraphFixture interruptedCoedgeRingFixture = complexFixture;
    interruptedCoedgeRingFixture.link(root.coedge, 1u, -1);
    const std::vector<std::uint8_t> interruptedCoedgeRingBytes =
        sab21800FromGraph(interruptedCoedgeRingFixture.data);
    DRW_SabData interruptedCoedgeRingSab;
    DRW_AcisModel interruptedCoedgeRingModel;
    DRW_SabParseOutcome interruptedCoedgeRingBuildOutcome;
    DRW_AcisRootedProjectionReport interruptedCoedgeRingReport;
    std::vector<DRW_AcisBodyProjection> interruptedCoedgeRingProjections;
    const bool interruptedCoedgeRingSabParsed =
        !interruptedCoedgeRingBytes.empty()
        && drw_parseSab(interruptedCoedgeRingBytes.data(),
                        interruptedCoedgeRingBytes.size(),
                        interruptedCoedgeRingSab);
    t.expect(interruptedCoedgeRingSabParsed,
             "ACIS incomplete coedge ring generated SAB parses");
    const bool interruptedCoedgeRingModelBuilt = interruptedCoedgeRingSabParsed
        && drw_buildAcisModelWithLimits(
            interruptedCoedgeRingSab, interruptedCoedgeRingModel,
            interruptedCoedgeRingBuildOutcome, DRW_SabParseLimits());
    t.expect(interruptedCoedgeRingModelBuilt
                 && static_cast<std::size_t>(root.coedge)
                        < interruptedCoedgeRingModel.nodes.size()
                 && interruptedCoedgeRingModel.nodes[
                        static_cast<std::size_t>(root.coedge)].refs.size() > 3u
                 && interruptedCoedgeRingModel.nodes[
                        static_cast<std::size_t>(root.coedge)].refs[1] == -1,
             "ACIS incomplete coedge ring preserves a null exact next pointer through model building");
    const bool interruptedCoedgeRingProjected = interruptedCoedgeRingModelBuilt
        && drw_extractAcisBodyWireframes(
            interruptedCoedgeRingModel, interruptedCoedgeRingProjections,
            interruptedCoedgeRingReport);
    t.expect(interruptedCoedgeRingProjected,
             "ACIS incomplete coedge ring remains a reportable rooted projection");
    const DRW_AcisFaceLoop* interruptedCoedgeRingLoop = nullptr;
    if (interruptedCoedgeRingProjected
        && interruptedCoedgeRingProjections.size() == 1u
        && !interruptedCoedgeRingProjections.front().wireframe.faces.empty()
        && interruptedCoedgeRingProjections.front().wireframe.faces.front()
                .loops.size() > 1u) {
        interruptedCoedgeRingLoop =
            &interruptedCoedgeRingProjections.front().wireframe.faces.front()
                 .loops[1u];
    }
    const bool interruptedCoedgeRingObserved =
        interruptedCoedgeRingLoop
        && interruptedCoedgeRingLoop->coedgeNodeIndices
            == std::vector<int>{root.coedge}
        && interruptedCoedgeRingLoop->coedgeNextNodeIndices
            == std::vector<int>{-1}
        && interruptedCoedgeRingLoop->hasCoedgeRingClosure
        && !interruptedCoedgeRingLoop->coedgeRingClosed
        && !interruptedCoedgeRingLoop->coedgeLinksReciprocal
        && drw_acisProjectionCoverage(
               interruptedCoedgeRingProjections.front())
            == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps;
    t.expect(interruptedCoedgeRingObserved,
             "ACIS 21800 loop traversal does not borrow previous/partner links when next is null");

    AcisGraphFixture openCoedgeChainFixture = complexFixture;
    openCoedgeChainFixture.link(root.coedge, 2u, -1);
    openCoedgeChainFixture.link(fourthCoedge, 1u, -1);
    const std::vector<std::uint8_t> openCoedgeChainBytes =
        sab21800FromGraph(openCoedgeChainFixture.data);
    DRW_SabData openCoedgeChainSab;
    DRW_AcisModel openCoedgeChainModel;
    DRW_SabParseOutcome openCoedgeChainBuildOutcome;
    DRW_AcisRootedProjectionReport openCoedgeChainReport;
    std::vector<DRW_AcisBodyProjection> openCoedgeChainProjections;
    const bool openCoedgeChainProjected = !openCoedgeChainBytes.empty()
        && drw_parseSab(openCoedgeChainBytes.data(),
                        openCoedgeChainBytes.size(), openCoedgeChainSab)
        && drw_buildAcisModelWithLimits(
            openCoedgeChainSab, openCoedgeChainModel,
            openCoedgeChainBuildOutcome, DRW_SabParseLimits())
        && drw_extractAcisBodyWireframes(
            openCoedgeChainModel, openCoedgeChainProjections,
            openCoedgeChainReport);
    const DRW_AcisFaceLoop* openCoedgeLoop = nullptr;
    if (openCoedgeChainProjected
        && openCoedgeChainProjections.size() == 1u
        && !openCoedgeChainProjections.front().wireframe.faces.empty()) {
        const auto& loops =
            openCoedgeChainProjections.front().wireframe.faces.front().loops;
        const auto found = std::find_if(
            loops.begin(), loops.end(), [root](const DRW_AcisFaceLoop& loop) {
                return loop.nodeIndex == root.loop;
            });
        if (found != loops.end())
            openCoedgeLoop = &*found;
    }
    t.expect(openCoedgeLoop
                 && openCoedgeLoop->coedgeNodeIndices
                    == std::vector<int>{root.coedge, thirdCoedge, fourthCoedge}
                 && openCoedgeLoop->coedgeNextNodeIndices
                    == std::vector<int>{thirdCoedge, fourthCoedge, -1}
                 && openCoedgeLoop->coedgePreviousNodeIndices
                    == std::vector<int>{-1, root.coedge, thirdCoedge}
                 && openCoedgeLoop->hasCoedgeRingClosure
                 && !openCoedgeLoop->coedgeRingClosed
                 && openCoedgeLoop->coedgeRingOpen
                 && openCoedgeLoop->coedgeLinksReciprocal,
             "ACIS 21800 parser-fed loop distinguishes a reciprocal open chain from an incomplete ring");

    AcisGraphFixture closedNonReciprocalFixture = complexFixture;
    closedNonReciprocalFixture.link(thirdCoedge, 2u, thirdCoedge);
    const std::vector<std::uint8_t> closedNonReciprocalBytes =
        sab21800FromGraph(closedNonReciprocalFixture.data);
    DRW_SabData closedNonReciprocalSab;
    DRW_AcisModel closedNonReciprocalModel;
    DRW_SabParseOutcome closedNonReciprocalBuildOutcome;
    DRW_AcisRootedProjectionReport closedNonReciprocalReport;
    std::vector<DRW_AcisBodyProjection> closedNonReciprocalProjections;
    const bool closedNonReciprocalProjected = !closedNonReciprocalBytes.empty()
        && drw_parseSab(closedNonReciprocalBytes.data(),
                        closedNonReciprocalBytes.size(),
                        closedNonReciprocalSab)
        && drw_buildAcisModelWithLimits(
            closedNonReciprocalSab, closedNonReciprocalModel,
            closedNonReciprocalBuildOutcome, DRW_SabParseLimits())
        && drw_extractAcisBodyWireframes(
            closedNonReciprocalModel, closedNonReciprocalProjections,
            closedNonReciprocalReport);
    const DRW_AcisFaceLoop* closedNonReciprocalLoop = nullptr;
    if (closedNonReciprocalProjected
        && closedNonReciprocalProjections.size() == 1u
        && !closedNonReciprocalProjections.front().wireframe.faces.empty()) {
        const auto& loops = closedNonReciprocalProjections.front()
                                .wireframe.faces.front().loops;
        const auto found = std::find_if(
            loops.begin(), loops.end(), [root](const DRW_AcisFaceLoop& loop) {
                return loop.nodeIndex == root.loop;
            });
        if (found != loops.end())
            closedNonReciprocalLoop = &*found;
    }
    t.expect(closedNonReciprocalLoop
                 && closedNonReciprocalLoop->coedgeRingClosed
                 && !closedNonReciprocalLoop->coedgeLinksReciprocal
                 && drw_acisProjectionCoverage(
                        closedNonReciprocalProjections.front())
                    == DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps,
             "SAB 21800 parser-fed closed ring with a mismatched previous link remains a known gap");
    DRW_SabData parsedComplexSab;
    const bool complexSabParsed = !complexSabBytes.empty()
        && drw_parseSab(complexSabBytes.data(), complexSabBytes.size(),
                        parsedComplexSab);
    DRW_SabData countedComplexSab;
    DRW_SabParseOutcome complexParseBaseline;
    const bool complexParseBaselineValid = !complexSabBytes.empty()
        && drw_parseSabWithOutcome(
            complexSabBytes.data(), complexSabBytes.size(),
            countedComplexSab, complexParseBaseline)
        && complexParseBaseline.complete;
    DRW_SabParseLimits exactComplexParseLimits;
    exactComplexParseLimits.maxInputBytes = complexSabBytes.size();
    exactComplexParseLimits.maxRecords =
        complexParseBaseline.recordsProcessed;
    exactComplexParseLimits.maxTags = complexParseBaseline.tagsProcessed;
    exactComplexParseLimits.maxCumulativeStringBytes =
        complexParseBaseline.stringBytesAccounted;
    exactComplexParseLimits.maxTokenCapacityItems =
        complexParseBaseline.tokenCapacityItemsAccounted;
    exactComplexParseLimits.maxTypePartCapacityItems =
        complexParseBaseline.typePartCapacityPeakItems;
    DRW_SabData exactComplexParseSab;
    DRW_SabParseOutcome exactComplexParseOutcome;
    const bool exactComplexParseValid = complexParseBaselineValid
        && drw_parseSabWithLimits(
            complexSabBytes.data(), complexSabBytes.size(),
            exactComplexParseSab, exactComplexParseOutcome,
            exactComplexParseLimits);
    t.expect(complexParseBaselineValid && exactComplexParseValid
                 && exactComplexParseOutcome.complete
                 && exactComplexParseOutcome.recordsProcessed
                        == exactComplexParseLimits.maxRecords
                 && exactComplexParseOutcome.tagsProcessed
                        == exactComplexParseLimits.maxTags
                 && exactComplexParseOutcome.stringBytesAccounted
                        == exactComplexParseLimits.maxCumulativeStringBytes
                 && exactComplexParseOutcome.tokenCapacityItemsAccounted
                        == exactComplexParseLimits.maxTokenCapacityItems
                 && exactComplexParseOutcome.typePartCapacityPeakItems
                        == exactComplexParseLimits.maxTypePartCapacityItems,
             "generated complex SAB graph parses at exact input and resource budgets");

    DRW_SabParseLimits oneUnderComplexTagLimit = exactComplexParseLimits;
    oneUnderComplexTagLimit.maxTags =
        complexParseBaseline.tagsProcessed > 0u
            ? complexParseBaseline.tagsProcessed - 1u : 0u;
    DRW_SabData rejectedComplexParseSab;
    rejectedComplexParseSab.header.signature = "STALE";
    rejectedComplexParseSab.records.push_back(acisRecord("stale", {}));
    DRW_SabParseOutcome oneUnderComplexTagOutcome;
    const bool oneUnderComplexTagAccepted = drw_parseSabWithLimits(
        complexSabBytes.data(), complexSabBytes.size(),
        rejectedComplexParseSab, oneUnderComplexTagOutcome,
        oneUnderComplexTagLimit);
    t.expect(complexParseBaselineValid && !oneUnderComplexTagAccepted
                 && rejectedComplexParseSab.records.empty()
                 && rejectedComplexParseSab.header.signature.empty()
                 && oneUnderComplexTagOutcome.resourceLimitExceeded
                 && oneUnderComplexTagOutcome.tagsProcessed
                        == oneUnderComplexTagLimit.maxTags,
             "generated complex SAB graph clears output one tag below its exact parser budget");

    const DRW_AcisModel parsedComplexModel =
        drw_buildAcisModel(parsedComplexSab);
    std::vector<DRW_AcisBodyProjection> parsedComplexProjections;
    DRW_AcisRootedProjectionReport parsedComplexReport;
    const bool parsedComplexProjected = drw_extractAcisBodyWireframes(
        parsedComplexModel, parsedComplexProjections, parsedComplexReport);
    bool sharedHeadersMatch = complexSabParsed
        && parsedComplexSab.records.size()
            == complexFixture.data.records.size() + 1u;
    for (std::size_t index = 0;
         sharedHeadersMatch && index < complexFixture.data.records.size();
         ++index) {
        const DRW_SabRecord& record = parsedComplexSab.records[index];
        sharedHeadersMatch = record.tokens.size() >= 3u
            && record.tokens[0].tag == DRW_SabTag::EntityType
            && record.tokens[0].sval == record.type
            && record.tokens[1].tag == DRW_SabTag::Pointer
            && record.tokens[1].ival == -1
            && record.tokens[2].tag == DRW_SabTag::Int
            && record.tokens[2].ival == static_cast<long long>(index + 1u);
    }
    const DRW_AcisModel expectedComplexModel =
        drw_buildAcisModel(complexFixture.data);
    bool semanticReferenceRowsMatch = parsedComplexModel.nodes.size()
            == expectedComplexModel.nodes.size() + 1u
        && parsedComplexModel.nodes.back().type == "End-of-ASM-data";
    for (std::size_t index = 0;
         semanticReferenceRowsMatch && index < expectedComplexModel.nodes.size();
         ++index) {
        const bool rowMatches =
            parsedComplexModel.nodes[index].record.type
                == expectedComplexModel.nodes[index].record.type
            && parsedComplexModel.nodes[index].refs
                == expectedComplexModel.nodes[index].refs;
        semanticReferenceRowsMatch = rowMatches;
        if (!rowMatches) {
            t.expect(false,
                     "generated complex SAB reference row matches source node "
                         + std::to_string(index));
        }
    }
    bool wireProjectionMatches = complexProjections.size() == 1u
        && parsedComplexProjections.size() == 1u
        && parsedComplexReport.bodyCount == complexReport.bodyCount
        && parsedComplexReport.verticesOutsideBodyRoots
            == complexReport.verticesOutsideBodyRoots
        && parsedComplexReport.edgesOutsideBodyRoots
            == complexReport.edgesOutsideBodyRoots
        && parsedComplexReport.facesOutsideBodyRoots
            == complexReport.facesOutsideBodyRoots;
    if (wireProjectionMatches) {
        const DRW_AcisBodyProjection& expected = complexProjections.front();
        const DRW_AcisBodyProjection& actual =
            parsedComplexProjections.front();
        wireProjectionMatches = actual.bodyNodeIndex == expected.bodyNodeIndex
            && actual.hasLumpChainObservation
                == expected.hasLumpChainObservation
            && actual.lumpChainNullTerminated
                == expected.lumpChainNullTerminated
            && actual.lumpChainCycleDetected
                == expected.lumpChainCycleDetected
            && actual.firstRepeatedLumpChainNodeIndex
                == expected.firstRepeatedLumpChainNodeIndex
            && actual.lumpChainUnclassifiedLinkCount
                == expected.lumpChainUnclassifiedLinkCount
            && actual.firstUnclassifiedLumpLinkSourceNodeIndex
                == expected.firstUnclassifiedLumpLinkSourceNodeIndex
            && actual.lumpNodeIndices == expected.lumpNodeIndices
            && actual.lumpNextNodeIndices == expected.lumpNextNodeIndices
            && actual.hasLumpUseIndexObservation
                == expected.hasLumpUseIndexObservation
            && actual.lumpUseIndexCoversParsedLumps
                == expected.lumpUseIndexCoversParsedLumps
            && actual.lumpUseIndexUnclassifiedLumpCount
                == expected.lumpUseIndexUnclassifiedLumpCount
            && actual.firstUnclassifiedLumpOwnerNodeIndex
                == expected.firstUnclassifiedLumpOwnerNodeIndex
            && actual.lumpChainMatchesUseIndex
                == expected.lumpChainMatchesUseIndex
            && actual.lumpUseNodeIndices == expected.lumpUseNodeIndices
            && actual.reachableNodeIndices == expected.reachableNodeIndices
            && actual.graphLinks.size() == expected.graphLinks.size()
            && actual.coedgeNextCycleCount == expected.coedgeNextCycleCount
            && actual.coedgePartnerCycleCount
                == expected.coedgePartnerCycleCount
            && actual.report.unsupportedPatternNodeCount
                == expected.report.unsupportedPatternNodeCount
            && actual.report.unsupportedWireNodeCount
                == expected.report.unsupportedWireNodeCount
            && actual.report.unsupportedSubshellNodeCount
                == expected.report.unsupportedSubshellNodeCount
            && actual.report.unsupportedPCurveNodeCount
                == expected.report.unsupportedPCurveNodeCount
            && actual.report.unsupportedCurveNodeCount
                == expected.report.unsupportedCurveNodeCount
            && actual.report.unsupportedSurfaceNodeCount
                == expected.report.unsupportedSurfaceNodeCount
            && actual.wireframe.vertices.size()
                == expected.wireframe.vertices.size()
            && actual.wireframe.lumps.size()
                == expected.wireframe.lumps.size()
            && actual.wireframe.shells.size()
                == expected.wireframe.shells.size()
            && !actual.wireframe.faces.empty()
            && !expected.wireframe.faces.empty()
            && actual.wireframe.faces.size() == expected.wireframe.faces.size()
            && actual.wireframe.faces.front().hasSense
                == expected.wireframe.faces.front().hasSense
            && actual.wireframe.faces.front().shellNodeIndex
                == expected.wireframe.faces.front().shellNodeIndex
            && actual.wireframe.faces.front().hasLoopChainObservation
                == expected.wireframe.faces.front()
                    .hasLoopChainObservation
            && actual.wireframe.faces.front().loopChainNullTerminated
                == expected.wireframe.faces.front()
                    .loopChainNullTerminated
            && actual.wireframe.faces.front().loopChainCycleDetected
                == expected.wireframe.faces.front()
                    .loopChainCycleDetected
            && actual.wireframe.faces.front()
                    .firstRepeatedLoopChainNodeIndex
                == expected.wireframe.faces.front()
                    .firstRepeatedLoopChainNodeIndex
            && actual.wireframe.faces.front()
                    .loopChainUnclassifiedNextLinkCount
                == expected.wireframe.faces.front()
                    .loopChainUnclassifiedNextLinkCount
            && actual.wireframe.faces.front()
                    .firstUnclassifiedLoopNextNodeIndex
                == expected.wireframe.faces.front()
                    .firstUnclassifiedLoopNextNodeIndex
            && actual.wireframe.faces.front().loopNextNodeIndices
                == expected.wireframe.faces.front().loopNextNodeIndices
            && actual.wireframe.faces.front().hasLoopUseIndexObservation
                == expected.wireframe.faces.front()
                    .hasLoopUseIndexObservation
            && actual.wireframe.faces.front().loopUseIndexCoversParsedLoops
                == expected.wireframe.faces.front()
                    .loopUseIndexCoversParsedLoops
            && actual.wireframe.faces.front()
                    .loopUseIndexUnclassifiedLoopCount
                == expected.wireframe.faces.front()
                    .loopUseIndexUnclassifiedLoopCount
            && actual.wireframe.faces.front()
                    .firstUnclassifiedLoopOwnerNodeIndex
                == expected.wireframe.faces.front()
                    .firstUnclassifiedLoopOwnerNodeIndex
            && actual.wireframe.faces.front().faceLoopChainMatchesUseIndex
                == expected.wireframe.faces.front()
                    .faceLoopChainMatchesUseIndex
            && actual.wireframe.faces.front().loopUseNodeIndices
                == expected.wireframe.faces.front().loopUseNodeIndices
            && actual.wireframe.faces.front().surfaceNodeIndex
                == expected.wireframe.faces.front().surfaceNodeIndex
            && actual.wireframe.faces.front().senseReversed
                == expected.wireframe.faces.front().senseReversed
            && actual.wireframe.faces.front().hasDoubleSided
                == expected.wireframe.faces.front().hasDoubleSided
            && actual.wireframe.faces.front().doubleSided
                == expected.wireframe.faces.front().doubleSided
            && actual.wireframe.faces.front().hasContainment
                == expected.wireframe.faces.front().hasContainment
            && actual.wireframe.faces.front().containmentIn
                == expected.wireframe.faces.front().containmentIn
            && actual.wireframe.faces.front().loops.size()
                == expected.wireframe.faces.front().loops.size();
        for (std::size_t index = 0;
             wireProjectionMatches
                 && index < expected.wireframe.vertices.size();
             ++index) {
            const DRW_AcisVertex& expectedVertex =
                expected.wireframe.vertices[index];
            const DRW_AcisVertex& actualVertex =
                actual.wireframe.vertices[index];
            wireProjectionMatches =
                actualVertex.nodeIndex == expectedVertex.nodeIndex
                && actualVertex.pointNodeIndex
                    == expectedVertex.pointNodeIndex
                && actualVertex.valid == expectedVertex.valid
                && (!expectedVertex.valid
                    || acisCoordNear(actualVertex.point,
                                     expectedVertex.point.x,
                                     expectedVertex.point.y,
                                     expectedVertex.point.z));
        }
        for (std::size_t index = 0;
             wireProjectionMatches && index < expected.graphLinks.size();
             ++index) {
            const DRW_AcisBodyGraphLink& expectedLink =
                expected.graphLinks[index];
            const DRW_AcisBodyGraphLink& actualLink =
                actual.graphLinks[index];
            wireProjectionMatches =
                actualLink.sourceNodeIndex == expectedLink.sourceNodeIndex
                && actualLink.pointerOrdinal == expectedLink.pointerOrdinal
                && actualLink.targetNodeIndex == expectedLink.targetNodeIndex;
        }
        wireProjectionMatches = wireProjectionMatches
            && actual.wireframe.edges.size() == expected.wireframe.edges.size();
        for (std::size_t index = 0;
             wireProjectionMatches
                 && index < expected.wireframe.edges.size();
             ++index) {
            const DRW_AcisEdge& expectedEdge =
                expected.wireframe.edges[index];
            const DRW_AcisEdge& actualEdge =
                actual.wireframe.edges[index];
            wireProjectionMatches =
                actualEdge.nodeIndex == expectedEdge.nodeIndex
                && actualEdge.curveNodeIndex == expectedEdge.curveNodeIndex
                && actualEdge.startVertexNodeIndex
                    == expectedEdge.startVertexNodeIndex
                && actualEdge.endVertexNodeIndex
                    == expectedEdge.endVertexNodeIndex
                && actualEdge.coedgeNodeIndex
                    == expectedEdge.coedgeNodeIndex
                && actualEdge.hasReciprocalCoedgeLink
                    == expectedEdge.hasReciprocalCoedgeLink
                && actualEdge.hasCoedgePartnerRingObservation
                    == expectedEdge.hasCoedgePartnerRingObservation
                && actualEdge.coedgePartnerRingClosed
                    == expectedEdge.coedgePartnerRingClosed
                && actualEdge.coedgePartnerRingNodeIndices
                    == expectedEdge.coedgePartnerRingNodeIndices
                && actualEdge.coedgePartnerRingNextNodeIndices
                    == expectedEdge.coedgePartnerRingNextNodeIndices
                && actualEdge.hasCoedgeEdgeUseIndexObservation
                    == expectedEdge.hasCoedgeEdgeUseIndexObservation
                && actualEdge.coedgeEdgeUseIndexCoversParsedCoedges
                    == expectedEdge.coedgeEdgeUseIndexCoversParsedCoedges
                && actualEdge.coedgeEdgeUseIndexUnclassifiedCoedgeCount
                    == expectedEdge
                        .coedgeEdgeUseIndexUnclassifiedCoedgeCount
                && actualEdge.firstUnclassifiedCoedgeEdgeUseNodeIndex
                    == expectedEdge
                        .firstUnclassifiedCoedgeEdgeUseNodeIndex
                && actualEdge.coedgePartnerRingMatchesEdgeUseIndex
                    == expectedEdge.coedgePartnerRingMatchesEdgeUseIndex
                && actualEdge.coedgeEdgeUseNodeIndices
                    == expectedEdge.coedgeEdgeUseNodeIndices;
        }
        for (std::size_t index = 0;
             wireProjectionMatches
                 && index < expected.wireframe.lumps.size();
             ++index) {
            const DRW_AcisLump& expectedLump =
                expected.wireframe.lumps[index];
            const DRW_AcisLump& actualLump =
                actual.wireframe.lumps[index];
            wireProjectionMatches =
                actualLump.nodeIndex == expectedLump.nodeIndex
                && actualLump.bodyNodeIndex == expectedLump.bodyNodeIndex
                && actualLump.hasShellChainObservation
                    == expectedLump.hasShellChainObservation
                && actualLump.shellChainNullTerminated
                    == expectedLump.shellChainNullTerminated
                && actualLump.shellChainCycleDetected
                    == expectedLump.shellChainCycleDetected
                && actualLump.firstRepeatedShellChainNodeIndex
                    == expectedLump.firstRepeatedShellChainNodeIndex
                && actualLump.shellChainUnclassifiedLinkCount
                    == expectedLump.shellChainUnclassifiedLinkCount
                && actualLump.firstUnclassifiedShellLinkSourceNodeIndex
                    == expectedLump.firstUnclassifiedShellLinkSourceNodeIndex
                && actualLump.shellNodeIndices
                    == expectedLump.shellNodeIndices
                && actualLump.shellNextNodeIndices
                    == expectedLump.shellNextNodeIndices
                && actualLump.hasShellUseIndexObservation
                    == expectedLump.hasShellUseIndexObservation
                && actualLump.shellUseIndexCoversParsedShells
                    == expectedLump.shellUseIndexCoversParsedShells
                && actualLump.shellUseIndexUnclassifiedShellCount
                    == expectedLump.shellUseIndexUnclassifiedShellCount
                && actualLump.firstUnclassifiedShellOwnerNodeIndex
                    == expectedLump.firstUnclassifiedShellOwnerNodeIndex
                && actualLump.shellChainMatchesUseIndex
                    == expectedLump.shellChainMatchesUseIndex
                && actualLump.shellUseNodeIndices
                    == expectedLump.shellUseNodeIndices;
        }
        for (std::size_t index = 0;
             wireProjectionMatches
                 && index < expected.wireframe.shells.size();
             ++index) {
            const DRW_AcisShell& expectedShell =
                expected.wireframe.shells[index];
            const DRW_AcisShell& actualShell =
                actual.wireframe.shells[index];
            wireProjectionMatches =
                actualShell.nodeIndex == expectedShell.nodeIndex
                && actualShell.lumpNodeIndex == expectedShell.lumpNodeIndex
                && actualShell.hasFaceChainObservation
                    == expectedShell.hasFaceChainObservation
                && actualShell.faceChainNullTerminated
                    == expectedShell.faceChainNullTerminated
                && actualShell.faceChainCycleDetected
                    == expectedShell.faceChainCycleDetected
                && actualShell.firstRepeatedFaceChainNodeIndex
                    == expectedShell.firstRepeatedFaceChainNodeIndex
                && actualShell.faceChainUnclassifiedLinkCount
                    == expectedShell.faceChainUnclassifiedLinkCount
                && actualShell.firstUnclassifiedFaceLinkSourceNodeIndex
                    == expectedShell.firstUnclassifiedFaceLinkSourceNodeIndex
                && actualShell.faceNodeIndices
                    == expectedShell.faceNodeIndices
                && actualShell.faceNextNodeIndices
                    == expectedShell.faceNextNodeIndices
                && actualShell.hasFaceUseIndexObservation
                    == expectedShell.hasFaceUseIndexObservation
                && actualShell.faceUseIndexCoversParsedFaces
                    == expectedShell.faceUseIndexCoversParsedFaces
                && actualShell.faceUseIndexUnclassifiedFaceCount
                    == expectedShell.faceUseIndexUnclassifiedFaceCount
                && actualShell.firstUnclassifiedFaceOwnerNodeIndex
                    == expectedShell.firstUnclassifiedFaceOwnerNodeIndex
                && actualShell.faceChainMatchesUseIndex
                    == expectedShell.faceChainMatchesUseIndex
                && actualShell.faceUseNodeIndices
                    == expectedShell.faceUseNodeIndices;
        }
        for (std::size_t index = 0;
             wireProjectionMatches
                 && index < expected.wireframe.faces.front().loops.size();
             ++index) {
            const DRW_AcisFaceLoop& expectedLoop =
                expected.wireframe.faces.front().loops[index];
            const DRW_AcisFaceLoop& actualLoop =
                actual.wireframe.faces.front().loops[index];
            wireProjectionMatches = actualLoop.nodeIndex == expectedLoop.nodeIndex
                && actualLoop.coedgeCount == expectedLoop.coedgeCount
                && actualLoop.coedgeNodeIndices
                    == expectedLoop.coedgeNodeIndices
                && actualLoop.coedgeNextNodeIndices
                    == expectedLoop.coedgeNextNodeIndices
                && actualLoop.coedgePreviousNodeIndices
                    == expectedLoop.coedgePreviousNodeIndices
                && actualLoop.coedgePartnerNodeIndices
                    == expectedLoop.coedgePartnerNodeIndices
                && actualLoop.hasCoedgeRingClosure
                    == expectedLoop.hasCoedgeRingClosure
                && actualLoop.coedgeRingClosed
                    == expectedLoop.coedgeRingClosed
                && actualLoop.coedgeRingOpen
                    == expectedLoop.coedgeRingOpen
                && actualLoop.coedgeLinksReciprocal
                    == expectedLoop.coedgeLinksReciprocal
                && actualLoop.coedgeEdgeNodeIndices
                    == expectedLoop.coedgeEdgeNodeIndices
                && actualLoop.coedgeEdgeStartVertexNodeIndices
                    == expectedLoop.coedgeEdgeStartVertexNodeIndices
                && actualLoop.coedgeEdgeEndVertexNodeIndices
                    == expectedLoop.coedgeEdgeEndVertexNodeIndices
                && actualLoop.coedgeOrientedStartVertexNodeIndices
                    == expectedLoop.coedgeOrientedStartVertexNodeIndices
                && actualLoop.coedgeOrientedEndVertexNodeIndices
                    == expectedLoop.coedgeOrientedEndVertexNodeIndices
                && actualLoop.coedgeLoopNodeIndices
                    == expectedLoop.coedgeLoopNodeIndices
                && actualLoop.coedgePCurveNodeIndices
                    == expectedLoop.coedgePCurveNodeIndices
                && actualLoop.coedgeSenseFlags
                    == expectedLoop.coedgeSenseFlags
                && actualLoop.hasCoedgeEndpointChainObservation
                    == expectedLoop.hasCoedgeEndpointChainObservation
                && actualLoop.coedgeEndpointChainContinuous
                    == expectedLoop.coedgeEndpointChainContinuous
                && actualLoop.coedgeEndpointDiscontinuityCount
                    == expectedLoop.coedgeEndpointDiscontinuityCount
                && actualLoop.firstCoedgeEndpointDiscontinuityNodeIndex
                    == expectedLoop.firstCoedgeEndpointDiscontinuityNodeIndex
                && actualLoop.hasCoedgeUseIndexObservation
                    == expectedLoop.hasCoedgeUseIndexObservation
                && actualLoop.coedgeUseIndexCoversParsedCoedges
                    == expectedLoop.coedgeUseIndexCoversParsedCoedges
                && actualLoop.coedgeUseIndexUnclassifiedCoedgeCount
                    == expectedLoop.coedgeUseIndexUnclassifiedCoedgeCount
                && actualLoop.firstUnclassifiedCoedgeOwnerNodeIndex
                    == expectedLoop.firstUnclassifiedCoedgeOwnerNodeIndex
                && actualLoop.coedgeChainMatchesUseIndex
                    == expectedLoop.coedgeChainMatchesUseIndex
                && actualLoop.coedgeUseNodeIndices
                    == expectedLoop.coedgeUseNodeIndices;
        }
    }
    t.expect(complexSabParsed,
             "generated complex SAB graph parses as a complete 21800 stream");
    t.expect(sharedHeadersMatch,
             "generated complex SAB graph preserves each shared record header");
    t.expect(parsedComplexModel.nodes.size()
                    == complexFixture.data.records.size() + 1u
                 && semanticReferenceRowsMatch,
             "generated complex SAB graph preserves semantic pointer rows and its explicit terminal node");
    t.expect(parsedComplexProjected
                 && parsedComplexReport.bodyCount == complexReport.bodyCount
                 && parsedComplexProjections.size() == complexProjections.size(),
             "parsed complex SAB graph reaches the same body projection outcome");
    const DRW_AcisEdge* parsedUnsupportedCurveEdge = findProjectedEdge(
        parsedComplexProjections, secondEdge);
    t.expect(parsedComplexProjected && parsedUnsupportedCurveEdge
                 && parsedUnsupportedCurveEdge->curveNodeIndex
                    == unsupportedCurve
                 && !parsedUnsupportedCurveEdge->hasCurve,
             "parser-fed SAB 21800 retains the exact source identity of an unsupported edge curve without claiming curve support");
    t.expect(wireProjectionMatches,
             "SAB 21800 parser preserves multi-lump, ordered-loop, cyclic, and unsupported rooted graph structure");
    std::size_t unsupportedCurveOutgoingLinkCount = 0u;
    bool unsupportedCurveNullSlotRetained = false;
    if (parsedComplexProjections.size() == 1u) {
        for (const DRW_AcisBodyGraphLink& link :
             parsedComplexProjections.front().graphLinks) {
            if (link.sourceNodeIndex != unsupportedCurve)
                continue;
            ++unsupportedCurveOutgoingLinkCount;
            unsupportedCurveNullSlotRetained =
                unsupportedCurveNullSlotRetained
                || (link.pointerOrdinal == 0u
                    && link.targetNodeIndex == -1);
        }
    }
    t.expect(parsedComplexProjected
                 && unsupportedCurveOutgoingLinkCount == 1u
                 && unsupportedCurveNullSlotRetained,
             "SAB 21800 parser-fed unsupported parabola curve retains its outgoing null pointer row");

    AcisGraphFixture orderedUnsupportedCurveFixture;
    const SyntheticAcisBody orderedUnsupportedCurveBody =
        addSyntheticAcisBody(orderedUnsupportedCurveFixture, 23.0);
    const int rawOnlyPoint = orderedUnsupportedCurveFixture.addNode(
        "point", 1u, {acisLocation(91.0, 92.0, 93.0)});
    const int orderedUnsupportedCurve =
        orderedUnsupportedCurveFixture.addNode("parabola-curve", 3u);
    orderedUnsupportedCurveFixture.link(
        orderedUnsupportedCurveBody.edge, 4u, orderedUnsupportedCurve);
    orderedUnsupportedCurveFixture.link(
        orderedUnsupportedCurve, 1u, rawOnlyPoint);
    orderedUnsupportedCurveFixture.link(
        orderedUnsupportedCurve, 2u, rawOnlyPoint);
    const std::vector<std::uint8_t> orderedUnsupportedCurveBytes =
        sab21800FromGraph(orderedUnsupportedCurveFixture.data);
    DRW_SabData parsedOrderedUnsupportedCurve;
    const bool orderedUnsupportedCurveParsed =
        !orderedUnsupportedCurveBytes.empty()
        && drw_parseSab(orderedUnsupportedCurveBytes.data(),
                        orderedUnsupportedCurveBytes.size(),
                        parsedOrderedUnsupportedCurve);
    const DRW_AcisModel orderedUnsupportedCurveModel =
        drw_buildAcisModel(parsedOrderedUnsupportedCurve);
    std::vector<DRW_AcisBodyProjection> orderedUnsupportedCurveProjections;
    DRW_AcisRootedProjectionReport orderedUnsupportedCurveReport;
    const bool orderedUnsupportedCurveProjected =
        drw_extractAcisBodyWireframes(
            orderedUnsupportedCurveModel,
            orderedUnsupportedCurveProjections,
            orderedUnsupportedCurveReport);
    std::vector<DRW_AcisBodyGraphLink> orderedUnsupportedCurveLinks;
    bool rawOnlyPointWasNotTraversed = false;
    if (orderedUnsupportedCurveProjections.size() == 1u) {
        const DRW_AcisBodyProjection& projection =
            orderedUnsupportedCurveProjections.front();
        for (const DRW_AcisBodyGraphLink& link : projection.graphLinks) {
            if (link.sourceNodeIndex == orderedUnsupportedCurve)
                orderedUnsupportedCurveLinks.push_back(link);
        }
        rawOnlyPointWasNotTraversed = std::find(
            projection.reachableNodeIndices.begin(),
            projection.reachableNodeIndices.end(), rawOnlyPoint)
            == projection.reachableNodeIndices.end();
    }
    const bool orderedUnsupportedCurveRowsMatch =
        orderedUnsupportedCurveLinks.size() == 3u
        && orderedUnsupportedCurveLinks[0].pointerOrdinal == 0u
        && orderedUnsupportedCurveLinks[0].targetNodeIndex == -1
        && orderedUnsupportedCurveLinks[1].pointerOrdinal == 1u
        && orderedUnsupportedCurveLinks[1].targetNodeIndex == rawOnlyPoint
        && orderedUnsupportedCurveLinks[2].pointerOrdinal == 2u
        && orderedUnsupportedCurveLinks[2].targetNodeIndex == rawOnlyPoint;
    t.expect(orderedUnsupportedCurveParsed
                 && orderedUnsupportedCurveProjected
                 && orderedUnsupportedCurveReport.bodyCount == 1u
                 && orderedUnsupportedCurveProjections.size() == 1u
                 && orderedUnsupportedCurveProjections.front()
                        .report.unsupportedCurveNodeCount == 1u
                 && orderedUnsupportedCurveRowsMatch
                 && rawOnlyPointWasNotTraversed,
             "SAB 21800 parser-fed unsupported curve preserves mixed null/reference slots in ordinal order without traversing raw-only targets");

    AcisGraphFixture invalidUnsupportedCurveFixture = complexFixture;
    invalidUnsupportedCurveFixture.link(
        unsupportedCurve, 0u, std::numeric_limits<int>::max());
    const std::vector<std::uint8_t> invalidUnsupportedCurveBytes =
        sab21800FromGraph(invalidUnsupportedCurveFixture.data);
    DRW_SabData parsedInvalidUnsupportedCurve;
    const bool invalidUnsupportedCurveParsed =
        !invalidUnsupportedCurveBytes.empty()
        && drw_parseSab(invalidUnsupportedCurveBytes.data(),
                        invalidUnsupportedCurveBytes.size(),
                        parsedInvalidUnsupportedCurve);
    const DRW_AcisModel invalidUnsupportedCurveModel =
        drw_buildAcisModel(parsedInvalidUnsupportedCurve);
    std::vector<DRW_AcisBodyProjection>
        invalidUnsupportedCurveProjections(1u);
    DRW_AcisRootedProjectionReport invalidUnsupportedCurveReport;
    const bool invalidUnsupportedCurveProjected =
        drw_extractAcisBodyWireframes(
            invalidUnsupportedCurveModel,
            invalidUnsupportedCurveProjections,
            invalidUnsupportedCurveReport);
    t.expect(invalidUnsupportedCurveParsed
                 && !invalidUnsupportedCurveProjected
                 && invalidUnsupportedCurveProjections.empty()
                 && invalidUnsupportedCurveReport.failure
                        == DRW_AcisProjectionFailure::InvalidReference
                 && invalidUnsupportedCurveReport.failedNodeIndex
                        == unsupportedCurve
                 && invalidUnsupportedCurveReport.failedPointerOrdinal == 0u,
             "SAB 21800 parser-fed unsupported curve rejects an out-of-range raw outgoing pointer transactionally");

    AcisGraphFixture sharedUnsupportedAcrossBodiesFixture;
    const SyntheticAcisBody firstUnsupportedBody =
        addSyntheticAcisBody(sharedUnsupportedAcrossBodiesFixture, 13.0);
    const SyntheticAcisBody secondUnsupportedBody =
        addSyntheticAcisBody(sharedUnsupportedAcrossBodiesFixture, 113.0);
    const int sharedUnsupportedCurve =
        sharedUnsupportedAcrossBodiesFixture.addNode("parabola-curve", 1u);
    sharedUnsupportedAcrossBodiesFixture.link(
        firstUnsupportedBody.edge, 4u, sharedUnsupportedCurve);
    sharedUnsupportedAcrossBodiesFixture.link(
        secondUnsupportedBody.edge, 4u, sharedUnsupportedCurve);
    const std::vector<std::uint8_t> sharedUnsupportedAcrossBodiesBytes =
        sab21800FromGraph(sharedUnsupportedAcrossBodiesFixture.data);
    DRW_SabData parsedSharedUnsupportedAcrossBodies;
    const bool sharedUnsupportedAcrossBodiesParsed =
        !sharedUnsupportedAcrossBodiesBytes.empty()
        && drw_parseSab(sharedUnsupportedAcrossBodiesBytes.data(),
                        sharedUnsupportedAcrossBodiesBytes.size(),
                        parsedSharedUnsupportedAcrossBodies);
    const DRW_AcisModel sharedUnsupportedAcrossBodiesModel =
        drw_buildAcisModel(parsedSharedUnsupportedAcrossBodies);
    std::vector<DRW_AcisBodyProjection>
        sharedUnsupportedAcrossBodiesProjections;
    DRW_AcisRootedProjectionReport sharedUnsupportedAcrossBodiesReport;
    const bool sharedUnsupportedAcrossBodiesProjected =
        drw_extractAcisBodyWireframes(
            sharedUnsupportedAcrossBodiesModel,
            sharedUnsupportedAcrossBodiesProjections,
            sharedUnsupportedAcrossBodiesReport);
    bool sharedUnsupportedRowsAreBodyLocal =
        sharedUnsupportedAcrossBodiesProjections.size() == 2u;
    const std::array<int, 2> expectedUnsupportedBodyRoots = {{
        firstUnsupportedBody.body, secondUnsupportedBody.body
    }};
    for (std::size_t bodyOrdinal = 0;
         sharedUnsupportedRowsAreBodyLocal
             && bodyOrdinal < expectedUnsupportedBodyRoots.size();
         ++bodyOrdinal) {
        const DRW_AcisBodyProjection& projection =
            sharedUnsupportedAcrossBodiesProjections[bodyOrdinal];
        std::size_t outgoingRows = 0u;
        bool nullPointerRetained = false;
        for (const DRW_AcisBodyGraphLink& link : projection.graphLinks) {
            if (link.sourceNodeIndex != sharedUnsupportedCurve)
                continue;
            ++outgoingRows;
            nullPointerRetained = link.pointerOrdinal == 0u
                && link.targetNodeIndex == -1;
        }
        const bool curveIsReachable = std::find(
            projection.reachableNodeIndices.begin(),
            projection.reachableNodeIndices.end(), sharedUnsupportedCurve)
            != projection.reachableNodeIndices.end();
        sharedUnsupportedRowsAreBodyLocal =
            projection.bodyNodeIndex == expectedUnsupportedBodyRoots[bodyOrdinal]
            && projection.report.unsupportedCurveNodeCount == 1u
            && curveIsReachable && outgoingRows == 1u
            && nullPointerRetained;
    }
    t.expect(sharedUnsupportedAcrossBodiesParsed
                 && sharedUnsupportedAcrossBodiesProjected
                 && sharedUnsupportedAcrossBodiesReport.bodyCount == 2u
                 && sharedUnsupportedAcrossBodiesReport.failure
                        == DRW_AcisProjectionFailure::NoFailure
                 && sharedUnsupportedRowsAreBodyLocal,
             "SAB 21800 shared unsupported curve retains its raw pointer row once in each rooted body projection");

    AcisGraphFixture invalidSecondRootUnsupportedCurveFixture;
    const SyntheticAcisBody validFirstRoot =
        addSyntheticAcisBody(invalidSecondRootUnsupportedCurveFixture, 17.0);
    const SyntheticAcisBody invalidSecondRoot =
        addSyntheticAcisBody(invalidSecondRootUnsupportedCurveFixture, 117.0);
    const int invalidSecondRootCurve =
        invalidSecondRootUnsupportedCurveFixture.addNode(
            "parabola-curve", 1u);
    invalidSecondRootUnsupportedCurveFixture.link(
        invalidSecondRoot.edge, 4u, invalidSecondRootCurve);
    invalidSecondRootUnsupportedCurveFixture.link(
        invalidSecondRootCurve, 0u, std::numeric_limits<int>::max());
    const std::vector<std::uint8_t> invalidSecondRootBytes =
        sab21800FromGraph(invalidSecondRootUnsupportedCurveFixture.data);
    DRW_SabData parsedInvalidSecondRoot;
    const bool invalidSecondRootParsed = !invalidSecondRootBytes.empty()
        && drw_parseSab(invalidSecondRootBytes.data(),
                        invalidSecondRootBytes.size(), parsedInvalidSecondRoot);
    const DRW_AcisModel invalidSecondRootModel =
        drw_buildAcisModel(parsedInvalidSecondRoot);
    std::vector<DRW_AcisBodyProjection> invalidSecondRootProjections(1u);
    DRW_AcisRootedProjectionReport invalidSecondRootReport;
    const bool invalidSecondRootProjected = drw_extractAcisBodyWireframes(
        invalidSecondRootModel, invalidSecondRootProjections,
        invalidSecondRootReport);
    t.expect(validFirstRoot.body != invalidSecondRoot.body
                 && invalidSecondRootParsed
                 && !invalidSecondRootProjected
                 && invalidSecondRootProjections.empty()
                 && invalidSecondRootReport.failure
                        == DRW_AcisProjectionFailure::InvalidReference
                 && invalidSecondRootReport.failedNodeIndex
                        == invalidSecondRootCurve
                 && invalidSecondRootReport.failedPointerOrdinal == 0u,
             "SAB 21800 invalid unsupported row in a later body clears earlier rooted projections transactionally");

    AcisGraphFixture sphereSurfaceFixture = complexFixture;
    DRW_SabRecord& sphereSurfaceRecord =
        sphereSurfaceFixture.data.records[
            static_cast<std::size_t>(unsupportedSurface)];
    sphereSurfaceRecord.type = "sphere-surface";
    sphereSurfaceRecord.tokens = acisRecord(
        "sphere-surface",
        {acisPointer(-1), acisLocation(4.0, 5.0, 6.0),
         acisDouble(-7.5), acisDirection(0.0, 1.0, 0.0),
         acisDirection(0.0, 0.0, 1.0)}).tokens;
    const std::vector<std::uint8_t> sphereSurfaceBytes =
        sab21800FromGraph(sphereSurfaceFixture.data);
    DRW_SabData parsedSphereSurface;
    const bool sphereSurfaceParsed = !sphereSurfaceBytes.empty()
        && drw_parseSab(sphereSurfaceBytes.data(), sphereSurfaceBytes.size(),
                        parsedSphereSurface);
    const DRW_AcisModel sphereSurfaceModel =
        drw_buildAcisModel(parsedSphereSurface);
    std::vector<DRW_AcisBodyProjection> sphereSurfaceProjections;
    DRW_AcisRootedProjectionReport sphereSurfaceReport;
    const bool sphereSurfaceProjected = drw_extractAcisBodyWireframes(
        sphereSurfaceModel, sphereSurfaceProjections, sphereSurfaceReport);
    const bool sphereSurfaceReachable = sphereSurfaceProjections.size() == 1u
        && std::find(
               sphereSurfaceProjections.front().reachableNodeIndices.begin(),
               sphereSurfaceProjections.front().reachableNodeIndices.end(),
               unsupportedSurface)
               != sphereSurfaceProjections.front().reachableNodeIndices.end();
    const bool sphereSurfaceParametersMatch =
        sphereSurfaceProjections.size() == 1u
        && !sphereSurfaceProjections.front().wireframe.faces.empty()
        && sphereSurfaceProjections.front().wireframe.faces.front()
                   .surfaceType == DRW_AcisSurface::Sphere
        && sphereSurfaceProjections.front().wireframe.faces.front()
                   .hasSurface
        && acisCoordNear(
            sphereSurfaceProjections.front().wireframe.faces.front().p0,
            4.0, 5.0, 6.0)
        && acisCoordNear(
            sphereSurfaceProjections.front().wireframe.faces.front().p1,
            0.0, 0.0, 1.0)
        && acisCoordNear(
            sphereSurfaceProjections.front().wireframe.faces.front().p2,
            0.0, 1.0, 0.0)
        && sphereSurfaceProjections.front().wireframe.faces.front().hasUDir
        && sphereSurfaceProjections.front().wireframe.faces.front().majorRadius
            == -7.5;
    t.expect(sphereSurfaceParsed && sphereSurfaceProjected
                 && sphereSurfaceReport.bodyCount == 1u
                 && sphereSurfaceReport.failure
                        == DRW_AcisProjectionFailure::NoFailure,
             "SAB 21800 sphere-surface parsed and rooted projection succeeds");
    t.expect(sphereSurfaceProjections.size() == 1u
                 && sphereSurfaceReachable
                 && sphereSurfaceProjections.front()
                        .report.unsupportedSurfaceNodeCount == 0u
                 && !sphereSurfaceProjections.front()
                        .report.brepValidityChecked,
             "SAB 21800 sphere-surface is reachable, recognized, and remains outside B-rep validity claims");
    t.expect(sphereSurfaceParametersMatch,
             "SAB 21800 sphere surface projects its signed radius and oriented analytic frame without claiming B-rep validity");
    const DRW_AcisFace* projectedSphereFace = nullptr;
    if (sphereSurfaceProjections.size() == 1u
        && !sphereSurfaceProjections.front().wireframe.faces.empty())
        projectedSphereFace =
            &sphereSurfaceProjections.front().wireframe.faces.front();
    if (projectedSphereFace) {
        DRW_Coord sphereProjection(91.0, 92.0, 93.0);
        DRW_Coord evaluatedSpherePoint(91.0, 92.0, 93.0);
        const bool signedSphereUPoint = drw_evaluateAcisSphereSurface(
            *projectedSphereFace, 0.0, 0.0, 1e-14,
            evaluatedSpherePoint)
            && acisCoordNear(evaluatedSpherePoint, 4.0, -2.5, 6.0);
        const bool signedSphereVPoint = drw_evaluateAcisSphereSurface(
            *projectedSphereFace, 1.57079632679489661923, 0.0, 1e-14,
            evaluatedSpherePoint)
            && acisCoordNear(evaluatedSpherePoint, 11.5, 5.0, 6.0);
        const bool signedSpherePolePoint = drw_evaluateAcisSphereSurface(
            *projectedSphereFace, 0.0, 1.57079632679489661923, 1e-14,
            evaluatedSpherePoint)
            && acisCoordNear(evaluatedSpherePoint, 4.0, 5.0, -1.5);
        DRW_AcisFace obliqueSphere = *projectedSphereFace;
        obliqueSphere.p1 = DRW_Coord(0.0, 3.0, 4.0);
        obliqueSphere.p2 = DRW_Coord(5.0, 0.0, 0.0);
        obliqueSphere.majorRadius = 3.0;
        const bool obliqueSpherePoint = drw_evaluateAcisSphereSurface(
            obliqueSphere, 1.57079632679489661923, 0.0, 1e-14,
            evaluatedSpherePoint)
            && std::fabs(evaluatedSpherePoint.x - 4.0) < 1e-12
            && std::fabs(evaluatedSpherePoint.y - 7.4) < 1e-12
            && std::fabs(evaluatedSpherePoint.z - 4.2) < 1e-12;
        DRW_AcisFace cancellationSphere = *projectedSphereFace;
        const double maxSphereEvaluation =
            std::numeric_limits<double>::max();
        cancellationSphere.p0 = DRW_Coord(1.25, 0.0, 0.0);
        cancellationSphere.p1 = DRW_Coord(0.0, 0.0, 1.0);
        const double cancellationParameter =
            1.57079632679489661923;
        const double cancellationCosine =
            std::cos(cancellationParameter);
        cancellationSphere.p2 = DRW_Coord(
            1.0, cancellationCosine, 0.0);
        cancellationSphere.majorRadius = maxSphereEvaluation / 4.0;
        DRW_Coord cancellationSpherePoint;
        const bool sphereFrameCancellation = drw_evaluateAcisSphereSurface(
            cancellationSphere, cancellationParameter, 0.0, 0.0,
            cancellationSpherePoint)
            && cancellationSpherePoint.x == 1.25
            && std::isfinite(cancellationSpherePoint.y)
            && cancellationSpherePoint.z == 0.0;
        DRW_AcisFace missingSphereU = *projectedSphereFace;
        missingSphereU.hasUDir = false;
        DRW_AcisFace nonorthogonalSphereU = *projectedSphereFace;
        nonorthogonalSphereU.p2 = DRW_Coord(0.0, 1.0, 1.0);
        DRW_AcisFace overflowingSphereEvaluation = *projectedSphereFace;
        overflowingSphereEvaluation.p0 = DRW_Coord(
            0.0, std::numeric_limits<double>::max(), 0.0);
        overflowingSphereEvaluation.majorRadius =
            std::numeric_limits<double>::max();
        const bool invalidSphereEvaluationRefused =
            !drw_evaluateAcisSphereSurface(
                missingSphereU, 0.2, 0.4, 1e-14,
                evaluatedSpherePoint)
            && acisCoordNear(evaluatedSpherePoint, 4.0, 7.4, 4.2)
            && !drw_evaluateAcisSphereSurface(
                nonorthogonalSphereU, 0.2, 0.4, 1e-14,
                evaluatedSpherePoint)
            && acisCoordNear(evaluatedSpherePoint, 4.0, 7.4, 4.2)
            && !drw_evaluateAcisSphereSurface(
                *projectedSphereFace, 0.0, 0.0, -1.0,
                evaluatedSpherePoint)
            && acisCoordNear(evaluatedSpherePoint, 4.0, 7.4, 4.2)
            && !drw_evaluateAcisSphereSurface(
                overflowingSphereEvaluation, 0.0, 0.0, 1e-14,
                evaluatedSpherePoint)
            && acisCoordNear(evaluatedSpherePoint, 4.0, 7.4, 4.2);
        t.expect(signedSphereUPoint,
                 "parser-fed sphere evaluator covers its signed-radius U point");
        t.expect(signedSphereVPoint,
                 "parser-fed sphere evaluator covers its signed-radius V point");
        t.expect(signedSpherePolePoint,
                 "parser-fed sphere evaluator covers its signed-radius pole point");
        t.expect(obliqueSpherePoint,
                 "sphere evaluator normalizes an oblique non-unit frame");
        t.expect(sphereFrameCancellation,
                 "sphere evaluator preserves small center through symmetric large tangent cancellation");
        t.expect(invalidSphereEvaluationRefused,
                 "sphere evaluator refuses invalid frames, tolerance, and overflow atomically");

        const bool parserFedSphereProjection =
            drw_projectPointToAcisSphereSurface(
                *projectedSphereFace, DRW_Coord(7.0, 9.0, 6.0),
                sphereProjection)
            && acisCoordNear(sphereProjection, 8.5, 11.0, 6.0);
        DRW_AcisFace positiveSphere = *projectedSphereFace;
        positiveSphere.majorRadius = 7.5;
        const bool signedRadiusSameLocus =
            drw_projectPointToAcisSphereSurface(
                positiveSphere, DRW_Coord(7.0, 9.0, 6.0), sphereProjection)
            && acisCoordNear(sphereProjection, 8.5, 11.0, 6.0);

        DRW_Coord unchangedSphereProjection(4.0, 5.0, 6.0);
        const bool centerRefusal =
            !drw_projectPointToAcisSphereSurface(
                *projectedSphereFace, projectedSphereFace->p0,
                unchangedSphereProjection)
            && acisCoordNear(unchangedSphereProjection, 4.0, 5.0, 6.0);
        DRW_AcisFace zeroRadiusSphere = *projectedSphereFace;
        zeroRadiusSphere.majorRadius = 0.0;
        const bool zeroRadiusProjection =
            drw_projectPointToAcisSphereSurface(
                zeroRadiusSphere, DRW_Coord(7.0, 9.0, 6.0),
                unchangedSphereProjection)
            && acisCoordNear(unchangedSphereProjection, 4.0, 5.0, 6.0);

        const double maxSphereCoordinate =
            std::numeric_limits<double>::max();
        DRW_AcisFace overflowSphere = *projectedSphereFace;
        overflowSphere.p0 = DRW_Coord(0.75 * maxSphereCoordinate, 0.0, 0.0);
        overflowSphere.majorRadius = maxSphereCoordinate;
        const bool overflowRefusal =
            !drw_projectPointToAcisSphereSurface(
                overflowSphere,
                DRW_Coord(maxSphereCoordinate, 0.0, 0.0),
                unchangedSphereProjection)
            && acisCoordNear(unchangedSphereProjection, 4.0, 5.0, 6.0);

        DRW_AcisFace scaledDeltaSphere = *projectedSphereFace;
        scaledDeltaSphere.p0 = DRW_Coord(-0.5 * maxSphereCoordinate,
                                         0.5 * maxSphereCoordinate, 0.0);
        scaledDeltaSphere.majorRadius = -0.25 * maxSphereCoordinate;
        const bool scaledDeltaProjection =
            drw_projectPointToAcisSphereSurface(
                scaledDeltaSphere,
                DRW_Coord(maxSphereCoordinate,
                          -maxSphereCoordinate, 0.0), sphereProjection)
            && std::fabs(sphereProjection.x / maxSphereCoordinate
                         - (-0.5 + 0.25 * std::sqrt(0.5))) < 1e-15
            && std::fabs(sphereProjection.y / maxSphereCoordinate
                         - (0.5 - 0.25 * std::sqrt(0.5))) < 1e-15;

        t.expect(parserFedSphereProjection && signedRadiusSameLocus,
                 "parser-fed sphere projection uses the signed-radius magnitude for the geometric locus");
        t.expect(centerRefusal && zeroRadiusProjection && overflowRefusal,
                 "sphere projection refuses ambiguous/unrepresentable cases and maps a degenerate locus to its center");
        t.expect(scaledDeltaProjection,
                 "sphere projection derives direction safely when coordinate subtraction overflows");

        double sphereSignedDistance = 55.0;
        const bool insideSphereBoundary =
            drw_pointWithinToleranceOfAcisSphereSurface(
                *projectedSphereFace, DRW_Coord(7.0, 9.0, 6.0), 2.5,
                sphereSignedDistance)
            && std::fabs(sphereSignedDistance + 2.5) < 1e-12;
        const bool outsideSphereBoundary =
            drw_pointWithinToleranceOfAcisSphereSurface(
                *projectedSphereFace, DRW_Coord(12.0, 5.0, 6.0), 0.5,
                sphereSignedDistance)
            && sphereSignedDistance == 0.5;
        const bool centerSphereDistance =
            drw_pointWithinToleranceOfAcisSphereSurface(
                *projectedSphereFace, projectedSphereFace->p0, 7.5,
                sphereSignedDistance)
            && sphereSignedDistance == -7.5;
        double zeroRadiusSignedDistance = 71.0;
        const bool zeroRadiusDistance =
            drw_pointWithinToleranceOfAcisSphereSurface(
                zeroRadiusSphere, DRW_Coord(7.0, 9.0, 6.0), 5.0,
                zeroRadiusSignedDistance)
            && zeroRadiusSignedDistance == 5.0;
        double unchangedZeroRadiusDistance = 72.0;
        const bool zeroRadiusOutsideTolerance =
            !drw_pointWithinToleranceOfAcisSphereSurface(
                zeroRadiusSphere, DRW_Coord(7.0, 9.0, 6.0),
                std::nextafter(5.0, 0.0), unchangedZeroRadiusDistance)
            && unchangedZeroRadiusDistance == 72.0;
        double unchangedSphereDistance = 67.0;
        const bool outsideToleranceRefusal =
            !drw_pointWithinToleranceOfAcisSphereSurface(
                *projectedSphereFace, DRW_Coord(12.0, 5.0, 6.0),
                std::nextafter(0.5, 0.0), unchangedSphereDistance)
            && unchangedSphereDistance == 67.0;
        DRW_AcisFace wrongSphereType = *projectedSphereFace;
        wrongSphereType.surfaceType = DRW_AcisSurface::Plane;
        const bool invalidSphereRefusal =
            !drw_pointWithinToleranceOfAcisSphereSurface(
                wrongSphereType, DRW_Coord(12.0, 5.0, 6.0), 1.0,
                unchangedSphereDistance)
            && unchangedSphereDistance == 67.0
            && !drw_pointWithinToleranceOfAcisSphereSurface(
                *projectedSphereFace,
                DRW_Coord(std::numeric_limits<double>::infinity(), 5.0, 6.0),
                1.0, unchangedSphereDistance)
            && unchangedSphereDistance == 67.0;
        DRW_AcisFace scaledDistanceSphere = *projectedSphereFace;
        scaledDistanceSphere.p0 = DRW_Coord(
            -0.1 * maxSphereCoordinate, 0.0, 0.0);
        scaledDistanceSphere.majorRadius = maxSphereCoordinate;
        const bool scaledSphereDistance =
            drw_pointWithinToleranceOfAcisSphereSurface(
                scaledDistanceSphere,
                DRW_Coord(maxSphereCoordinate, 0.0, 0.0),
                0.11 * maxSphereCoordinate, sphereSignedDistance)
            && std::fabs(sphereSignedDistance / maxSphereCoordinate - 0.1)
                < 1e-14;
        t.expect(insideSphereBoundary && outsideSphereBoundary
                     && centerSphereDistance && zeroRadiusDistance,
                 "sphere signed distance handles inside/outside/boundary/center points and the zero-radius point locus");
        t.expect(outsideToleranceRefusal && zeroRadiusOutsideTolerance
                     && invalidSphereRefusal,
                 "sphere tolerance predicate refuses invalid/outside cases atomically");
        t.expect(scaledSphereDistance,
                 "sphere signed distance remains representable when point-center subtraction overflows");

        DRW_Coord reversedSphereFaceNormal(4.0, 5.0, 6.0);
        const bool reversedSphereFaceNormalOk =
            drw_evaluateAcisSphereFaceNormal(
                *projectedSphereFace, DRW_Coord(11.5, 5.0, 6.0), 0.0,
                reversedSphereFaceNormal);
        DRW_AcisFace forwardSphereFace = *projectedSphereFace;
        forwardSphereFace.senseReversed = false;
        DRW_AcisFace unknownSphereFaceSense = forwardSphereFace;
        unknownSphereFaceSense.hasSense = false;
        DRW_Coord forwardSphereFaceNormal(4.0, 5.0, 6.0);
        const bool forwardSphereFaceNormalOk =
            drw_evaluateAcisSphereFaceNormal(
                forwardSphereFace, DRW_Coord(11.5, 5.0, 6.0), 0.0,
                forwardSphereFaceNormal);
        DRW_Coord toleratedSphereFaceNormal(4.0, 5.0, 6.0);
        const bool toleratedSphereFaceNormalOk =
            drw_evaluateAcisSphereFaceNormal(
                *projectedSphereFace, DRW_Coord(12.0, 5.0, 6.0), 0.5,
                toleratedSphereFaceNormal);
        DRW_Coord unchangedSphereFaceNormal(4.0, 5.0, 6.0);
        const bool invalidSphereFaceNormalRefused =
            !drw_evaluateAcisSphereFaceNormal(
                *projectedSphereFace, DRW_Coord(12.0, 5.0, 6.0),
                std::nextafter(0.5, 0.0), unchangedSphereFaceNormal)
            && !drw_evaluateAcisSphereFaceNormal(
                *projectedSphereFace, projectedSphereFace->p0, 7.5,
                unchangedSphereFaceNormal)
            && !drw_evaluateAcisSphereFaceNormal(
                zeroRadiusSphere, DRW_Coord(7.0, 9.0, 6.0), 5.0,
                unchangedSphereFaceNormal)
            && !drw_evaluateAcisSphereFaceNormal(
                unknownSphereFaceSense, DRW_Coord(11.5, 5.0, 6.0), 0.0,
                unchangedSphereFaceNormal)
            && unchangedSphereFaceNormal.x == 4.0
            && unchangedSphereFaceNormal.y == 5.0
            && unchangedSphereFaceNormal.z == 6.0;
        DRW_Coord scaledSphereFaceNormal(4.0, 5.0, 6.0);
        const bool scaledSphereFaceNormalOk =
            drw_evaluateAcisSphereFaceNormal(
                scaledDistanceSphere,
                DRW_Coord(maxSphereCoordinate, 0.0, 0.0),
                0.11 * maxSphereCoordinate, scaledSphereFaceNormal)
            && scaledSphereFaceNormal.x == -1.0
            && scaledSphereFaceNormal.y == 0.0
            && scaledSphereFaceNormal.z == 0.0;
        t.expect(reversedSphereFaceNormalOk
                     && reversedSphereFaceNormal.x == -1.0
                     && reversedSphereFaceNormal.y == 0.0
                     && reversedSphereFaceNormal.z == 0.0
                     && forwardSphereFaceNormalOk
                     && forwardSphereFaceNormal.x == 1.0
                     && forwardSphereFaceNormal.y == 0.0
                     && forwardSphereFaceNormal.z == 0.0
                     && toleratedSphereFaceNormalOk
                     && toleratedSphereFaceNormal.x == -1.0
                     && toleratedSphereFaceNormal.y == 0.0
                     && toleratedSphereFaceNormal.z == 0.0,
                 "parser-fed sphere face normal follows radial direction, caller tolerance, and face sense");
        t.expect(invalidSphereFaceNormalRefused
                     && scaledSphereFaceNormalOk,
                 "sphere face normal rejects ambiguity, zero radius, missing sense, and outside tolerance while scaling overflowing deltas");
    }

    AcisGraphFixture planeSurfaceFixture = complexFixture;
    DRW_SabRecord& planeSurfaceRecord =
        planeSurfaceFixture.data.records[
            static_cast<std::size_t>(unsupportedSurface)];
    planeSurfaceRecord.type = "plane-surface";
    planeSurfaceRecord.tokens = acisRecord(
        "plane-surface",
        {acisPointer(-1), acisLocation(5.0, 6.0, 7.0),
         acisDirection(0.0, 0.0, 1.0), acisDirection(1.0, 0.0, 0.0)}).tokens;
    const std::vector<std::uint8_t> planeSurfaceBytes =
        sab21800FromGraph(planeSurfaceFixture.data);
    DRW_SabData parsedPlaneSurface;
    const bool planeSurfaceParsed = !planeSurfaceBytes.empty()
        && drw_parseSab(planeSurfaceBytes.data(), planeSurfaceBytes.size(),
                        parsedPlaneSurface);
    const DRW_AcisModel planeSurfaceModel =
        drw_buildAcisModel(parsedPlaneSurface);
    std::vector<DRW_AcisBodyProjection> planeSurfaceProjections;
    DRW_AcisRootedProjectionReport planeSurfaceReport;
    const bool planeSurfaceProjected = drw_extractAcisBodyWireframes(
        planeSurfaceModel, planeSurfaceProjections, planeSurfaceReport);
    const DRW_AcisFace* projectedPlaneFace = nullptr;
    if (planeSurfaceProjections.size() == 1u
        && !planeSurfaceProjections.front().wireframe.faces.empty())
        projectedPlaneFace =
            &planeSurfaceProjections.front().wireframe.faces.front();
    t.expect(planeSurfaceParsed && planeSurfaceProjected
                 && planeSurfaceReport.failure
                        == DRW_AcisProjectionFailure::NoFailure
                 && projectedPlaneFace
                 && projectedPlaneFace->surfaceType
                        == DRW_AcisSurface::Plane
                 && projectedPlaneFace->hasSurface
                 && acisCoordNear(projectedPlaneFace->p0, 5.0, 6.0, 7.0)
                 && acisCoordNear(projectedPlaneFace->p1, 0.0, 0.0, 1.0)
                 && acisCoordNear(projectedPlaneFace->p2, 1.0, 0.0, 0.0)
                 && projectedPlaneFace->hasUDir,
             "SAB 21800 plane surface projects ordered origin, normal, and optional U direction");
    if (projectedPlaneFace) {
        double signedDistance = 101.0;
        const bool exactPlanePoint = drw_pointWithinToleranceOfAcisPlaneSurface(
            *projectedPlaneFace, DRW_Coord(8.0, 10.0, 7.0), 0.0,
            signedDistance) && signedDistance == 0.0;
        const bool abovePlane = drw_pointWithinToleranceOfAcisPlaneSurface(
            *projectedPlaneFace, DRW_Coord(8.5, 10.5, 7.25), 0.25,
            signedDistance) && signedDistance == 0.25;
        const bool belowPlane = drw_pointWithinToleranceOfAcisPlaneSurface(
            *projectedPlaneFace, DRW_Coord(8.5, 10.5, 6.75), 0.25,
            signedDistance) && signedDistance == -0.25;
        DRW_AcisFace nonUnitPlane = *projectedPlaneFace;
        nonUnitPlane.p1 = DRW_Coord(0.0, 0.0, 2.0);
        const bool nonUnitNormal = drw_pointWithinToleranceOfAcisPlaneSurface(
            nonUnitPlane, DRW_Coord(8.0, 10.0, 7.25), 0.25,
            signedDistance) && signedDistance == 0.25;
        DRW_Coord reversedPlaneFaceNormal(4.0, 5.0, 6.0);
        const bool reversedPlaneFaceNormalOk =
            drw_evaluateAcisPlaneFaceNormal(
                *projectedPlaneFace, reversedPlaneFaceNormal);
        DRW_AcisFace forwardPlaneFace = *projectedPlaneFace;
        forwardPlaneFace.senseReversed = false;
        DRW_Coord forwardPlaneFaceNormal(4.0, 5.0, 6.0);
        const bool forwardPlaneFaceNormalOk =
            drw_evaluateAcisPlaneFaceNormal(
                forwardPlaneFace, forwardPlaneFaceNormal);
        DRW_Coord nonUnitPlaneFaceNormal(4.0, 5.0, 6.0);
        const bool nonUnitPlaneFaceNormalOk =
            drw_evaluateAcisPlaneFaceNormal(
                nonUnitPlane, nonUnitPlaneFaceNormal);
        DRW_AcisFace unknownPlaneSense = forwardPlaneFace;
        unknownPlaneSense.hasSense = false;
        DRW_AcisFace zeroPlaneNormal = forwardPlaneFace;
        zeroPlaneNormal.p1 = DRW_Coord(0.0, 0.0, 0.0);
        DRW_AcisFace nonFinitePlaneNormal = forwardPlaneFace;
        nonFinitePlaneNormal.p1.x =
            std::numeric_limits<double>::quiet_NaN();
        DRW_AcisFace wrongPlaneSurfaceType = forwardPlaneFace;
        wrongPlaneSurfaceType.surfaceType = DRW_AcisSurface::Sphere;
        DRW_Coord unchangedPlaneFaceNormal(4.0, 5.0, 6.0);
        const bool invalidPlaneFaceNormalRefused =
            !drw_evaluateAcisPlaneFaceNormal(
                unknownPlaneSense, unchangedPlaneFaceNormal)
            && !drw_evaluateAcisPlaneFaceNormal(
                zeroPlaneNormal, unchangedPlaneFaceNormal)
            && !drw_evaluateAcisPlaneFaceNormal(
                nonFinitePlaneNormal, unchangedPlaneFaceNormal)
            && !drw_evaluateAcisPlaneFaceNormal(
                wrongPlaneSurfaceType, unchangedPlaneFaceNormal)
            && unchangedPlaneFaceNormal.x == 4.0
            && unchangedPlaneFaceNormal.y == 5.0
            && unchangedPlaneFaceNormal.z == 6.0;
        t.expect(reversedPlaneFaceNormalOk
                     && reversedPlaneFaceNormal.x == 0.0
                     && reversedPlaneFaceNormal.y == 0.0
                     && reversedPlaneFaceNormal.z == -1.0
                     && forwardPlaneFaceNormalOk
                     && forwardPlaneFaceNormal.x == 0.0
                     && forwardPlaneFaceNormal.y == 0.0
                     && forwardPlaneFaceNormal.z == 1.0
                     && nonUnitPlaneFaceNormalOk
                     && nonUnitPlaneFaceNormal.x == 0.0
                     && nonUnitPlaneFaceNormal.y == 0.0
                     && nonUnitPlaneFaceNormal.z == -1.0
                     && invalidPlaneFaceNormalRefused,
                 "parser-fed plane face normal normalizes and applies retained face sense, refusing unknown or degenerate normals atomically");

        const auto refusesAtomically = [](const DRW_AcisFace& face,
                                          const DRW_Coord& point,
                                          double tolerance) {
            double output = 73.0;
            return !drw_pointWithinToleranceOfAcisPlaneSurface(
                       face, point, tolerance, output)
                && output == 73.0;
        };
        DRW_AcisFace wrongSurface = *projectedPlaneFace;
        wrongSurface.surfaceType = DRW_AcisSurface::Sphere;
        DRW_AcisFace missingSurface = *projectedPlaneFace;
        missingSurface.hasSurface = false;
        DRW_AcisFace zeroNormal = *projectedPlaneFace;
        zeroNormal.p1 = DRW_Coord(0.0, 0.0, 0.0);
        DRW_AcisFace nonFiniteNormal = *projectedPlaneFace;
        nonFiniteNormal.p1 = DRW_Coord(
            0.0, std::numeric_limits<double>::quiet_NaN(), 1.0);
        const bool invalidRefusals =
            refusesAtomically(*projectedPlaneFace,
                              DRW_Coord(8.0, 10.0,
                                        std::nextafter(7.25, 8.0)), 0.25)
            && refusesAtomically(*projectedPlaneFace,
                                 DRW_Coord(8.0, 10.0, 7.0), -1.0)
            && refusesAtomically(*projectedPlaneFace,
                                 DRW_Coord(8.0, 10.0, 7.0),
                                 std::numeric_limits<double>::quiet_NaN())
            && refusesAtomically(*projectedPlaneFace,
                                 DRW_Coord(8.0, 10.0, 7.0),
                                 std::numeric_limits<double>::infinity())
            && refusesAtomically(wrongSurface,
                                 DRW_Coord(8.0, 10.0, 7.0), 0.0)
            && refusesAtomically(missingSurface,
                                 DRW_Coord(8.0, 10.0, 7.0), 0.0)
            && refusesAtomically(zeroNormal,
                                 DRW_Coord(8.0, 10.0, 7.0), 0.0)
            && refusesAtomically(nonFiniteNormal,
                                 DRW_Coord(8.0, 10.0, 7.0), 0.0)
            && refusesAtomically(*projectedPlaneFace,
                                 DRW_Coord(
                                     std::numeric_limits<double>::infinity(),
                                     10.0, 7.0), 0.0);

        DRW_AcisFace diagonalPlane = *projectedPlaneFace;
        diagonalPlane.p0 = DRW_Coord(-std::numeric_limits<double>::max(),
                                      std::numeric_limits<double>::max(), 0.0);
        diagonalPlane.p1 = DRW_Coord(1.0, 1.0, 0.0);
        double extremeSignedDistance = 19.0;
        const bool extremeCancellation =
            drw_pointWithinToleranceOfAcisPlaneSurface(
                diagonalPlane,
                DRW_Coord(std::numeric_limits<double>::max(),
                          -std::numeric_limits<double>::max(), 0.0),
                0.0, extremeSignedDistance)
            && extremeSignedDistance == 0.0;

        t.expect(exactPlanePoint && abovePlane && belowPlane && nonUnitNormal,
                 "parser-fed SAB plane supports exact and signed explicit-tolerance distance with raw normal scaling");
        t.expect(invalidRefusals,
                 "plane distance rejects invalid, unsupported, and out-of-tolerance inputs without changing output");
        t.expect(extremeCancellation,
                 "plane distance handles overflow-sized coordinate differences when signed distance remains representable");

        DRW_Coord projectedPoint(91.0, 92.0, 93.0);
        const bool parserFedProjection =
            drw_projectPointToAcisPlaneSurface(
                *projectedPlaneFace, DRW_Coord(8.0, 10.0, 7.25),
                projectedPoint)
            && acisCoordNear(projectedPoint, 8.0, 10.0, 7.0);
        DRW_AcisFace obliquePlane = *projectedPlaneFace;
        obliquePlane.p0 = DRW_Coord(0.0, 0.0, 0.0);
        obliquePlane.p1 = DRW_Coord(1.0, 1.0, 0.0);
        const bool obliqueProjection = drw_projectPointToAcisPlaneSurface(
            obliquePlane, DRW_Coord(3.0, 1.0, 5.0), projectedPoint)
            && acisCoordNear(projectedPoint, 1.0, -1.0, 5.0);

        DRW_AcisFace overflowCorrectionPlane = *projectedPlaneFace;
        overflowCorrectionPlane.p0 = DRW_Coord(
            -std::numeric_limits<double>::max(),
            -std::numeric_limits<double>::max(), 0.0);
        overflowCorrectionPlane.p1 = DRW_Coord(1.0, 1.0, 0.0);
        const double maxFinite = std::numeric_limits<double>::max();
        const bool overflowCorrectionProjection =
            drw_projectPointToAcisPlaneSurface(
                overflowCorrectionPlane,
                DRW_Coord(maxFinite, maxFinite, 0.0), projectedPoint)
            && std::fabs(projectedPoint.x / maxFinite + 1.0) < 1e-15
            && std::fabs(projectedPoint.y / maxFinite + 1.0) < 1e-15
            && projectedPoint.z == 0.0;

        DRW_AcisFace unrepresentableProjectionPlane = *projectedPlaneFace;
        unrepresentableProjectionPlane.p0 = DRW_Coord(0.0, 0.0, 0.0);
        unrepresentableProjectionPlane.p1 = DRW_Coord(1.0, -1.0, 1.0);
        DRW_Coord unchangedProjection(6.0, 7.0, 8.0);
        const bool unrepresentableProjectionRefused =
            !drw_projectPointToAcisPlaneSurface(
                unrepresentableProjectionPlane,
                DRW_Coord(maxFinite, maxFinite, -maxFinite),
                unchangedProjection)
            && acisCoordNear(unchangedProjection, 6.0, 7.0, 8.0);
        t.expect(parserFedProjection && obliqueProjection,
                 "plane projection handles parser-fed and oblique analytic planes");
        t.expect(overflowCorrectionProjection,
                 "plane projection preserves representable results despite overflowing correction intermediates");
        t.expect(unrepresentableProjectionRefused,
                 "plane projection refuses an unrepresentable coordinate atomically");
        DRW_Coord unchangedInvalidProjection(4.0, 5.0, 6.0);
        const bool invalidProjectionRefused =
            !drw_projectPointToAcisPlaneSurface(
                wrongSurface, DRW_Coord(8.0, 10.0, 7.0),
                unchangedInvalidProjection)
            && acisCoordNear(unchangedInvalidProjection, 4.0, 5.0, 6.0);
        DRW_Coord unchangedNonFiniteProjection(7.0, 8.0, 9.0);
        const bool nonFiniteProjectionRefused =
            !drw_projectPointToAcisPlaneSurface(
                *projectedPlaneFace,
                DRW_Coord(std::numeric_limits<double>::quiet_NaN(),
                          10.0, 7.0), unchangedNonFiniteProjection)
            && acisCoordNear(unchangedNonFiniteProjection, 7.0, 8.0, 9.0);
        t.expect(invalidProjectionRefused && nonFiniteProjectionRefused,
                 "plane projection rejects wrong-surface/non-finite inputs without changing output");

        DRW_Coord evaluatedPlanePoint(91.0, 92.0, 93.0);
        const bool parserFedPlaneEvaluation =
            drw_evaluateAcisPlaneSurface(
                *projectedPlaneFace, 3.0, 4.0, 0.0,
                evaluatedPlanePoint)
            && acisCoordNear(evaluatedPlanePoint, 8.0, 10.0, 7.0);
        DRW_AcisFace obliqueEvaluationPlane = *projectedPlaneFace;
        obliqueEvaluationPlane.p0 = DRW_Coord(0.0, 0.0, 0.0);
        obliqueEvaluationPlane.p1 = DRW_Coord(0.0, 3.0, 4.0);
        obliqueEvaluationPlane.p2 = DRW_Coord(5.0, 0.0, 0.0);
        const bool obliquePlaneEvaluation = drw_evaluateAcisPlaneSurface(
            obliqueEvaluationPlane, 2.0, 3.0, 0.0,
            evaluatedPlanePoint)
            && acisCoordNear(evaluatedPlanePoint, 2.0, 2.4, -1.8);
        DRW_AcisFace toleratedResidualPlane = *projectedPlaneFace;
        toleratedResidualPlane.p2 = DRW_Coord(1.0, 0.0, 1e-8);
        const bool toleratedResidualEvaluation =
            drw_evaluateAcisPlaneSurface(
                toleratedResidualPlane, 3.0, 4.0, 1e-7,
                evaluatedPlanePoint)
            && acisCoordNear(evaluatedPlanePoint, 8.0, 10.0, 7.0);

        const auto refusesPlaneEvaluationAtomically = [](
            const DRW_AcisFace& face, double u, double v,
            double frameTolerance) {
            DRW_Coord output(73.0, 74.0, 75.0);
            return !drw_evaluateAcisPlaneSurface(
                       face, u, v, frameTolerance, output)
                && acisCoordNear(output, 73.0, 74.0, 75.0);
        };
        DRW_AcisFace missingUPlane = *projectedPlaneFace;
        missingUPlane.hasUDir = false;
        DRW_AcisFace wrongSurfaceEvaluationPlane = *projectedPlaneFace;
        wrongSurfaceEvaluationPlane.surfaceType = DRW_AcisSurface::Sphere;
        DRW_AcisFace missingSurfaceEvaluationPlane = *projectedPlaneFace;
        missingSurfaceEvaluationPlane.hasSurface = false;
        DRW_AcisFace nonOrthogonalEvaluationPlane = *projectedPlaneFace;
        nonOrthogonalEvaluationPlane.p2 = DRW_Coord(1.0, 0.0, 0.01);
        DRW_AcisFace overflowEvaluationPlane = *projectedPlaneFace;
        overflowEvaluationPlane.p0 = DRW_Coord(
            std::numeric_limits<double>::max(), 0.0, 0.0);
        DRW_AcisFace cancellationEvaluationPlane = *projectedPlaneFace;
        cancellationEvaluationPlane.p0 = DRW_Coord(
            1.25, -std::numeric_limits<double>::max(), 0.0);
        cancellationEvaluationPlane.p1 = DRW_Coord(0.0, 0.0, 1.0);
        cancellationEvaluationPlane.p2 = DRW_Coord(1.0, 1.0, 0.0);
        const bool representableCancellationEvaluated =
            drw_evaluateAcisPlaneSurface(
                cancellationEvaluationPlane,
                std::numeric_limits<double>::max(),
                std::numeric_limits<double>::max(), 0.0,
                evaluatedPlanePoint);
        const bool representableCancellationCoordinates =
            evaluatedPlanePoint.x == 1.25
            && std::isfinite(evaluatedPlanePoint.y)
            && evaluatedPlanePoint.y > 0.0
            && std::fabs(evaluatedPlanePoint.y
                             / std::numeric_limits<double>::max()
                         - (std::sqrt(2.0) - 1.0)) < 1e-14;
        const bool invalidPlaneEvaluationRefused =
            refusesPlaneEvaluationAtomically(missingUPlane, 1.0, 2.0, 0.0)
            && refusesPlaneEvaluationAtomically(
                wrongSurfaceEvaluationPlane, 1.0, 2.0, 0.0)
            && refusesPlaneEvaluationAtomically(
                missingSurfaceEvaluationPlane, 1.0, 2.0, 0.0)
            && refusesPlaneEvaluationAtomically(
                nonOrthogonalEvaluationPlane, 1.0, 2.0, 0.0)
            && refusesPlaneEvaluationAtomically(
                *projectedPlaneFace,
                std::numeric_limits<double>::infinity(), 2.0, 0.0)
            && refusesPlaneEvaluationAtomically(
                *projectedPlaneFace, 1.0,
                std::numeric_limits<double>::quiet_NaN(), 0.0)
            && refusesPlaneEvaluationAtomically(
                *projectedPlaneFace, 1.0, 2.0, -1.0)
            && refusesPlaneEvaluationAtomically(
                *projectedPlaneFace, 1.0, 2.0,
                std::numeric_limits<double>::quiet_NaN())
            && refusesPlaneEvaluationAtomically(
                *projectedPlaneFace, 1.0, 2.0,
                std::numeric_limits<double>::infinity())
            && refusesPlaneEvaluationAtomically(
                overflowEvaluationPlane,
                std::numeric_limits<double>::max(), 0.0, 0.0);
        t.expect(parserFedPlaneEvaluation && obliquePlaneEvaluation
                     && toleratedResidualEvaluation,
                 "plane evaluation covers parsed fields, oblique frame, and bounded residual");
        t.expect(invalidPlaneEvaluationRefused,
                 "plane evaluation rejects invalid frames, parameters, and overflow atomically");
        t.expect(representableCancellationEvaluated,
                 "plane evaluation accepts representable cancellation after overflowing intermediate sums");
        t.expect(representableCancellationCoordinates,
                 "plane evaluation returns expected finite cancelled coordinates");
    }

    AcisGraphFixture torusSurfaceFixture = complexFixture;
    DRW_SabRecord& torusSurfaceRecord =
        torusSurfaceFixture.data.records[
            static_cast<std::size_t>(unsupportedSurface)];
    torusSurfaceRecord.type = "torus-surface";
    torusSurfaceRecord.tokens = acisRecord(
        "torus-surface",
        {acisPointer(-1), acisLocation(2.0, 3.0, 4.0),
         acisDirection(0.0, 0.0, 1.0), acisDouble(12.0),
         acisDouble(2.5), acisDirection(1.0, 0.0, 0.0)}).tokens;
    const std::vector<std::uint8_t> torusSurfaceBytes =
        sab21800FromGraph(torusSurfaceFixture.data);
    DRW_SabData parsedTorusSurface;
    const bool torusSurfaceParsed = !torusSurfaceBytes.empty()
        && drw_parseSab(torusSurfaceBytes.data(), torusSurfaceBytes.size(),
                        parsedTorusSurface);
    const DRW_AcisModel torusSurfaceModel =
        drw_buildAcisModel(parsedTorusSurface);
    std::vector<DRW_AcisBodyProjection> torusSurfaceProjections;
    DRW_AcisRootedProjectionReport torusSurfaceReport;
    const bool torusSurfaceProjected = drw_extractAcisBodyWireframes(
        torusSurfaceModel, torusSurfaceProjections, torusSurfaceReport);
    const DRW_AcisFace* projectedTorusFace = nullptr;
    if (torusSurfaceProjections.size() == 1u
        && !torusSurfaceProjections.front().wireframe.faces.empty())
        projectedTorusFace =
            &torusSurfaceProjections.front().wireframe.faces.front();
    t.expect(torusSurfaceParsed && torusSurfaceProjected
                 && torusSurfaceReport.failure
                        == DRW_AcisProjectionFailure::NoFailure
                 && projectedTorusFace
                 && projectedTorusFace->surfaceType
                        == DRW_AcisSurface::Torus
                 && projectedTorusFace->hasSurface
                 && acisCoordNear(projectedTorusFace->p0, 2.0, 3.0, 4.0)
                 && acisCoordNear(projectedTorusFace->p1, 0.0, 0.0, 1.0)
                 && acisCoordNear(projectedTorusFace->p2, 1.0, 0.0, 0.0)
                 && projectedTorusFace->hasUDir
                 && projectedTorusFace->majorRadius == 12.0
                 && projectedTorusFace->minorRadius == 2.5,
             "SAB 21800 torus surface consumes center, axis, radii, and U direction in schema order");
    if (projectedTorusFace) {
        DRW_Coord evaluatedTorusPoint(91.0, 92.0, 93.0);
        const bool torusOuterParameterPoint =
            drw_evaluateAcisTorusSurface(
                *projectedTorusFace, 0.0, 0.0, 1e-14,
                evaluatedTorusPoint)
            && acisCoordNear(evaluatedTorusPoint, 16.5, 3.0, 4.0);
        const bool torusQuarterParameterPoint =
            drw_evaluateAcisTorusSurface(
                *projectedTorusFace, 1.57079632679489661923,
                1.57079632679489661923, 1e-14,
                evaluatedTorusPoint)
            && std::fabs(evaluatedTorusPoint.x - 2.0) < 1e-12
            && std::fabs(evaluatedTorusPoint.y - 15.0) < 1e-12
            && std::fabs(evaluatedTorusPoint.z - 6.5) < 1e-12;
        DRW_AcisFace obliqueTorus = *projectedTorusFace;
        obliqueTorus.p1 = DRW_Coord(0.0, 3.0, 4.0);
        obliqueTorus.p2 = DRW_Coord(5.0, 0.0, 0.0);
        const double obliqueU = 0.7;
        const double obliqueV = -0.4;
        DRW_Coord obliqueTorusPoint;
        const bool obliqueTorusEvaluated = drw_evaluateAcisTorusSurface(
            obliqueTorus, obliqueU, obliqueV, 1e-14,
            obliqueTorusPoint);
        const double obliqueRadial = 12.0
            + 2.5 * std::cos(obliqueV);
        const DRW_Coord obliqueTorusExpected(
            2.0 + obliqueRadial * std::cos(obliqueU),
            3.0 + obliqueRadial * std::sin(obliqueU) * 0.8
                + 2.5 * std::sin(obliqueV) * 0.6,
            4.0 - obliqueRadial * std::sin(obliqueU) * 0.6
                + 2.5 * std::sin(obliqueV) * 0.8);
        const bool obliqueTorusExpectation = obliqueTorusEvaluated
            && std::hypot(
                   std::hypot(obliqueTorusPoint.x
                                  - obliqueTorusExpected.x,
                              obliqueTorusPoint.y
                                  - obliqueTorusExpected.y),
                   obliqueTorusPoint.z - obliqueTorusExpected.z) < 1e-11;
        DRW_Coord reversedTorusFaceNormal(91.0, 92.0, 93.0);
        const bool reversedTorusFaceNormalOk =
            drw_evaluateAcisTorusFaceNormal(
                *projectedTorusFace, 0.0, 0.0, 1e-14,
                reversedTorusFaceNormal)
            && projectedTorusFace->hasSense
            && projectedTorusFace->senseReversed
            && reversedTorusFaceNormal.x == -1.0
            && reversedTorusFaceNormal.y == 0.0
            && reversedTorusFaceNormal.z == 0.0;
        DRW_AcisFace forwardTorusFace = *projectedTorusFace;
        forwardTorusFace.senseReversed = false;
        DRW_Coord forwardTorusFaceNormal(91.0, 92.0, 93.0);
        const bool forwardTorusFaceNormalOk =
            drw_evaluateAcisTorusFaceNormal(
                forwardTorusFace, 0.0, 0.0, 1e-14,
                forwardTorusFaceNormal)
            && forwardTorusFaceNormal.x == 1.0
            && forwardTorusFaceNormal.y == 0.0
            && forwardTorusFaceNormal.z == 0.0;
        DRW_Coord obliqueTorusFaceNormal(91.0, 92.0, 93.0);
        const double obliqueCosineV = std::cos(obliqueV);
        const double obliqueSineV = std::sin(obliqueV);
        const double obliqueCosineU = std::cos(obliqueU);
        const double obliqueSineU = std::sin(obliqueU);
        const bool obliqueTorusFaceNormalOk =
            drw_evaluateAcisTorusFaceNormal(
                obliqueTorus, obliqueU, obliqueV, 1e-14,
                obliqueTorusFaceNormal)
            && std::fabs(obliqueTorusFaceNormal.x
                         + obliqueCosineV * obliqueCosineU) < 1e-14
            && std::fabs(obliqueTorusFaceNormal.y
                         + obliqueCosineV * obliqueSineU * 0.8
                         + obliqueSineV * 0.6) < 1e-14
            && std::fabs(obliqueTorusFaceNormal.z
                         - obliqueCosineV * obliqueSineU * 0.6
                         + obliqueSineV * 0.8) < 1e-14;
        const double torusPi = std::acos(-1.0);
        DRW_AcisFace hornPinchNormalFace = *projectedTorusFace;
        hornPinchNormalFace.majorRadius = 2.5;
        hornPinchNormalFace.minorRadius = 2.5;
        DRW_AcisFace spindleNormalFace = *projectedTorusFace;
        spindleNormalFace.majorRadius = 2.0;
        spindleNormalFace.minorRadius = 2.5;
        DRW_Coord spindleTorusFaceNormal(91.0, 92.0, 93.0);
        const bool spindleTorusFaceNormalOk =
            drw_evaluateAcisTorusFaceNormal(
                spindleNormalFace, 0.0, torusPi, 1e-14,
                spindleTorusFaceNormal)
            && std::fabs(spindleTorusFaceNormal.x + 1.0) < 1e-14
            && std::fabs(spindleTorusFaceNormal.y) < 1e-14
            && std::fabs(spindleTorusFaceNormal.z) < 1e-14;
        DRW_Coord unchangedTorusFaceNormal(91.0, 92.0, 93.0);
        DRW_AcisFace unknownTorusSense = forwardTorusFace;
        unknownTorusSense.hasSense = false;
        const bool torusNormalRefusals =
            !drw_evaluateAcisTorusFaceNormal(
                hornPinchNormalFace, 0.0, torusPi, 1e-14,
                unchangedTorusFaceNormal)
            && !drw_evaluateAcisTorusFaceNormal(
                unknownTorusSense, 0.0, 0.0, 1e-14,
                unchangedTorusFaceNormal)
            && !drw_evaluateAcisTorusFaceNormal(
                *projectedTorusFace, 0.0, 0.0, -1.0,
                unchangedTorusFaceNormal)
            && unchangedTorusFaceNormal.x == 91.0
            && unchangedTorusFaceNormal.y == 92.0
            && unchangedTorusFaceNormal.z == 93.0;
        const auto matchesTorusFiniteDifference = [](
            const DRW_AcisFace& face, double u, double v,
            bool senseReversed) {
            const double step = 1e-5;
            DRW_AcisFace orientedFace = face;
            orientedFace.senseReversed = senseReversed;
            DRW_Coord uPlus;
            DRW_Coord uMinus;
            DRW_Coord vPlus;
            DRW_Coord vMinus;
            DRW_Coord analyticNormal;
            if (!drw_evaluateAcisTorusSurface(
                    orientedFace, u + step, v, 1e-14, uPlus)
                || !drw_evaluateAcisTorusSurface(
                    orientedFace, u - step, v, 1e-14, uMinus)
                || !drw_evaluateAcisTorusSurface(
                    orientedFace, u, v + step, 1e-14, vPlus)
                || !drw_evaluateAcisTorusSurface(
                    orientedFace, u, v - step, 1e-14, vMinus)
                || !drw_evaluateAcisTorusFaceNormal(
                    orientedFace, u, v, 1e-14, analyticNormal))
                return false;
            const DRW_Coord tangentU(
                (uPlus.x - uMinus.x) / (2.0 * step),
                (uPlus.y - uMinus.y) / (2.0 * step),
                (uPlus.z - uMinus.z) / (2.0 * step));
            const DRW_Coord tangentV(
                (vPlus.x - vMinus.x) / (2.0 * step),
                (vPlus.y - vMinus.y) / (2.0 * step),
                (vPlus.z - vMinus.z) / (2.0 * step));
            const DRW_Coord cross(
                tangentU.y * tangentV.z - tangentU.z * tangentV.y,
                tangentU.z * tangentV.x - tangentU.x * tangentV.z,
                tangentU.x * tangentV.y - tangentU.y * tangentV.x);
            const double crossLength = std::hypot(
                std::hypot(cross.x, cross.y), cross.z);
            if (!std::isfinite(crossLength) || crossLength == 0.0)
                return false;
            const double direction = senseReversed ? -1.0 : 1.0;
            return std::fabs(analyticNormal.x
                             - direction * cross.x / crossLength) < 2e-9
                && std::fabs(analyticNormal.y
                             - direction * cross.y / crossLength) < 2e-9
                && std::fabs(analyticNormal.z
                             - direction * cross.z / crossLength) < 2e-9;
        };
        const bool torusNormalFiniteDifference =
            matchesTorusFiniteDifference(obliqueTorus, 0.7, -0.4, false)
            && matchesTorusFiniteDifference(obliqueTorus, 2.3, 1.1, true)
            && matchesTorusFiniteDifference(spindleNormalFace, 0.4, 3.0,
                                            false)
            && matchesTorusFiniteDifference(spindleNormalFace, 2.2, 4.0,
                                            true);
        DRW_AcisFace cancellationTorus = *projectedTorusFace;
        const double maxTorusEvaluation =
            std::numeric_limits<double>::max();
        const double torusCancellationParameter =
            1.57079632679489661923;
        const double torusCancellationCosine =
            std::cos(torusCancellationParameter);
        cancellationTorus.p0 = DRW_Coord(1.25, 0.0, 0.0);
        cancellationTorus.p1 = DRW_Coord(0.0, 0.0, 1.0);
        cancellationTorus.p2 = DRW_Coord(
            1.0, torusCancellationCosine, 0.0);
        cancellationTorus.majorRadius = maxTorusEvaluation / 8.0;
        cancellationTorus.minorRadius = maxTorusEvaluation / 8.0;
        DRW_Coord cancellationTorusPoint;
        const bool torusFrameCancellation = drw_evaluateAcisTorusSurface(
            cancellationTorus, torusCancellationParameter, 0.0, 0.0,
            cancellationTorusPoint)
            && cancellationTorusPoint.x == 1.25
            && std::isfinite(cancellationTorusPoint.y)
            && cancellationTorusPoint.z == 0.0;
        DRW_AcisFace missingTorusU = *projectedTorusFace;
        missingTorusU.hasUDir = false;
        DRW_AcisFace nonorthogonalTorusU = *projectedTorusFace;
        nonorthogonalTorusU.p2 = DRW_Coord(0.0, 0.0, 1.0);
        DRW_AcisFace overflowingTorus = *projectedTorusFace;
        overflowingTorus.p0 = DRW_Coord(
            std::numeric_limits<double>::max(), 0.0, 0.0);
        overflowingTorus.majorRadius =
            std::numeric_limits<double>::max();
        overflowingTorus.minorRadius =
            std::numeric_limits<double>::max();
        const bool invalidTorusEvaluationRefused =
            !drw_evaluateAcisTorusSurface(
                missingTorusU, 0.2, 0.4, 1e-14,
                evaluatedTorusPoint)
            && acisCoordNear(evaluatedTorusPoint, 2.0, 15.0, 6.5)
            && !drw_evaluateAcisTorusSurface(
                nonorthogonalTorusU, 0.2, 0.4, 1e-14,
                evaluatedTorusPoint)
            && acisCoordNear(evaluatedTorusPoint, 2.0, 15.0, 6.5)
            && !drw_evaluateAcisTorusSurface(
                *projectedTorusFace,
                std::numeric_limits<double>::infinity(), 0.0, 1e-14,
                evaluatedTorusPoint)
            && acisCoordNear(evaluatedTorusPoint, 2.0, 15.0, 6.5)
            && !drw_evaluateAcisTorusSurface(
                overflowingTorus, 0.0, 0.0, 1e-14,
                evaluatedTorusPoint)
            && acisCoordNear(evaluatedTorusPoint, 2.0, 15.0, 6.5);
        t.expect(torusOuterParameterPoint && torusQuarterParameterPoint,
                 "parser-fed torus evaluation covers outer and quarter-turn points");
        t.expect(obliqueTorusExpectation,
                 "torus evaluation normalizes an oblique non-unit frame");
        t.expect(torusFrameCancellation,
                 "torus evaluator preserves small center through symmetric large tangent cancellation");
        t.expect(invalidTorusEvaluationRefused,
                 "torus evaluation refuses absent, malformed, non-finite, and overflowing inputs atomically");
        t.expect(reversedTorusFaceNormalOk && forwardTorusFaceNormalOk,
                 "parser-fed torus face normal applies decoded forward and reversed sense");
        t.expect(obliqueTorusFaceNormalOk,
                 "torus face normal follows the differential in an oblique non-unit frame");
        t.expect(spindleTorusFaceNormalOk,
                 "spindle torus face normal preserves orientation across the inner nappe");
        t.expect(torusNormalRefusals,
                 "torus face normal refuses a singular pinch and invalid sense or tolerance atomically");
        t.expect(torusNormalFiniteDifference,
                 "torus face normal matches generated finite differences on ring and spindle nappes");

        double torusSignedDistance = 81.0;
        const bool parserFedOuterBoundary =
            drw_pointWithinToleranceOfAcisTorusSurface(
                *projectedTorusFace, DRW_Coord(16.5, 3.0, 4.0), 0.0,
                torusSignedDistance)
            && torusSignedDistance == 0.0;
        const bool parserFedTubeCenter =
            drw_pointWithinToleranceOfAcisTorusSurface(
                *projectedTorusFace, DRW_Coord(14.0, 3.0, 4.0), 2.5,
                torusSignedDistance)
            && torusSignedDistance == -2.5;
        const bool parserFedOuterToleranceBoundary =
            drw_pointWithinToleranceOfAcisTorusSurface(
                *projectedTorusFace, DRW_Coord(17.0, 3.0, 4.0), 0.5,
                torusSignedDistance)
            && torusSignedDistance == 0.5;
        double unchangedTorusDistance = 82.0;
        const bool justOutsideTorusTolerance =
            !drw_pointWithinToleranceOfAcisTorusSurface(
                *projectedTorusFace, DRW_Coord(17.0, 3.0, 4.0),
                std::nextafter(0.5, 0.0), unchangedTorusDistance)
            && unchangedTorusDistance == 82.0;

        DRW_AcisFace nonUnitTorusAxis = *projectedTorusFace;
        nonUnitTorusAxis.p1 = DRW_Coord(0.0, 0.0, 3.0);
        const bool nonUnitTorusAxisDistance =
            drw_pointWithinToleranceOfAcisTorusSurface(
                nonUnitTorusAxis, DRW_Coord(17.0, 3.0, 4.0), 0.5,
                torusSignedDistance)
            && torusSignedDistance == 0.5;
        const bool torusHoleDistance =
            drw_pointWithinToleranceOfAcisTorusSurface(
                *projectedTorusFace, DRW_Coord(2.0, 3.0, 4.0), 9.5,
                torusSignedDistance)
            && torusSignedDistance == 9.5;

        DRW_AcisFace hornTorus = *projectedTorusFace;
        hornTorus.majorRadius = hornTorus.minorRadius;
        DRW_AcisFace spindleTorus = *projectedTorusFace;
        spindleTorus.majorRadius = 2.0;
        const auto refusesTorusAtomically = [](
            const DRW_AcisFace& face, const DRW_Coord& point,
            double tolerance) {
            double output = 82.0;
            return !drw_pointWithinToleranceOfAcisTorusSurface(
                       face, point, tolerance, output)
                && output == 82.0;
        };
        DRW_AcisFace wrongTorusType = *projectedTorusFace;
        wrongTorusType.surfaceType = DRW_AcisSurface::Sphere;
        DRW_AcisFace missingTorusSurface = *projectedTorusFace;
        missingTorusSurface.hasSurface = false;
        DRW_AcisFace zeroTorusAxis = *projectedTorusFace;
        zeroTorusAxis.p1 = DRW_Coord(0.0, 0.0, 0.0);
        DRW_AcisFace nonFiniteTorusAxis = *projectedTorusFace;
        nonFiniteTorusAxis.p1 = DRW_Coord(
            0.0, std::numeric_limits<double>::quiet_NaN(), 1.0);
        DRW_AcisFace nonFiniteTorusRadius = *projectedTorusFace;
        nonFiniteTorusRadius.majorRadius =
            std::numeric_limits<double>::infinity();
        const bool invalidTorusRefusals =
            refusesTorusAtomically(
                hornTorus, DRW_Coord(16.5, 3.0, 4.0), 0.0)
            && refusesTorusAtomically(
                spindleTorus, DRW_Coord(16.5, 3.0, 4.0), 0.0)
            && refusesTorusAtomically(
                wrongTorusType, DRW_Coord(16.5, 3.0, 4.0), 0.0)
            && refusesTorusAtomically(
                missingTorusSurface, DRW_Coord(16.5, 3.0, 4.0), 0.0)
            && refusesTorusAtomically(
                zeroTorusAxis, DRW_Coord(16.5, 3.0, 4.0), 0.0)
            && refusesTorusAtomically(
                nonFiniteTorusAxis, DRW_Coord(16.5, 3.0, 4.0), 0.0)
            && refusesTorusAtomically(
                nonFiniteTorusRadius, DRW_Coord(16.5, 3.0, 4.0), 0.0)
            && refusesTorusAtomically(
                *projectedTorusFace,
                DRW_Coord(std::numeric_limits<double>::infinity(), 3.0, 4.0),
                1.0)
            && refusesTorusAtomically(
                *projectedTorusFace, DRW_Coord(17.0, 3.0, 4.0), -1.0)
            && refusesTorusAtomically(
                *projectedTorusFace, DRW_Coord(17.0, 3.0, 4.0),
                std::numeric_limits<double>::quiet_NaN())
            && refusesTorusAtomically(
                *projectedTorusFace, DRW_Coord(17.0, 3.0, 4.0),
                std::numeric_limits<double>::infinity());

        const double maxTorusCoordinate =
            std::numeric_limits<double>::max();
        DRW_AcisFace scaledOverflowTorus = *projectedTorusFace;
        scaledOverflowTorus.p0 =
            DRW_Coord(-0.75 * maxTorusCoordinate, 0.0, 0.0);
        scaledOverflowTorus.majorRadius = 0.5 * maxTorusCoordinate;
        scaledOverflowTorus.minorRadius = 0.25 * maxTorusCoordinate;
        const bool overflowDifferenceDistance =
            drw_pointWithinToleranceOfAcisTorusSurface(
                scaledOverflowTorus,
                DRW_Coord(maxTorusCoordinate, 0.0, 0.0),
                maxTorusCoordinate, torusSignedDistance)
            && std::fabs(torusSignedDistance / maxTorusCoordinate - 1.0)
                < 1e-15;

        t.expect(parserFedOuterBoundary && parserFedTubeCenter
                     && parserFedOuterToleranceBoundary
                     && nonUnitTorusAxisDistance && torusHoleDistance,
                 "parser-fed ring-torus distance covers its locus, tube interior, hole, and normalized axis");
        t.expect(justOutsideTorusTolerance && invalidTorusRefusals,
                 "torus distance rejects just-outside and unsupported inputs atomically");
        t.expect(overflowDifferenceDistance,
                 "torus distance retains a representable result when point-center subtraction overflows");

        DRW_Coord torusProjectedPoint(91.0, 92.0, 93.0);
        const bool parserFedOuterProjection =
            drw_projectPointToAcisTorusSurface(
                *projectedTorusFace, DRW_Coord(18.0, 3.0, 4.0),
                torusProjectedPoint)
            && acisCoordNear(torusProjectedPoint, 16.5, 3.0, 4.0);
        const bool parserFedInnerProjection =
            drw_projectPointToAcisTorusSurface(
                *projectedTorusFace, DRW_Coord(10.0, 3.0, 4.0),
                torusProjectedPoint)
            && acisCoordNear(torusProjectedPoint, 11.5, 3.0, 4.0);
        DRW_AcisFace nonUnitProjectionAxis = *projectedTorusFace;
        nonUnitProjectionAxis.p1 = DRW_Coord(0.0, 0.0, 3.0);
        const double inverseRootTwo = 1.0 / std::sqrt(2.0);
        const bool obliqueProjection =
            drw_projectPointToAcisTorusSurface(
                nonUnitProjectionAxis, DRW_Coord(17.0, 3.0, 7.0),
                torusProjectedPoint)
            && std::fabs(torusProjectedPoint.x
                         - (2.0 + 12.0 + 2.5 * inverseRootTwo)) < 1e-12
            && torusProjectedPoint.y == 3.0
            && std::fabs(torusProjectedPoint.z
                         - (4.0 + 2.5 * inverseRootTwo)) < 1e-12;

        DRW_Coord unchangedTorusProjection(7.0, 8.0, 9.0);
        DRW_AcisFace zeroMajorRadiusTorus = *projectedTorusFace;
        zeroMajorRadiusTorus.majorRadius = 0.0;
        DRW_AcisFace negativeMinorRadiusTorus = *projectedTorusFace;
        negativeMinorRadiusTorus.minorRadius = -2.5;
        DRW_AcisFace zeroTorusProjectionAxis = *projectedTorusFace;
        zeroTorusProjectionAxis.p1 = DRW_Coord(0.0, 0.0, 0.0);
        DRW_AcisFace nonFiniteTorusProjectionRadius = *projectedTorusFace;
        nonFiniteTorusProjectionRadius.minorRadius =
            std::numeric_limits<double>::infinity();
        const bool ambiguousTorusRefusals =
            !drw_projectPointToAcisTorusSurface(
                *projectedTorusFace, DRW_Coord(14.0, 3.0, 4.0),
                unchangedTorusProjection)
            && acisCoordNear(unchangedTorusProjection, 7.0, 8.0, 9.0)
            && !drw_projectPointToAcisTorusSurface(
                *projectedTorusFace, DRW_Coord(2.0, 3.0, 10.0),
                unchangedTorusProjection)
            && acisCoordNear(unchangedTorusProjection, 7.0, 8.0, 9.0)
            && !drw_projectPointToAcisTorusSurface(
                *projectedTorusFace,
                DRW_Coord(std::numeric_limits<double>::infinity(), 3.0, 4.0),
                unchangedTorusProjection)
            && acisCoordNear(unchangedTorusProjection, 7.0, 8.0, 9.0)
            && !drw_projectPointToAcisTorusSurface(
                zeroMajorRadiusTorus, DRW_Coord(18.0, 3.0, 4.0),
                unchangedTorusProjection)
            && acisCoordNear(unchangedTorusProjection, 7.0, 8.0, 9.0)
            && !drw_projectPointToAcisTorusSurface(
                negativeMinorRadiusTorus, DRW_Coord(18.0, 3.0, 4.0),
                unchangedTorusProjection)
            && acisCoordNear(unchangedTorusProjection, 7.0, 8.0, 9.0)
            && !drw_projectPointToAcisTorusSurface(
                zeroTorusProjectionAxis, DRW_Coord(18.0, 3.0, 4.0),
                unchangedTorusProjection)
            && acisCoordNear(unchangedTorusProjection, 7.0, 8.0, 9.0)
            && !drw_projectPointToAcisTorusSurface(
                nonFiniteTorusProjectionRadius, DRW_Coord(18.0, 3.0, 4.0),
                unchangedTorusProjection)
            && acisCoordNear(unchangedTorusProjection, 7.0, 8.0, 9.0)
            && !drw_projectPointToAcisTorusSurface(
                nonFiniteTorusAxis, DRW_Coord(18.0, 3.0, 4.0),
                unchangedTorusProjection)
            && acisCoordNear(unchangedTorusProjection, 7.0, 8.0, 9.0)
            && !drw_projectPointToAcisTorusSurface(
                wrongTorusType, DRW_Coord(18.0, 3.0, 4.0),
                unchangedTorusProjection)
            && acisCoordNear(unchangedTorusProjection, 7.0, 8.0, 9.0)
            && !drw_projectPointToAcisTorusSurface(
                missingTorusSurface, DRW_Coord(18.0, 3.0, 4.0),
                unchangedTorusProjection)
            && acisCoordNear(unchangedTorusProjection, 7.0, 8.0, 9.0);
        const bool hornAxisProjection =
            drw_projectPointToAcisTorusSurface(
                hornTorus, DRW_Coord(2.0, 3.0, 4.0),
                torusProjectedPoint)
            && acisCoordNear(torusProjectedPoint, 2.0, 3.0, 4.0);
        const bool spindleOuterProjection =
            drw_projectPointToAcisTorusSurface(
                spindleTorus, DRW_Coord(7.0, 3.0, 4.0),
                torusProjectedPoint)
            && acisCoordNear(torusProjectedPoint, 6.5, 3.0, 4.0);
        const bool spindleNegativeRadialProjection =
            drw_projectPointToAcisTorusSurface(
                spindleTorus, DRW_Coord(2.5, 3.0, 4.0),
                torusProjectedPoint)
            && acisCoordNear(torusProjectedPoint, 1.5, 3.0, 4.0);
        const bool spindleAxisAmbiguity =
            !drw_projectPointToAcisTorusSurface(
                spindleTorus, DRW_Coord(2.0, 3.0, 4.0),
                unchangedTorusProjection)
            && acisCoordNear(unchangedTorusProjection, 7.0, 8.0, 9.0);
        const bool overflowingDifferenceProjection =
            drw_projectPointToAcisTorusSurface(
                scaledOverflowTorus,
                DRW_Coord(maxTorusCoordinate, 0.0, 0.0),
                torusProjectedPoint)
            && acisCoordNear(torusProjectedPoint, 0.0, 0.0, 0.0);

        t.expect(parserFedOuterProjection && parserFedInnerProjection
                     && obliqueProjection,
                 "parser-fed torus projection covers outer, inner, and oblique points on the analytic locus");
        t.expect(ambiguousTorusRefusals,
                 "torus projection refuses non-unique axis/major-circle and invalid-input cases atomically");
        t.expect(hornAxisProjection,
                 "horn-torus pinch at the axis projects to its unique point");
        t.expect(spindleOuterProjection,
                 "spindle-torus outer point projects to the torus locus");
        t.expect(spindleNegativeRadialProjection,
                 "spindle-torus projection reverses the radial direction when the closest cross-section radius is negative");
        t.expect(spindleAxisAmbiguity,
                 "spindle-torus axis ambiguity refuses without changing output");
        t.expect(overflowingDifferenceProjection,
                 "torus projection retains a representable point when point-center subtraction overflows");
    }

    AcisGraphFixture cylinderSurfaceFixture = complexFixture;
    DRW_SabRecord& cylinderSurfaceRecord =
        cylinderSurfaceFixture.data.records[
            static_cast<std::size_t>(unsupportedSurface)];
    cylinderSurfaceRecord.type = "cone-surface";
    cylinderSurfaceRecord.tokens = acisRecord(
        "cone-surface",
        {acisPointer(-1), acisLocation(2.0, 3.0, 4.0),
         acisDirection(0.0, 0.0, 2.0), acisDirection(0.0, 4.0, 0.0),
         acisDouble(1.0), acisDouble(0.0), acisDouble(1.0)}).tokens;
    const std::vector<std::uint8_t> cylinderSurfaceBytes =
        sab21800FromGraph(cylinderSurfaceFixture.data);
    DRW_SabData parsedCylinderSurface;
    const bool cylinderSurfaceParsed = !cylinderSurfaceBytes.empty()
        && drw_parseSab(cylinderSurfaceBytes.data(),
                        cylinderSurfaceBytes.size(), parsedCylinderSurface);
    std::vector<DRW_AcisBodyProjection> cylinderSurfaceProjections;
    DRW_AcisRootedProjectionReport cylinderSurfaceReport;
    const bool cylinderSurfaceProjected = drw_extractAcisBodyWireframes(
        drw_buildAcisModel(parsedCylinderSurface),
        cylinderSurfaceProjections, cylinderSurfaceReport);
    const DRW_AcisFace* projectedCylinderFace = nullptr;
    if (cylinderSurfaceProjections.size() == 1u
        && !cylinderSurfaceProjections.front().wireframe.faces.empty())
        projectedCylinderFace =
            &cylinderSurfaceProjections.front().wireframe.faces.front();
    t.expect(cylinderSurfaceParsed && cylinderSurfaceProjected
                 && cylinderSurfaceReport.failure
                        == DRW_AcisProjectionFailure::NoFailure
                 && projectedCylinderFace
                 && projectedCylinderFace->surfaceType
                        == DRW_AcisSurface::Cone
                 && projectedCylinderFace->hasSurface
                 && acisCoordNear(projectedCylinderFace->p0, 2.0, 3.0, 4.0)
                 && acisCoordNear(projectedCylinderFace->p1, 0.0, 0.0, 2.0)
                 && acisCoordNear(projectedCylinderFace->p2, 0.0, 4.0, 0.0)
                 && projectedCylinderFace->ratio == 1.0
                 && projectedCylinderFace->sineAngle == 0.0
                 && projectedCylinderFace->cosineAngle == 1.0,
             "SAB 21800 canonical cone-surface cylinder profile is retained for bounded projection");
    if (projectedCylinderFace) {
        DRW_Coord cylinderProjection(91.0, 92.0, 93.0);
        const bool parserFedRadialProjection =
            drw_projectPointToAcisCylinderSurface(
                *projectedCylinderFace, DRW_Coord(10.0, 3.0, 9.0),
                cylinderProjection)
            && acisCoordNear(cylinderProjection, 6.0, 3.0, 9.0);
        double cylinderSignedDistance = 81.0;
        const bool cylinderSurfaceDistance =
            drw_pointWithinToleranceOfAcisCylinderSurface(
                *projectedCylinderFace, DRW_Coord(6.0, 3.0, 9.0), 0.0,
                cylinderSignedDistance)
            && cylinderSignedDistance == 0.0;
        const bool cylinderOutsideBoundary =
            drw_pointWithinToleranceOfAcisCylinderSurface(
                *projectedCylinderFace, DRW_Coord(8.0, 3.0, 9.0), 2.0,
                cylinderSignedDistance)
            && cylinderSignedDistance == 2.0;
        const bool cylinderInsideBoundary =
            drw_pointWithinToleranceOfAcisCylinderSurface(
                *projectedCylinderFace, DRW_Coord(4.0, 3.0, 9.0), 2.0,
                cylinderSignedDistance)
            && cylinderSignedDistance == -2.0;
        double unchangedCylinderDistance = 82.0;
        const bool cylinderJustOutsideTolerance =
            !drw_pointWithinToleranceOfAcisCylinderSurface(
                *projectedCylinderFace, DRW_Coord(8.0, 3.0, 9.0),
                std::nextafter(2.0, 0.0), unchangedCylinderDistance)
            && unchangedCylinderDistance == 82.0;
        const bool cylinderAxisSignedDistance =
            drw_pointWithinToleranceOfAcisCylinderSurface(
                *projectedCylinderFace, DRW_Coord(2.0, 3.0, 10.0), 4.0,
                cylinderSignedDistance)
            && cylinderSignedDistance == -4.0;
        DRW_AcisFace nonUnitCylinderAxis = *projectedCylinderFace;
        nonUnitCylinderAxis.p1 = DRW_Coord(0.0, 0.0, 7.0);
        const double radiusOverRootTwo = 4.0 / std::sqrt(2.0);
        const bool obliqueCylinderProjection =
            drw_projectPointToAcisCylinderSurface(
                nonUnitCylinderAxis, DRW_Coord(10.0, 11.0, 10.0),
                cylinderProjection)
            && std::fabs(cylinderProjection.x - (2.0 + radiusOverRootTwo))
                < 1e-12
            && std::fabs(cylinderProjection.y - (3.0 + radiusOverRootTwo))
                < 1e-12
            && cylinderProjection.z == 10.0;

        const auto refusesCylinderProjectionAtomically = [](
            const DRW_AcisFace& face, const DRW_Coord& point) {
            DRW_Coord output(7.0, 8.0, 9.0);
            return !drw_projectPointToAcisCylinderSurface(face, point, output)
                && acisCoordNear(output, 7.0, 8.0, 9.0);
        };
        DRW_AcisFace wrongCylinderType = *projectedCylinderFace;
        wrongCylinderType.surfaceType = DRW_AcisSurface::Torus;
        DRW_AcisFace missingCylinderSurface = *projectedCylinderFace;
        missingCylinderSurface.hasSurface = false;
        DRW_AcisFace zeroCylinderAxis = *projectedCylinderFace;
        zeroCylinderAxis.p1 = DRW_Coord(0.0, 0.0, 0.0);
        DRW_AcisFace zeroCylinderRadius = *projectedCylinderFace;
        zeroCylinderRadius.p2 = DRW_Coord(0.0, 0.0, 0.0);
        DRW_AcisFace ellipticConeProfile = *projectedCylinderFace;
        ellipticConeProfile.ratio = 0.5;
        DRW_AcisFace slopedConeProfile = *projectedCylinderFace;
        slopedConeProfile.sineAngle = 0.5;
        DRW_AcisFace nonCanonicalCylinderProfile = *projectedCylinderFace;
        nonCanonicalCylinderProfile.cosineAngle = -1.0;
        DRW_AcisFace nonFiniteCylinderAxis = *projectedCylinderFace;
        nonFiniteCylinderAxis.p1.y =
            std::numeric_limits<double>::quiet_NaN();
        const bool cylinderAmbiguityAndInvalidRefusals =
            refusesCylinderProjectionAtomically(
                *projectedCylinderFace, DRW_Coord(2.0, 3.0, 10.0))
            && refusesCylinderProjectionAtomically(
                wrongCylinderType, DRW_Coord(10.0, 3.0, 9.0))
            && refusesCylinderProjectionAtomically(
                missingCylinderSurface, DRW_Coord(10.0, 3.0, 9.0))
            && refusesCylinderProjectionAtomically(
                zeroCylinderAxis, DRW_Coord(10.0, 3.0, 9.0))
            && refusesCylinderProjectionAtomically(
                zeroCylinderRadius, DRW_Coord(10.0, 3.0, 9.0))
            && refusesCylinderProjectionAtomically(
                ellipticConeProfile, DRW_Coord(10.0, 3.0, 9.0))
            && refusesCylinderProjectionAtomically(
                slopedConeProfile, DRW_Coord(10.0, 3.0, 9.0))
            && refusesCylinderProjectionAtomically(
                nonCanonicalCylinderProfile, DRW_Coord(10.0, 3.0, 9.0))
            && refusesCylinderProjectionAtomically(
                nonFiniteCylinderAxis, DRW_Coord(10.0, 3.0, 9.0))
            && refusesCylinderProjectionAtomically(
                *projectedCylinderFace,
                DRW_Coord(std::numeric_limits<double>::infinity(), 3.0, 9.0));

        const double maxCylinderCoordinate =
            std::numeric_limits<double>::max();
        DRW_AcisFace overflowCylinder = *projectedCylinderFace;
        overflowCylinder.p0 =
            DRW_Coord(-0.75 * maxCylinderCoordinate, 0.0, 0.0);
        overflowCylinder.p1 = DRW_Coord(0.0, 0.0, 1.0);
        overflowCylinder.p2 =
            DRW_Coord(0.25 * maxCylinderCoordinate, 0.0, 0.0);
        const bool overflowingDifferenceProjection =
            drw_projectPointToAcisCylinderSurface(
                overflowCylinder,
                DRW_Coord(maxCylinderCoordinate, 0.0, 1.0),
                cylinderProjection)
            && std::fabs(cylinderProjection.x / maxCylinderCoordinate + 0.5)
                < 1e-15
            && cylinderProjection.y == 0.0
            && std::fabs(cylinderProjection.z - 1.0) < 1e-12;

        DRW_AcisFace overflowDistanceCylinder = *projectedCylinderFace;
        overflowDistanceCylinder.p0 =
            DRW_Coord(-0.25 * maxCylinderCoordinate, 0.0, 0.0);
        overflowDistanceCylinder.p1 = DRW_Coord(0.0, 0.0, 1.0);
        overflowDistanceCylinder.p2 =
            DRW_Coord(0.5 * maxCylinderCoordinate, 0.0, 0.0);
        const bool overflowingDifferenceDistance =
            drw_pointWithinToleranceOfAcisCylinderSurface(
                overflowDistanceCylinder,
                DRW_Coord(0.9 * maxCylinderCoordinate, 0.0, 1.0),
                maxCylinderCoordinate, cylinderSignedDistance)
            && std::fabs(cylinderSignedDistance / maxCylinderCoordinate - 0.65)
                < 1e-14;

        DRW_AcisFace unrepresentableCylinder = *projectedCylinderFace;
        unrepresentableCylinder.p0 =
            DRW_Coord(0.5 * maxCylinderCoordinate, 0.0, 0.0);
        unrepresentableCylinder.p2 =
            DRW_Coord(maxCylinderCoordinate, 0.0, 0.0);
        const bool unrepresentableProjectionRefused =
            refusesCylinderProjectionAtomically(
                unrepresentableCylinder,
                DRW_Coord(maxCylinderCoordinate, 0.0, 1.0));

        DRW_AcisFace tinyRadiusCylinder = *projectedCylinderFace;
        tinyRadiusCylinder.p0 = DRW_Coord(0.0, 0.0,
                                          -maxCylinderCoordinate);
        tinyRadiusCylinder.p1 = DRW_Coord(0.0, 0.0, 1.0);
        tinyRadiusCylinder.p2 = DRW_Coord(1e-300, 0.0, 0.0);
        const bool tinyRadiusProjectionSurvivesScaling =
            drw_projectPointToAcisCylinderSurface(
                tinyRadiusCylinder,
                DRW_Coord(1e300, 0.0, maxCylinderCoordinate),
                cylinderProjection)
            && cylinderProjection.x == 1e-300
            && cylinderProjection.y == 0.0
            && cylinderProjection.z == maxCylinderCoordinate;

        DRW_AcisFace tinyRadialLargeAxialCylinder = tinyRadiusCylinder;
        tinyRadialLargeAxialCylinder.p0 = DRW_Coord(0.0, 0.0, 0.0);
        const DRW_Coord tinyRadialLargeAxialPoint(
            1e-300, 0.0, maxCylinderCoordinate);
        const bool tinyRadialLargeAxialProjection =
            drw_projectPointToAcisCylinderSurface(
                tinyRadialLargeAxialCylinder, tinyRadialLargeAxialPoint,
                cylinderProjection)
            && cylinderProjection.x == 1e-300
            && cylinderProjection.y == 0.0
            && cylinderProjection.z == maxCylinderCoordinate;
        const bool tinyRadialLargeAxialDistance =
            drw_pointWithinToleranceOfAcisCylinderSurface(
                tinyRadialLargeAxialCylinder, tinyRadialLargeAxialPoint, 0.0,
                cylinderSignedDistance)
            && cylinderSignedDistance == 0.0;
        const DRW_Coord tinyRadialOverflowAxialPoint(
            1e-300, 0.0, maxCylinderCoordinate);
        const bool tinyRadialOverflowAxialProjection =
            drw_projectPointToAcisCylinderSurface(
                tinyRadiusCylinder, tinyRadialOverflowAxialPoint,
                cylinderProjection)
            && cylinderProjection.x == 1e-300
            && cylinderProjection.y == 0.0
            && cylinderProjection.z == maxCylinderCoordinate;
        const bool tinyRadialOverflowAxialDistance =
            drw_pointWithinToleranceOfAcisCylinderSurface(
                tinyRadiusCylinder, tinyRadialOverflowAxialPoint, 0.0,
                cylinderSignedDistance)
            && cylinderSignedDistance == 0.0;

        DRW_AcisFace tinyXAxisCylinder = tinyRadialLargeAxialCylinder;
        tinyXAxisCylinder.p1 = DRW_Coord(3.0, 0.0, 0.0);
        tinyXAxisCylinder.p2 = DRW_Coord(0.0, 1e-300, 0.0);
        const DRW_Coord tinyXAxisPoint(maxCylinderCoordinate, 1e-300, 0.0);
        const bool tinyXAxisProjection =
            drw_projectPointToAcisCylinderSurface(
                tinyXAxisCylinder, tinyXAxisPoint, cylinderProjection)
            && cylinderProjection.x == maxCylinderCoordinate
            && cylinderProjection.y == 1e-300
            && cylinderProjection.z == 0.0;
        const bool tinyXAxisDistance =
            drw_pointWithinToleranceOfAcisCylinderSurface(
                tinyXAxisCylinder, tinyXAxisPoint, 0.0,
                cylinderSignedDistance)
            && cylinderSignedDistance == 0.0;

        DRW_AcisFace tinyYAxisCylinder = tinyRadialLargeAxialCylinder;
        tinyYAxisCylinder.p1 = DRW_Coord(0.0, -5.0, 0.0);
        tinyYAxisCylinder.p2 = DRW_Coord(0.0, 0.0, 1e-300);
        const DRW_Coord tinyYAxisPoint(0.0, maxCylinderCoordinate, 1e-300);
        const bool tinyYAxisProjection =
            drw_projectPointToAcisCylinderSurface(
                tinyYAxisCylinder, tinyYAxisPoint, cylinderProjection)
            && cylinderProjection.x == 0.0
            && cylinderProjection.y == maxCylinderCoordinate
            && cylinderProjection.z == 1e-300;
        const bool tinyYAxisDistance =
            drw_pointWithinToleranceOfAcisCylinderSurface(
                tinyYAxisCylinder, tinyYAxisPoint, 0.0,
                cylinderSignedDistance)
            && cylinderSignedDistance == 0.0;

        DRW_AcisFace obliqueCylinderAxis = *projectedCylinderFace;
        obliqueCylinderAxis.p1 = DRW_Coord(2.0, 4.0, 6.0);
        const double inverseRootFourteen = 1.0 / std::sqrt(14.0);
        const double inverseRootFive = 1.0 / std::sqrt(5.0);
        const DRW_Coord obliqueAxisOffset(
            2.0 * inverseRootFourteen + 10.0 * inverseRootFive,
            4.0 * inverseRootFourteen - 5.0 * inverseRootFive,
            6.0 * inverseRootFourteen);
        const DRW_Coord obliqueCylinderPoint(
            2.0 + obliqueAxisOffset.x,
            3.0 + obliqueAxisOffset.y,
            4.0 + obliqueAxisOffset.z);
        const bool obliqueCylinderAxisProjection =
            drw_projectPointToAcisCylinderSurface(
                obliqueCylinderAxis, obliqueCylinderPoint,
                cylinderProjection)
            && std::fabs(cylinderProjection.x
                         - (2.0 + 2.0 * inverseRootFourteen
                            + 8.0 * inverseRootFive)) < 1e-12
            && std::fabs(cylinderProjection.y
                         - (3.0 + 4.0 * inverseRootFourteen
                            - 4.0 * inverseRootFive)) < 1e-12
            && std::fabs(cylinderProjection.z
                         - (4.0 + 6.0 * inverseRootFourteen)) < 1e-12;
        const bool obliqueCylinderAxisDistance =
            drw_pointWithinToleranceOfAcisCylinderSurface(
                obliqueCylinderAxis, obliqueCylinderPoint, 1.0,
                cylinderSignedDistance)
            && std::fabs(cylinderSignedDistance - 1.0) < 1e-12;

        t.expect(parserFedRadialProjection,
                 "parser-fed cylinder projection preserves radial position and axial offset");
        t.expect(cylinderSurfaceDistance && cylinderOutsideBoundary
                     && cylinderInsideBoundary && cylinderAxisSignedDistance,
                 "cylinder signed distance handles its locus, both sides, and the axis");
        t.expect(cylinderJustOutsideTolerance,
                 "cylinder tolerance predicate refuses just-outside input atomically");
        t.expect(obliqueCylinderProjection,
                 "cylinder projection handles oblique radial queries and non-unit axes");
        t.expect(cylinderAmbiguityAndInvalidRefusals,
                 "cylinder projection refuses axis ambiguity, unsupported cone profiles, and malformed inputs atomically");
        t.expect(overflowingDifferenceProjection
                     && unrepresentableProjectionRefused,
                 "cylinder projection handles overflowing input differences and refuses unrepresentable output atomically");
        t.expect(overflowingDifferenceDistance,
                 "cylinder signed distance preserves a representable result when point-origin subtraction overflows");
        t.expect(tinyRadiusProjectionSurvivesScaling,
                 "cylinder projection preserves a representable tiny radius alongside extreme axial coordinates");
        t.expect(tinyRadialLargeAxialProjection,
                 "cylinder projection retains a tiny radial component beside an extreme finite axial component");
        t.expect(tinyRadialLargeAxialDistance,
                 "cylinder distance retains a tiny radial component beside an extreme finite axial component");
        t.expect(tinyRadialOverflowAxialProjection,
                 "cylinder projection retains a tiny radial component when axial point-origin subtraction overflows");
        t.expect(tinyRadialOverflowAxialDistance,
                 "cylinder distance retains a tiny radial component when axial point-origin subtraction overflows");
        t.expect(tinyXAxisProjection && tinyXAxisDistance
                     && tinyYAxisProjection && tinyYAxisDistance,
                 "cylinder exact-axis fast path handles X/Y directions and negative axes with tiny radii");
        t.expect(obliqueCylinderAxisProjection
                     && obliqueCylinderAxisDistance,
                 "cylinder scaled general-axis path handles an oblique non-unit axis with analytic expectations");
    }

    AcisGraphFixture coneEvaluationFixture = complexFixture;
    DRW_SabRecord& coneEvaluationRecord =
        coneEvaluationFixture.data.records[
            static_cast<std::size_t>(unsupportedSurface)];
    coneEvaluationRecord.type = "cone-surface";
    coneEvaluationRecord.tokens = acisRecord(
        "cone-surface",
        {acisPointer(-1), acisLocation(2.0, 3.0, 4.0),
         acisDirection(0.0, 0.0, 2.0), acisDirection(0.0, 4.0, 0.0),
         acisDouble(0.5), acisDouble(0.6), acisDouble(0.8)}).tokens;
    const std::vector<std::uint8_t> coneEvaluationBytes =
        sab21800FromGraph(coneEvaluationFixture.data);
    DRW_SabData parsedConeEvaluation;
    const bool coneEvaluationParsed = !coneEvaluationBytes.empty()
        && drw_parseSab(coneEvaluationBytes.data(), coneEvaluationBytes.size(),
                        parsedConeEvaluation);
    std::vector<DRW_AcisBodyProjection> coneEvaluationProjections;
    DRW_AcisRootedProjectionReport coneEvaluationReport;
    const bool coneEvaluationProjected = drw_extractAcisBodyWireframes(
        drw_buildAcisModel(parsedConeEvaluation), coneEvaluationProjections,
        coneEvaluationReport);
    const DRW_AcisFace* projectedConeFace = nullptr;
    if (coneEvaluationProjections.size() == 1u) {
        const std::vector<DRW_AcisFace>& faces =
            coneEvaluationProjections.front().wireframe.faces;
        const auto found = std::find_if(
            faces.begin(), faces.end(), [unsupportedSurface](
                const DRW_AcisFace& face) {
                    return face.surfaceNodeIndex == unsupportedSurface;
                });
        if (found != faces.end())
            projectedConeFace = &*found;
    }
    const bool coneFieldsParsed = coneEvaluationParsed
        && coneEvaluationProjected && projectedConeFace
        && projectedConeFace->hasSurface
        && projectedConeFace->surfaceType == DRW_AcisSurface::Cone
        && projectedConeFace->ratio == 0.5
        && projectedConeFace->sineAngle == 0.6
        && projectedConeFace->cosineAngle == 0.8;
    t.expect(coneFieldsParsed,
             "SAB 21800 cone surface fields feed the analytic evaluator");
    if (projectedConeFace) {
        const double pi = std::acos(-1.0);
        DRW_Coord conePoint(91.0, 92.0, 93.0);
        const bool referenceSection = drw_evaluateAcisConeSurface(
            *projectedConeFace, 0.0, 0.0, 1e-14, conePoint)
            && acisCoordNear(conePoint, 2.0, 7.0, 4.0);
        const bool majorGenerator = drw_evaluateAcisConeSurface(
            *projectedConeFace, 0.0, 5.0, 1e-14, conePoint)
            && acisCoordNear(conePoint, 2.0, 10.0, 8.0);
        const bool minorGenerator = drw_evaluateAcisConeSurface(
            *projectedConeFace, pi / 2.0, 5.0, 1e-14, conePoint)
            && acisCoordNear(conePoint, -1.5, 3.0, 8.0);
        const bool signedGeneratorParameter = drw_evaluateAcisConeSurface(
            *projectedConeFace, 0.0, -5.0, 1e-14, conePoint)
            && acisCoordNear(conePoint, 2.0, 4.0, 0.0);

        DRW_AcisFace obliqueConeFrame = *projectedConeFace;
        obliqueConeFrame.p1 = DRW_Coord(0.0, 3.0, 3.0);
        obliqueConeFrame.p2 = DRW_Coord(2.0, 0.0, 0.0);
        const double inverseRootTwo = 1.0 / std::sqrt(2.0);
        const bool obliqueConeEvaluation = drw_evaluateAcisConeSurface(
            obliqueConeFrame, pi / 2.0, 5.0, 1e-14, conePoint)
            && std::fabs(conePoint.x - 2.0) < 1e-12
            && std::fabs(conePoint.y
                         - (3.0 + 6.5 * inverseRootTwo)) < 1e-12
            && std::fabs(conePoint.z
                         - (4.0 + 1.5 * inverseRootTwo)) < 1e-12;

        DRW_Coord reversedConeFaceNormal(91.0, 92.0, 93.0);
        const bool reversedConeFaceNormalOk =
            drw_evaluateAcisConeFaceNormal(
                *projectedConeFace, 0.0, 0.0, 1e-14,
                reversedConeFaceNormal)
            && projectedConeFace->hasSense
            && projectedConeFace->senseReversed
            && std::fabs(reversedConeFaceNormal.x) < 1e-15
            && std::fabs(reversedConeFaceNormal.y + 0.8) < 1e-15
            && std::fabs(reversedConeFaceNormal.z - 0.6) < 1e-15;
        DRW_AcisFace forwardConeFace = *projectedConeFace;
        forwardConeFace.senseReversed = false;
        DRW_Coord forwardConeFaceNormal(91.0, 92.0, 93.0);
        const bool forwardConeFaceNormalOk =
            drw_evaluateAcisConeFaceNormal(
                forwardConeFace, 0.0, 0.0, 1e-14,
                forwardConeFaceNormal)
            && std::fabs(forwardConeFaceNormal.x) < 1e-15
            && std::fabs(forwardConeFaceNormal.y - 0.8) < 1e-15
            && std::fabs(forwardConeFaceNormal.z + 0.6) < 1e-15;
        DRW_Coord unchangedConeFaceNormal(91.0, 92.0, 93.0);
        DRW_AcisFace unknownConeSense = forwardConeFace;
        unknownConeSense.hasSense = false;
        const bool coneNormalRefusals =
            !drw_evaluateAcisConeFaceNormal(
                *projectedConeFace, 0.0, -4.0 / 0.6, 1e-14,
                unchangedConeFaceNormal)
            && !drw_evaluateAcisConeFaceNormal(
                unknownConeSense, 0.0, 0.0, 1e-14,
                unchangedConeFaceNormal)
            && !drw_evaluateAcisConeFaceNormal(
                *projectedConeFace,
                std::numeric_limits<double>::quiet_NaN(), 0.0, 1e-14,
                unchangedConeFaceNormal)
            && unchangedConeFaceNormal.x == 91.0
            && unchangedConeFaceNormal.y == 92.0
            && unchangedConeFaceNormal.z == 93.0;

        DRW_AcisFace cylinderProfile = *projectedConeFace;
        cylinderProfile.ratio = 1.0;
        cylinderProfile.sineAngle = 0.0;
        cylinderProfile.cosineAngle = 1.0;
        const bool cylinderSpecialCase = drw_evaluateAcisConeSurface(
            cylinderProfile, 0.0, 3.0, 0.0, conePoint)
            && acisCoordNear(conePoint, 2.0, 7.0, 7.0);

        const auto refusesConeAtomically = [](
            const DRW_AcisFace& face, double u, double v, double tolerance) {
            DRW_Coord output(91.0, 92.0, 93.0);
            return !drw_evaluateAcisConeSurface(
                       face, u, v, tolerance, output)
                && output.x == 91.0 && output.y == 92.0
                && output.z == 93.0;
        };
        DRW_AcisFace zeroConeRatio = *projectedConeFace;
        zeroConeRatio.ratio = 0.0;
        DRW_AcisFace nonOrthogonalConeFrame = *projectedConeFace;
        nonOrthogonalConeFrame.p2 = DRW_Coord(0.0, 1.0, 1.0);
        DRW_AcisFace invalidConeAngle = *projectedConeFace;
        invalidConeAngle.cosineAngle = 0.5;
        DRW_AcisFace missingConeSurface = *projectedConeFace;
        missingConeSurface.hasSurface = false;
        const bool invalidConeRefusals =
            refusesConeAtomically(zeroConeRatio, 0.0, 1.0, 1e-14)
            && refusesConeAtomically(
                nonOrthogonalConeFrame, 0.0, 1.0, 1e-6)
            && refusesConeAtomically(invalidConeAngle, 0.0, 1.0, 1e-14)
            && refusesConeAtomically(missingConeSurface, 0.0, 1.0, 1e-14)
            && refusesConeAtomically(*projectedConeFace,
                                     std::numeric_limits<double>::quiet_NaN(),
                                     1.0, 1e-14)
            && refusesConeAtomically(*projectedConeFace, 0.0,
                                     std::numeric_limits<double>::infinity(),
                                     1e-14)
            && refusesConeAtomically(*projectedConeFace, 0.0, 1.0, -1.0);

        const double maximumConeCoordinate =
            std::numeric_limits<double>::max();
        DRW_AcisFace cancellingCone = cylinderProfile;
        cancellingCone.p0 = DRW_Coord(-maximumConeCoordinate, 0.0, 0.0);
        cancellingCone.p1 = DRW_Coord(1.0, 0.0, 0.0);
        cancellingCone.p2 = DRW_Coord(0.0, 1.0, 0.0);
        const bool scaledCancellation = drw_evaluateAcisConeSurface(
            cancellingCone, 0.0, maximumConeCoordinate, 0.0, conePoint)
            && conePoint.x == 0.0 && conePoint.y == 1.0
            && conePoint.z == 0.0;
        DRW_AcisFace overflowingCone = cancellingCone;
        overflowingCone.p0.x = maximumConeCoordinate;
        DRW_Coord unchangedConePoint(91.0, 92.0, 93.0);
        const bool unrepresentableConeRefusal =
            !drw_evaluateAcisConeSurface(
                overflowingCone, 0.0, maximumConeCoordinate, 0.0,
                unchangedConePoint)
            && unchangedConePoint.x == 91.0
            && unchangedConePoint.y == 92.0
            && unchangedConePoint.z == 93.0;

        t.expect(referenceSection && majorGenerator && minorGenerator
                     && signedGeneratorParameter && obliqueConeEvaluation,
                 "cone evaluation covers both ellipse axes, oblique frames, and signed generator parameters");
        t.expect(reversedConeFaceNormalOk && forwardConeFaceNormalOk,
                 "parser-fed cone differential normal follows local parameter order and retained face sense");
        t.expect(coneNormalRefusals,
                 "cone face normal refuses the apex, unknown sense, and non-finite parameter atomically");
        t.expect(cylinderSpecialCase,
                 "cone evaluation retains the circular-cylinder limiting profile");
        t.expect(invalidConeRefusals,
                 "cone evaluation refuses invalid ratios, frames, angles, and parameters atomically");
        t.expect(scaledCancellation && unrepresentableConeRefusal,
                 "cone evaluation preserves representable cancellation and refuses overflow atomically");
    }

    AcisGraphFixture circularConeProjectionFixture = coneEvaluationFixture;
    DRW_SabRecord& circularConeProjectionRecord =
        circularConeProjectionFixture.data.records[
            static_cast<std::size_t>(unsupportedSurface)];
    circularConeProjectionRecord.tokens = acisRecord(
        "cone-surface",
        {acisPointer(-1), acisLocation(2.0, 3.0, 4.0),
         acisDirection(0.0, 0.0, 2.0), acisDirection(0.0, 4.0, 0.0),
         acisDouble(1.0), acisDouble(0.6), acisDouble(0.8)}).tokens;
    const std::vector<std::uint8_t> circularConeProjectionBytes =
        sab21800FromGraph(circularConeProjectionFixture.data);
    DRW_SabData parsedCircularConeProjection;
    const bool circularConeProjectionParsed =
        !circularConeProjectionBytes.empty()
        && drw_parseSab(circularConeProjectionBytes.data(),
                        circularConeProjectionBytes.size(),
                        parsedCircularConeProjection);
    std::vector<DRW_AcisBodyProjection> circularConeProjections;
    DRW_AcisRootedProjectionReport circularConeReport;
    const bool circularConeProjectionExtracted = drw_extractAcisBodyWireframes(
        drw_buildAcisModel(parsedCircularConeProjection),
        circularConeProjections, circularConeReport);
    const DRW_AcisFace* projectedCircularConeFace = nullptr;
    if (circularConeProjections.size() == 1u) {
        const std::vector<DRW_AcisFace>& faces =
            circularConeProjections.front().wireframe.faces;
        const auto found = std::find_if(
            faces.begin(), faces.end(), [unsupportedSurface](
                const DRW_AcisFace& face) {
                    return face.surfaceNodeIndex == unsupportedSurface;
                });
        if (found != faces.end())
            projectedCircularConeFace = &*found;
    }
    const bool circularConeParserFed = circularConeProjectionParsed
        && circularConeProjectionExtracted && projectedCircularConeFace
        && projectedCircularConeFace->surfaceType == DRW_AcisSurface::Cone
        && projectedCircularConeFace->ratio == 1.0
        && projectedCircularConeFace->sineAngle == 0.6
        && projectedCircularConeFace->cosineAngle == 0.8;
    t.expect(circularConeParserFed,
             "SAB 21800 circular cone fields feed closest-point projection");
    if (projectedCircularConeFace) {
        DRW_Coord coneProjection(91.0, 92.0, 93.0);
        const bool exactConeLocusProjection =
            drw_projectPointToAcisConeSurface(
                *projectedCircularConeFace, DRW_Coord(2.0, 10.0, 8.0),
                1e-14, coneProjection)
            && acisCoordNear(coneProjection, 2.0, 10.0, 8.0);
        const bool analyticConeProjection =
            drw_projectPointToAcisConeSurface(
                *projectedCircularConeFace, DRW_Coord(2.0, 13.0, 8.0),
                1e-14, coneProjection)
            && std::fabs(coneProjection.x - 2.0) < 1e-12
            && std::fabs(coneProjection.y - 11.08) < 1e-12
            && std::fabs(coneProjection.z - 9.44) < 1e-12;
        const bool negativeNappeProjection =
            drw_projectPointToAcisConeSurface(
                *projectedCircularConeFace, DRW_Coord(2.0, 2.2, -2.4),
                1e-14, coneProjection)
            && acisCoordNear(coneProjection, 2.0, 2.2, -2.4);
        const double coneApexAxial = -4.0 * 0.8 / 0.6;
        const DRW_Coord coneApexQuery(2.0, 3.0, 4.0 + coneApexAxial);
        const bool coneApexProjectionResult = drw_projectPointToAcisConeSurface(
            *projectedCircularConeFace, coneApexQuery, 1e-14, coneProjection);
        const bool uniqueApexProjection = coneApexProjectionResult
            && std::fabs(coneProjection.x - coneApexQuery.x) < 1e-12
            && std::fabs(coneProjection.y - coneApexQuery.y) < 1e-12
            && std::fabs(coneProjection.z - coneApexQuery.z) < 1e-12;
        const bool coneLocusDistance =
            drw_pointWithinToleranceOfAcisConeSurface(
                *projectedCircularConeFace, DRW_Coord(2.0, 10.0, 8.0),
                1e-14, 0.0)
            && drw_pointWithinToleranceOfAcisConeSurface(
                *projectedCircularConeFace, DRW_Coord(2.0, 13.0, 8.0),
                1e-14, 2.4000000001)
            && !drw_pointWithinToleranceOfAcisConeSurface(
                *projectedCircularConeFace, DRW_Coord(2.0, 13.0, 8.0),
                1e-14, 2.399);
        coneProjection = DRW_Coord(2.0, 11.08, 9.44);
        const bool axialProjectionRefused =
            !drw_projectPointToAcisConeSurface(
                *projectedCircularConeFace, DRW_Coord(2.0, 3.0, 4.0),
                1e-14, coneProjection);
        const bool axialProjectionOutputUnchanged =
            std::fabs(coneProjection.x - 2.0) < 1e-12
            && std::fabs(coneProjection.y - 11.08) < 1e-12
            && std::fabs(coneProjection.z - 9.44) < 1e-12;
        const bool axialDistanceWithinTolerance =
            drw_pointWithinToleranceOfAcisConeSurface(
                *projectedCircularConeFace, DRW_Coord(2.0, 3.0, 4.0),
                1e-14, 3.2000001);
        const bool axialDistanceOutsideTolerance =
            !drw_pointWithinToleranceOfAcisConeSurface(
                *projectedCircularConeFace, DRW_Coord(2.0, 3.0, 4.0),
                1e-14, 3.199);

        DRW_AcisFace obliqueConeProjection = *projectedCircularConeFace;
        obliqueConeProjection.p1 = DRW_Coord(0.0, 3.0, 3.0);
        obliqueConeProjection.p2 = DRW_Coord(2.0, 0.0, 0.0);
        const double inverseRootTwo = 1.0 / std::sqrt(2.0);
        const DRW_Coord obliqueConeExpected(
            2.0, 3.0 + 9.0 * inverseRootTwo,
            4.0 - inverseRootTwo);
        const bool obliqueConeProjectionOk =
            drw_projectPointToAcisConeSurface(
                obliqueConeProjection, obliqueConeExpected, 1e-14,
                coneProjection)
            && std::fabs(coneProjection.x - obliqueConeExpected.x) < 1e-12
            && std::fabs(coneProjection.y - obliqueConeExpected.y) < 1e-12
            && std::fabs(coneProjection.z - obliqueConeExpected.z) < 1e-12;

        DRW_AcisFace cylinderLimit = *projectedCircularConeFace;
        cylinderLimit.sineAngle = 0.0;
        cylinderLimit.cosineAngle = 1.0;
        const bool cylinderProjectionAndBoundary =
            drw_projectPointToAcisConeSurface(
                cylinderLimit, DRW_Coord(10.0, 3.0, 9.0), 0.0,
                coneProjection)
            && acisCoordNear(coneProjection, 6.0, 3.0, 9.0)
            && drw_pointWithinToleranceOfAcisConeSurface(
                cylinderLimit, DRW_Coord(8.0, 3.0, 9.0), 0.0, 2.0)
            && !drw_pointWithinToleranceOfAcisConeSurface(
                cylinderLimit, DRW_Coord(8.0, 3.0, 9.0), 0.0,
                std::nextafter(2.0, 0.0));

        const auto refusesConeProjectionAtomically = [](
            const DRW_AcisFace& face, const DRW_Coord& query,
            double frameTolerance) {
            DRW_Coord output(91.0, 92.0, 93.0);
            return !drw_projectPointToAcisConeSurface(
                       face, query, frameTolerance, output)
                && output.x == 91.0 && output.y == 92.0
                && output.z == 93.0;
        };
        DRW_AcisFace noncircularCone = *projectedCircularConeFace;
        noncircularCone.ratio = 0.0;
        DRW_AcisFace wrongConeType = *projectedCircularConeFace;
        wrongConeType.surfaceType = DRW_AcisSurface::Sphere;
        DRW_AcisFace missingConeProjectionSurface =
            *projectedCircularConeFace;
        missingConeProjectionSurface.hasSurface = false;
        const bool unsupportedConeRefusals =
            refusesConeProjectionAtomically(
                noncircularCone, DRW_Coord(2.0, 10.0, 8.0), 1e-14)
            && refusesConeProjectionAtomically(
                wrongConeType, DRW_Coord(2.0, 10.0, 8.0), 1e-14)
            && refusesConeProjectionAtomically(
                missingConeProjectionSurface,
                DRW_Coord(2.0, 10.0, 8.0), 1e-14)
            && refusesConeProjectionAtomically(
                *projectedCircularConeFace,
                DRW_Coord(std::numeric_limits<double>::infinity(), 0.0, 0.0),
                1e-14)
            && refusesConeProjectionAtomically(
                *projectedCircularConeFace,
                DRW_Coord(2.0, 10.0, 8.0), -1.0)
            && !drw_pointWithinToleranceOfAcisConeSurface(
                *projectedCircularConeFace, DRW_Coord(2.0, 10.0, 8.0),
                1e-14, -1.0)
            && !drw_pointWithinToleranceOfAcisConeSurface(
                *projectedCircularConeFace, DRW_Coord(2.0, 10.0, 8.0),
                std::numeric_limits<double>::quiet_NaN(), 1.0);

        const double maximumConeProjectionCoordinate =
            std::numeric_limits<double>::max();
        DRW_AcisFace overflowingConeProjection =
            *projectedCircularConeFace;
        overflowingConeProjection.p0 = DRW_Coord(
            maximumConeProjectionCoordinate, 0.0, 0.0);
        overflowingConeProjection.p1 = DRW_Coord(1.0, 0.0, 0.0);
        overflowingConeProjection.p2 = DRW_Coord(
            0.0, 0.1 * maximumConeProjectionCoordinate, 0.0);
        DRW_Coord unchangedConeProjection(91.0, 92.0, 93.0);
        const bool unrepresentableConeProjectionRefused =
            !drw_projectPointToAcisConeSurface(
                overflowingConeProjection,
                DRW_Coord(maximumConeProjectionCoordinate,
                          0.9 * maximumConeProjectionCoordinate, 0.0),
                0.0, unchangedConeProjection)
            && unchangedConeProjection.x == 91.0
            && unchangedConeProjection.y == 92.0
            && unchangedConeProjection.z == 93.0;

        t.expect(exactConeLocusProjection && analyticConeProjection,
                 "circular-cone projection handles exact and off-locus queries");
        t.expect(negativeNappeProjection,
                 "circular-cone projection handles the opposite nappe");
        t.expect(uniqueApexProjection,
                 "circular-cone projection recognizes the unique cone apex");
        t.expect(coneApexProjectionResult,
                 "circular-cone apex closest point is publishable");
        t.expect(coneLocusDistance,
                 "circular-cone distance accepts locus and tolerance-boundary queries");
        t.expect(axialProjectionRefused && axialProjectionOutputUnchanged,
                 "circular-cone projection refuses non-unique axial queries atomically");
        t.expect(axialDistanceWithinTolerance && axialDistanceOutsideTolerance,
                 "circular-cone distance accepts non-unique axial queries by their Euclidean distance");
        t.expect(obliqueConeProjectionOk && cylinderProjectionAndBoundary,
                 "circular-cone projection handles oblique axes and the cylinder limit");
        t.expect(unsupportedConeRefusals
                     && unrepresentableConeProjectionRefused,
                 "circular-cone projection refuses ambiguity, invalid profiles, and overflow atomically");
    }

    AcisGraphFixture ellipticConeFixture = circularConeProjectionFixture;
    ellipticConeFixture.data.records[
        static_cast<std::size_t>(unsupportedSurface)] = acisRecord(
            "cone-surface",
            {acisPointer(-1), acisLocation(2.0, 3.0, 4.0),
             acisDirection(0.0, 0.0, 2.0), acisDirection(0.0, 4.0, 0.0),
             acisDouble(0.5), acisDouble(0.6), acisDouble(0.8)});
    const std::vector<std::uint8_t> ellipticConeBytes =
        sab21800FromGraph(ellipticConeFixture.data);
    DRW_SabData parsedEllipticCone;
    const bool ellipticConeParsed = !ellipticConeBytes.empty()
        && drw_parseSab(ellipticConeBytes.data(), ellipticConeBytes.size(),
                        parsedEllipticCone);
    std::vector<DRW_AcisBodyProjection> ellipticConeProjections;
    DRW_AcisRootedProjectionReport ellipticConeReport;
    const bool ellipticConeExtracted = drw_extractAcisBodyWireframes(
        drw_buildAcisModel(parsedEllipticCone), ellipticConeProjections,
        ellipticConeReport);
    const DRW_AcisFace* projectedEllipticConeFace = nullptr;
    if (ellipticConeProjections.size() == 1u) {
        const std::vector<DRW_AcisFace>& faces =
            ellipticConeProjections.front().wireframe.faces;
        const auto found = std::find_if(faces.begin(), faces.end(),
            [unsupportedSurface](const DRW_AcisFace& face) {
                return face.surfaceNodeIndex == unsupportedSurface;
            });
        if (found != faces.end())
            projectedEllipticConeFace = &*found;
    }
    const bool ellipticConeParserFed = ellipticConeParsed
        && ellipticConeExtracted && projectedEllipticConeFace
        && projectedEllipticConeFace->surfaceType == DRW_AcisSurface::Cone
        && projectedEllipticConeFace->ratio == 0.5;
    t.expect(ellipticConeParserFed,
             "SAB 21800 elliptic cone ratio feeds closest-point projection");
    if (projectedEllipticConeFace) {
        const double ellipticPi = std::acos(-1.0);
        const double ellipticNormalLength = std::sqrt(0.73);
        DRW_Coord ellipticConeNormal(91.0, 92.0, 93.0);
        const bool ellipticConeNormalOk =
            drw_evaluateAcisConeFaceNormal(
                *projectedEllipticConeFace, ellipticPi / 2.0, 5.0,
                1e-14, ellipticConeNormal)
            && projectedEllipticConeFace->hasSense
            && projectedEllipticConeFace->senseReversed
            && std::fabs(ellipticConeNormal.x
                         - 0.8 / ellipticNormalLength) < 1e-14
            && std::fabs(ellipticConeNormal.y) < 1e-14
            && std::fabs(ellipticConeNormal.z
                         - 0.3 / ellipticNormalLength) < 1e-14;
        DRW_Coord negativeNappeConeNormal(91.0, 92.0, 93.0);
        const bool negativeNappeConeNormalOk =
            drw_evaluateAcisConeFaceNormal(
                *projectedEllipticConeFace, 0.0, -10.0, 1e-14,
                negativeNappeConeNormal)
            && std::fabs(negativeNappeConeNormal.x) < 1e-14
            && std::fabs(negativeNappeConeNormal.y - 0.8) < 1e-14
            && std::fabs(negativeNappeConeNormal.z + 0.6) < 1e-14;
        DRW_Coord unchangedEllipticConeNormal(91.0, 92.0, 93.0);
        const bool ellipticApexNormalRefused =
            !drw_evaluateAcisConeFaceNormal(
                *projectedEllipticConeFace, 0.0, -4.0 / 0.6, 1e-14,
                unchangedEllipticConeNormal)
            && unchangedEllipticConeNormal.x == 91.0
            && unchangedEllipticConeNormal.y == 92.0
            && unchangedEllipticConeNormal.z == 93.0;
        t.expect(ellipticConeNormalOk && negativeNappeConeNormalOk,
                 "elliptic-cone differential normals cover ratio scaling, reversed faces, and the negative nappe");
        t.expect(ellipticApexNormalRefused,
                 "elliptic-cone face normal refuses its singular apex atomically");

        DRW_Coord ellipticProjection(91.0, 92.0, 93.0);
        const DRW_Coord exactEllipticPoint(2.0, 8.2, 5.6);
        const bool exactEllipticLocusProjection =
            drw_projectPointToAcisConeSurface(
                *projectedEllipticConeFace, exactEllipticPoint, 1e-14,
                ellipticProjection)
            && std::fabs(ellipticProjection.x - exactEllipticPoint.x) < 1e-11
            && std::fabs(ellipticProjection.y - exactEllipticPoint.y) < 1e-11
            && std::fabs(ellipticProjection.z - exactEllipticPoint.z) < 1e-11;

        const DRW_Coord normalOffsetQuery(2.0, 9.8, 4.4);
        const bool ellipticNormalProjection =
            drw_projectPointToAcisConeSurface(
                *projectedEllipticConeFace, normalOffsetQuery, 1e-14,
                ellipticProjection)
            && std::fabs(ellipticProjection.x - exactEllipticPoint.x) < 1e-10
            && std::fabs(ellipticProjection.y - exactEllipticPoint.y) < 1e-10
            && std::fabs(ellipticProjection.z - exactEllipticPoint.z) < 1e-10
            && drw_pointWithinToleranceOfAcisConeSurface(
                *projectedEllipticConeFace, normalOffsetQuery, 1e-14,
                2.00000001)
            && !drw_pointWithinToleranceOfAcisConeSurface(
                *projectedEllipticConeFace, normalOffsetQuery, 1e-14,
                1.99999999);

        DRW_Coord ellipticApex;
        const bool ellipticApexEvaluated = drw_evaluateAcisConeSurface(
            *projectedEllipticConeFace, 0.0, -4.0 / 0.6, 1e-14,
            ellipticApex);
        DRW_Coord ellipticApexProjection(91.0, 92.0, 93.0);
        const bool ellipticApexProjectionOk = ellipticApexEvaluated
            && drw_projectPointToAcisConeSurface(
                *projectedEllipticConeFace, ellipticApex, 1e-14,
                ellipticApexProjection)
            && std::hypot(
                   std::hypot(ellipticApexProjection.x - ellipticApex.x,
                              ellipticApexProjection.y - ellipticApex.y),
                   ellipticApexProjection.z - ellipticApex.z) < 1e-10;

        const DRW_Coord arbitraryQuery(7.3, -9.2, 15.7);
        const bool arbitraryProjection = drw_projectPointToAcisConeSurface(
            *projectedEllipticConeFace, arbitraryQuery, 1e-14,
            ellipticProjection);
        double projectedMajor = ellipticProjection.y - 3.0;
        double projectedMinor = -(ellipticProjection.x - 2.0);
        double projectedAxial = ellipticProjection.z - 4.0;
        const double projectedGeneratorParameter = projectedAxial / 0.8;
        const double projectedRadialScale =
            4.0 + projectedGeneratorParameter * 0.6;
        const double projectedEllipseEquation =
            projectedMajor * projectedMajor
                / (projectedRadialScale * projectedRadialScale)
            + projectedMinor * projectedMinor
                / (0.25 * projectedRadialScale * projectedRadialScale);

        const double coneSlope = 0.8 / 0.6;
        const double queryMajor = arbitraryQuery.y - 3.0;
        const double queryMinor = -(arbitraryQuery.x - 2.0);
        const double queryAxial = arbitraryQuery.z - 4.0 + 4.0 * coneSlope;
        double sampledMinimum = std::numeric_limits<double>::infinity();
        for (unsigned int sample = 0u; sample < 8192u; ++sample) {
            const double angle = 6.28318530717958647692
                * static_cast<double>(sample) / 8192.0;
            const double directionMajor = std::cos(angle);
            const double directionMinor = 0.5 * std::sin(angle);
            const double directionAxial = coneSlope;
            const double denominator = directionMajor * directionMajor
                + directionMinor * directionMinor
                + directionAxial * directionAxial;
            const double numerator = queryMajor * directionMajor
                + queryMinor * directionMinor + queryAxial * directionAxial;
            const double parameter = numerator / denominator;
            const double residualMajor = queryMajor
                - parameter * directionMajor;
            const double residualMinor = queryMinor
                - parameter * directionMinor;
            const double residualAxial = queryAxial
                - parameter * directionAxial;
            sampledMinimum = std::min(
                sampledMinimum,
                std::hypot(std::hypot(residualMajor, residualMinor),
                           residualAxial));
        }
        const double arbitraryProjectionDistance = std::hypot(
            std::hypot(arbitraryQuery.x - ellipticProjection.x,
                       arbitraryQuery.y - ellipticProjection.y),
            arbitraryQuery.z - ellipticProjection.z);
        const bool ellipticGlobalMinimumControl = arbitraryProjection
            && std::fabs(projectedEllipseEquation - 1.0) < 1e-9
            && arbitraryProjectionDistance <= sampledMinimum + 1e-9
            && sampledMinimum - arbitraryProjectionDistance < 1e-3;

        DRW_Coord unchangedAmbiguousEllipticProjection(91.0, 92.0, 93.0);
        const bool ambiguousEllipticProjectionRefused =
            !drw_projectPointToAcisConeSurface(
                *projectedEllipticConeFace, DRW_Coord(2.0, 3.0, 6.0),
                1e-14, unchangedAmbiguousEllipticProjection)
            && unchangedAmbiguousEllipticProjection.x == 91.0
            && unchangedAmbiguousEllipticProjection.y == 92.0
            && unchangedAmbiguousEllipticProjection.z == 93.0
            && drw_pointWithinToleranceOfAcisConeSurface(
                *projectedEllipticConeFace, DRW_Coord(2.0, 3.0, 6.0),
                1e-14, 20.0);

        DRW_AcisFace unsupportedEllipticCylinder =
            *projectedEllipticConeFace;
        unsupportedEllipticCylinder.sineAngle = 0.0;
        unsupportedEllipticCylinder.cosineAngle = 1.0;
        const bool ellipticCylinderProjectionOk =
            drw_projectPointToAcisConeSurface(
                unsupportedEllipticCylinder, DRW_Coord(2.0, 12.0, 9.0),
                1e-14, ellipticProjection)
            && std::fabs(ellipticProjection.x - 2.0) < 1e-10
            && std::fabs(ellipticProjection.y - 7.0) < 1e-10
            && std::fabs(ellipticProjection.z - 9.0) < 1e-10
            && drw_pointWithinToleranceOfAcisConeSurface(
                unsupportedEllipticCylinder, DRW_Coord(2.0, 12.0, 9.0),
                1e-14, 5.00000001)
            && !drw_pointWithinToleranceOfAcisConeSurface(
                unsupportedEllipticCylinder, DRW_Coord(2.0, 12.0, 9.0),
                1e-14, 4.99999999);
        const bool ellipticCylinderAmbiguityRefused =
            !drw_projectPointToAcisConeSurface(
                unsupportedEllipticCylinder, DRW_Coord(2.0, 3.0, 9.0),
                1e-14, unchangedAmbiguousEllipticProjection)
            && unchangedAmbiguousEllipticProjection.x == 91.0
            && unchangedAmbiguousEllipticProjection.y == 92.0
            && unchangedAmbiguousEllipticProjection.z == 93.0
            && drw_pointWithinToleranceOfAcisConeSurface(
                unsupportedEllipticCylinder, DRW_Coord(2.0, 3.0, 9.0),
                1e-14, 2.00000001);

        DRW_AcisFace unresolvableRatioCone = *projectedEllipticConeFace;
        unresolvableRatioCone.ratio = 1e-200;
        const bool unresolvableRatioRefusedAtomically =
            !drw_projectPointToAcisConeSurface(
                unresolvableRatioCone, arbitraryQuery, 1e-14,
                unchangedAmbiguousEllipticProjection)
            && unchangedAmbiguousEllipticProjection.x == 91.0
            && unchangedAmbiguousEllipticProjection.y == 92.0
            && unchangedAmbiguousEllipticProjection.z == 93.0;

        const double testRatios[] = {0.2, 0.5, 1.75, 3.0};
        const DRW_Coord testQueries[] = {
            DRW_Coord(-8.0, 4.0, 2.0), DRW_Coord(7.3, -9.2, 15.7),
            DRW_Coord(3.1, 0.4, -5.0), DRW_Coord(8.3, 5.9, 12.2)};
        bool ellipticConeRatioMatrix = true;
        for (double ratio : testRatios) {
            DRW_AcisFace ratioFace = *projectedEllipticConeFace;
            ratioFace.ratio = ratio;
            for (const DRW_Coord& query : testQueries) {
                DRW_Coord candidateProjection;
                if (!drw_projectPointToAcisConeSurface(
                        ratioFace, query, 1e-14, candidateProjection)) {
                    ellipticConeRatioMatrix = false;
                    continue;
                }

                const double queryMajor = query.y - 3.0;
                const double queryMinor = -(query.x - 2.0);
                const double queryAxial = query.z - 4.0
                    + 4.0 * coneSlope;
                double sampledMinimum = std::numeric_limits<double>::infinity();
                for (unsigned int sample = 0u; sample < 2048u; ++sample) {
                    const double angle = 6.28318530717958647692
                        * static_cast<double>(sample) / 2048.0;
                    const double directionMajor = std::cos(angle);
                    const double directionMinor = ratio * std::sin(angle);
                    const double denominator = directionMajor * directionMajor
                        + directionMinor * directionMinor
                        + coneSlope * coneSlope;
                    const double numerator = queryMajor * directionMajor
                        + queryMinor * directionMinor
                        + queryAxial * coneSlope;
                    const double parameter = numerator / denominator;
                    const double residualMajor = queryMajor
                        - parameter * directionMajor;
                    const double residualMinor = queryMinor
                        - parameter * directionMinor;
                    const double residualAxial = queryAxial
                        - parameter * coneSlope;
                    sampledMinimum = std::min(
                        sampledMinimum,
                        std::hypot(std::hypot(residualMajor, residualMinor),
                                   residualAxial));
                }
                const double resultDistance = std::hypot(
                    std::hypot(query.x - candidateProjection.x,
                               query.y - candidateProjection.y),
                    query.z - candidateProjection.z);
                const double projectedMajor = candidateProjection.y - 3.0;
                const double projectedMinor = -(candidateProjection.x - 2.0);
                const double projectedAxial = candidateProjection.z - 4.0;
                const double generator = projectedAxial / 0.8;
                const double radial = 4.0 + generator * 0.6;
                const double surfaceEquation =
                    projectedMajor * projectedMajor / (radial * radial)
                    + projectedMinor * projectedMinor
                        / (ratio * ratio * radial * radial);
                ellipticConeRatioMatrix = ellipticConeRatioMatrix
                    && std::isfinite(resultDistance)
                    && std::fabs(surfaceEquation - 1.0) < 1e-8
                    && resultDistance <= sampledMinimum + 1e-8
                    && sampledMinimum - resultDistance < 2e-3;
            }
        }

        DRW_AcisFace obliqueEllipticCone = *projectedEllipticConeFace;
        obliqueEllipticCone.p1 = DRW_Coord(0.0, 3.0, 3.0);
        obliqueEllipticCone.p2 = DRW_Coord(2.0, 0.0, 0.0);
        DRW_Coord obliqueEllipticPoint;
        const bool obliqueEllipticPointEvaluated =
            drw_evaluateAcisConeSurface(
                obliqueEllipticCone, 0.37, -1.2, 1e-14,
                obliqueEllipticPoint);
        DRW_Coord obliqueEllipticProjection(91.0, 92.0, 93.0);
        const bool obliqueEllipticProjectionOk =
            obliqueEllipticPointEvaluated
            && drw_projectPointToAcisConeSurface(
                obliqueEllipticCone, obliqueEllipticPoint, 1e-14,
                obliqueEllipticProjection)
            && std::hypot(
                   std::hypot(obliqueEllipticProjection.x
                                  - obliqueEllipticPoint.x,
                              obliqueEllipticProjection.y
                                  - obliqueEllipticPoint.y),
                   obliqueEllipticProjection.z
                       - obliqueEllipticPoint.z) < 1e-9;

        t.expect(exactEllipticLocusProjection && ellipticNormalProjection,
                 "elliptic-cone projection handles exact and normal-offset points");
        t.expect(ellipticApexProjectionOk,
                 "elliptic-cone projection returns its unique apex");
        t.expect(ellipticGlobalMinimumControl,
                 "elliptic-cone projection matches an independent angular minimum scan");
        t.expect(ambiguousEllipticProjectionRefused,
                 "elliptic-cone projection refuses symmetric non-unique points but retains distance");
        t.expect(ellipticCylinderProjectionOk
                     && ellipticCylinderAmbiguityRefused,
                 "elliptic-cylinder limit projects as an ellipse with atomic ambiguity refusal");
        t.expect(unresolvableRatioRefusedAtomically,
                 "elliptic-cone projection refuses underflowed directions atomically");
        t.expect(ellipticConeRatioMatrix,
                 "elliptic-cone projections across ratios match independent angular scans");
        t.expect(obliqueEllipticProjectionOk,
                 "elliptic-cone projection handles an oblique non-unit frame");
    }

    AcisGraphFixture straightCurveFixture = complexFixture;
    straightCurveFixture.data.records[
        static_cast<std::size_t>(unsupportedCurve)] = acisRecord(
            "straight-curve",
            {acisPointer(-1), acisLocation(3.0, 2.0, 3.0),
             acisDirection(4.0, 3.0, 3.0)});
    const std::vector<std::uint8_t> straightCurveBytes =
        sab21800FromGraph(straightCurveFixture.data);
    DRW_SabData parsedStraightCurve;
    const bool straightCurveParsed = !straightCurveBytes.empty()
        && drw_parseSab(straightCurveBytes.data(), straightCurveBytes.size(),
                        parsedStraightCurve);
    std::vector<DRW_AcisBodyProjection> straightCurveProjections;
    DRW_AcisRootedProjectionReport straightCurveReport;
    const bool straightCurveProjected = drw_extractAcisBodyWireframes(
        drw_buildAcisModel(parsedStraightCurve), straightCurveProjections,
        straightCurveReport);
    const DRW_AcisEdge* projectedStraightCurve = nullptr;
    if (straightCurveProjections.size() == 1u) {
        const std::vector<DRW_AcisEdge>& edges =
            straightCurveProjections.front().wireframe.edges;
        const auto found = std::find_if(edges.begin(), edges.end(),
            [secondEdge](const DRW_AcisEdge& edge) {
                return edge.nodeIndex == secondEdge;
            });
        if (found != edges.end())
            projectedStraightCurve = &*found;
    }
    DRW_Coord evaluatedProjectedStraight;
    double projectedStartParameter = -1.0;
    double projectedEndParameter = -1.0;
    double withinStartParameter = -2.0;
    double withinEndParameter = -3.0;
    DRW_Coord projectedStartPoint;
    DRW_Coord projectedEndPoint;
    DRW_Coord evaluatedFiniteEdgeMidpoint;
    const bool projectedStartOk = projectedStraightCurve
        && drw_projectPointToAcisStraightCurve(
            *projectedStraightCurve, projectedStraightCurve->start,
            projectedStartParameter, projectedStartPoint);
    const bool projectedEndOk = projectedStraightCurve
        && drw_projectPointToAcisStraightCurve(
            *projectedStraightCurve, projectedStraightCurve->end,
            projectedEndParameter, projectedEndPoint);
    const bool withinStartOk = projectedStraightCurve
        && drw_pointWithinToleranceOfAcisStraightCurve(
            *projectedStraightCurve, projectedStraightCurve->start, 0.0,
            withinStartParameter);
    const bool withinEndOk = projectedStraightCurve
        && drw_pointWithinToleranceOfAcisStraightCurve(
            *projectedStraightCurve, projectedStraightCurve->end, 0.0,
            withinEndParameter);
    double finiteStartParameter = -4.0;
    double finiteEndParameter = -5.0;
    DRW_Coord finiteStartProjection;
    DRW_Coord finiteEndProjection;
    const bool finiteStartProjectionOk = projectedStraightCurve
        && drw_projectPointToAcisStraightEdge(
            *projectedStraightCurve, DRW_Coord(-1.0, 2.0, 3.0), 0.0,
            finiteStartParameter, finiteStartProjection);
    const bool finiteEndProjectionOk = projectedStraightCurve
        && drw_projectPointToAcisStraightEdge(
            *projectedStraightCurve, DRW_Coord(15.0, 11.0, 12.0), 0.0,
            finiteEndParameter, finiteEndProjection);
    double finiteEndToleranceParameter = -6.0;
    const bool finiteEndToleranceOk = projectedStraightCurve
        && drw_pointWithinToleranceOfAcisStraightEdge(
            *projectedStraightCurve, DRW_Coord(7.0, 5.5, 6.0),
            0.0, 0.5, finiteEndToleranceParameter);
    const bool parserFedFiniteEdgeEvaluation = projectedStraightCurve
        && drw_evaluateAcisStraightEdge(
            *projectedStraightCurve, 0.5, 0.0,
            evaluatedFiniteEdgeMidpoint);
    t.expect(straightCurveParsed && straightCurveProjected
                 && straightCurveReport.failure
                        == DRW_AcisProjectionFailure::NoFailure
                 && projectedStraightCurve
                 && projectedStraightCurve->curveNodeIndex == unsupportedCurve
                 && projectedStraightCurve->hasCurve
                 && projectedStraightCurve->curveType
                        == DRW_AcisCurve::Straight
                 && acisCoordNear(projectedStraightCurve->p0,
                                 3.0, 2.0, 3.0)
                 && acisCoordNear(projectedStraightCurve->p1,
                                 4.0, 3.0, 3.0)
                 && drw_evaluateAcisStraightCurve(
                        *projectedStraightCurve, 1.0,
                        evaluatedProjectedStraight)
                 && acisCoordNear(evaluatedProjectedStraight,
                                  7.0, 5.0, 6.0)
                 && projectedStartOk && projectedStartParameter == 0.0
                 && acisCoordNear(projectedStartPoint, 3.0, 2.0, 3.0)
                 && projectedEndOk && projectedEndParameter == 1.0
                 && acisCoordNear(projectedEndPoint, 7.0, 5.0, 6.0)
                 && withinStartOk && withinStartParameter == 0.0
                 && withinEndOk && withinEndParameter == 1.0
                 && finiteStartProjectionOk && finiteStartParameter == 0.0
                 && acisCoordNear(finiteStartProjection, 3.0, 2.0, 3.0)
                 && finiteEndProjectionOk && finiteEndParameter == 1.0
                 && acisCoordNear(finiteEndProjection, 7.0, 5.0, 6.0)
                 && finiteEndToleranceOk
                 && finiteEndToleranceParameter == 1.0
                 && parserFedFiniteEdgeEvaluation
                 && acisCoordNear(evaluatedFiniteEdgeMidpoint,
                                  5.0, 3.5, 4.5),
             "SAB 21800 straight edge clamps line projections to its decoded endpoint interval");

    AcisGraphFixture ellipseCurveFixture = complexFixture;
    ellipseCurveFixture.data.records[
        static_cast<std::size_t>(unsupportedCurve)] = acisRecord(
            "ellipse-curve",
            {acisPointer(-1), acisLocation(14.0, 15.0, 16.0),
             acisDirection(0.0, 0.0, 1.0),
             acisDirection(4.0, 0.0, 0.0), acisDouble(0.25)});
    const std::vector<std::uint8_t> ellipseCurveBytes =
        sab21800FromGraph(ellipseCurveFixture.data);
    DRW_SabData parsedEllipseCurve;
    const bool ellipseCurveParsed = !ellipseCurveBytes.empty()
        && drw_parseSab(ellipseCurveBytes.data(), ellipseCurveBytes.size(),
                        parsedEllipseCurve);
    std::vector<DRW_AcisBodyProjection> ellipseCurveProjections;
    DRW_AcisRootedProjectionReport ellipseCurveReport;
    const bool ellipseCurveProjected = drw_extractAcisBodyWireframes(
        drw_buildAcisModel(parsedEllipseCurve), ellipseCurveProjections,
        ellipseCurveReport);
    const DRW_AcisEdge* projectedEllipseCurve = nullptr;
    if (ellipseCurveProjections.size() == 1u) {
        const std::vector<DRW_AcisEdge>& edges =
            ellipseCurveProjections.front().wireframe.edges;
        const auto found = std::find_if(edges.begin(), edges.end(),
            [secondEdge](const DRW_AcisEdge& edge) {
                return edge.nodeIndex == secondEdge;
            });
        if (found != edges.end())
            projectedEllipseCurve = &*found;
    }
    t.expect(ellipseCurveParsed && ellipseCurveProjected
                 && ellipseCurveReport.failure
                        == DRW_AcisProjectionFailure::NoFailure
                 && projectedEllipseCurve
                 && projectedEllipseCurve->hasCurve
                 && projectedEllipseCurve->curveType
                        == DRW_AcisCurve::Ellipse
                 && acisCoordNear(projectedEllipseCurve->p0,
                                 14.0, 15.0, 16.0)
                 && acisCoordNear(projectedEllipseCurve->p1,
                                 0.0, 0.0, 1.0)
                 && acisCoordNear(projectedEllipseCurve->p2,
                                 4.0, 0.0, 0.0)
                 && projectedEllipseCurve->ratio == 0.25,
             "SAB 21800 ellipse curve consumes ordered center, frame, and ratio fields");

    const double ellipseQuarterTurn = std::acos(-1.0) / 2.0;
    DRW_Coord evaluatedEllipseStart;
    DRW_Coord evaluatedEllipseQuarter;
    DRW_Coord evaluatedEllipseHalf;
    const bool parserFedEllipseStart = projectedEllipseCurve
        && drw_evaluateAcisEllipseCurve(
            *projectedEllipseCurve, 0.0, 0.0, evaluatedEllipseStart);
    const bool parserFedEllipseQuarter = projectedEllipseCurve
        && drw_evaluateAcisEllipseCurve(
            *projectedEllipseCurve, ellipseQuarterTurn, 0.0,
            evaluatedEllipseQuarter);
    const bool parserFedEllipseHalf = projectedEllipseCurve
        && drw_evaluateAcisEllipseCurve(
            *projectedEllipseCurve, std::acos(-1.0), 0.0,
            evaluatedEllipseHalf);
    t.expect(parserFedEllipseStart && parserFedEllipseQuarter
                 && parserFedEllipseHalf
                 && acisCoordNear(evaluatedEllipseStart, 18.0, 15.0, 16.0)
                 && acisCoordNear(evaluatedEllipseQuarter,
                                  14.0, 16.0, 16.0)
                 && acisCoordNear(evaluatedEllipseHalf,
                                  10.0, 15.0, 16.0),
             "Parser-fed ellipse evaluator follows center + major*cos + right-handed minor*sin");

    DRW_Coord projectedEllipseMajor;
    DRW_Coord projectedEllipseMinor;
    DRW_Coord projectedEllipseInterior;
    DRW_Coord projectedEllipseCusp;
    DRW_Coord projectedEllipseBoundary;
    const bool ellipseMajorProjection = projectedEllipseCurve
        && drw_projectPointToAcisEllipseCurve(
            *projectedEllipseCurve, DRW_Coord(20.0, 15.0, 19.0), 0.0,
            projectedEllipseMajor);
    const bool ellipseMinorProjection = projectedEllipseCurve
        && drw_projectPointToAcisEllipseCurve(
            *projectedEllipseCurve, DRW_Coord(14.0, 17.0, 16.0), 0.0,
            projectedEllipseMinor);
    const bool ellipseInteriorProjection = projectedEllipseCurve
        && drw_projectPointToAcisEllipseCurve(
            *projectedEllipseCurve, DRW_Coord(14.0, 15.5, 16.0), 0.0,
            projectedEllipseInterior);
    const bool ellipseCuspProjection = projectedEllipseCurve
        && drw_projectPointToAcisEllipseCurve(
            *projectedEllipseCurve, DRW_Coord(17.75, 15.0, 16.0), 0.0,
            projectedEllipseCusp);
    const bool ellipseBoundaryProjection = projectedEllipseCurve
        && drw_projectPointToAcisEllipseCurve(
            *projectedEllipseCurve, evaluatedEllipseQuarter, 0.0,
            projectedEllipseBoundary);
    DRW_AcisEdge tinyEllipse;
    tinyEllipse.hasCurve = true;
    tinyEllipse.curveType = DRW_AcisCurve::Ellipse;
    tinyEllipse.p0 = DRW_Coord(0.0, 0.0, 0.0);
    tinyEllipse.p1 = DRW_Coord(0.0, 0.0, 1.0);
    tinyEllipse.p2 = DRW_Coord(1e-100, 0.0, 0.0);
    tinyEllipse.ratio = 1.0;
    DRW_Coord projectedTinyEllipse;
    const bool tinyEllipseProjection = drw_projectPointToAcisEllipseCurve(
        tinyEllipse, DRW_Coord(1.0, 0.0, 0.0), 0.0,
        projectedTinyEllipse);
    DRW_Coord unchangedEllipseProjection(71.0, 72.0, 73.0);
    const bool ellipseAmbiguityRefused = projectedEllipseCurve
        && !drw_projectPointToAcisEllipseCurve(
            *projectedEllipseCurve, DRW_Coord(15.0, 15.0, 16.0), 0.0,
            unchangedEllipseProjection)
        && !drw_projectPointToAcisEllipseCurve(
            *projectedEllipseCurve, DRW_Coord(14.0, 15.0, 16.0), 0.0,
            unchangedEllipseProjection)
        && unchangedEllipseProjection.x == 71.0
        && unchangedEllipseProjection.y == 72.0
        && unchangedEllipseProjection.z == 73.0;
    t.expect(ellipseMajorProjection && ellipseMinorProjection
                 && ellipseInteriorProjection && ellipseCuspProjection
                 && ellipseBoundaryProjection && tinyEllipseProjection
                 && ellipseAmbiguityRefused
                 && acisCoordNear(projectedEllipseMajor, 18.0, 15.0, 16.0)
                 && acisCoordNear(projectedEllipseMinor, 14.0, 16.0, 16.0)
                 && acisCoordNear(projectedEllipseInterior,
                                  14.0, 16.0, 16.0)
                 && acisCoordNear(projectedEllipseCusp, 18.0, 15.0, 16.0)
                 && acisCoordNear(projectedEllipseBoundary,
                                  evaluatedEllipseQuarter.x,
                                  evaluatedEllipseQuarter.y,
                                  evaluatedEllipseQuarter.z)
                 && std::fabs(projectedTinyEllipse.x / 1e-100 - 1.0)
                        < 1e-12
                 && projectedTinyEllipse.y == 0.0
                 && projectedTinyEllipse.z == 0.0,
             "Parser-fed ellipse projection finds the closest full-locus point and refuses nonunique medial-axis cases atomically");

    bool ellipseProjectionGridOk = projectedEllipseCurve != nullptr;
    const double ellipseQueryX[] = {
        -6.0, -3.9, -3.75, -3.0, -1.0, 0.0, 1.0, 3.0, 3.75, 3.9, 6.0};
    const double ellipseQueryY[] = {-2.0, -0.5, 0.0, 0.5, 2.0};
    const double fullTurn = 2.0 * std::acos(-1.0);
    for (double queryX : ellipseQueryX) {
        for (double queryY : ellipseQueryY) {
            const DRW_Coord query(14.0 + queryX, 15.0 + queryY, 16.75);
            DRW_Coord candidate(91.0, 92.0, 93.0);
            const bool projected = projectedEllipseCurve
                && drw_projectPointToAcisEllipseCurve(
                    *projectedEllipseCurve, query, 0.0, candidate);
            const bool medialAxisAmbiguity = queryY == 0.0
                && std::fabs(queryX) < 3.75;
            if (medialAxisAmbiguity) {
                ellipseProjectionGridOk = ellipseProjectionGridOk
                    && !projected && candidate.x == 91.0
                    && candidate.y == 92.0 && candidate.z == 93.0;
                continue;
            }
            if (!projected) {
                ellipseProjectionGridOk = false;
                continue;
            }
            const double candidateDx = candidate.x - query.x;
            const double candidateDy = candidate.y - query.y;
            const double candidateDz = candidate.z - query.z;
            const double candidateDistanceSquared = candidateDx * candidateDx
                + candidateDy * candidateDy + candidateDz * candidateDz;
            double sampledDistanceSquared =
                std::numeric_limits<double>::infinity();
            for (unsigned int sample = 0u; sample < 1024u; ++sample) {
                DRW_Coord sampledPoint;
                const double parameter = fullTurn
                    * static_cast<double>(sample) / 1024.0;
                if (!drw_evaluateAcisEllipseCurve(
                        *projectedEllipseCurve, parameter, 0.0,
                        sampledPoint)) {
                    ellipseProjectionGridOk = false;
                    break;
                }
                const double dx = sampledPoint.x - query.x;
                const double dy = sampledPoint.y - query.y;
                const double dz = sampledPoint.z - query.z;
                sampledDistanceSquared = std::min(
                    sampledDistanceSquared,
                    dx * dx + dy * dy + dz * dz);
            }
            ellipseProjectionGridOk = ellipseProjectionGridOk
                && candidateDistanceSquared
                    <= sampledDistanceSquared + 1e-10;
        }
    }
    t.expect(ellipseProjectionGridOk,
             "Ellipse projection agrees with a dense generated full-locus minimum grid and refuses the medial axis");

    DRW_AcisEdge circularEllipse;
    if (projectedEllipseCurve)
        circularEllipse = *projectedEllipseCurve;
    circularEllipse.ratio = 1.0;
    DRW_AcisEdge invalidProximityEllipse = circularEllipse;
    invalidProximityEllipse.curveType = DRW_AcisCurve::Straight;
    const bool ellipseProximityOk = projectedEllipseCurve
        && drw_pointWithinToleranceOfAcisEllipseCurve(
            *projectedEllipseCurve, evaluatedEllipseQuarter, 0.0, 0.0)
        && drw_pointWithinToleranceOfAcisEllipseCurve(
            *projectedEllipseCurve, DRW_Coord(14.0, 16.0, 19.0), 0.0, 3.0)
        && !drw_pointWithinToleranceOfAcisEllipseCurve(
            *projectedEllipseCurve, DRW_Coord(14.0, 16.0, 19.0), 0.0,
            2.999)
        && drw_pointWithinToleranceOfAcisEllipseCurve(
            *projectedEllipseCurve, DRW_Coord(14.0, 15.0, 16.0), 0.0, 1.0)
        && !drw_pointWithinToleranceOfAcisEllipseCurve(
            *projectedEllipseCurve, DRW_Coord(14.0, 15.0, 16.0), 0.0,
            0.999)
        && drw_pointWithinToleranceOfAcisEllipseCurve(
            circularEllipse, DRW_Coord(14.0, 15.0, 16.0), 0.0, 4.0)
        && !drw_pointWithinToleranceOfAcisEllipseCurve(
            circularEllipse, DRW_Coord(14.0, 15.0, 16.0), 0.0, 3.999)
        && !drw_pointWithinToleranceOfAcisEllipseCurve(
            *projectedEllipseCurve, evaluatedEllipseQuarter, 0.0, -1.0)
        && !drw_pointWithinToleranceOfAcisEllipseCurve(
            *projectedEllipseCurve, evaluatedEllipseQuarter,
            std::numeric_limits<double>::infinity(), 1.0)
        && !drw_pointWithinToleranceOfAcisEllipseCurve(
            invalidProximityEllipse, evaluatedEllipseQuarter, 0.0, 1.0);
    t.expect(ellipseProximityOk,
             "Ellipse proximity uses explicit inclusive tolerances and reports distance for tied closest points");

    DRW_AcisEdge ellipseEdgeEndpoints;
    if (projectedEllipseCurve)
        ellipseEdgeEndpoints = *projectedEllipseCurve;
    DRW_Coord ellipseEdgeStart;
    DRW_Coord ellipseEdgeEnd;
    const bool ellipseEdgeEndpointsEvaluated =
        projectedEllipseCurve
        && drw_evaluateAcisEllipseCurve(
            ellipseEdgeEndpoints, 0.0, 0.0, ellipseEdgeStart)
        && drw_evaluateAcisEllipseCurve(
            ellipseEdgeEndpoints, ellipseQuarterTurn, 0.0,
            ellipseEdgeEnd);
    ellipseEdgeEndpoints.start = ellipseEdgeStart;
    ellipseEdgeEndpoints.end = ellipseEdgeEnd;
    ellipseEdgeEndpoints.hasStart = true;
    ellipseEdgeEndpoints.hasEnd = true;
    DRW_AcisEdge offCurveEllipseEdgeEndpoint = ellipseEdgeEndpoints;
    offCurveEllipseEdgeEndpoint.end.z += 0.25;
    DRW_AcisEdge missingEllipseEdgeEndpoint = ellipseEdgeEndpoints;
    missingEllipseEdgeEndpoint.hasStart = false;
    DRW_AcisEdge wrongTypeEllipseEdge = ellipseEdgeEndpoints;
    wrongTypeEllipseEdge.curveType = DRW_AcisCurve::Straight;
    t.expect(ellipseEdgeEndpointsEvaluated
                 && drw_acisEllipseEdgeEndpointsWithinTolerance(
                     ellipseEdgeEndpoints, 0.0, 0.0),
             "Parser-fed ellipse edge endpoints on the analytic locus pass zero tolerance");
    t.expect(drw_acisEllipseEdgeEndpointsWithinTolerance(
                 offCurveEllipseEdgeEndpoint, 0.0, 0.25)
                 && !drw_acisEllipseEdgeEndpointsWithinTolerance(
                     offCurveEllipseEdgeEndpoint, 0.0, 0.249),
             "Ellipse edge endpoint incidence honors its inclusive explicit distance boundary");
    t.expect(!drw_acisEllipseEdgeEndpointsWithinTolerance(
                 missingEllipseEdgeEndpoint, 0.0, 0.0)
                 && !drw_acisEllipseEdgeEndpointsWithinTolerance(
                     ellipseEdgeEndpoints, -1.0, 0.0)
                 && !drw_acisEllipseEdgeEndpointsWithinTolerance(
                     ellipseEdgeEndpoints, 0.0, -1.0)
                 && !drw_acisEllipseEdgeEndpointsWithinTolerance(
                     wrongTypeEllipseEdge, 0.0, 0.0),
             "Ellipse edge endpoint incidence rejects absent endpoints, invalid tolerances, and non-ellipse profiles");

    DRW_AcisEdge nonUnitNormalEllipse;
    if (projectedEllipseCurve)
        nonUnitNormalEllipse = *projectedEllipseCurve;
    nonUnitNormalEllipse.p1 = DRW_Coord(0.0, 0.0, 17.0);
    DRW_Coord evaluatedNonUnitEllipse;
    const bool nonUnitNormalEllipseOk = drw_evaluateAcisEllipseCurve(
        nonUnitNormalEllipse, ellipseQuarterTurn, 0.0,
        evaluatedNonUnitEllipse);
    DRW_AcisEdge alternateRatioEllipse = nonUnitNormalEllipse;
    alternateRatioEllipse.ratio = 0.5;
    DRW_Coord evaluatedAlternateRatioEllipse;
    const bool alternateRatioEllipseOk = drw_evaluateAcisEllipseCurve(
        alternateRatioEllipse, ellipseQuarterTurn, 0.0,
        evaluatedAlternateRatioEllipse);
    DRW_AcisEdge swappedAxisEllipse = nonUnitNormalEllipse;
    swappedAxisEllipse.ratio = 2.0;
    DRW_Coord projectedSwappedAxisEllipse;
    const bool swappedAxisEllipseOk = drw_projectPointToAcisEllipseCurve(
        swappedAxisEllipse, DRW_Coord(14.0, 25.0, 16.0), 0.0,
        projectedSwappedAxisEllipse);
    t.expect(nonUnitNormalEllipseOk && alternateRatioEllipseOk
                 && swappedAxisEllipseOk
                 && acisCoordNear(evaluatedNonUnitEllipse,
                                  14.0, 16.0, 16.0)
                 && acisCoordNear(evaluatedAlternateRatioEllipse,
                                  14.0, 17.0, 16.0)
                 && acisCoordNear(projectedSwappedAxisEllipse,
                                  14.0, 23.0, 16.0),
             "Ellipse operations normalize the frame, scale the minor radius, and sort axes when the ratio exceeds one");

    DRW_AcisEdge obliqueEllipse = nonUnitNormalEllipse;
    obliqueEllipse.p1 = DRW_Coord(5.0, 5.0, 5.0);
    obliqueEllipse.p2 = DRW_Coord(3.0, -3.0, 0.0);
    DRW_Coord evaluatedObliqueEllipse;
    const bool obliqueEllipseOk = drw_evaluateAcisEllipseCurve(
        obliqueEllipse, ellipseQuarterTurn, 1e-14,
        evaluatedObliqueEllipse);
    const double inverseSqrt2 = 1.0 / std::sqrt(2.0);
    const double inverseSqrt3 = 1.0 / std::sqrt(3.0);
    DRW_Coord projectedObliqueEllipse;
    const bool obliqueEllipseProjectionOk =
        drw_projectPointToAcisEllipseCurve(
            obliqueEllipse,
            DRW_Coord(14.0 + 6.0 * inverseSqrt2
                              + 2.0 * inverseSqrt3,
                      15.0 - 6.0 * inverseSqrt2
                              + 2.0 * inverseSqrt3,
                      16.0 + 2.0 * inverseSqrt3),
            1e-14, projectedObliqueEllipse);
    const double obliqueMinorComponent = std::sqrt(3.0) / 4.0;
    t.expect(obliqueEllipseOk && obliqueEllipseProjectionOk
                 && acisCoordNear(evaluatedObliqueEllipse,
                                  14.0 + obliqueMinorComponent,
                                  15.0 + obliqueMinorComponent,
                                  16.0 - 2.0 * obliqueMinorComponent)
                 && acisCoordNear(projectedObliqueEllipse,
                                  17.0, 12.0, 16.0),
             "Ellipse evaluation and projection handle an oblique non-unit normal and major axis");

    DRW_AcisEdge toleratedEllipseFrame = nonUnitNormalEllipse;
    toleratedEllipseFrame.p2 = DRW_Coord(4.0, 0.0, 1e-9);
    DRW_Coord evaluatedToleratedEllipseFrame;
    const bool toleratedEllipseFrameOk = drw_evaluateAcisEllipseCurve(
        toleratedEllipseFrame, 0.0, 1e-8,
        evaluatedToleratedEllipseFrame);
    DRW_Coord unchangedEllipseOutput(31.0, 32.0, 33.0);
    const DRW_Coord originalEllipseOutput = unchangedEllipseOutput;
    DRW_AcisEdge zeroRatioEllipse = nonUnitNormalEllipse;
    zeroRatioEllipse.ratio = 0.0;
    DRW_AcisEdge wrongTypeEllipse = nonUnitNormalEllipse;
    wrongTypeEllipse.curveType = DRW_AcisCurve::Straight;
    DRW_AcisEdge parallelFrameEllipse = nonUnitNormalEllipse;
    parallelFrameEllipse.p2 = DRW_Coord(0.0, 0.0, 4.0);
    const bool rejectedEllipseFrame = !drw_evaluateAcisEllipseCurve(
        toleratedEllipseFrame, 0.0, 0.0, unchangedEllipseOutput);
    const bool rejectedInvalidEllipseInputs =
        !drw_evaluateAcisEllipseCurve(
            nonUnitNormalEllipse, std::numeric_limits<double>::quiet_NaN(),
            0.0, unchangedEllipseOutput)
        && !drw_evaluateAcisEllipseCurve(
            nonUnitNormalEllipse, 0.0,
            std::numeric_limits<double>::infinity(), unchangedEllipseOutput)
        && !drw_evaluateAcisEllipseCurve(
            nonUnitNormalEllipse, 0.0, -1.0, unchangedEllipseOutput)
        && !drw_evaluateAcisEllipseCurve(
            zeroRatioEllipse, 0.0, 0.0, unchangedEllipseOutput)
        && !drw_evaluateAcisEllipseCurve(
            wrongTypeEllipse, 0.0, 0.0, unchangedEllipseOutput)
        && !drw_evaluateAcisEllipseCurve(
            parallelFrameEllipse, 0.0, 1.0, unchangedEllipseOutput)
        && unchangedEllipseOutput.x == originalEllipseOutput.x
        && unchangedEllipseOutput.y == originalEllipseOutput.y
        && unchangedEllipseOutput.z == originalEllipseOutput.z;
    DRW_Coord unchangedInvalidEllipseProjection(81.0, 82.0, 83.0);
    DRW_AcisEdge overflowDeltaEllipse = nonUnitNormalEllipse;
    overflowDeltaEllipse.p0.x = -std::numeric_limits<double>::max();
    const bool rejectedInvalidEllipseProjection =
        !drw_projectPointToAcisEllipseCurve(
            zeroRatioEllipse, DRW_Coord(14.0, 15.0, 16.0), 0.0,
            unchangedInvalidEllipseProjection)
        && !drw_projectPointToAcisEllipseCurve(
            wrongTypeEllipse, DRW_Coord(14.0, 15.0, 16.0), 0.0,
            unchangedInvalidEllipseProjection)
        && !drw_projectPointToAcisEllipseCurve(
            nonUnitNormalEllipse,
            DRW_Coord(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0),
            0.0, unchangedInvalidEllipseProjection)
        && !drw_projectPointToAcisEllipseCurve(
            toleratedEllipseFrame, DRW_Coord(14.0, 15.0, 16.0), 0.0,
            unchangedInvalidEllipseProjection)
        && !drw_projectPointToAcisEllipseCurve(
            nonUnitNormalEllipse, DRW_Coord(14.0, 15.0, 16.0),
            std::numeric_limits<double>::infinity(),
            unchangedInvalidEllipseProjection)
        && !drw_projectPointToAcisEllipseCurve(
            nonUnitNormalEllipse, DRW_Coord(14.0, 15.0, 16.0), -1.0,
            unchangedInvalidEllipseProjection)
        && !drw_projectPointToAcisEllipseCurve(
            overflowDeltaEllipse,
            DRW_Coord(std::numeric_limits<double>::max(), 0.0, 0.0), 0.0,
            unchangedInvalidEllipseProjection)
        && unchangedInvalidEllipseProjection.x == 81.0
        && unchangedInvalidEllipseProjection.y == 82.0
        && unchangedInvalidEllipseProjection.z == 83.0;
    t.expect(toleratedEllipseFrameOk && rejectedEllipseFrame
                 && rejectedInvalidEllipseInputs
                 && rejectedInvalidEllipseProjection
                 && acisCoordNear(evaluatedToleratedEllipseFrame,
                                  18.0, 15.0, 16.0),
             "Ellipse evaluation and projection apply only the explicit frame tolerance and refuse invalid inputs atomically");

    DRW_AcisEdge unrepresentableEllipse;
    if (projectedEllipseCurve)
        unrepresentableEllipse = *projectedEllipseCurve;
    unrepresentableEllipse.p0.x = std::numeric_limits<double>::max();
    unrepresentableEllipse.p2.x = std::numeric_limits<double>::max();
    DRW_Coord unchangedOverflowEllipse(41.0, 42.0, 43.0);
    const bool rejectedUnrepresentableEllipse =
        !drw_evaluateAcisEllipseCurve(
            unrepresentableEllipse, 0.0, 0.0,
            unchangedOverflowEllipse)
        && unchangedOverflowEllipse.x == 41.0
        && unchangedOverflowEllipse.y == 42.0
        && unchangedOverflowEllipse.z == 43.0;
    DRW_AcisEdge cancellingEllipse;
    if (projectedEllipseCurve)
        cancellingEllipse = *projectedEllipseCurve;
    cancellingEllipse.p0.x = std::numeric_limits<double>::max();
    cancellingEllipse.p2.x = -std::numeric_limits<double>::max();
    DRW_Coord cancelledEllipsePoint;
    const bool largeEllipseCancellation = drw_evaluateAcisEllipseCurve(
        cancellingEllipse, 0.0, 0.0, cancelledEllipsePoint);
    DRW_AcisEdge smallCenterEllipseCancellation;
    if (projectedEllipseCurve)
        smallCenterEllipseCancellation = *projectedEllipseCurve;
    smallCenterEllipseCancellation.p0 = DRW_Coord(1.25, -3.5, 2.75);
    smallCenterEllipseCancellation.p1 = DRW_Coord(0.0, 0.0, 1.0);
    const double ellipseCancellationRadius = std::ldexp(1.0, 996);
    smallCenterEllipseCancellation.p2 = DRW_Coord(
        ellipseCancellationRadius, ellipseCancellationRadius, 0.0);
    const double ellipseCancellationParameter = std::acos(-1.0) / 4.0;
    const double ellipseCancellationCosine =
        std::cos(ellipseCancellationParameter);
    const double ellipseCancellationSine =
        std::sin(ellipseCancellationParameter);
    smallCenterEllipseCancellation.ratio = ellipseCancellationCosine
        / ellipseCancellationSine;
    DRW_Coord smallCenterEllipsePoint;
    const bool smallCenterEllipseCancellationOk = projectedEllipseCurve
        && drw_evaluateAcisEllipseCurve(
            smallCenterEllipseCancellation, ellipseCancellationParameter, 0.0,
            smallCenterEllipsePoint);
    t.expect(smallCenterEllipseCancellationOk,
             "Ellipse evaluator accepts parser-fed large-radius cancellation control");
    t.expect(std::fabs(smallCenterEllipsePoint.x - 1.25) < 1e-14,
             "Ellipse evaluator retains small x center after opposing offsets cancel");
    t.expect(std::fabs(smallCenterEllipsePoint.z - 2.75) < 1e-14,
             "Ellipse evaluator retains small z center after opposing offsets cancel");
    const double expectedEllipseCancellationY =
        2.0 * ellipseCancellationRadius * ellipseCancellationCosine;
    t.expect(std::fabs(smallCenterEllipsePoint.y
                       / expectedEllipseCancellationY - 1.0) < 1e-15,
             "Ellipse evaluator retains the finite summed large coordinate");
    DRW_AcisEdge zeroNormalEllipse = nonUnitNormalEllipse;
    zeroNormalEllipse.p1 = DRW_Coord();
    t.expect(rejectedUnrepresentableEllipse
                 && largeEllipseCancellation
                 && cancelledEllipsePoint.x == 0.0
                 && !drw_evaluateAcisEllipseCurve(
                     zeroNormalEllipse, 0.0, 0.0,
                     unchangedOverflowEllipse)
                 && unchangedOverflowEllipse.x == 41.0
                 && unchangedOverflowEllipse.y == 42.0
                 && unchangedOverflowEllipse.z == 43.0,
             "Ellipse evaluation preserves small centers across exact large-offset cancellation and refuses degenerate or unrepresentable results atomically");

    const auto refusesMalformedScalarCurve = [&](AcisGraphFixture fixture) {
        const std::vector<std::uint8_t> bytes = sab21800FromGraph(fixture.data);
        DRW_SabData parsed;
        const bool parsedOk = !bytes.empty()
            && drw_parseSab(bytes.data(), bytes.size(), parsed);
        const DRW_AcisModel malformedModel = drw_buildAcisModel(parsed);
        std::vector<DRW_AcisBodyProjection> projections(1u);
        DRW_AcisRootedProjectionReport report;
        const bool projected = drw_extractAcisBodyWireframes(
            malformedModel, projections, report);
        return parsedOk && !projected && projections.empty()
            && report.failure == DRW_AcisProjectionFailure::InvalidModel
            && report.failedNodeIndex == unsupportedCurve;
    };
    AcisGraphFixture malformedStraightCurveFixture = complexFixture;
    malformedStraightCurveFixture.data.records[
        static_cast<std::size_t>(unsupportedCurve)] = acisRecord(
            "straight-curve",
            {acisPointer(-1), acisLiteral("unexpected-field"),
             acisLocation(11.0, 12.0, 13.0),
             acisDirection(0.0, 1.0, 0.0)});
    t.expect(refusesMalformedScalarCurve(
                 malformedStraightCurveFixture),
             "SAB 21800 straight curve refuses a shifted location token transactionally");

    AcisGraphFixture malformedEllipseCurveFixture = complexFixture;
    malformedEllipseCurveFixture.data.records[
        static_cast<std::size_t>(unsupportedCurve)] = acisRecord(
            "ellipse-curve",
            {acisPointer(-1), acisLocation(14.0, 15.0, 16.0),
             acisLiteral("unexpected-field"),
             acisDirection(0.0, 0.0, 1.0),
             acisDirection(4.0, 0.0, 0.0), acisDouble(0.25)});
    t.expect(refusesMalformedScalarCurve(
                 malformedEllipseCurveFixture),
             "SAB 21800 ellipse curve refuses a shifted normal token transactionally");

    const DRW_AcisModel handBuiltSphereModel =
        drw_buildAcisModel(sphereSurfaceFixture.data);
    std::vector<DRW_AcisBodyProjection> handBuiltSphereProjections;
    DRW_AcisRootedProjectionReport handBuiltSphereReport;
    const bool handBuiltSphereProjected = drw_extractAcisBodyWireframes(
        handBuiltSphereModel, handBuiltSphereProjections,
        handBuiltSphereReport);
    t.expect(handBuiltSphereProjected
                 && handBuiltSphereProjections.size() == 1u
                 && handBuiltSphereProjections.front()
                        .wireframe.faces.front().hasSurface
                 && handBuiltSphereProjections.front()
                        .wireframe.faces.front().majorRadius == -7.5,
             "caller-built sphere record without the 21800 shared prefix retains offset-zero projection");

    AcisGraphFixture malformedPlaneFixture = complexFixture;
    DRW_SabRecord& malformedPlaneRecord =
        malformedPlaneFixture.data.records[
            static_cast<std::size_t>(unsupportedSurface)];
    malformedPlaneRecord.type = "plane-surface";
    malformedPlaneRecord.tokens = acisRecord(
        "plane-surface",
        {acisPointer(-1), acisLocation(0.0, 0.0, 0.0),
         acisLiteral("unexpected-field"),
         acisDirection(0.0, 0.0, 1.0),
         acisDirection(1.0, 0.0, 0.0)}).tokens;
    const std::vector<std::uint8_t> malformedPlaneBytes =
        sab21800FromGraph(malformedPlaneFixture.data);
    DRW_SabData parsedMalformedPlane;
    const bool malformedPlaneParsed = !malformedPlaneBytes.empty()
        && drw_parseSab(malformedPlaneBytes.data(), malformedPlaneBytes.size(),
                        parsedMalformedPlane);
    const DRW_AcisModel malformedPlaneModel =
        drw_buildAcisModel(parsedMalformedPlane);
    std::vector<DRW_AcisBodyProjection> malformedPlaneProjections(1u);
    DRW_AcisRootedProjectionReport malformedPlaneReport;
    const bool malformedPlaneProjected = drw_extractAcisBodyWireframes(
        malformedPlaneModel, malformedPlaneProjections,
        malformedPlaneReport);
    t.expect(malformedPlaneParsed && !malformedPlaneProjected
                 && malformedPlaneProjections.empty()
                 && malformedPlaneReport.failure
                        == DRW_AcisProjectionFailure::InvalidModel
                 && malformedPlaneReport.failedNodeIndex == unsupportedSurface,
             "SAB 21800 plane surface refuses a shifted normal field rather than scanning past the malformed token");

    AcisGraphFixture malformedTorusFixture = complexFixture;
    DRW_SabRecord& malformedTorusRecord =
        malformedTorusFixture.data.records[
            static_cast<std::size_t>(unsupportedSurface)];
    malformedTorusRecord.type = "torus-surface";
    malformedTorusRecord.tokens = acisRecord(
        "torus-surface",
        {acisPointer(-1), acisLocation(0.0, 0.0, 0.0),
         acisDirection(0.0, 0.0, 1.0), acisDirection(1.0, 0.0, 0.0),
         acisDouble(10.0), acisDouble(2.0)}).tokens;
    const std::vector<std::uint8_t> malformedTorusBytes =
        sab21800FromGraph(malformedTorusFixture.data);
    DRW_SabData parsedMalformedTorus;
    const bool malformedTorusParsed = !malformedTorusBytes.empty()
        && drw_parseSab(malformedTorusBytes.data(),
                        malformedTorusBytes.size(), parsedMalformedTorus);
    const DRW_AcisModel malformedTorusModel =
        drw_buildAcisModel(parsedMalformedTorus);
    std::vector<DRW_AcisBodyProjection> malformedTorusProjections(1u);
    DRW_AcisRootedProjectionReport malformedTorusReport;
    const bool malformedTorusProjected = drw_extractAcisBodyWireframes(
        malformedTorusModel, malformedTorusProjections,
        malformedTorusReport);
    t.expect(malformedTorusParsed && !malformedTorusProjected
                 && malformedTorusProjections.empty()
                 && malformedTorusReport.failure
                        == DRW_AcisProjectionFailure::InvalidModel
                 && malformedTorusReport.failedNodeIndex == unsupportedSurface,
             "SAB 21800 torus surface refuses a U direction before its radius pair");

    AcisGraphFixture shiftedSphereFieldFixture = sphereSurfaceFixture;
    DRW_SabRecord& shiftedSphereFieldRecord =
        shiftedSphereFieldFixture.data.records[
            static_cast<std::size_t>(unsupportedSurface)];
    shiftedSphereFieldRecord.tokens = acisRecord(
        "sphere-surface",
        {acisPointer(-1), acisLocation(4.0, 5.0, 6.0),
         acisLiteral("unexpected-field"),
         acisDirection(0.0, 1.0, 0.0),
         acisDirection(0.0, 0.0, 1.0), acisDouble(123.0)}).tokens;
    const std::vector<std::uint8_t> shiftedSphereFieldBytes =
        sab21800FromGraph(shiftedSphereFieldFixture.data);
    DRW_SabData parsedShiftedSphereField;
    const bool shiftedSphereFieldParsed =
        !shiftedSphereFieldBytes.empty()
        && drw_parseSab(shiftedSphereFieldBytes.data(),
                        shiftedSphereFieldBytes.size(),
                        parsedShiftedSphereField);
    const DRW_AcisModel shiftedSphereFieldModel =
        drw_buildAcisModel(parsedShiftedSphereField);
    std::vector<DRW_AcisBodyProjection> shiftedSphereFieldProjections(1u);
    DRW_AcisRootedProjectionReport shiftedSphereFieldReport;
    const bool shiftedSphereFieldProjected = drw_extractAcisBodyWireframes(
        shiftedSphereFieldModel, shiftedSphereFieldProjections,
        shiftedSphereFieldReport);
    t.expect(shiftedSphereFieldParsed && !shiftedSphereFieldProjected
                 && shiftedSphereFieldProjections.empty()
                 && shiftedSphereFieldReport.failure
                        == DRW_AcisProjectionFailure::InvalidModel,
             "SAB 21800 sphere surface refuses a skipped malformed field instead of binding a later radius token");
    t.expect(shiftedSphereFieldReport.failedNodeIndex == unsupportedSurface,
             "SAB 21800 malformed sphere surface reports the exact failing surface node");

    AcisGraphFixture truncatedSphereFixture = sphereSurfaceFixture;
    DRW_SabRecord& truncatedSphereRecord =
        truncatedSphereFixture.data.records[
            static_cast<std::size_t>(unsupportedSurface)];
    truncatedSphereRecord.tokens = acisRecord(
        "sphere-surface",
        {acisPointer(-1), acisLocation(4.0, 5.0, 6.0),
         acisDouble(7.5), acisDirection(0.0, 1.0, 0.0)}).tokens;
    const std::vector<std::uint8_t> truncatedSphereBytes =
        sab21800FromGraph(truncatedSphereFixture.data);
    DRW_SabData parsedTruncatedSphere;
    const bool truncatedSphereParsed = !truncatedSphereBytes.empty()
        && drw_parseSab(truncatedSphereBytes.data(), truncatedSphereBytes.size(),
                        parsedTruncatedSphere);
    const DRW_AcisModel truncatedSphereModel =
        drw_buildAcisModel(parsedTruncatedSphere);
    std::vector<DRW_AcisBodyProjection> truncatedSphereProjections(1u);
    DRW_AcisRootedProjectionReport truncatedSphereReport;
    const bool truncatedSphereProjected = drw_extractAcisBodyWireframes(
        truncatedSphereModel, truncatedSphereProjections,
        truncatedSphereReport);
    t.expect(truncatedSphereParsed && !truncatedSphereProjected
                 && truncatedSphereProjections.empty()
                 && truncatedSphereReport.failure
                        == DRW_AcisProjectionFailure::InvalidModel,
             "SAB 21800 sphere surface refuses a missing pole vector transactionally");
    t.expect(truncatedSphereReport.failedNodeIndex == unsupportedSurface,
             "SAB 21800 truncated sphere surface reports the exact failing surface node");

    struct SphereNonFiniteField {
        const char* name;
        std::size_t tokenIndex;
        int component;
    };
    const std::array<SphereNonFiniteField, 10> sphereNonFiniteFields {{
        {"center.x", 2u, 0}, {"center.y", 2u, 1}, {"center.z", 2u, 2},
        {"radius", 3u, -1},
        {"u-direction.x", 4u, 0}, {"u-direction.y", 4u, 1},
        {"u-direction.z", 4u, 2},
        {"pole-direction.x", 5u, 0}, {"pole-direction.y", 5u, 1},
        {"pole-direction.z", 5u, 2}
    }};
    const std::array<std::pair<const char*, double>, 3> nonFiniteValues {{
        {"NaN", std::numeric_limits<double>::quiet_NaN()},
        {"+infinity", std::numeric_limits<double>::infinity()},
        {"-infinity", -std::numeric_limits<double>::infinity()}
    }};
    for (const SphereNonFiniteField& field : sphereNonFiniteFields) {
        for (const auto& invalidValue : nonFiniteValues) {
            AcisGraphFixture invalidSphereFixture = sphereSurfaceFixture;
            DRW_SabRecord& invalidSphereRecord =
                invalidSphereFixture.data.records[
                    static_cast<std::size_t>(unsupportedSurface)];
            bool fieldPresent = field.tokenIndex
                < invalidSphereRecord.tokens.size();
            if (fieldPresent) {
                DRW_SabToken& token =
                    invalidSphereRecord.tokens[field.tokenIndex];
                if (field.component < 0) {
                    fieldPresent = token.tag == DRW_SabTag::Double;
                    if (fieldPresent)
                        token.dval = invalidValue.second;
                } else {
                    fieldPresent = token.tag == DRW_SabTag::LocationVec
                        || token.tag == DRW_SabTag::DirectionVec;
                    if (fieldPresent) {
                        double* components[] = {
                            &token.vec.x, &token.vec.y, &token.vec.z
                        };
                        *components[field.component] = invalidValue.second;
                    }
                }
            }

            const std::vector<std::uint8_t> invalidSphereBytes =
                sab21800FromGraph(invalidSphereFixture.data);
            DRW_SabData parsedInvalidSphere;
            const bool invalidSphereParsed = !invalidSphereBytes.empty()
                && drw_parseSab(invalidSphereBytes.data(),
                                invalidSphereBytes.size(), parsedInvalidSphere);
            const DRW_AcisModel invalidSphereModel =
                drw_buildAcisModel(parsedInvalidSphere);
            std::vector<DRW_AcisBodyProjection> invalidSphereProjections(1u);
            DRW_AcisRootedProjectionReport invalidSphereReport;
            const bool invalidSphereProjected = drw_extractAcisBodyWireframes(
                invalidSphereModel, invalidSphereProjections,
                invalidSphereReport);
            t.expect(fieldPresent && invalidSphereParsed
                         && !invalidSphereProjected
                         && invalidSphereProjections.empty()
                         && invalidSphereReport.failure
                                == DRW_AcisProjectionFailure::InvalidModel
                         && invalidSphereReport.failedNodeIndex
                                == unsupportedSurface,
                     std::string("SAB 21800 sphere ") + field.name + "="
                         + invalidValue.first
                         + " is refused transactionally at its surface node");
        }
    }

    AcisGraphFixture sharedUnsupportedSurfaceFixture = complexFixture;
    const int sharedUnsupportedFace =
        sharedUnsupportedSurfaceFixture.addNode("face", 6u);
    const int sharedUnsupportedLoop =
        sharedUnsupportedSurfaceFixture.addNode("loop", 4u);
    sharedUnsupportedSurfaceFixture.link(
        root.face, 1u, sharedUnsupportedFace);
    sharedUnsupportedSurfaceFixture.link(
        sharedUnsupportedFace, 2u, sharedUnsupportedLoop);
    sharedUnsupportedSurfaceFixture.link(
        sharedUnsupportedFace, 3u, root.shell);
    sharedUnsupportedSurfaceFixture.link(
        sharedUnsupportedFace, 5u, unsupportedSurface);
    sharedUnsupportedSurfaceFixture.link(
        sharedUnsupportedLoop, 3u, sharedUnsupportedFace);
    const std::vector<std::uint8_t> sharedUnsupportedSurfaceBytes =
        sab21800FromGraph(sharedUnsupportedSurfaceFixture.data);
    DRW_SabData parsedSharedUnsupportedSurface;
    const bool sharedUnsupportedSurfaceParsed =
        !sharedUnsupportedSurfaceBytes.empty()
        && drw_parseSab(sharedUnsupportedSurfaceBytes.data(),
                        sharedUnsupportedSurfaceBytes.size(),
                        parsedSharedUnsupportedSurface);
    const DRW_AcisModel sharedUnsupportedSurfaceModel =
        drw_buildAcisModel(parsedSharedUnsupportedSurface);
    std::vector<DRW_AcisBodyProjection> sharedUnsupportedSurfaceProjections;
    DRW_AcisRootedProjectionReport sharedUnsupportedSurfaceReport;
    const bool sharedUnsupportedSurfaceProjected =
        drw_extractAcisBodyWireframes(
            sharedUnsupportedSurfaceModel,
            sharedUnsupportedSurfaceProjections,
            sharedUnsupportedSurfaceReport);
    std::size_t sharedSurfaceReachableCount = 0u;
    std::size_t sharedSurfaceFaceLinkCount = 0u;
    bool sharedSurfaceFaceLinksMatch = true;
    std::size_t sharedSurfaceOutgoingLinkCount = 0u;
    bool sharedSurfaceNullSlotRetained = false;
    if (sharedUnsupportedSurfaceProjections.size() == 1u) {
        const DRW_AcisBodyProjection& projected =
            sharedUnsupportedSurfaceProjections.front();
        sharedSurfaceReachableCount = static_cast<std::size_t>(std::count(
            projected.reachableNodeIndices.begin(),
            projected.reachableNodeIndices.end(), unsupportedSurface));
        for (const DRW_AcisBodyGraphLink& link : projected.graphLinks) {
            if (link.pointerOrdinal != 5u
                || link.targetNodeIndex != unsupportedSurface)
                continue;
            ++sharedSurfaceFaceLinkCount;
            sharedSurfaceFaceLinksMatch = sharedSurfaceFaceLinksMatch
                && (link.sourceNodeIndex == root.face
                    || link.sourceNodeIndex == sharedUnsupportedFace);
        }
        for (const DRW_AcisBodyGraphLink& link : projected.graphLinks) {
            if (link.sourceNodeIndex != unsupportedSurface)
                continue;
            ++sharedSurfaceOutgoingLinkCount;
            sharedSurfaceNullSlotRetained =
                sharedSurfaceNullSlotRetained
                || (link.pointerOrdinal == 0u
                    && link.targetNodeIndex == -1);
        }
    }
    t.expect(sharedUnsupportedSurfaceParsed
                 && sharedUnsupportedSurfaceProjected
                 && sharedUnsupportedSurfaceReport.bodyCount == 1u
                 && sharedUnsupportedSurfaceReport.failure
                        == DRW_AcisProjectionFailure::NoFailure
                 && sharedUnsupportedSurfaceProjections.size() == 1u
                 && sharedUnsupportedSurfaceProjections.front()
                        .wireframe.faces.size() == 2u
                 && sharedUnsupportedSurfaceProjections.front()
                        .report.unsupportedSurfaceNodeCount == 1u
                 && sharedSurfaceReachableCount == 1u
                 && sharedSurfaceFaceLinkCount == 2u
                 && sharedSurfaceFaceLinksMatch
                 && sharedSurfaceOutgoingLinkCount == 1u
                 && sharedSurfaceNullSlotRetained
                 && std::all_of(
                     sharedUnsupportedSurfaceProjections.front()
                         .wireframe.faces.begin(),
                     sharedUnsupportedSurfaceProjections.front()
                         .wireframe.faces.end(),
                     [](const DRW_AcisFace& face) {
                         return face.surfaceType == DRW_AcisSurface::Spline
                             && !face.hasSurface;
                     })
                 && !sharedUnsupportedSurfaceProjections.front()
                        .report.brepValidityChecked,
             "SAB 21800 shared unsupported surface is counted once with incoming links and its raw null pointer row retained");

    AcisGraphFixture noUnsupportedSurfaceRowFixture =
        sharedUnsupportedSurfaceFixture;
    DRW_SabRecord& noUnsupportedSurfaceRow =
        noUnsupportedSurfaceRowFixture.data.records[
            static_cast<std::size_t>(unsupportedSurface)];
    const std::size_t surfacePointerCount = static_cast<std::size_t>(
        std::count_if(noUnsupportedSurfaceRow.tokens.begin(),
                      noUnsupportedSurfaceRow.tokens.end(),
                      [](const DRW_SabToken& token) {
                          return token.tag == DRW_SabTag::Pointer;
                      }));
    noUnsupportedSurfaceRow.tokens.erase(
        std::remove_if(noUnsupportedSurfaceRow.tokens.begin(),
                       noUnsupportedSurfaceRow.tokens.end(),
                       [](const DRW_SabToken& token) {
                           return token.tag == DRW_SabTag::Pointer;
                       }),
        noUnsupportedSurfaceRow.tokens.end());
    const std::vector<std::uint8_t> noUnsupportedSurfaceRowBytes =
        sab21800FromGraph(noUnsupportedSurfaceRowFixture.data);
    DRW_SabData parsedNoUnsupportedSurfaceRow;
    const bool noUnsupportedSurfaceRowParsed =
        !noUnsupportedSurfaceRowBytes.empty()
        && drw_parseSab(noUnsupportedSurfaceRowBytes.data(),
                        noUnsupportedSurfaceRowBytes.size(),
                        parsedNoUnsupportedSurfaceRow);
    const DRW_AcisModel noUnsupportedSurfaceRowModel =
        drw_buildAcisModel(parsedNoUnsupportedSurfaceRow);
    const auto minimumOutputBudget = [](const DRW_AcisModel& model) {
        const std::size_t invalidBudget =
            std::numeric_limits<std::size_t>::max();
        DRW_SabParseLimits defaults;
        std::size_t low = 0u;
        std::size_t high = defaults.maxOutputItems;
        while (low < high) {
            const std::size_t candidate = low + (high - low) / 2u;
            DRW_SabParseLimits limits;
            limits.maxOutputItems = candidate;
            std::vector<DRW_AcisBodyProjection> projections;
            DRW_AcisRootedProjectionReport report;
            if (drw_extractAcisBodyWireframes(model, projections, report,
                                              limits)) {
                high = candidate;
            } else if (report.failure
                       == DRW_AcisProjectionFailure::ResourceLimit) {
                low = candidate + 1u;
            } else {
                return invalidBudget;
            }
        }
        DRW_SabParseLimits exactLimits;
        exactLimits.maxOutputItems = low;
        std::vector<DRW_AcisBodyProjection> exactProjections;
        DRW_AcisRootedProjectionReport exactReport;
        return drw_extractAcisBodyWireframes(
                   model, exactProjections, exactReport, exactLimits)
            ? low : invalidBudget;
    };
    const DRW_AcisModel complexCoedgeIdentityModel =
        drw_buildAcisModel(complexFixture.data);
    const std::size_t complexCoedgeIdentityBudget =
        minimumOutputBudget(complexCoedgeIdentityModel);
    DRW_SabParseLimits exactComplexCoedgeIdentityBudget;
    exactComplexCoedgeIdentityBudget.maxOutputItems =
        complexCoedgeIdentityBudget;
    std::vector<DRW_AcisBodyProjection> exactComplexCoedgeIdentityProjections;
    DRW_AcisRootedProjectionReport exactComplexCoedgeIdentityReport;
    const bool exactComplexCoedgeIdentityProjected =
        complexCoedgeIdentityBudget != std::numeric_limits<std::size_t>::max()
        && drw_extractAcisBodyWireframes(
            complexCoedgeIdentityModel,
            exactComplexCoedgeIdentityProjections,
            exactComplexCoedgeIdentityReport,
            exactComplexCoedgeIdentityBudget);
    DRW_SabParseLimits oneUnderComplexCoedgeIdentityBudget;
    if (complexCoedgeIdentityBudget
            != std::numeric_limits<std::size_t>::max()
        && complexCoedgeIdentityBudget > 0u) {
        oneUnderComplexCoedgeIdentityBudget.maxOutputItems =
            complexCoedgeIdentityBudget - 1u;
    }
    std::vector<DRW_AcisBodyProjection>
        rejectedComplexCoedgeIdentityProjections(1u);
    DRW_AcisRootedProjectionReport rejectedComplexCoedgeIdentityReport;
    const bool oneUnderComplexCoedgeIdentityProjected =
        drw_extractAcisBodyWireframes(
            complexCoedgeIdentityModel,
            rejectedComplexCoedgeIdentityProjections,
            rejectedComplexCoedgeIdentityReport,
            oneUnderComplexCoedgeIdentityBudget);
    t.expect(exactComplexCoedgeIdentityProjected
                 && exactComplexCoedgeIdentityProjections.size() == 1u
                 && !exactComplexCoedgeIdentityProjections.front()
                         .wireframe.faces.empty()
                 && exactComplexCoedgeIdentityProjections.front()
                        .wireframe.faces.front().loops[1u]
                        .coedgeNextNodeIndices
                    == std::vector<int>{thirdCoedge, fourthCoedge,
                                        root.coedge}
                 && exactComplexCoedgeIdentityProjections.front()
                        .wireframe.faces.front().loops[1u]
                        .coedgePreviousNodeIndices
                    == std::vector<int>{fourthCoedge, root.coedge,
                                        thirdCoedge}
                 && exactComplexCoedgeIdentityProjections.front()
                        .wireframe.faces.front().loops[1u]
                        .coedgePartnerNodeIndices
                    == std::vector<int>{secondCoedge, fourthCoedge,
                                        root.coedge}
                 && !exactComplexCoedgeIdentityProjections.front()
                        .wireframe.edges.empty()
                 && exactComplexCoedgeIdentityProjections.front()
                        .wireframe.edges.front()
                        .coedgePartnerRingNodeIndices
                    == std::vector<int>{root.coedge, secondCoedge,
                                        thirdCoedge, fourthCoedge}
                 && exactComplexCoedgeIdentityProjections.front()
                        .wireframe.edges.front()
                        .coedgePartnerRingNextNodeIndices
                    == std::vector<int>{secondCoedge, thirdCoedge,
                                        fourthCoedge, root.coedge}
                 && exactComplexCoedgeIdentityProjections.front()
                        .wireframe.edges.front()
                        .coedgeEdgeUseNodeIndices
                    == std::vector<int>{root.coedge, secondCoedge,
                                        fourthCoedge}
                 && !exactComplexCoedgeIdentityProjections.front()
                        .wireframe.edges.front()
                        .coedgePartnerRingMatchesEdgeUseIndex
                 && !oneUnderComplexCoedgeIdentityProjected
                 && rejectedComplexCoedgeIdentityProjections.empty()
                 && rejectedComplexCoedgeIdentityReport.failure
                        == DRW_AcisProjectionFailure::ResourceLimit,
             "ACIS coedge next/previous/partner rows fit the exact output budget and refuse one item below it transactionally");
    const std::size_t noUnsupportedSurfaceRowBudget =
        minimumOutputBudget(noUnsupportedSurfaceRowModel);
    const std::size_t sharedUnsupportedSurfaceBudget =
        minimumOutputBudget(sharedUnsupportedSurfaceModel);
    DRW_SabParseLimits oneUnderUnsupportedSurfaceBudget;
    if (sharedUnsupportedSurfaceBudget
        != std::numeric_limits<std::size_t>::max()
        && sharedUnsupportedSurfaceBudget > 0u) {
        oneUnderUnsupportedSurfaceBudget.maxOutputItems =
            sharedUnsupportedSurfaceBudget - 1u;
    }
    std::vector<DRW_AcisBodyProjection>
        rejectedUnsupportedSurfaceBudget(1u);
    DRW_AcisRootedProjectionReport rejectedUnsupportedSurfaceBudgetReport;
    const bool oneUnderUnsupportedSurfaceBudgetProjected =
        drw_extractAcisBodyWireframes(
            sharedUnsupportedSurfaceModel,
            rejectedUnsupportedSurfaceBudget,
            rejectedUnsupportedSurfaceBudgetReport,
            oneUnderUnsupportedSurfaceBudget);
    t.expect(surfacePointerCount == 1u
                 && noUnsupportedSurfaceRowParsed
                 && noUnsupportedSurfaceRowBudget
                        != std::numeric_limits<std::size_t>::max()
                 && sharedUnsupportedSurfaceBudget
                        == noUnsupportedSurfaceRowBudget + 1u
                 && !oneUnderUnsupportedSurfaceBudgetProjected
                 && rejectedUnsupportedSurfaceBudget.empty()
                 && rejectedUnsupportedSurfaceBudgetReport.failure
                        == DRW_AcisProjectionFailure::ResourceLimit,
             "ACIS rooted projection charges the retained unsupported-surface pointer row and refuses one output item below the exact bound");

    AcisGraphFixture noSharedUnsupportedCurveRowFixture =
        sharedUnsupportedAcrossBodiesFixture;
    DRW_SabRecord& noSharedUnsupportedCurveRow =
        noSharedUnsupportedCurveRowFixture.data.records[
            static_cast<std::size_t>(sharedUnsupportedCurve)];
    const std::size_t sharedCurvePointerCount = static_cast<std::size_t>(
        std::count_if(noSharedUnsupportedCurveRow.tokens.begin(),
                      noSharedUnsupportedCurveRow.tokens.end(),
                      [](const DRW_SabToken& token) {
                          return token.tag == DRW_SabTag::Pointer;
                      }));
    noSharedUnsupportedCurveRow.tokens.erase(
        std::remove_if(noSharedUnsupportedCurveRow.tokens.begin(),
                       noSharedUnsupportedCurveRow.tokens.end(),
                       [](const DRW_SabToken& token) {
                           return token.tag == DRW_SabTag::Pointer;
                       }),
        noSharedUnsupportedCurveRow.tokens.end());
    const std::vector<std::uint8_t> noSharedUnsupportedCurveRowBytes =
        sab21800FromGraph(noSharedUnsupportedCurveRowFixture.data);
    DRW_SabData parsedNoSharedUnsupportedCurveRow;
    const bool noSharedUnsupportedCurveRowParsed =
        !noSharedUnsupportedCurveRowBytes.empty()
        && drw_parseSab(noSharedUnsupportedCurveRowBytes.data(),
                        noSharedUnsupportedCurveRowBytes.size(),
                        parsedNoSharedUnsupportedCurveRow);
    const DRW_AcisModel noSharedUnsupportedCurveRowModel =
        drw_buildAcisModel(parsedNoSharedUnsupportedCurveRow);
    const std::size_t noSharedUnsupportedCurveRowBudget =
        minimumOutputBudget(noSharedUnsupportedCurveRowModel);
    const std::size_t sharedUnsupportedAcrossBodiesBudget =
        minimumOutputBudget(sharedUnsupportedAcrossBodiesModel);
    DRW_SabParseLimits oneUnderSharedUnsupportedAcrossBodiesBudget;
    if (sharedUnsupportedAcrossBodiesBudget
        != std::numeric_limits<std::size_t>::max()
        && sharedUnsupportedAcrossBodiesBudget > 0u) {
        oneUnderSharedUnsupportedAcrossBodiesBudget.maxOutputItems =
            sharedUnsupportedAcrossBodiesBudget - 1u;
    }
    std::vector<DRW_AcisBodyProjection>
        rejectedSharedUnsupportedAcrossBodiesBudget(1u);
    DRW_AcisRootedProjectionReport
        rejectedSharedUnsupportedAcrossBodiesBudgetReport;
    const bool sharedUnsupportedAcrossBodiesWithinOneUnderBudget =
        drw_extractAcisBodyWireframes(
            sharedUnsupportedAcrossBodiesModel,
            rejectedSharedUnsupportedAcrossBodiesBudget,
            rejectedSharedUnsupportedAcrossBodiesBudgetReport,
            oneUnderSharedUnsupportedAcrossBodiesBudget);
    t.expect(sharedCurvePointerCount == 1u
                 && noSharedUnsupportedCurveRowParsed
                 && noSharedUnsupportedCurveRowBudget
                        != std::numeric_limits<std::size_t>::max()
                 && sharedUnsupportedAcrossBodiesBudget
                        == noSharedUnsupportedCurveRowBudget + 2u
                 && !sharedUnsupportedAcrossBodiesWithinOneUnderBudget
                 && rejectedSharedUnsupportedAcrossBodiesBudget.empty()
                 && rejectedSharedUnsupportedAcrossBodiesBudgetReport.failure
                        == DRW_AcisProjectionFailure::ResourceLimit,
             "ACIS output budget charges one shared unsupported-curve pointer row per rooted body and refuses one item below the exact bound");

    AcisGraphFixture noOrderedUnsupportedCurveRowFixture =
        orderedUnsupportedCurveFixture;
    DRW_SabRecord& noOrderedUnsupportedCurveRow =
        noOrderedUnsupportedCurveRowFixture.data.records[
            static_cast<std::size_t>(orderedUnsupportedCurve)];
    const std::size_t orderedUnsupportedCurvePointerCount =
        static_cast<std::size_t>(std::count_if(
            noOrderedUnsupportedCurveRow.tokens.begin(),
            noOrderedUnsupportedCurveRow.tokens.end(),
            [](const DRW_SabToken& token) {
                return token.tag == DRW_SabTag::Pointer;
            }));
    noOrderedUnsupportedCurveRow.tokens.erase(
        std::remove_if(noOrderedUnsupportedCurveRow.tokens.begin(),
                       noOrderedUnsupportedCurveRow.tokens.end(),
                       [](const DRW_SabToken& token) {
                           return token.tag == DRW_SabTag::Pointer;
                       }),
        noOrderedUnsupportedCurveRow.tokens.end());
    const std::vector<std::uint8_t> noOrderedUnsupportedCurveRowBytes =
        sab21800FromGraph(noOrderedUnsupportedCurveRowFixture.data);
    DRW_SabData parsedNoOrderedUnsupportedCurveRow;
    const bool noOrderedUnsupportedCurveRowParsed =
        !noOrderedUnsupportedCurveRowBytes.empty()
        && drw_parseSab(noOrderedUnsupportedCurveRowBytes.data(),
                        noOrderedUnsupportedCurveRowBytes.size(),
                        parsedNoOrderedUnsupportedCurveRow);
    const DRW_AcisModel noOrderedUnsupportedCurveRowModel =
        drw_buildAcisModel(parsedNoOrderedUnsupportedCurveRow);
    const std::size_t noOrderedUnsupportedCurveRowBudget =
        minimumOutputBudget(noOrderedUnsupportedCurveRowModel);
    const std::size_t orderedUnsupportedCurveRowBudget =
        minimumOutputBudget(orderedUnsupportedCurveModel);
    DRW_SabParseLimits oneUnderOrderedUnsupportedCurveRowBudget;
    if (orderedUnsupportedCurveRowBudget
            != std::numeric_limits<std::size_t>::max()
        && orderedUnsupportedCurveRowBudget > 0u) {
        oneUnderOrderedUnsupportedCurveRowBudget.maxOutputItems =
            orderedUnsupportedCurveRowBudget - 1u;
    }
    std::vector<DRW_AcisBodyProjection>
        rejectedOrderedUnsupportedCurveRowBudget(1u);
    DRW_AcisRootedProjectionReport
        rejectedOrderedUnsupportedCurveRowBudgetReport;
    const bool orderedUnsupportedCurveRowFitsOneUnderBudget =
        drw_extractAcisBodyWireframes(
            orderedUnsupportedCurveModel,
            rejectedOrderedUnsupportedCurveRowBudget,
            rejectedOrderedUnsupportedCurveRowBudgetReport,
            oneUnderOrderedUnsupportedCurveRowBudget);
    t.expect(orderedUnsupportedCurvePointerCount == 3u
                 && noOrderedUnsupportedCurveRowParsed
                 && noOrderedUnsupportedCurveRowBudget
                        != std::numeric_limits<std::size_t>::max()
                 && orderedUnsupportedCurveRowBudget
                        == noOrderedUnsupportedCurveRowBudget + 3u
                 && !orderedUnsupportedCurveRowFitsOneUnderBudget
                 && rejectedOrderedUnsupportedCurveRowBudget.empty()
                 && rejectedOrderedUnsupportedCurveRowBudgetReport.failure
                        == DRW_AcisProjectionFailure::ResourceLimit,
             "ACIS output budget charges every ordered unsupported-curve pointer slot and refuses one item below the exact bound");

    const auto minimumExtractionWorkBudget = [](const DRW_AcisModel& model) {
        const std::size_t invalidBudget =
            std::numeric_limits<std::size_t>::max();
        DRW_SabParseLimits defaults;
        std::size_t low = 0u;
        std::size_t high = defaults.maxExtractionWorkUnits;
        while (low < high) {
            const std::size_t candidate = low + (high - low) / 2u;
            DRW_SabParseLimits limits;
            limits.maxExtractionWorkUnits = candidate;
            std::vector<DRW_AcisBodyProjection> projections;
            DRW_AcisRootedProjectionReport report;
            if (drw_extractAcisBodyWireframes(model, projections, report,
                                              limits)) {
                high = candidate;
            } else if (report.failure
                       == DRW_AcisProjectionFailure::ResourceLimit) {
                low = candidate + 1u;
            } else {
                return invalidBudget;
            }
        }
        DRW_SabParseLimits exactLimits;
        exactLimits.maxExtractionWorkUnits = low;
        std::vector<DRW_AcisBodyProjection> exactProjections;
        DRW_AcisRootedProjectionReport exactReport;
        return drw_extractAcisBodyWireframes(
                   model, exactProjections, exactReport, exactLimits)
            ? low : invalidBudget;
    };
    const std::size_t noOrderedUnsupportedCurveWorkBudget =
        minimumExtractionWorkBudget(noOrderedUnsupportedCurveRowModel);
    const std::size_t orderedUnsupportedCurveWorkBudget =
        minimumExtractionWorkBudget(orderedUnsupportedCurveModel);
    DRW_SabParseLimits oneUnderOrderedUnsupportedCurveWorkBudget;
    if (orderedUnsupportedCurveWorkBudget
            != std::numeric_limits<std::size_t>::max()
        && orderedUnsupportedCurveWorkBudget > 0u) {
        oneUnderOrderedUnsupportedCurveWorkBudget.maxExtractionWorkUnits =
            orderedUnsupportedCurveWorkBudget - 1u;
    }
    std::vector<DRW_AcisBodyProjection>
        rejectedOrderedUnsupportedCurveWorkBudget(1u);
    DRW_AcisRootedProjectionReport
        rejectedOrderedUnsupportedCurveWorkBudgetReport;
    const bool orderedUnsupportedCurveWithinOneUnderWorkBudget =
        drw_extractAcisBodyWireframes(
            orderedUnsupportedCurveModel,
            rejectedOrderedUnsupportedCurveWorkBudget,
            rejectedOrderedUnsupportedCurveWorkBudgetReport,
            oneUnderOrderedUnsupportedCurveWorkBudget);
    t.expect(noOrderedUnsupportedCurveWorkBudget
                    != std::numeric_limits<std::size_t>::max()
                 && orderedUnsupportedCurveWorkBudget
                        == noOrderedUnsupportedCurveWorkBudget + 3u
                 && !orderedUnsupportedCurveWithinOneUnderWorkBudget
                 && rejectedOrderedUnsupportedCurveWorkBudget.empty()
                 && rejectedOrderedUnsupportedCurveWorkBudgetReport.failure
                        == DRW_AcisProjectionFailure::ResourceLimit,
             "ACIS extraction-work budget charges every ordered unsupported-curve pointer slot and refuses one unit below the exact bound");

    DRW_SabParseLimits wireProjectionWorkLimit;
    wireProjectionWorkLimit.maxExtractionWorkUnits = 0u;
    std::vector<DRW_AcisBodyProjection> rejectedWireProjections(1u);
    DRW_AcisRootedProjectionReport rejectedWireProjectionReport;
    const bool wireProjectionWithinZeroBudget = drw_extractAcisBodyWireframes(
        parsedComplexModel, rejectedWireProjections,
        rejectedWireProjectionReport, wireProjectionWorkLimit);
    t.expect(parsedComplexProjected && !wireProjectionWithinZeroBudget
                 && rejectedWireProjections.empty()
                 && rejectedWireProjectionReport.failure
                        == DRW_AcisProjectionFailure::ResourceLimit,
             "parsed complex SAB graph refuses rooted traversal at zero extraction-work budget");

    struct WireNonFiniteLocationCase {
        double value;
        const char* label;
    };
    const std::array<WireNonFiniteLocationCase, 3> nonFiniteLocationCases {{
        {std::numeric_limits<double>::quiet_NaN(), "NaN"},
        {std::numeric_limits<double>::infinity(), "+infinity"},
        {-std::numeric_limits<double>::infinity(), "-infinity"}
    }};
    const std::array<const char*, 3> coordinateNames {{"x", "y", "z"}};
    for (const WireNonFiniteLocationCase& invalidLocation :
         nonFiniteLocationCases) {
        for (std::size_t component = 0u; component < coordinateNames.size();
             ++component) {
            AcisGraphFixture invalidLocationFixture = complexFixture;
            bool locationChanged = false;
            for (DRW_SabToken& token : invalidLocationFixture.data.records[
                     static_cast<std::size_t>(root.point0)].tokens) {
                if (token.tag != DRW_SabTag::LocationVec)
                    continue;
                if (component == 0u)
                    token.vec.x = invalidLocation.value;
                else if (component == 1u)
                    token.vec.y = invalidLocation.value;
                else
                  token.vec.z = invalidLocation.value;
                locationChanged = true;
                break;
            }
            const std::vector<std::uint8_t> invalidLocationBytes =
                sab21800FromGraph(invalidLocationFixture.data);
            DRW_SabData invalidLocationSab;
            const bool invalidLocationParsed =
                locationChanged && !invalidLocationBytes.empty() &&
                drw_parseSab(invalidLocationBytes.data(),
                             invalidLocationBytes.size(), invalidLocationSab);
            const DRW_AcisModel invalidLocationModel =
                drw_buildAcisModel(invalidLocationSab);
            std::vector<DRW_AcisBodyProjection> invalidLocationProjections(1u);
            DRW_AcisRootedProjectionReport invalidLocationReport;
            const bool invalidLocationProjected = drw_extractAcisBodyWireframes(
                invalidLocationModel, invalidLocationProjections,
                invalidLocationReport);
            t.expect(invalidLocationParsed && !invalidLocationProjected &&
                         invalidLocationProjections.empty() &&
                         invalidLocationReport.failure ==
                             DRW_AcisProjectionFailure::InvalidModel &&
                         invalidLocationReport.failedNodeIndex == root.point0,
                     std::string("SAB 21800 parser-fed non-finite point ") +
                         invalidLocation.label + " coordinate " +
                         coordinateNames[component] +
                         " refuses rooted projection transactionally");
        }
    }

    for (std::size_t component = 0u; component < coordinateNames.size();
         ++component) {
      for (const WireNonFiniteLocationCase &invalidSurfaceOrigin :
           nonFiniteLocationCases) {
        AcisGraphFixture invalidSurfaceFixture = complexFixture;
        double originComponents[] = {0.0, 0.0, 0.0};
        originComponents[component] = invalidSurfaceOrigin.value;
        invalidSurfaceFixture.data
            .records[static_cast<std::size_t>(unsupportedSurface)] = acisRecord(
            "plane-surface",
            {acisPointer(-1),
             acisLocation(originComponents[0], originComponents[1],
                          originComponents[2]),
             acisDirection(0.0, 0.0, 1.0), acisDirection(1.0, 0.0, 0.0)});
        const std::vector<std::uint8_t> invalidSurfaceBytes =
            sab21800FromGraph(invalidSurfaceFixture.data);
        DRW_SabData invalidSurfaceSab;
        const bool invalidSurfaceParsed =
            !invalidSurfaceBytes.empty() &&
            drw_parseSab(invalidSurfaceBytes.data(), invalidSurfaceBytes.size(),
                         invalidSurfaceSab);
        const DRW_AcisModel invalidSurfaceModel =
            drw_buildAcisModel(invalidSurfaceSab);
        std::vector<DRW_AcisBodyProjection> invalidSurfaceProjections(1u);
        DRW_AcisRootedProjectionReport invalidSurfaceReport;
        const bool invalidSurfaceProjected = drw_extractAcisBodyWireframes(
            invalidSurfaceModel, invalidSurfaceProjections,
            invalidSurfaceReport);
        t.expect(invalidSurfaceParsed && !invalidSurfaceProjected &&
                     invalidSurfaceProjections.empty() &&
                     invalidSurfaceReport.failure ==
                         DRW_AcisProjectionFailure::InvalidModel &&
                     invalidSurfaceReport.failedNodeIndex == unsupportedSurface,
                 std::string("SAB 21800 parser-fed non-finite plane origin ") +
                     coordinateNames[component] + " " +
                     invalidSurfaceOrigin.label +
                     " refuses at its surface node transactionally");
      }
    }

    struct WireNonFinitePlaneVectorCase {
      int tag;
      std::size_t occurrence;
      const char *field;
      double value;
      const char *label;
    };
    const std::array<WireNonFinitePlaneVectorCase, 6> nonFinitePlaneVectorCases{
        {{DRW_SabTag::DirectionVec, 0u, "normal",
          std::numeric_limits<double>::quiet_NaN(), "NaN"},
         {DRW_SabTag::DirectionVec, 0u, "normal",
          std::numeric_limits<double>::infinity(), "+infinity"},
         {DRW_SabTag::DirectionVec, 0u, "normal",
          -std::numeric_limits<double>::infinity(), "-infinity"},
         {DRW_SabTag::DirectionVec, 1u, "U direction",
          std::numeric_limits<double>::quiet_NaN(), "NaN"},
         {DRW_SabTag::DirectionVec, 1u, "U direction",
          std::numeric_limits<double>::infinity(), "+infinity"},
         {DRW_SabTag::DirectionVec, 1u, "U direction",
          -std::numeric_limits<double>::infinity(), "-infinity"}}};
    for (std::size_t component = 0u; component < coordinateNames.size();
         ++component) {
      for (const WireNonFinitePlaneVectorCase &invalidPlaneVector :
           nonFinitePlaneVectorCases) {
        AcisGraphFixture invalidPlaneVectorFixture = complexFixture;
        DRW_SabRecord &planeRecord =
            invalidPlaneVectorFixture.data
                .records[static_cast<std::size_t>(unsupportedSurface)];
        planeRecord = acisRecord("plane-surface",
                                 {acisPointer(-1), acisLocation(0.0, 0.0, 0.0),
                                  acisDirection(0.0, 0.0, 1.0),
                                  acisDirection(1.0, 0.0, 0.0)});
        std::size_t matchingVectors = 0;
        bool vectorChanged = false;
        for (DRW_SabToken &token : planeRecord.tokens) {
          if (token.tag != invalidPlaneVector.tag)
            continue;
          if (matchingVectors++ == invalidPlaneVector.occurrence) {
            if (component == 0u)
              token.vec.x = invalidPlaneVector.value;
            else if (component == 1u)
              token.vec.y = invalidPlaneVector.value;
            else
              token.vec.z = invalidPlaneVector.value;
            vectorChanged = true;
            break;
          }
        }
        const std::vector<std::uint8_t> invalidPlaneVectorBytes =
            sab21800FromGraph(invalidPlaneVectorFixture.data);
        DRW_SabData invalidPlaneVectorSab;
        const bool invalidPlaneVectorParsed =
            vectorChanged && !invalidPlaneVectorBytes.empty() &&
            drw_parseSab(invalidPlaneVectorBytes.data(),
                         invalidPlaneVectorBytes.size(), invalidPlaneVectorSab);
        const DRW_AcisModel invalidPlaneVectorModel =
            drw_buildAcisModel(invalidPlaneVectorSab);
        std::vector<DRW_AcisBodyProjection> invalidPlaneVectorProjections(1u);
        DRW_AcisRootedProjectionReport invalidPlaneVectorReport;
        const bool invalidPlaneVectorProjected = drw_extractAcisBodyWireframes(
            invalidPlaneVectorModel, invalidPlaneVectorProjections,
            invalidPlaneVectorReport);
        t.expect(
            invalidPlaneVectorParsed && !invalidPlaneVectorProjected &&
                invalidPlaneVectorProjections.empty() &&
                invalidPlaneVectorReport.failure ==
                    DRW_AcisProjectionFailure::InvalidModel &&
                invalidPlaneVectorReport.failedNodeIndex == unsupportedSurface,
            std::string("SAB 21800 parser-fed non-finite plane ") +
                invalidPlaneVector.field + " " + coordinateNames[component] +
                " " + invalidPlaneVector.label +
                " refuses at its surface node transactionally");
      }
    }

    struct WireNonFiniteStraightCurveVectorCase {
      int tag;
      std::size_t occurrence;
      const char *field;
      double value;
      const char *label;
    };
    const std::array<WireNonFiniteStraightCurveVectorCase, 6>
        nonFiniteStraightCurveVectorCases{
            {{DRW_SabTag::LocationVec, 0u, "origin",
              std::numeric_limits<double>::quiet_NaN(), "NaN"},
             {DRW_SabTag::LocationVec, 0u, "origin",
              std::numeric_limits<double>::infinity(), "+infinity"},
             {DRW_SabTag::LocationVec, 0u, "origin",
              -std::numeric_limits<double>::infinity(), "-infinity"},
             {DRW_SabTag::DirectionVec, 0u, "direction",
              std::numeric_limits<double>::quiet_NaN(), "NaN"},
             {DRW_SabTag::DirectionVec, 0u, "direction",
              std::numeric_limits<double>::infinity(), "+infinity"},
             {DRW_SabTag::DirectionVec, 0u, "direction",
              -std::numeric_limits<double>::infinity(), "-infinity"}}};
    for (std::size_t component = 0u; component < coordinateNames.size();
         ++component) {
      for (const WireNonFiniteStraightCurveVectorCase &invalidCurveVector :
           nonFiniteStraightCurveVectorCases) {
        AcisGraphFixture invalidCurveVectorFixture = complexFixture;
        DRW_SabRecord &curveRecord =
            invalidCurveVectorFixture.data
                .records[static_cast<std::size_t>(unsupportedCurve)];
        curveRecord = acisRecord(
            "straight-curve", {acisPointer(-1), acisLocation(10.0, 20.0, 30.0),
                               acisDirection(1.0, 0.0, 0.0)});
        std::size_t matchingVectors = 0;
        bool vectorChanged = false;
        for (DRW_SabToken &token : curveRecord.tokens) {
          if (token.tag != invalidCurveVector.tag)
            continue;
          if (matchingVectors++ == invalidCurveVector.occurrence) {
            if (component == 0u)
              token.vec.x = invalidCurveVector.value;
            else if (component == 1u)
              token.vec.y = invalidCurveVector.value;
            else
              token.vec.z = invalidCurveVector.value;
            vectorChanged = true;
            break;
          }
        }
        const std::vector<std::uint8_t> invalidCurveVectorBytes =
            sab21800FromGraph(invalidCurveVectorFixture.data);
        DRW_SabData invalidCurveVectorSab;
        const bool invalidCurveVectorParsed =
            vectorChanged && !invalidCurveVectorBytes.empty() &&
            drw_parseSab(invalidCurveVectorBytes.data(),
                         invalidCurveVectorBytes.size(), invalidCurveVectorSab);
        const DRW_AcisModel invalidCurveVectorModel =
            drw_buildAcisModel(invalidCurveVectorSab);
        std::vector<DRW_AcisBodyProjection> invalidCurveVectorProjections(1u);
        DRW_AcisRootedProjectionReport invalidCurveVectorReport;
        const bool invalidCurveVectorProjected = drw_extractAcisBodyWireframes(
            invalidCurveVectorModel, invalidCurveVectorProjections,
            invalidCurveVectorReport);
        t.expect(
            invalidCurveVectorParsed && !invalidCurveVectorProjected &&
                invalidCurveVectorProjections.empty() &&
                invalidCurveVectorReport.failure ==
                    DRW_AcisProjectionFailure::InvalidModel &&
                invalidCurveVectorReport.failedNodeIndex == unsupportedCurve,
            std::string("SAB 21800 parser-fed non-finite straight-curve ") +
                invalidCurveVector.field + " " + coordinateNames[component] +
                " " + invalidCurveVector.label +
                " refuses rooted projection transactionally");
      }
    }

    struct WireEllipseFiniteField {
      int tag;
      std::size_t occurrence;
      int component;
      const char *name;
    };
    const std::array<WireEllipseFiniteField, 10> ellipseFiniteFields{
        {{DRW_SabTag::LocationVec, 0u, 0, "center"},
         {DRW_SabTag::LocationVec, 0u, 1, "center"},
         {DRW_SabTag::LocationVec, 0u, 2, "center"},
         {DRW_SabTag::DirectionVec, 0u, 0, "normal"},
         {DRW_SabTag::DirectionVec, 0u, 1, "normal"},
         {DRW_SabTag::DirectionVec, 0u, 2, "normal"},
         {DRW_SabTag::DirectionVec, 1u, 0, "major axis"},
         {DRW_SabTag::DirectionVec, 1u, 1, "major axis"},
         {DRW_SabTag::DirectionVec, 1u, 2, "major axis"},
         {DRW_SabTag::Double, 0u, -1, "ratio"}}};
    for (const WireEllipseFiniteField &finiteField : ellipseFiniteFields) {
      for (const WireNonFiniteLocationCase &invalidValue :
           nonFiniteLocationCases) {
        AcisGraphFixture invalidEllipseFixture = complexFixture;
        DRW_SabRecord &ellipseRecord =
            invalidEllipseFixture.data
                .records[static_cast<std::size_t>(unsupportedCurve)];
        ellipseRecord = acisRecord(
            "ellipse-curve", {acisPointer(-1), acisLocation(0.0, 0.0, 5.0),
                              acisDirection(0.0, 0.0, 1.0),
                              acisDirection(3.0, 0.0, 0.0), acisDouble(0.5)});
        std::size_t matchingFields = 0;
        bool fieldChanged = false;
        for (DRW_SabToken &token : ellipseRecord.tokens) {
          if (token.tag != finiteField.tag)
            continue;
          if (matchingFields++ != finiteField.occurrence)
            continue;
          if (finiteField.component < 0) {
            token.dval = invalidValue.value;
          } else if (finiteField.component == 0) {
            token.vec.x = invalidValue.value;
          } else if (finiteField.component == 1) {
            token.vec.y = invalidValue.value;
          } else {
            token.vec.z = invalidValue.value;
          }
          fieldChanged = true;
          break;
        }
        const std::vector<std::uint8_t> invalidEllipseBytes =
            sab21800FromGraph(invalidEllipseFixture.data);
        DRW_SabData invalidEllipseSab;
        const bool invalidEllipseParsed =
            fieldChanged && !invalidEllipseBytes.empty() &&
            drw_parseSab(invalidEllipseBytes.data(), invalidEllipseBytes.size(),
                         invalidEllipseSab);
        const DRW_AcisModel invalidEllipseModel =
            drw_buildAcisModel(invalidEllipseSab);
        std::vector<DRW_AcisBodyProjection> invalidEllipseProjections(1u);
        DRW_AcisRootedProjectionReport invalidEllipseReport;
        const bool invalidEllipseProjected = drw_extractAcisBodyWireframes(
            invalidEllipseModel, invalidEllipseProjections,
            invalidEllipseReport);
        t.expect(invalidEllipseParsed && !invalidEllipseProjected &&
                     invalidEllipseProjections.empty() &&
                     invalidEllipseReport.failure ==
                         DRW_AcisProjectionFailure::InvalidModel &&
                     invalidEllipseReport.failedNodeIndex == unsupportedCurve,
                 std::string("SAB 21800 parser-fed non-finite ellipse ") +
                     finiteField.name + " " +
                     (finiteField.component >= 0
                          ? coordinateNames[static_cast<std::size_t>(
                                finiteField.component)] +
                                std::string(" ")
                          : std::string()) +
                     invalidValue.label +
                     " refuses rooted projection transactionally");
      }
    }

    struct WireSurfaceFiniteField {
      const char *surfaceType;
      int tag;
      std::size_t occurrence;
      bool vector;
      const char *name;
    };
    const std::array<WireSurfaceFiniteField, 11> surfaceFiniteFields{
        {{"cone-surface", DRW_SabTag::LocationVec, 0u, true, "origin"},
         {"cone-surface", DRW_SabTag::DirectionVec, 0u, true, "axis"},
         {"cone-surface", DRW_SabTag::DirectionVec, 1u, true, "major axis"},
         {"cone-surface", DRW_SabTag::Double, 0u, false, "ratio"},
         {"cone-surface", DRW_SabTag::Double, 1u, false, "sine angle"},
         {"cone-surface", DRW_SabTag::Double, 2u, false, "cosine angle"},
         {"torus-surface", DRW_SabTag::LocationVec, 0u, true, "center"},
         {"torus-surface", DRW_SabTag::DirectionVec, 0u, true, "axis"},
         {"torus-surface", DRW_SabTag::Double, 0u, false, "major radius"},
         {"torus-surface", DRW_SabTag::Double, 1u, false, "minor radius"},
         {"torus-surface", DRW_SabTag::DirectionVec, 1u, true, "U direction"}}};
    for (const WireSurfaceFiniteField &finiteField : surfaceFiniteFields) {
      const std::size_t componentCount =
          finiteField.vector ? coordinateNames.size() : 1u;
      for (std::size_t component = 0u; component < componentCount;
           ++component) {
        for (const WireNonFiniteLocationCase &invalidValue :
             nonFiniteLocationCases) {
          AcisGraphFixture invalidSurfaceFixture = complexFixture;
          std::vector<DRW_SabToken> surfaceTokens{acisPointer(-1)};
          if (std::string(finiteField.surfaceType) == "cone-surface") {
            surfaceTokens.insert(surfaceTokens.end(),
                                 {acisLocation(0.0, 0.0, 0.0),
                                  acisDirection(0.0, 0.0, 1.0),
                                  acisDirection(1.0, 0.0, 0.0), acisDouble(0.5),
                                  acisDouble(0.3), acisDouble(0.95)});
          } else if (std::string(finiteField.surfaceType) == "torus-surface") {
            surfaceTokens.insert(surfaceTokens.end(),
                                 {acisLocation(0.0, 0.0, 0.0),
                                  acisDirection(0.0, 0.0, 1.0),
                                  acisDouble(10.0), acisDouble(2.0),
                                  acisDirection(1.0, 0.0, 0.0)});
          } else {
            surfaceTokens.insert(surfaceTokens.end(),
                                 {acisLocation(0.0, 0.0, 0.0),
                                  acisDirection(0.0, 0.0, 1.0),
                                  acisDouble(10.0), acisDouble(2.0)});
          }
          DRW_SabRecord &surfaceRecord =
              invalidSurfaceFixture.data
                  .records[static_cast<std::size_t>(unsupportedSurface)];
          surfaceRecord =
              acisRecord(finiteField.surfaceType, std::move(surfaceTokens));
          std::size_t matchingFields = 0;
          bool fieldChanged = false;
          for (DRW_SabToken &token : surfaceRecord.tokens) {
            if (token.tag != finiteField.tag)
              continue;
            if (matchingFields++ != finiteField.occurrence)
              continue;
            if (!finiteField.vector)
              token.dval = invalidValue.value;
            else if (component == 0u)
              token.vec.x = invalidValue.value;
            else if (component == 1u)
              token.vec.y = invalidValue.value;
            else
              token.vec.z = invalidValue.value;
            fieldChanged = true;
            break;
          }
          const std::vector<std::uint8_t> invalidSurfaceBytes =
              sab21800FromGraph(invalidSurfaceFixture.data);
          DRW_SabData invalidSurfaceSab;
          const bool invalidSurfaceParsed =
              fieldChanged && !invalidSurfaceBytes.empty() &&
              drw_parseSab(invalidSurfaceBytes.data(),
                           invalidSurfaceBytes.size(), invalidSurfaceSab);
          const DRW_AcisModel invalidSurfaceModel =
              drw_buildAcisModel(invalidSurfaceSab);
          std::vector<DRW_AcisBodyProjection> invalidSurfaceProjections(1u);
          DRW_AcisRootedProjectionReport invalidSurfaceReport;
          const bool invalidSurfaceProjected = drw_extractAcisBodyWireframes(
              invalidSurfaceModel, invalidSurfaceProjections,
              invalidSurfaceReport);
          t.expect(invalidSurfaceParsed && !invalidSurfaceProjected &&
                       invalidSurfaceProjections.empty() &&
                       invalidSurfaceReport.failure ==
                           DRW_AcisProjectionFailure::InvalidModel &&
                       invalidSurfaceReport.failedNodeIndex ==
                           unsupportedSurface,
                   std::string("SAB 21800 parser-fed non-finite ") +
                       finiteField.surfaceType + " " + finiteField.name +
                       (finiteField.vector
                            ? std::string(" ") + coordinateNames[component]
                            : std::string()) +
                       " " + invalidValue.label +
                       " refuses at its surface node transactionally");
        }
      }
    }

    for (std::size_t controlPointComponent = 0u; controlPointComponent < 6u;
         ++controlPointComponent) {
      for (const WireNonFiniteLocationCase &invalidValue :
           nonFiniteLocationCases) {
        AcisGraphFixture invalidIntcurveFixture = complexFixture;
        DRW_SabRecord &intcurveRecord =
            invalidIntcurveFixture.data
                .records[static_cast<std::size_t>(unsupportedCurve)];
        intcurveRecord =
            acisRecord("intcurve-curve",
                       {acisPointer(-1), acisEntityType("nubs"), acisInteger(2),
                        acisDouble(0.0), acisDouble(1.0), acisDouble(7.0),
                        acisDouble(8.0), acisDouble(9.0), acisDouble(10.0),
                        acisDouble(11.0), acisDouble(12.0)});
        std::size_t doubleOccurrence = 0u;
        bool controlPointChanged = false;
        for (DRW_SabToken &token : intcurveRecord.tokens) {
          if (token.tag != DRW_SabTag::Double)
            continue;
          const std::size_t currentDouble = doubleOccurrence++;
          if (currentDouble != controlPointComponent + 2u)
            continue;
          token.dval = invalidValue.value;
          controlPointChanged = true;
          break;
        }
        const std::vector<std::uint8_t> invalidIntcurveBytes =
            sab21800FromGraph(invalidIntcurveFixture.data);
        DRW_SabData invalidIntcurveSab;
        const bool invalidIntcurveParsed =
            controlPointChanged && !invalidIntcurveBytes.empty() &&
            drw_parseSab(invalidIntcurveBytes.data(),
                         invalidIntcurveBytes.size(), invalidIntcurveSab);
        const DRW_AcisModel invalidIntcurveModel =
            drw_buildAcisModel(invalidIntcurveSab);
        std::vector<DRW_AcisBodyProjection> invalidIntcurveProjections(1u);
        DRW_AcisRootedProjectionReport invalidIntcurveReport;
        const bool invalidIntcurveProjected = drw_extractAcisBodyWireframes(
            invalidIntcurveModel, invalidIntcurveProjections,
            invalidIntcurveReport);
        const std::string intcurveLabel =
            std::string("SAB 21800 parser-fed non-finite intcurve ") +
            "control point component " + std::to_string(controlPointComponent) +
            " " + invalidValue.label;
        t.expect(controlPointChanged,
                 intcurveLabel + " targets a control-point token");
        t.expect(invalidIntcurveParsed, intcurveLabel + " parses and builds");
        t.expect(!invalidIntcurveProjected &&
                     invalidIntcurveProjections.empty() &&
                     invalidIntcurveReport.failure ==
                         DRW_AcisProjectionFailure::InvalidModel &&
                     invalidIntcurveReport.failedNodeIndex == unsupportedCurve,
                 intcurveLabel + " refuses rooted projection transactionally");
      }
    }

    struct WireNonFiniteTransformCase {
        const char* value;
        const char* label;
    };
    const std::array<WireNonFiniteTransformCase, 3> nonFiniteTransformCases {{
        {"nan", "NaN"}, {"inf", "+infinity"}, {"-inf", "-infinity"}
    }};
    const auto checkNonFiniteTransformCandidate = [
        &t, &complexFixture, &root](
            std::size_t componentCount, std::size_t componentIndex,
            const WireNonFiniteTransformCase& invalidTransform) {
            AcisGraphFixture invalidTransformFixture = complexFixture;
            std::string transformLiteral;
            for (std::size_t index = 0u; index < componentCount; ++index) {
                if (index != 0u)
                    transformLiteral += ' ';
                transformLiteral += index == componentIndex
                    ? invalidTransform.value
                    : std::to_string(index + 1u);
            }
            bool transformLiteralChanged = false;
            for (DRW_SabToken& token : invalidTransformFixture.data.records[
                     static_cast<std::size_t>(root.transform)].tokens) {
                if (token.tag == DRW_SabTag::LiteralStr) {
                    token.sval = transformLiteral;
                    transformLiteralChanged = true;
                    break;
                }
            }
            const std::vector<std::uint8_t> invalidTransformBytes =
                sab21800FromGraph(invalidTransformFixture.data);
            DRW_SabData invalidTransformSab;
            const bool invalidTransformParsed = transformLiteralChanged
                && !invalidTransformBytes.empty()
                && drw_parseSab(invalidTransformBytes.data(),
                                invalidTransformBytes.size(),
                                invalidTransformSab);
            const DRW_AcisModel invalidTransformModel =
                drw_buildAcisModel(invalidTransformSab);
            std::vector<DRW_AcisBodyProjection> invalidTransformProjections;
            DRW_AcisRootedProjectionReport invalidTransformReport;
            const bool invalidTransformProjected = drw_extractAcisBodyWireframes(
                invalidTransformModel, invalidTransformProjections,
                invalidTransformReport);
            const bool transformCandidateRefused = invalidTransformProjected
                && invalidTransformProjections.size() == 1u
                && invalidTransformProjections.front().transform.state
                    == DRW_AcisTransformState::Invalid
                && invalidTransformProjections.front().transform
                    .rawLiteralPresent
                && invalidTransformProjections.front().transform.rawLiteral
                    == transformLiteral
                && invalidTransformProjections.front().transform
                    .rawLiteralTokenIndex == 3u
                && !invalidTransformProjections.front().transform
                    .candidateComponentsPresent
                && !invalidTransformProjections.front().report
                    .bodyTransformsApplied
                && invalidTransformReport.failure
                    == DRW_AcisProjectionFailure::NoFailure;
            t.expect(invalidTransformParsed && transformCandidateRefused,
                     std::string("SAB 21800 parser-fed non-finite transform ")
                         + "component " + std::to_string(componentIndex + 1u)
                         + " of " + std::to_string(componentCount) + " "
                         + invalidTransform.label
                         + " remains raw and unapplied with an invalid candidate state");
    };
    for (std::size_t componentIndex = 0u; componentIndex < 12u;
         ++componentIndex) {
        for (const WireNonFiniteTransformCase& invalidTransform :
             nonFiniteTransformCases) {
            checkNonFiniteTransformCandidate(
                12u, componentIndex, invalidTransform);
        }
    }
    for (const WireNonFiniteTransformCase& invalidTransform :
         nonFiniteTransformCases) {
        checkNonFiniteTransformCandidate(13u, 12u, invalidTransform);
    }

    AcisGraphFixture overlongTransformFixture = complexFixture;
    std::string overlongTransformLiteral;
    for (std::size_t index = 0u; index < 14u; ++index) {
        if (index != 0u)
            overlongTransformLiteral += ' ';
        overlongTransformLiteral += std::to_string(index + 1u);
    }
    bool overlongTransformLiteralChanged = false;
    for (DRW_SabToken& token : overlongTransformFixture.data.records[
             static_cast<std::size_t>(root.transform)].tokens) {
        if (token.tag == DRW_SabTag::LiteralStr) {
            token.sval = overlongTransformLiteral;
            overlongTransformLiteralChanged = true;
            break;
        }
    }
    const std::vector<std::uint8_t> overlongTransformBytes =
        sab21800FromGraph(overlongTransformFixture.data);
    DRW_SabData overlongTransformSab;
    const bool overlongTransformParsed = overlongTransformLiteralChanged
        && !overlongTransformBytes.empty()
        && drw_parseSab(overlongTransformBytes.data(),
                        overlongTransformBytes.size(), overlongTransformSab);
    const DRW_AcisModel overlongTransformModel =
        drw_buildAcisModel(overlongTransformSab);
    std::vector<DRW_AcisBodyProjection> overlongTransformProjections;
    DRW_AcisRootedProjectionReport overlongTransformReport;
    const bool overlongTransformProjected = drw_extractAcisBodyWireframes(
        overlongTransformModel, overlongTransformProjections,
        overlongTransformReport);
    t.expect(overlongTransformParsed && overlongTransformProjected
                 && overlongTransformProjections.size() == 1u
                 && overlongTransformReport.failure
                        == DRW_AcisProjectionFailure::NoFailure
                 && overlongTransformProjections.front().transform.state
                        == DRW_AcisTransformState::Invalid
                 && overlongTransformProjections.front().transform
                        .rawLiteralPresent
                 && overlongTransformProjections.front().transform.rawLiteral
                        == overlongTransformLiteral
                 && overlongTransformProjections.front().transform
                        .rawLiteralTokenIndex == 3u
                 && !overlongTransformProjections.front().transform
                        .candidateComponentsPresent
                 && !overlongTransformProjections.front().report
                        .bodyTransformsApplied,
             "SAB 21800 parser-fed 14-value transform candidate remains raw and invalid");

    AcisGraphFixture vectorTransformFixture = complexFixture;
    vectorTransformFixture.data.records[
        static_cast<std::size_t>(root.transform)].tokens = {
            acisEntityType("transform"),
            acisLocation(1.0, 0.0, 0.0),
            acisLocation(0.0, 1.0, 0.0),
            acisLocation(0.0, 0.0, 1.0),
            acisLocation(11.0, 12.0, 13.0),
            acisDouble(0.5), acisLiteral("rotation-hint")};
    const std::vector<std::uint8_t> vectorTransformBytes =
        sab21800FromGraph(vectorTransformFixture.data);
    DRW_SabData vectorTransformSab;
    const bool vectorTransformParsed = !vectorTransformBytes.empty()
        && drw_parseSab(vectorTransformBytes.data(),
                        vectorTransformBytes.size(), vectorTransformSab);
    const DRW_AcisModel vectorTransformModel =
        drw_buildAcisModel(vectorTransformSab);
    std::vector<DRW_AcisBodyProjection> vectorTransformProjections;
    DRW_AcisRootedProjectionReport vectorTransformReport;
    const bool vectorTransformProjected = drw_extractAcisBodyWireframes(
        vectorTransformModel, vectorTransformProjections,
        vectorTransformReport);
    const DRW_AcisTransformObservation* parsedVectorTransform =
        vectorTransformProjections.size() == 1u
            ? &vectorTransformProjections.front().transform : nullptr;
    t.expect(vectorTransformParsed,
             "SAB 21800 parser accepts generated vector transform grammar");
    t.expect(vectorTransformProjected && parsedVectorTransform != nullptr
                 && vectorTransformReport.failure
                        == DRW_AcisProjectionFailure::NoFailure
                 && !vectorTransformProjections.front()
                        .report.bodyTransformsApplied,
             "SAB 21800 vector transform remains an unapplied structural projection");
    t.expect(parsedVectorTransform != nullptr
                 && parsedVectorTransform->state
                        == DRW_AcisTransformState::UnsupportedVariant
                 && parsedVectorTransform->rawLiteralPresent
                 && parsedVectorTransform->rawLiteral == "rotation-hint"
                 && parsedVectorTransform->rawLiteralTokenIndex == 8u
                 && !parsedVectorTransform->candidateComponentsPresent,
             "SAB 21800 vector transform classifies the raw hint as unsupported without a numeric candidate");
    t.expect(parsedVectorTransform != nullptr
                 && parsedVectorTransform->rawTokens.size() == 9u
                 && parsedVectorTransform->rawTokens[0].tag
                        == DRW_SabTag::EntityType
                 && parsedVectorTransform->rawTokens[1].tag
                        == DRW_SabTag::Pointer
                 && parsedVectorTransform->rawTokens[2].tag == DRW_SabTag::Int
                 && parsedVectorTransform->rawTokens[3].tag
                        == DRW_SabTag::LocationVec
                 && parsedVectorTransform->rawTokens[4].tag
                        == DRW_SabTag::LocationVec
                 && parsedVectorTransform->rawTokens[5].tag
                        == DRW_SabTag::LocationVec
                 && parsedVectorTransform->rawTokens[6].tag
                        == DRW_SabTag::LocationVec
                 && parsedVectorTransform->rawTokens[7].tag
                        == DRW_SabTag::Double
                 && parsedVectorTransform->rawTokens[8].tag
                        == DRW_SabTag::LiteralStr
                 && acisCoordNear(parsedVectorTransform->rawTokens[6].vec,
                                  11.0, 12.0, 13.0)
                 && std::fabs(parsedVectorTransform->rawTokens[7].dval - 0.5)
                        < 1e-9,
             "SAB 21800 vector transform preserves common header and semantic token order");

    AcisGraphFixture nonFiniteVectorTransformFixture = vectorTransformFixture;
    std::vector<DRW_SabToken>& nonFiniteVectorTransformTokens =
        nonFiniteVectorTransformFixture.data.records[
            static_cast<std::size_t>(root.transform)].tokens;
    nonFiniteVectorTransformTokens[1].vec.x =
        std::numeric_limits<double>::quiet_NaN();
    nonFiniteVectorTransformTokens[5].dval =
        std::numeric_limits<double>::infinity();
    const std::vector<std::uint8_t> nonFiniteVectorTransformBytes =
        sab21800FromGraph(nonFiniteVectorTransformFixture.data);
    DRW_SabData nonFiniteVectorTransformSab;
    const bool nonFiniteVectorTransformParsed =
        !nonFiniteVectorTransformBytes.empty()
        && drw_parseSab(nonFiniteVectorTransformBytes.data(),
                        nonFiniteVectorTransformBytes.size(),
                        nonFiniteVectorTransformSab);
    const DRW_AcisModel nonFiniteVectorTransformModel =
        drw_buildAcisModel(nonFiniteVectorTransformSab);
    std::vector<DRW_AcisBodyProjection> nonFiniteVectorTransformProjections;
    DRW_AcisRootedProjectionReport nonFiniteVectorTransformReport;
    const bool nonFiniteVectorTransformProjected = drw_extractAcisBodyWireframes(
        nonFiniteVectorTransformModel,
        nonFiniteVectorTransformProjections,
        nonFiniteVectorTransformReport);
    const DRW_AcisTransformObservation* parsedNonFiniteVectorTransform =
        nonFiniteVectorTransformProjections.size() == 1u
            ? &nonFiniteVectorTransformProjections.front().transform : nullptr;
    t.expect(nonFiniteVectorTransformParsed
                 && nonFiniteVectorTransformProjected
                 && parsedNonFiniteVectorTransform != nullptr
                 && nonFiniteVectorTransformReport.failure
                        == DRW_AcisProjectionFailure::NoFailure
                 && parsedNonFiniteVectorTransform->state
                        == DRW_AcisTransformState::UnsupportedVariant
                 && parsedNonFiniteVectorTransform->rawTokens.size() == 9u
                 && parsedNonFiniteVectorTransform->rawLiteralPresent
                 && parsedNonFiniteVectorTransform->rawLiteral
                        == "rotation-hint"
                 && std::isnan(parsedNonFiniteVectorTransform
                                   ->rawTokens[3].vec.x)
                 && std::isinf(parsedNonFiniteVectorTransform
                                   ->rawTokens[7].dval)
                 && !parsedNonFiniteVectorTransform
                        ->candidateComponentsPresent
                 && !nonFiniteVectorTransformProjections.front()
                        .report.bodyTransformsApplied,
             "SAB 21800 parser-fed non-finite vector-transform tokens remain raw and unapplied as an unsupported grammar");

    AcisGraphFixture wrongOwnerBase = complexFixture;
    const int wrongOwnerBody = wrongOwnerBase.addNode("body", 4u);
    struct WireOwnerMismatchCase {
        int sourceNode;
        std::size_t pointerOrdinal;
        int wrongOwner;
        const char* label;
    };
    const std::array<WireOwnerMismatchCase, 6> ownerMismatchCases {{
        {root.lump, 3u, wrongOwnerBody,
         "lump.body"},
        {root.shell, 5u, secondLump,
         "shell.lump"},
        {root.face, 3u, secondShell,
         "face.shell"},
        {root.loop, 3u, outsideFace,
         "loop.face"},
        {root.coedge, 5u, secondLoop,
         "coedge.loop"},
        {root.coedge, 4u, secondEdge,
         "edge.coedge reciprocity"}
    }};
    for (const WireOwnerMismatchCase& mismatch : ownerMismatchCases) {
        AcisGraphFixture wrongOwnerFixture = wrongOwnerBase;
        wrongOwnerFixture.link(mismatch.sourceNode, mismatch.pointerOrdinal,
                               mismatch.wrongOwner);
        const std::vector<std::uint8_t> wrongOwnerSabBytes =
            sab21800FromGraph(wrongOwnerFixture.data);
        DRW_SabData wrongOwnerSab;
        const bool wrongOwnerSabParsed = !wrongOwnerSabBytes.empty()
            && drw_parseSab(wrongOwnerSabBytes.data(),
                            wrongOwnerSabBytes.size(), wrongOwnerSab);
        const DRW_AcisModel wrongOwnerModel =
            drw_buildAcisModel(wrongOwnerSab);
        std::vector<DRW_AcisBodyProjection> wrongOwnerProjections(1u);
        DRW_AcisRootedProjectionReport wrongOwnerReport;
        const bool wrongOwnerProjected = drw_extractAcisBodyWireframes(
            wrongOwnerModel, wrongOwnerProjections, wrongOwnerReport);
        t.expect(wrongOwnerSabParsed && !wrongOwnerProjected
                     && wrongOwnerProjections.empty()
                     && wrongOwnerReport.failure
                            == DRW_AcisProjectionFailure::OwnerMismatch
                     && wrongOwnerReport.failedNodeIndex == mismatch.sourceNode
                     && wrongOwnerReport.failedPointerOrdinal
                            == mismatch.pointerOrdinal,
                 std::string("SAB 21800 parser-fed ") + mismatch.label
                     + " owner mismatch refuses transactionally with exact diagnostics");
    }

    AcisGraphFixture invalidReferenceBase = complexFixture;
    const int beyondWireTable = static_cast<int>(
        invalidReferenceBase.data.records.size() + 100u);
    struct WireInvalidReferenceCase {
        int sourceNode;
        std::size_t pointerOrdinal;
        int targetNode;
        const char* label;
    };
    const std::array<WireInvalidReferenceCase, 37> invalidReferenceCases {{
        {root.body, 0u, root.point0, "body-pattern"},
        {root.body, 1u, root.point0, "body-lump"},
        {root.body, 2u, root.point0, "body-wire"},
        {root.body, 3u, root.point0, "body-transform"},
        {root.lump, 0u, root.point0, "lump-pattern"},
        {root.lump, 1u, root.point0, "lump-next"},
        {root.lump, 2u, root.point0, "lump-shell"},
        {root.shell, 0u, root.point0, "shell-pattern"},
        {root.shell, 1u, root.point0, "shell-next"},
        {root.shell, 2u, root.point0, "shell-subshell"},
        {root.shell, 3u, root.point0, "shell-face"},
        {root.shell, 4u, root.point0, "shell-wire"},
        {root.face, 0u, root.point0, "face-pattern"},
        {root.face, 1u, root.point0, "face-next"},
        {root.face, 2u, root.point0, "face-loop"},
        {root.face, 4u, root.point0, "face-subshell"},
        {root.face, 5u, root.point0, "face-surface"},
        {root.loop, 0u, root.point0, "loop-pattern"},
        {root.loop, 1u, root.point0, "loop-next"},
        {root.loop, 2u, root.point0, "loop-coedge"},
        {root.coedge, 0u, root.point0, "coedge-pattern"},
        {root.coedge, 1u, root.point0, "coedge-next"},
        {root.coedge, 2u, root.point0, "coedge-previous"},
        {root.coedge, 3u, root.point0, "coedge-partner"},
        {root.coedge, 4u, root.point0, "coedge-edge"},
        {root.coedge, 6u, root.point0, "coedge-pcurve"},
        {root.edge, 0u, root.point0, "edge-pattern"},
        {root.edge, 1u, root.point0, "edge-start-vertex"},
        {root.edge, 2u, root.point0, "edge-end-vertex"},
        {root.edge, 3u, root.point0, "edge-owner-coedge"},
        {root.edge, 4u, root.point0, "edge-curve"},
        {root.vertex0, 0u, root.point0, "vertex-pattern"},
        {root.vertex0, 1u, root.point0, "vertex-edge"},
        {root.vertex0, 2u, root.edge, "vertex-point"},
        {root.point0, 0u, root.edge, "point-pattern"},
        {root.body, 3u, beyondWireTable, "out-of-table"},
        {root.body, 3u, -2, "invalid-negative"}
    }};
    for (const WireInvalidReferenceCase& invalidReference :
         invalidReferenceCases) {
        AcisGraphFixture invalidReferenceFixture = invalidReferenceBase;
        invalidReferenceFixture.link(invalidReference.sourceNode,
                                     invalidReference.pointerOrdinal,
                                     invalidReference.targetNode);
        const std::vector<std::uint8_t> invalidReferenceSabBytes =
            sab21800FromGraph(invalidReferenceFixture.data);
        DRW_SabData invalidReferenceSab;
        const bool invalidReferenceSabParsed =
            !invalidReferenceSabBytes.empty()
            && drw_parseSab(invalidReferenceSabBytes.data(),
                            invalidReferenceSabBytes.size(),
                            invalidReferenceSab);
        const DRW_AcisModel invalidReferenceModel =
            drw_buildAcisModel(invalidReferenceSab);
        std::vector<DRW_AcisBodyProjection> invalidReferenceProjections(1u);
        DRW_AcisRootedProjectionReport invalidReferenceReport;
        const bool invalidReferenceProjected = drw_extractAcisBodyWireframes(
            invalidReferenceModel, invalidReferenceProjections,
            invalidReferenceReport);
        t.expect(invalidReferenceSabParsed && !invalidReferenceProjected
                     && invalidReferenceProjections.empty()
                     && invalidReferenceReport.failure
                            == DRW_AcisProjectionFailure::InvalidReference
                     && invalidReferenceReport.failedNodeIndex
                            == invalidReference.sourceNode
                     && invalidReferenceReport.failedPointerOrdinal
                            == invalidReference.pointerOrdinal,
                 std::string("SAB 21800 parser-fed ")
                     + invalidReference.label
                     + " reference refuses transactionally with exact diagnostics");
    }

    for (const std::uint32_t unsupportedVersion : {
             20800u, 22300u, 23200u}) {
        std::vector<std::uint8_t> unsupportedVersionBytes =
            rootedVertexBodySab();
        for (std::size_t byteIndex = 0; byteIndex < 4u; ++byteIndex) {
            unsupportedVersionBytes[15u + byteIndex] =
                static_cast<std::uint8_t>(
                    unsupportedVersion >> (byteIndex * 8u));
        }
        DRW_SabData unsupportedVersionSab;
        const bool unsupportedVersionParsed = drw_parseSab(
            unsupportedVersionBytes.data(), unsupportedVersionBytes.size(),
            unsupportedVersionSab);
        const DRW_AcisModel unsupportedVersionModel =
            drw_buildAcisModel(unsupportedVersionSab);
        std::vector<DRW_AcisBodyProjection> unsupportedVersionProjections(1u);
        DRW_AcisRootedProjectionReport unsupportedVersionReport;
        const bool unsupportedVersionProjected = drw_extractAcisBodyWireframes(
            unsupportedVersionModel, unsupportedVersionProjections,
            unsupportedVersionReport);
        t.expect(unsupportedVersionParsed && !unsupportedVersionProjected
                     && unsupportedVersionProjections.empty()
                     && unsupportedVersionReport.failure
                            == DRW_AcisProjectionFailure::UnsupportedProfile,
                 "SAB rooted projection refuses an unreviewed wire save version "
                     + std::to_string(unsupportedVersion));
    }

    AcisGraphFixture emptyFixture;
    emptyFixture.addNode("body", 4u);
    DRW_AcisRootedProjectionReport emptyReport;
    std::vector<DRW_AcisBodyProjection> emptyProjections;
    t.expect(drw_extractAcisBodyWireframes(
                 drw_buildAcisModel(emptyFixture.data), emptyProjections,
                 emptyReport)
                 && emptyReport.bodyCount == 1u
                 && emptyReport.bodiesWithoutSupportedLump == 1u
                 && emptyProjections.size() == 1u
                 && emptyProjections.front().wireframe.empty(),
             "ACIS rooted projection reports an empty body root without inventing geometry");

    const std::vector<std::uint8_t> emptyBodyBytes =
        sab21800FromGraph(emptyFixture.data);
    DRW_SabData emptyBodySab;
    const bool emptyBodyParsed = !emptyBodyBytes.empty()
        && drw_parseSab(emptyBodyBytes.data(), emptyBodyBytes.size(),
                        emptyBodySab);
    const DRW_AcisModel emptyBodyModel = drw_buildAcisModel(emptyBodySab);
    std::vector<DRW_AcisBodyProjection> emptyBodyProjections;
    DRW_AcisRootedProjectionReport emptyBodyReport;
    const bool emptyBodyProjected = drw_extractAcisBodyWireframes(
        emptyBodyModel, emptyBodyProjections, emptyBodyReport);
    t.expect(emptyBodyParsed && emptyBodyProjected
                 && emptyBodySab.records.size() == 2u
                 && emptyBodyReport.bodyCount == 1u
                 && emptyBodyReport.bodiesWithoutSupportedLump == 1u
                 && emptyBodyReport.failure
                        == DRW_AcisProjectionFailure::NoFailure
                 && emptyBodyProjections.size() == 1u
                 && emptyBodyProjections.front().wireframe.empty(),
             "SAB 21800 parser-fed empty body remains a visible non-geometry root");

    AcisGraphFixture sharedFixture;
    const int firstBody = sharedFixture.addNode("body", 4u);
    const int secondBody = sharedFixture.addNode("body", 4u);
    const int sharedLump = sharedFixture.addNode("lump", 4u);
    sharedFixture.link(firstBody, 1u, sharedLump);
    sharedFixture.link(secondBody, 1u, sharedLump);
    const DRW_AcisModel sharedModel = drw_buildAcisModel(sharedFixture.data);
    DRW_AcisRootedProjectionReport sharedReport;
    std::vector<DRW_AcisBodyProjection> sharedProjections;
    t.expect(drw_extractAcisBodyWireframes(
                 sharedModel, sharedProjections,
                 sharedReport)
                 && sharedReport.bodyCount == 2u
                 && sharedReport.multiplyRootedNodeCount == 1u,
             "ACIS rooted projection reports nodes shared by multiple body roots");

    DRW_SabParseLimits sharedPointerBudget;
    std::size_t lowSharedPointerBudget = 0u;
    std::size_t highSharedPointerBudget = 1u;
    bool highSharedPointerBudgetFits = false;
    while (highSharedPointerBudget <= 4096u) {
        sharedPointerBudget.maxOutputItems = highSharedPointerBudget;
        std::vector<DRW_AcisBodyProjection> trialSharedProjections;
        DRW_AcisRootedProjectionReport trialSharedReport;
        if (drw_extractAcisBodyWireframes(
                sharedModel, trialSharedProjections, trialSharedReport,
                sharedPointerBudget)) {
            highSharedPointerBudgetFits = true;
            break;
        }
        lowSharedPointerBudget = highSharedPointerBudget;
        highSharedPointerBudget *= 2u;
    }
    while (highSharedPointerBudgetFits
           && lowSharedPointerBudget + 1u < highSharedPointerBudget) {
        const std::size_t trialBudget = lowSharedPointerBudget
            + (highSharedPointerBudget - lowSharedPointerBudget) / 2u;
        DRW_SabParseLimits trialLimits;
        trialLimits.maxOutputItems = trialBudget;
        std::vector<DRW_AcisBodyProjection> trialSharedProjections;
        DRW_AcisRootedProjectionReport trialSharedReport;
        if (drw_extractAcisBodyWireframes(
                sharedModel, trialSharedProjections, trialSharedReport,
                trialLimits))
            highSharedPointerBudget = trialBudget;
        else
            lowSharedPointerBudget = trialBudget;
    }
    sharedPointerBudget.maxOutputItems = highSharedPointerBudget;
    std::vector<DRW_AcisBodyProjection> boundedSharedProjections;
    DRW_AcisRootedProjectionReport boundedSharedReport;
    const bool boundedSharedOk = highSharedPointerBudgetFits
        && drw_extractAcisBodyWireframes(
            sharedModel, boundedSharedProjections, boundedSharedReport,
            sharedPointerBudget);
    const auto sameProjectionLinks = [](
        const DRW_AcisBodyProjection& expected,
        const DRW_AcisBodyProjection& actual) {
        if (expected.bodyNodeIndex != actual.bodyNodeIndex
            || expected.hasLumpChainObservation
                != actual.hasLumpChainObservation
            || expected.lumpChainNullTerminated
                != actual.lumpChainNullTerminated
            || expected.lumpChainCycleDetected
                != actual.lumpChainCycleDetected
            || expected.firstRepeatedLumpChainNodeIndex
                != actual.firstRepeatedLumpChainNodeIndex
            || expected.lumpChainUnclassifiedLinkCount
                != actual.lumpChainUnclassifiedLinkCount
            || expected.firstUnclassifiedLumpLinkSourceNodeIndex
                != actual.firstUnclassifiedLumpLinkSourceNodeIndex
            || expected.lumpNodeIndices != actual.lumpNodeIndices
            || expected.lumpNextNodeIndices != actual.lumpNextNodeIndices
            || expected.hasLumpUseIndexObservation
                != actual.hasLumpUseIndexObservation
            || expected.lumpUseIndexCoversParsedLumps
                != actual.lumpUseIndexCoversParsedLumps
            || expected.lumpUseIndexUnclassifiedLumpCount
                != actual.lumpUseIndexUnclassifiedLumpCount
            || expected.firstUnclassifiedLumpOwnerNodeIndex
                != actual.firstUnclassifiedLumpOwnerNodeIndex
            || expected.lumpChainMatchesUseIndex
                != actual.lumpChainMatchesUseIndex
            || expected.lumpUseNodeIndices != actual.lumpUseNodeIndices
            || expected.reachableNodeIndices != actual.reachableNodeIndices
            || expected.graphLinks.size() != actual.graphLinks.size()) {
            return false;
        }
        for (std::size_t index = 0; index < expected.graphLinks.size(); ++index) {
            const DRW_AcisBodyGraphLink& left = expected.graphLinks[index];
            const DRW_AcisBodyGraphLink& right = actual.graphLinks[index];
            if (left.sourceNodeIndex != right.sourceNodeIndex
                || left.pointerOrdinal != right.pointerOrdinal
                || left.targetNodeIndex != right.targetNodeIndex) {
                return false;
            }
        }
        return true;
    };
    t.expect(boundedSharedOk && boundedSharedReport.bodyCount == 2u
                 && boundedSharedReport.multiplyRootedNodeCount == 1u
                 && boundedSharedProjections.size() == sharedProjections.size()
                 && std::equal(sharedProjections.begin(),
                               sharedProjections.end(),
                               boundedSharedProjections.begin(),
                               sameProjectionLinks),
             "ACIS shared pointer rows fit one bounded validation-cache admission");

    AcisGraphFixture cycleFixture;
    const int cycleBody = cycleFixture.addNode("body", 4u);
    const int firstCycleLump = cycleFixture.addNode("lump", 4u);
    const int secondCycleLump = cycleFixture.addNode("lump", 4u);
    cycleFixture.link(cycleBody, 1u, firstCycleLump);
    cycleFixture.link(firstCycleLump, 1u, secondCycleLump);
    cycleFixture.link(firstCycleLump, 3u, cycleBody);
    cycleFixture.link(secondCycleLump, 1u, firstCycleLump);
    cycleFixture.link(secondCycleLump, 3u, cycleBody);
    const DRW_AcisModel cycleModel = drw_buildAcisModel(cycleFixture.data);
    std::vector<DRW_AcisBodyProjection> baselineCycleProjections;
    DRW_AcisRootedProjectionReport baselineCycleReport;
    const bool baselineCycleOk = drw_extractAcisBodyWireframes(
        cycleModel, baselineCycleProjections, baselineCycleReport);
    DRW_SabParseLimits exactCycleBudget;
    std::size_t lowCycleOutputBudget = 0u;
    std::size_t highCycleOutputBudget = 1u;
    bool highCycleOutputBudgetFits = false;
    while (highCycleOutputBudget <= 4096u) {
        exactCycleBudget.maxOutputItems = highCycleOutputBudget;
        std::vector<DRW_AcisBodyProjection> trialCycleProjections;
        DRW_AcisRootedProjectionReport trialCycleReport;
        if (drw_extractAcisBodyWireframes(
                cycleModel, trialCycleProjections, trialCycleReport,
                exactCycleBudget)) {
            highCycleOutputBudgetFits = true;
            break;
        }
        lowCycleOutputBudget = highCycleOutputBudget;
        highCycleOutputBudget *= 2u;
    }
    while (highCycleOutputBudgetFits
           && lowCycleOutputBudget + 1u < highCycleOutputBudget) {
        const std::size_t trialBudget = lowCycleOutputBudget
            + (highCycleOutputBudget - lowCycleOutputBudget) / 2u;
        DRW_SabParseLimits trialLimits;
        trialLimits.maxOutputItems = trialBudget;
        std::vector<DRW_AcisBodyProjection> trialCycleProjections;
        DRW_AcisRootedProjectionReport trialCycleReport;
        if (drw_extractAcisBodyWireframes(
                cycleModel, trialCycleProjections, trialCycleReport,
                trialLimits))
            highCycleOutputBudget = trialBudget;
        else
            lowCycleOutputBudget = trialBudget;
    }
    exactCycleBudget.maxOutputItems = highCycleOutputBudget;
    std::vector<DRW_AcisBodyProjection> boundedCycleProjections;
    DRW_AcisRootedProjectionReport boundedCycleReport;
    const bool boundedCycleOk = drw_extractAcisBodyWireframes(
        cycleModel, boundedCycleProjections, boundedCycleReport,
        exactCycleBudget);
    DRW_SabParseLimits underCycleBudget = exactCycleBudget;
    --underCycleBudget.maxOutputItems;
    std::vector<DRW_AcisBodyProjection> rejectedCycleProjections(1u);
    DRW_AcisRootedProjectionReport rejectedCycleReport;
    const bool underCycleOk = drw_extractAcisBodyWireframes(
        cycleModel, rejectedCycleProjections, rejectedCycleReport,
        underCycleBudget);
    t.expect(highCycleOutputBudgetFits
                 && baselineCycleOk && baselineCycleProjections.size() == 1u
                 && baselineCycleReport.bodyCount == 1u
                 && baselineCycleProjections.front().nextListCycleCount == 1u
                 && boundedCycleOk && boundedCycleReport.bodyCount == 1u
                 && boundedCycleProjections.size() == 1u
                 && boundedCycleProjections.front().nextListCycleCount == 1u
                 && sameProjectionLinks(baselineCycleProjections.front(),
                                        boundedCycleProjections.front()),
             "ACIS rooted cycle projection fits exact output budget without changing links");
    t.expect(!underCycleOk && rejectedCycleProjections.empty()
                 && rejectedCycleReport.failure
                        == DRW_AcisProjectionFailure::ResourceLimit,
             "ACIS rooted cycle projection refuses one item below its exact output budget");
}

void testAcisOrientationFlagsRequireExactPointerPrefix(TestContext& t) {
    const auto rejectsMisorderedFlags = [&t](bool faceFlags) {
        AcisGraphFixture fixture;
        const SyntheticAcisBody body = addSyntheticAcisBody(fixture, 5.0);
        if (faceFlags) {
            // Face sense/two-sided flags follow six pointer slots. Put them
            // before the sixth (surface) pointer to prove a later bool is
            // not borrowed across a required pointer field.
            fixture.setBooleanFieldsAfterPointerCount(
                body.face, 5u, {true, false});
        } else {
            // Coedge sense follows five pointer slots; the fifth is its edge.
            fixture.setBooleanFieldsAfterPointerCount(
                body.coedge, 4u, {true});
        }

        const std::vector<std::uint8_t> bytes =
            sab21800FromGraph(fixture.data);
        DRW_SabData parsed;
        const bool parseSucceeded = !bytes.empty()
            && drw_parseSab(bytes.data(), bytes.size(), parsed);
        if (!parseSucceeded)
            return false;

        const DRW_AcisModel model = drw_buildAcisModel(parsed);
        std::vector<DRW_AcisBodyProjection> projections;
        DRW_AcisRootedProjectionReport report;
        if (!drw_extractAcisBodyWireframes(model, projections, report)
            || projections.size() != 1u
            || projections.front().wireframe.faces.size() != 1u) {
            return false;
        }

        const DRW_AcisFace& face = projections.front().wireframe.faces.front();
        if (drw_acisProjectionCoverage(projections.front())
            != DRW_AcisProjectionCoverage::ProjectedSubsetWithKnownGaps) {
            return false;
        }
        if (faceFlags)
            return !face.hasSense && !face.hasDoubleSided
                && !face.hasContainment;
        return !face.loops.empty()
            && face.loops.front().coedgeSenseFlags
                == std::vector<std::int8_t>{-1}
            && face.loops.front().coedgeOrientedStartVertexNodeIndices
                == std::vector<int>{-1}
            && face.loops.front().coedgeOrientedEndVertexNodeIndices
                == std::vector<int>{-1}
            && !face.loops.front()
                    .hasCoedgeEndpointChainObservation;
    };

    t.expect(rejectsMisorderedFlags(true),
             "SAB 21800 face flags before the sixth pointer remain unknown");
    t.expect(rejectsMisorderedFlags(false),
             "SAB 21800 coedge sense before the edge pointer remains unknown");
}

void testAcisRootedBodyProjectionFailures(TestContext& t) {
    AcisGraphFixture fixture;
    const SyntheticAcisBody first = addSyntheticAcisBody(fixture, 0.0);
    const SyntheticAcisBody second = addSyntheticAcisBody(fixture, 10.0);
    const DRW_AcisModel baseline = drw_buildAcisModel(fixture.data);

    const auto rejects = [&t](const DRW_AcisModel& model,
                              DRW_AcisProjectionFailure failure,
                              int failedNode, std::size_t failedOrdinal,
                              const std::string& label) {
        std::vector<DRW_AcisBodyProjection> projections(1u);
        DRW_AcisRootedProjectionReport report;
        report.bodyCount = 999u;
        const bool extracted = drw_extractAcisBodyWireframes(
            model, projections, report);
        t.expect(!extracted && projections.empty()
                     && report.failure == failure
                     && report.failedNodeIndex == failedNode
                     && report.failedPointerOrdinal == failedOrdinal,
                 label);
    };

    DRW_AcisModel wrongType = baseline;
    setModelPointer(wrongType, first.body, 2u, first.point0);
    rejects(wrongType, DRW_AcisProjectionFailure::InvalidReference,
            first.body, 2u,
            "ACIS rooted projection reports and clears wrong-type pointers");

    DRW_AcisModel invalidResolvedReference = baseline;
    invalidResolvedReference.nodes[static_cast<std::size_t>(first.body)]
        .refs[1] = -1;
    rejects(invalidResolvedReference,
            DRW_AcisProjectionFailure::InvalidReference, first.body, 1u,
            "ACIS rooted projection reports raw/reference disagreement");

    AcisGraphFixture oversizedPointerFixture = fixture;
    oversizedPointerFixture.data.records[
        static_cast<std::size_t>(first.body)].tokens[1].ival =
        std::numeric_limits<long long>::max();
    const DRW_AcisModel oversizedPointerModel =
        drw_buildAcisModel(oversizedPointerFixture.data);
    t.expect(oversizedPointerModel.nodes.size() == baseline.nodes.size()
                 && oversizedPointerModel.nodes[
                        static_cast<std::size_t>(first.body)].refs[0] == -1,
             "ACIS model builder range-checks oversized raw pointers before narrowing");
    rejects(oversizedPointerModel,
            DRW_AcisProjectionFailure::InvalidReference, first.body, 0u,
            "ACIS rooted projection rejects oversized raw pointer transactionally with its exact ordinal");

    AcisGraphFixture underflowPointerFixture = fixture;
    underflowPointerFixture.data.records[
        static_cast<std::size_t>(first.body)].tokens[1].ival =
        std::numeric_limits<long long>::min();
    const DRW_AcisModel underflowPointerModel =
        drw_buildAcisModel(underflowPointerFixture.data);
    t.expect(underflowPointerModel.nodes.size() == baseline.nodes.size()
                 && underflowPointerModel.nodes[
                        static_cast<std::size_t>(first.body)].refs[0] == -1,
             "ACIS model builder preserves an unresolved slot for the minimum signed raw pointer");
    rejects(underflowPointerModel,
            DRW_AcisProjectionFailure::InvalidReference, first.body, 0u,
            "ACIS rooted projection rejects the minimum signed raw pointer transactionally with its exact ordinal");

    DRW_AcisModel ownerMismatch = baseline;
    setModelPointer(ownerMismatch, first.lump, 3u, second.body);
    rejects(ownerMismatch, DRW_AcisProjectionFailure::OwnerMismatch,
            first.lump, 3u,
            "ACIS rooted projection rejects and identifies wrong body ownership");

    DRW_AcisModel shellOwnerMismatch = baseline;
    setModelPointer(shellOwnerMismatch, first.shell, 5u, second.lump);
    rejects(shellOwnerMismatch, DRW_AcisProjectionFailure::OwnerMismatch,
            first.shell, 5u,
            "ACIS rooted projection rejects and identifies wrong shell ownership");

    DRW_AcisModel faceOwnerMismatch = baseline;
    setModelPointer(faceOwnerMismatch, first.face, 3u, second.shell);
    rejects(faceOwnerMismatch, DRW_AcisProjectionFailure::OwnerMismatch,
            first.face, 3u,
            "ACIS rooted projection rejects and identifies wrong face ownership");

    DRW_AcisModel loopOwnerMismatch = baseline;
    setModelPointer(loopOwnerMismatch, first.loop, 3u, second.face);
    rejects(loopOwnerMismatch, DRW_AcisProjectionFailure::OwnerMismatch,
            first.loop, 3u,
            "ACIS rooted projection rejects and identifies wrong loop ownership");

    DRW_AcisModel coedgeOwnerMismatch = baseline;
    setModelPointer(coedgeOwnerMismatch, first.coedge, 5u, second.loop);
    rejects(coedgeOwnerMismatch, DRW_AcisProjectionFailure::OwnerMismatch,
            first.coedge, 5u,
            "ACIS rooted projection rejects and identifies wrong coedge ownership");

    DRW_AcisModel vertexPointTargetMismatch = baseline;
    setModelPointer(vertexPointTargetMismatch, first.vertex0, 2u,
                    first.vertex1);
    rejects(vertexPointTargetMismatch,
            DRW_AcisProjectionFailure::InvalidReference, first.vertex0, 2u,
            "ACIS rooted projection rejects and identifies a wrong-type exact vertex point target");

    DRW_AcisModel edgeCurveTargetMismatch = baseline;
    setModelPointer(edgeCurveTargetMismatch, first.edge, 4u, first.point0);
    rejects(edgeCurveTargetMismatch,
            DRW_AcisProjectionFailure::InvalidReference, first.edge, 4u,
            "ACIS rooted projection rejects and identifies a wrong-type exact edge curve target");

    for (int version : {20800, 22300}) {
        DRW_AcisModel unsupported = baseline;
        unsupported.header.version = version;
        rejects(unsupported, DRW_AcisProjectionFailure::UnsupportedProfile,
                -1, std::numeric_limits<std::size_t>::max(),
                "ACIS rooted projection refuses an unreviewed save version");
    }

    DRW_AcisModel unsupportedSignature = baseline;
    unsupportedSignature.header.signature = "ASM BinaryFile4";
    rejects(unsupportedSignature,
            DRW_AcisProjectionFailure::UnsupportedProfile, -1,
            std::numeric_limits<std::size_t>::max(),
            "ACIS rooted projection refuses an unreviewed signature");

    std::vector<DRW_AcisBodyProjection> limitedProjections(1u);
    DRW_AcisRootedProjectionReport limitedReport;
    DRW_SabParseLimits limits;
    limits.maxExtractionWorkUnits = 0u;
    t.expect(!drw_extractAcisBodyWireframes(
                  baseline, limitedProjections, limitedReport, limits)
                 && limitedProjections.empty()
                 && limitedReport.failure
                        == DRW_AcisProjectionFailure::ResourceLimit,
             "ACIS rooted projection reports resource-limit refusal and clears output");

    std::vector<DRW_AcisBodyProjection> noOutputBudgetProjections(1u);
    DRW_AcisRootedProjectionReport noOutputBudgetReport;
    DRW_SabParseLimits noOutputBudget;
    noOutputBudget.maxOutputItems = 0u;
    t.expect(!drw_extractAcisBodyWireframes(
                  baseline, noOutputBudgetProjections, noOutputBudgetReport,
                  noOutputBudget)
                 && noOutputBudgetProjections.empty()
                 && noOutputBudgetReport.failure
                        == DRW_AcisProjectionFailure::ResourceLimit,
             "ACIS rooted projection refuses temporary graph admission at zero output budget");

    std::vector<DRW_AcisBodyProjection> partialOutputBudgetProjections(1u);
    DRW_AcisRootedProjectionReport partialOutputBudgetReport;
    DRW_SabParseLimits partialOutputBudget;
    partialOutputBudget.maxOutputItems = 4u;
    t.expect(!drw_extractAcisBodyWireframes(
                  baseline, partialOutputBudgetProjections,
                  partialOutputBudgetReport, partialOutputBudget)
                 && partialOutputBudgetProjections.empty()
                 && partialOutputBudgetReport.failure
                        == DRW_AcisProjectionFailure::ResourceLimit,
             "ACIS rooted projection clears caller output when a nonzero output budget is exhausted");

    std::vector<DRW_AcisBodyProjection> tokenLimitedProjections(1u);
    DRW_AcisRootedProjectionReport tokenLimitedReport;
    DRW_SabParseLimits tokenLimits;
    tokenLimits.maxTokenCapacityItems = 4u;
    t.expect(!drw_extractAcisBodyWireframes(
                  baseline, tokenLimitedProjections, tokenLimitedReport,
                  tokenLimits)
                 && tokenLimitedProjections.empty()
                 && tokenLimitedReport.failure
                        == DRW_AcisProjectionFailure::ResourceLimit,
             "ACIS rooted projection applies token-capacity limits to transform snapshots");
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
    // ODA §24.2.2.5: an empty first (unknown-property) table, then one
    // schema entry in the second table.
    putU32(schemaIndexOffset + 48u, 0u);
    putU32(schemaIndexOffset + 52u, 0u);
    putU64(schemaIndexOffset + 56u, 0x00000000000af10cull);
    putU32(schemaIndexOffset + 64u, 1u);
    putU32(schemaIndexOffset + 68u, 0u);
    putU32(schemaIndexOffset + 72u, 2u);
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
        std::size_t pageZeroSegmentSizeOffset = 0;
        std::size_t dataRegionOffset = 0;
        std::size_t suffixRecordHeaderOffset = 0;
        std::uint32_t pageSegmentSize = 0;
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
    const std::string acisMarker = "ACIS BinaryFile";
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
        fixture.pageZeroSegmentSizeOffset =
            segmentIndexOffset + DRW_DataStorageConst::SEGMENT_HEADER_SIZE
            + 4u * DRW_DataStorageConst::SEGMENT_INDEX_ENTRY_SIZE + 8u;
        fixture.pageSegmentSize = static_cast<std::uint32_t>(pageSegmentSize);
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
            std::uint32_t size, std::uint32_t objectDataAlignmentOffset) {
            putU16(offset, DRW_DataStorageConst::SEGMENT_SIGNATURE);
            for (std::size_t byte = 0; byte < 6u; ++byte)
                fixture.bytes[offset + 2u + byte] =
                    static_cast<std::uint8_t>(name[byte]);
            putU32(offset + 8u, index);
            putU32(offset + 16u, size);
            putU32(offset + 24u, 1u);
            putU32(offset + 36u, objectDataAlignmentOffset);
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
                       segments[index].second, index == 2u ? 6u : 0u);
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
        fixture.dataRegionOffset = dataRegionOffset;
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
        fixture.suffixRecordHeaderOffset =
            firstRecordHeader + DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE;
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
    const DRW_DataStorageSection metadataOnly =
        DRW_parseDataStorage(fixture.bytes, DRW::AC1027, false);
    const DRW_DataStorageRecord* metadataOnlyBlob =
        metadataOnly.findRecordByHandle(0xa17u);
    const DRW_DataStorageRecord* metadataOnlySuffix =
        metadataOnly.findRecordByHandle(0xa18u);
    t.expect(metadataOnly.structurallyValid && metadataOnly.replayAllowed
                 && !metadataOnly.payloadsRetained
                 && metadataOnly.records.size() == 2u
                 && metadataOnlyBlob != nullptr
                 && metadataOnlyBlob->isBlobReference
                 && !metadataOnlyBlob->blobPayloadRetained
                 && metadataOnlyBlob->payload.empty()
                 && metadataOnlyBlob->blobPages.size() == 2u
                 && metadataOnlySuffix != nullptr
                 && metadataOnlySuffix->payload.empty(),
             "DataStorage metadata-only parse validates writer section without copying payloads");
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

    const auto storeFixtureU32 = [](std::vector<std::uint8_t>& bytes,
                                    std::size_t offset,
                                    std::uint32_t value) {
        for (unsigned byte = 0; byte < 4u; ++byte)
            bytes[offset + byte] =
                static_cast<std::uint8_t>(value >> (byte * 8u));
    };
    const auto storeFixtureU64 = [&storeFixtureU32](
        std::vector<std::uint8_t>& bytes, std::size_t offset,
        std::uint64_t value) {
        storeFixtureU32(bytes, offset, static_cast<std::uint32_t>(value));
        storeFixtureU32(bytes, offset + 4u,
                        static_cast<std::uint32_t>(value >> 32u));
    };
    const auto checkSingleBlobPage = [&](std::size_t byteLength,
                                         const std::string& label) {
        BlobFixture singlePage = fixture;
        const std::vector<std::uint8_t> expectedPayload(
            payload.begin(),
            payload.begin() + static_cast<std::ptrdiff_t>(byteLength));
        const std::uint32_t pageLength =
            static_cast<std::uint32_t>(byteLength);
        storeFixtureU64(singlePage.bytes, singlePage.descriptorOffset,
                        byteLength);
        storeFixtureU32(singlePage.bytes,
                        singlePage.descriptorOffset + 8u, 1u);
        storeFixtureU32(singlePage.bytes,
                        singlePage.descriptorOffset + 12u,
                        DRW_DataStorageConst::DATA_BLOB_REFERENCE_FIXED_SIZE
                            + DRW_DataStorageConst::DATA_BLOB_PAGE_ENTRY_SIZE);
        storeFixtureU32(singlePage.bytes,
                        singlePage.descriptorOffset + 16u, pageLength);
        storeFixtureU32(singlePage.bytes,
                        singlePage.descriptorOffset + 20u, pageLength);
        storeFixtureU32(singlePage.bytes,
                        singlePage.descriptorOffset + 32u, 4u);
        storeFixtureU32(singlePage.bytes,
                        singlePage.descriptorOffset + 36u, pageLength);

        storeFixtureU64(singlePage.bytes, singlePage.pageZeroHeaderOffset,
                        byteLength);
        storeFixtureU64(singlePage.bytes,
                        singlePage.pageZeroHeaderOffset + 8u, 0u);
        storeFixtureU32(singlePage.bytes,
                        singlePage.pageZeroHeaderOffset + 16u, 0u);
        storeFixtureU32(singlePage.bytes,
                        singlePage.pageZeroHeaderOffset + 20u, 1u);
        storeFixtureU64(singlePage.bytes,
                        singlePage.pageZeroHeaderOffset + 24u, byteLength);
        const std::size_t pageDataOffset = singlePage.pageZeroHeaderOffset
            + DRW_DataStorageConst::DATA_BLOB_PAGE_HEADER_SIZE;
        std::copy(expectedPayload.begin(), expectedPayload.end(),
                  singlePage.bytes.begin()
                      + static_cast<std::ptrdiff_t>(pageDataOffset));
        const std::size_t pageSegmentStart =
            singlePage.pageZeroHeaderOffset
            - DRW_DataStorageConst::SEGMENT_HEADER_SIZE;
        const std::size_t pageSegmentEnd =
            pageSegmentStart + singlePage.pageSegmentSize;
        std::fill(singlePage.bytes.begin()
                      + static_cast<std::ptrdiff_t>(pageDataOffset + byteLength),
                  singlePage.bytes.begin()
                      + static_cast<std::ptrdiff_t>(pageSegmentEnd),
                  0x70u);

        const DRW_DataStorageSection singlePageParsed =
            DRW_parseDataStorage(singlePage.bytes, DRW::AC1027);
        const DRW_DataStorageRecord* singlePageRecord =
            singlePageParsed.findRecordByHandle(0xa17u);
        const DRW_DataStorageRecord* singlePageSuffix =
            singlePageParsed.findRecordByHandle(0xa18u);
        t.expect(!singlePageParsed.parseFailed
                     && singlePageParsed.structurallyValid
                     && singlePageParsed.replayAllowed
                     && singlePageRecord != nullptr
                     && singlePageRecord->isBlobReference
                     && singlePageRecord->blobPageCount == 1u
                     && singlePageRecord->blobTotalByteLength == byteLength
                     && singlePageRecord->blobPageByteSize == byteLength
                     && singlePageRecord->blobLastPageByteSize == byteLength
                     && singlePageRecord->blobPages.size() == 1u
                     && singlePageRecord->blobPages[0].segmentIndex == 4u
                     && singlePageRecord->blobPages[0].byteLength == byteLength
                     && singlePageRecord->blobPayloadRetained
                     && singlePageRecord->payload == expectedPayload
                     && singlePageSuffix != nullptr
                     && singlePageSuffix->payload == sentinel,
                 label);
    };
    checkSingleBlobPage(
        DRW_DataStorageConst::DATA_RECORD_INLINE_MAX_SIZE + 1u,
        "DataStorage assembles the minimum over-limit one-page blob and preserves the suffix record");
    checkSingleBlobPage(
        firstPageSize,
        "DataStorage accepts a one-page blob at the exact Blob01 page-size limit");

    BlobFixture capPlusOne = fixture;
    const std::uint64_t capPlusOneLength = firstPageSize + 1u;
    storeFixtureU64(capPlusOne.bytes, capPlusOne.descriptorOffset,
                    capPlusOneLength);
    storeFixtureU32(capPlusOne.bytes,
                    capPlusOne.descriptorOffset + 8u, 2u);
    storeFixtureU32(capPlusOne.bytes,
                    capPlusOne.descriptorOffset + 12u,
                    DRW_DataStorageConst::DATA_BLOB_REFERENCE_FIXED_SIZE
                        + 2u * DRW_DataStorageConst::DATA_BLOB_PAGE_ENTRY_SIZE);
    storeFixtureU32(capPlusOne.bytes,
                    capPlusOne.descriptorOffset + 16u,
                    static_cast<std::uint32_t>(firstPageSize));
    storeFixtureU32(capPlusOne.bytes,
                    capPlusOne.descriptorOffset + 20u, 1u);
    storeFixtureU32(capPlusOne.bytes,
                    capPlusOne.descriptorOffset + 32u, 4u);
    storeFixtureU32(capPlusOne.bytes,
                    capPlusOne.descriptorOffset + 36u,
                    static_cast<std::uint32_t>(firstPageSize));
    storeFixtureU32(capPlusOne.bytes,
                    capPlusOne.descriptorOffset + 40u, 3u);
    storeFixtureU32(capPlusOne.bytes,
                    capPlusOne.descriptorOffset + 44u, 1u);
    storeFixtureU64(capPlusOne.bytes, capPlusOne.pageZeroHeaderOffset,
                    capPlusOneLength);
    storeFixtureU32(capPlusOne.bytes,
                    capPlusOne.pageZeroHeaderOffset + 20u, 2u);
    storeFixtureU64(capPlusOne.bytes,
                    capPlusOne.pageOneHeaderOffset, capPlusOneLength);
    storeFixtureU64(capPlusOne.bytes,
                    capPlusOne.pageOneHeaderOffset + 8u, firstPageSize);
    storeFixtureU32(capPlusOne.bytes,
                    capPlusOne.pageOneHeaderOffset + 16u, 1u);
    storeFixtureU32(capPlusOne.bytes,
                    capPlusOne.pageOneHeaderOffset + 20u, 2u);
    storeFixtureU64(capPlusOne.bytes,
                    capPlusOne.pageOneHeaderOffset + 24u, 1u);
    const std::size_t pageOneDataOffset = capPlusOne.pageOneHeaderOffset
        + DRW_DataStorageConst::DATA_BLOB_PAGE_HEADER_SIZE;
    std::fill(capPlusOne.bytes.begin()
                  + static_cast<std::ptrdiff_t>(pageOneDataOffset + 1u),
              capPlusOne.bytes.begin()
                  + static_cast<std::ptrdiff_t>(
                      capPlusOne.pageOneHeaderOffset
                      - DRW_DataStorageConst::SEGMENT_HEADER_SIZE
                      + capPlusOne.pageSegmentSize),
              0x70u);
    const std::vector<std::uint8_t> capPlusOnePayload(
        payload.begin(),
        payload.begin() + static_cast<std::ptrdiff_t>(capPlusOneLength));
    const DRW_DataStorageSection capPlusOneParsed =
        DRW_parseDataStorage(capPlusOne.bytes, DRW::AC1027);
    const DRW_DataStorageRecord* capPlusOneRecord =
        capPlusOneParsed.findRecordByHandle(0xa17u);
    const DRW_DataStorageRecord* capPlusOneSuffix =
        capPlusOneParsed.findRecordByHandle(0xa18u);
    t.expect(!capPlusOneParsed.parseFailed
                 && capPlusOneParsed.structurallyValid
                 && capPlusOneParsed.replayAllowed
                 && capPlusOneRecord != nullptr
                 && capPlusOneRecord->blobPageCount == 2u
                 && capPlusOneRecord->blobTotalByteLength == capPlusOneLength
                 && capPlusOneRecord->blobPageByteSize == firstPageSize
                 && capPlusOneRecord->blobLastPageByteSize == 1u
                 && capPlusOneRecord->blobPages.size() == 2u
                 && capPlusOneRecord->blobPages[0].segmentIndex == 4u
                 && capPlusOneRecord->blobPages[0].byteLength == firstPageSize
                 && capPlusOneRecord->blobPages[1].segmentIndex == 3u
                 && capPlusOneRecord->blobPages[1].byteLength == 1u
                 && capPlusOneRecord->payload == capPlusOnePayload
                 && capPlusOneSuffix != nullptr
                 && capPlusOneSuffix->payload == sentinel,
             "DataStorage splits cap-plus-one bytes into a full page and one-byte suffix page");

    const auto replaceBlobPayload = [firstPageSize](
        BlobFixture& target, const std::vector<std::uint8_t>& replacement) {
        if (replacement.size() != firstPageSize * 2u)
            return false;
        const std::size_t pageDataOffset =
            DRW_DataStorageConst::DATA_BLOB_PAGE_HEADER_SIZE;
        std::copy(replacement.begin(),
                  replacement.begin() + static_cast<std::ptrdiff_t>(firstPageSize),
                  target.bytes.begin() + static_cast<std::ptrdiff_t>(
                                             target.pageZeroHeaderOffset
                                             + pageDataOffset));
        std::copy(replacement.begin() + static_cast<std::ptrdiff_t>(firstPageSize),
                  replacement.end(),
                  target.bytes.begin() + static_cast<std::ptrdiff_t>(
                                             target.pageOneHeaderOffset
                                             + pageDataOffset));
        return true;
    };

    std::vector<std::uint8_t> endMarkerPayload(payload.size(), 'x');
    std::copy(acisMarker.begin(), acisMarker.end(),
              endMarkerPayload.end()
                  - static_cast<std::ptrdiff_t>(acisMarker.size()));
    BlobFixture endMarkerFixture = fixture;
    const bool endMarkerPayloadInstalled =
        replaceBlobPayload(endMarkerFixture, endMarkerPayload);
    const DRW_DataStorageSection endMarkerParsed =
        DRW_parseDataStorage(endMarkerFixture.bytes, DRW::AC1027);
    const DRW_DataStorageRecord* endMarkerRecord =
        endMarkerParsed.findRecordByHandle(0xa17u);
    const DRW_DataStorageRecord* endMarkerSuffix =
        endMarkerParsed.findRecordByHandle(0xa18u);
    t.expect(endMarkerPayloadInstalled && !endMarkerParsed.parseFailed
                 && endMarkerRecord != nullptr
                 && endMarkerRecord->payload == endMarkerPayload
                 && endMarkerRecord->hasPayloadMarker
                 && endMarkerRecord->payloadMarkerOffset
                        == endMarkerPayload.size() - acisMarker.size()
                 && endMarkerRecord->payloadMarkerLength
                        == acisMarker.size()
                 && endMarkerSuffix != nullptr
                 && endMarkerSuffix->payload == sentinel,
             "DataStorage recognizes a complete ACIS marker at the final valid payload offset");

    std::vector<std::uint8_t> partialTailPayload(payload.size(), 'x');
    std::copy(acisMarker.begin(), acisMarker.end() - 1,
              partialTailPayload.end()
                  - static_cast<std::ptrdiff_t>(acisMarker.size() - 1u));
    BlobFixture partialTailFixture = fixture;
    const bool partialTailPayloadInstalled =
        replaceBlobPayload(partialTailFixture, partialTailPayload);
    const DRW_DataStorageSection partialTailParsed =
        DRW_parseDataStorage(partialTailFixture.bytes, DRW::AC1027);
    const DRW_DataStorageRecord* partialTailRecord =
        partialTailParsed.findRecordByHandle(0xa17u);
    const DRW_DataStorageRecord* partialTailSuffix =
        partialTailParsed.findRecordByHandle(0xa18u);
    t.expect(partialTailPayloadInstalled && !partialTailParsed.parseFailed
                 && partialTailRecord != nullptr
                 && partialTailRecord->payload == partialTailPayload
                 && !partialTailRecord->hasPayloadMarker
                 && partialTailSuffix != nullptr
                 && partialTailSuffix->payload == sentinel,
             "DataStorage rejects a partial ACIS marker at payload end and preserves the suffix record");

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

    // A small AcDs section may reference the same blob pages from multiple
    // records. Bound the aggregate copied payload, not just each individual
    // blob, while keeping the section itself structurally replayable.
    BlobFixture repeatedBlobs = fixture;
    constexpr std::size_t insertedDataRecordBytes = 256u;
    constexpr std::size_t repeatedDataSegmentSize = 448u;
    const std::size_t pageOneSegmentOffset = dataOffset + 192u;
    repeatedBlobs.bytes.insert(
        repeatedBlobs.bytes.begin()
            + static_cast<std::ptrdiff_t>(pageOneSegmentOffset),
        insertedDataRecordBytes, 0u);
    putU32(repeatedBlobs.bytes, 52u,
           static_cast<std::uint32_t>(repeatedBlobs.bytes.size()));
    putU32(repeatedBlobs.bytes, dataOffset + 16u,
           repeatedDataSegmentSize);
    putU32(repeatedBlobs.bytes,
           segmentIndexOffset + 48u + 2u * 12u + 8u,
           repeatedDataSegmentSize);
    putU32(repeatedBlobs.bytes, dataOffset + 36u, 10u);
    repeatedBlobs.pageOneHeaderOffset += insertedDataRecordBytes;
    repeatedBlobs.pageZeroHeaderOffset += insertedDataRecordBytes;
    const std::size_t pageOneIndexEntry = segmentIndexOffset + 48u + 3u * 12u;
    const std::size_t pageZeroIndexEntry = segmentIndexOffset + 48u + 4u * 12u;
    putU64(repeatedBlobs.bytes, pageOneIndexEntry,
           pageOneSegmentOffset + insertedDataRecordBytes);
    putU64(repeatedBlobs.bytes,
           pageZeroIndexEntry,
           pageOneSegmentOffset + insertedDataRecordBytes
               + repeatedBlobs.pageSegmentSize);
    constexpr std::size_t repeatedBlobCount = 5u;
    constexpr std::size_t dataRecordHeaderStart = dataOffset + 48u;
    constexpr std::size_t repeatedDataRegionOffset = dataOffset + 160u;
    putU32(repeatedBlobs.bytes, dataIndexOffset + 48u,
           repeatedBlobCount);
    for (std::size_t index = 0; index < repeatedBlobCount; ++index) {
        const std::size_t directoryEntry = dataIndexOffset + 56u + index * 12u;
        const std::size_t recordHeader =
            dataRecordHeaderStart
            + index * DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE;
        putU32(repeatedBlobs.bytes, directoryEntry, 2u);
        putU32(repeatedBlobs.bytes, directoryEntry + 4u,
               static_cast<std::uint32_t>(
                   index * DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE));
        putU32(repeatedBlobs.bytes, directoryEntry + 8u, 7u);
        putU32(repeatedBlobs.bytes, recordHeader,
               DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE);
        putU64(repeatedBlobs.bytes, recordHeader + 8u,
               0xb00u + index);
        putU32(repeatedBlobs.bytes, recordHeader + 16u,
               static_cast<std::uint32_t>(index * 52u));

        const std::size_t descriptor = repeatedDataRegionOffset + index * 52u;
        putU32(repeatedBlobs.bytes, descriptor,
               DRW_DataStorageConst::DATA_BLOB_REFERENCE_MARKER);
        const std::size_t blobDescriptor = descriptor + 4u;
        putU64(repeatedBlobs.bytes, blobDescriptor, payload.size());
        putU32(repeatedBlobs.bytes, blobDescriptor + 8u, 2u);
        putU32(repeatedBlobs.bytes, blobDescriptor + 12u,
               DRW_DataStorageConst::DATA_BLOB_REFERENCE_FIXED_SIZE
                   + 2u * DRW_DataStorageConst::DATA_BLOB_PAGE_ENTRY_SIZE);
        putU32(repeatedBlobs.bytes, blobDescriptor + 16u,
               static_cast<std::uint32_t>(firstPageSize));
        putU32(repeatedBlobs.bytes, blobDescriptor + 20u,
               static_cast<std::uint32_t>(firstPageSize));
        putU32(repeatedBlobs.bytes, blobDescriptor + 32u, 4u);
        putU32(repeatedBlobs.bytes, blobDescriptor + 36u,
               static_cast<std::uint32_t>(firstPageSize));
        putU32(repeatedBlobs.bytes, blobDescriptor + 40u, 3u);
        putU32(repeatedBlobs.bytes, blobDescriptor + 44u,
               static_cast<std::uint32_t>(firstPageSize));
    }
    DRW_DataStorageSection repeatedBlobsParsed =
        DRW_parseDataStorage(repeatedBlobs.bytes, DRW::AC1027);
    const bool hasRetentionBudgetDiagnostic = std::any_of(
        repeatedBlobsParsed.diagnostics.cbegin(),
        repeatedBlobsParsed.diagnostics.cend(),
        [](const DRW_DataStorageDiagnostic& diagnostic) {
            return diagnostic.code
                    == "datastorage-payload-retention-budget-exceeded"
                && diagnostic.kind
                       == DRW_DataStorageDiagnostic::Kind::SupportedOpaque;
        });
    bool firstFourBlobsRetained = repeatedBlobsParsed.records.size() == 5u;
    for (std::size_t index = 0;
         firstFourBlobsRetained && index < repeatedBlobCount - 1u; ++index) {
        const DRW_DataStorageRecord* repeatedRecord =
            repeatedBlobsParsed.findRecordByHandle(0xb00u + index);
        firstFourBlobsRetained = repeatedRecord != nullptr
            && repeatedRecord->isBlobReference
            && repeatedRecord->blobPayloadRetained
            && repeatedRecord->payload == payload;
    }
    const DRW_DataStorageRecord* budgetOmittedBlob =
        repeatedBlobsParsed.findRecordByHandle(0xb04u);
    t.expect(repeatedBlobsParsed.structurallyValid
                 && repeatedBlobsParsed.replayAllowed
                 && !repeatedBlobsParsed.payloadsRetained
                 && firstFourBlobsRetained && budgetOmittedBlob != nullptr
                 && budgetOmittedBlob->isBlobReference
                 && !budgetOmittedBlob->blobPayloadRetained
                 && budgetOmittedBlob->payload.empty()
                 && hasRetentionBudgetDiagnostic,
             "DataStorage aggregate payload retention cap preserves replay while omitting over-budget blob copies");

    std::vector<std::uint8_t> aggregateLinkReaderBacking{0u};
    dwgRW aggregateLinkParent(nullptr);
    DataStorageLinkTestReader aggregateLinkReader(
        std::unique_ptr<dwgBuffer>(new dwgBuffer(
            aggregateLinkReaderBacking.data(),
            aggregateLinkReaderBacking.size())),
        &aggregateLinkParent);
    aggregateLinkReader.setFormatVersion(DRW::AC1027);
    aggregateLinkReader.m_dataStorageSections.push_back(
        std::move(repeatedBlobsParsed));
    std::array<DRW_ModelerGeometry, 4> aggregateLinkedBodies;
    std::size_t expectedLinkedPayloadBytes = 0u;
    bool allAggregateSourcesFound = true;
    for (std::size_t index = 0; index < aggregateLinkedBodies.size(); ++index) {
        DRW_ModelerGeometry& linkedBody = aggregateLinkedBodies[index];
        const DRW_DataStorageRecord* sourceRecord =
            aggregateLinkReader.m_dataStorageSections.front()
                .findRecordByHandle(0xb00u + index);
        if (sourceRecord == nullptr) {
            allAggregateSourcesFound = false;
            continue;
        }
        linkedBody.eType = DRW::E3DSOLID;
        linkedBody.handle = 0xc00u + static_cast<std::uint32_t>(index);
        linkedBody.setHasDataStorageBinaryData(true);
        linkedBody.dataStorageHandle = sourceRecord->handle;
        linkedBody.dataStorageHandleKey = sourceRecord->handleKey;
        expectedLinkedPayloadBytes += sourceRecord->payload.size();
        aggregateLinkReader.linkDataStorage(linkedBody);
    }
    aggregateLinkReader.linkDataStorage(aggregateLinkedBodies.front());
    aggregateLinkReader.finalizeDataStorageLinks();
    bool allAggregateCopiesMatch = allAggregateSourcesFound;
    for (const DRW_ModelerGeometry& linkedBody : aggregateLinkedBodies) {
        allAggregateCopiesMatch = allAggregateCopiesMatch
            && linkedBody.hasDataStorageRecord
            && linkedBody.dataStorageData == payload;
    }
    t.expect(allAggregateCopiesMatch
                 && expectedLinkedPayloadBytes
                        == aggregateLinkReader.m_dataStorageLinkedPayloadBytes
                 && aggregateLinkReader.m_dataStorageLinkedPayloadBytes
                        <= DRW_DataStorageConst::PAYLOAD_BLOB_SECTION_CAP
                 && aggregateLinkReader.m_dataStorageLinkedRecords.size() == 4u
                 && aggregateLinkReader.m_dataStorageLinkFailures == 0u
                 && aggregateLinkReader.m_dataStorageOrphanRecords == 1u,
             "DataStorage linked entity payload copies remain cumulatively budgeted and idempotent");

    DRW_DataStorageSection linkBudgetSection;
    linkBudgetSection.m_version = DRW::AC1027;
    const auto appendLinkBudgetRecord = [&linkBudgetSection](
        std::uint64_t handle, const UTF8STRING& handleKey,
        std::vector<std::uint8_t> payload) {
        DRW_DataStorageRecord record;
        record.handle = handle;
        record.handleKey = handleKey;
        record.payload = std::move(payload);
        const std::size_t index = linkBudgetSection.records.size();
        linkBudgetSection.recordIndexByHandle.emplace(handle, index);
        linkBudgetSection.recordIndexByHandleKey.emplace(handleKey, index);
        linkBudgetSection.records.push_back(std::move(record));
    };
    std::vector<std::uint8_t> exactLinkBudgetPayload(
        DRW_DataStorageConst::PAYLOAD_BLOB_SECTION_CAP, 0x5au);
    appendLinkBudgetRecord(0xc10u, "C10", std::move(exactLinkBudgetPayload));
    appendLinkBudgetRecord(0xc11u, "C11", {0xa5u});

    std::vector<std::uint8_t> linkBudgetReaderBacking{0u};
    dwgRW linkBudgetParent(nullptr);
    DataStorageLinkTestReader linkBudgetReader(
        std::unique_ptr<dwgBuffer>(new dwgBuffer(
            linkBudgetReaderBacking.data(), linkBudgetReaderBacking.size())),
        &linkBudgetParent);
    linkBudgetReader.setFormatVersion(DRW::AC1027);
    linkBudgetReader.m_dataStorageSections.push_back(
        std::move(linkBudgetSection));
    DRW_ModelerGeometry atLinkBudget(DRW::E3DSOLID);
    atLinkBudget.handle = 0xc20u;
    atLinkBudget.setHasDataStorageBinaryData(true);
    atLinkBudget.dataStorageHandle = 0xc10u;
    atLinkBudget.dataStorageHandleKey = "C10";
    linkBudgetReader.linkDataStorage(atLinkBudget);
    DRW_ModelerGeometry overLinkBudget(DRW::E3DSOLID);
    overLinkBudget.handle = 0xc21u;
    overLinkBudget.setHasDataStorageBinaryData(true);
    overLinkBudget.dataStorageHandle = 0xc11u;
    overLinkBudget.dataStorageHandleKey = "C11";
    overLinkBudget.m_isEmpty = true;
    overLinkBudget.m_modelerVersion = 168u;
    linkBudgetReader.linkDataStorage(overLinkBudget);
    linkBudgetReader.finalizeDataStorageLinks();
    const DRW_DataStorageSection& linkBudgetLinkedSection =
        linkBudgetReader.m_dataStorageSections.front();
    t.expect(atLinkBudget.hasDataStorageRecord
                 && atLinkBudget.dataStorageData.size()
                        == DRW_DataStorageConst::PAYLOAD_BLOB_SECTION_CAP
                 && atLinkBudget.dataStorageData
                        == linkBudgetLinkedSection.records[0].payload
                 && !overLinkBudget.hasDataStorageRecord
                 && overLinkBudget.dataStorageData.empty()
                 && overLinkBudget.m_isEmpty
                 && overLinkBudget.m_modelerVersion == 168u
                 && linkBudgetReader.m_dataStorageLinkedPayloadBytes
                        == DRW_DataStorageConst::PAYLOAD_BLOB_SECTION_CAP
                 && linkBudgetReader.m_dataStorageLinkedRecords.size() == 1u
                 && linkBudgetReader.m_dataStorageLinkFailures == 1u
                 && linkBudgetReader.m_dataStorageOrphanRecords == 1u
                 && linkBudgetLinkedSection.orphanRecordCount == 1u,
             "over-budget DataStorage link is refused atomically without changing prior links or modeler state");

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

    BlobFixture nonzeroFirstPageStart = makeFixture();
    putU64(nonzeroFirstPageStart.bytes,
           nonzeroFirstPageStart.pageZeroHeaderOffset + 8u, 1u);
    rejectsBlobAndKeepsSuffix(std::move(nonzeroFirstPageStart),
                              "DataStorage requires the first blob page to start at offset zero");

    BlobFixture missingPageSegment = makeFixture();
    putU32(missingPageSegment.bytes,
           missingPageSegment.descriptorOffset + 32u, UINT32_MAX);
    rejectsBlobAndKeepsSuffix(std::move(missingPageSegment),
                              "DataStorage rejects a page reference to a missing segment");

    BlobFixture pageDataLengthMismatch = makeFixture();
    putU64(pageDataLengthMismatch.bytes,
           pageDataLengthMismatch.pageZeroHeaderOffset + 24u,
           firstPageSize - 1u);
    rejectsBlobAndKeepsSuffix(std::move(pageDataLengthMismatch),
                              "DataStorage rejects Blob01 page-header data length disagreement");

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

    BlobFixture overflowingTotal = makeFixture();
    const std::uint64_t maximumTotal =
        (std::numeric_limits<std::uint64_t>::max)();
    putU64(overflowingTotal.bytes, overflowingTotal.descriptorOffset,
           maximumTotal);
    putU64(overflowingTotal.bytes, overflowingTotal.pageZeroHeaderOffset,
           maximumTotal);
    putU64(overflowingTotal.bytes, overflowingTotal.pageOneHeaderOffset,
           maximumTotal);
    rejectsBlobAndKeepsSuffix(std::move(overflowingTotal),
                              "DataStorage rejects maximal declared total without losing suffix");

    BlobFixture truncatedPageSegment = makeFixture();
    putU32(truncatedPageSegment.bytes,
           truncatedPageSegment.pageZeroSegmentSizeOffset,
           truncatedPageSegment.pageSegmentSize - 1u);
    const DRW_DataStorageSection rejectedTruncatedPage =
        DRW_parseDataStorage(truncatedPageSegment.bytes, DRW::AC1027);
    const DRW_DataStorageRecord* truncatedPageSuffix =
        rejectedTruncatedPage.findRecordByHandle(0xa18u);
    t.expect(!rejectedTruncatedPage.structurallyValid
                 && !rejectedTruncatedPage.replayAllowed
                 && rejectedTruncatedPage.findRecordByHandle(0xa17u) == nullptr
                 && truncatedPageSuffix != nullptr
                 && truncatedPageSuffix->payload == sentinel,
             "DataStorage rejects a page segment truncated by one byte and preserves suffix");

    BlobFixture truncatedPageHeader = makeFixture();
    putU32(truncatedPageHeader.bytes,
           truncatedPageHeader.pageZeroSegmentSizeOffset, 64u);
    const DRW_DataStorageSection rejectedTruncatedPageHeader =
        DRW_parseDataStorage(truncatedPageHeader.bytes, DRW::AC1027);
    const DRW_DataStorageRecord* pageHeaderSuffix =
        rejectedTruncatedPageHeader.findRecordByHandle(0xa18u);
    const bool hasTruncatedPageHeaderDiagnostic = std::any_of(
        rejectedTruncatedPageHeader.diagnostics.cbegin(),
        rejectedTruncatedPageHeader.diagnostics.cend(),
        [](const DRW_DataStorageDiagnostic& diagnostic) {
            return diagnostic.code
                == "datastorage-blob-page-segment-invalid";
        });
    t.expect(!rejectedTruncatedPageHeader.structurallyValid
                 && !rejectedTruncatedPageHeader.replayAllowed
                 && rejectedTruncatedPageHeader.findRecordByHandle(0xa17u)
                        == nullptr
                 && pageHeaderSuffix != nullptr
                 && pageHeaderSuffix->payload == sentinel
                 && hasTruncatedPageHeaderDiagnostic,
             "DataStorage rejects a page segment truncated before its page header");

    BlobFixture truncatedDescriptor = makeFixture();
    constexpr std::uint32_t shortBlobRecordLength = 35u;
    putU32(truncatedDescriptor.bytes,
           truncatedDescriptor.suffixRecordHeaderOffset + 16u,
           shortBlobRecordLength);
    const std::size_t overlappedSuffixOffset =
        truncatedDescriptor.dataRegionOffset + shortBlobRecordLength;
    putU32(truncatedDescriptor.bytes, overlappedSuffixOffset,
           static_cast<std::uint32_t>(sentinel.size()));
    std::copy(sentinel.begin(), sentinel.end(),
              truncatedDescriptor.bytes.begin()
                  + static_cast<std::ptrdiff_t>(overlappedSuffixOffset + 4u));
    const DRW_DataStorageSection rejectedTruncatedDescriptor =
        DRW_parseDataStorage(truncatedDescriptor.bytes, DRW::AC1027);
    const DRW_DataStorageRecord* truncatedDescriptorSuffix =
        rejectedTruncatedDescriptor.findRecordByHandle(0xa18u);
    const bool hasTruncatedDescriptorDiagnostic = std::any_of(
        rejectedTruncatedDescriptor.diagnostics.cbegin(),
        rejectedTruncatedDescriptor.diagnostics.cend(),
        [](const DRW_DataStorageDiagnostic& diagnostic) {
            return diagnostic.code
                == "datastorage-blob-reference-truncated";
        });
    t.expect(!rejectedTruncatedDescriptor.structurallyValid
                 && !rejectedTruncatedDescriptor.replayAllowed
                 && rejectedTruncatedDescriptor.findRecordByHandle(0xa17u)
                        == nullptr
                 && truncatedDescriptorSuffix != nullptr
                 && truncatedDescriptorSuffix->payload == sentinel
                 && hasTruncatedDescriptorDiagnostic,
             "DataStorage bounds a truncated blob descriptor at the next record");

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
        std::size_t dataSegmentOffset = 0;
        std::size_t dataSegmentSize = 0;
        std::size_t dataRegionOffset = 0;
    };
    const auto makeDirectFixture = [&](
        std::uint32_t directSize, std::size_t dataRegionRelativeOffset,
        bool reverseDirectoryAndPayload) {
        constexpr std::size_t segmentIndexOffset = 64u;
        constexpr std::size_t dataIndexOffset = 256u;
        constexpr std::size_t dataOffset = 384u;
        DirectFixture directFixture;
        directFixture.dataSegmentOffset = dataOffset;
        const std::string directSuffixText = "DIRECT_RECORD_TAIL";
        directFixture.suffix.assign(directSuffixText.begin(),
                                    directSuffixText.end());
        const std::size_t secondRecordLocalOffset =
            static_cast<std::size_t>(directSize) + 4u;
        const std::size_t requiredSegmentSize = dataRegionRelativeOffset + 4u
            + static_cast<std::size_t>(directSize) + 4u
            + directFixture.suffix.size();
        const std::size_t dataSegmentSize = (requiredSegmentSize + 63u) & ~63u;
        directFixture.dataSegmentSize = dataSegmentSize;
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
                                              std::uint32_t size,
                                              std::uint32_t
                                                  objectDataAlignmentOffset) {
            putU16Local(offset, DRW_DataStorageConst::SEGMENT_SIGNATURE);
            for (std::size_t byte = 0; byte < 6u; ++byte)
                directFixture.bytes[offset + 2u + byte] =
                    static_cast<std::uint8_t>(name[byte]);
            putU32(directFixture.bytes, offset + 8u, index);
            putU32(directFixture.bytes, offset + 16u, size);
            putU32(directFixture.bytes, offset + 24u, 1u);
            putU32(directFixture.bytes, offset + 36u,
                   objectDataAlignmentOffset);
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
                            segments[index].second,
                            index == 2u
                                ? static_cast<std::uint32_t>(
                                      dataRegionRelativeOffset / 16u)
                                : 0u);
            const std::size_t entry =
                segmentIndexOffset + 48u + index * 12u;
            putU64Local(entry, segments[index].first);
            putU32(directFixture.bytes, entry + 8u, segments[index].second);
        }

        putU32(directFixture.bytes, dataIndexOffset + 48u, 2u);
        const std::uint32_t firstHeaderOffset = reverseDirectoryAndPayload
            ? DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE : 0u;
        const std::uint32_t secondHeaderOffset = reverseDirectoryAndPayload
            ? 0u : DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE;
        putU32(directFixture.bytes, dataIndexOffset + 56u, 2u);
        putU32(directFixture.bytes, dataIndexOffset + 60u, firstHeaderOffset);
        putU32(directFixture.bytes, dataIndexOffset + 64u, 0u);
        putU32(directFixture.bytes, dataIndexOffset + 68u, 2u);
        putU32(directFixture.bytes, dataIndexOffset + 72u, secondHeaderOffset);
        putU32(directFixture.bytes, dataIndexOffset + 76u, 0u);

        const std::size_t firstRecordHeader = dataOffset + 48u;
        putU32(directFixture.bytes, firstRecordHeader,
               DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE);
        putU64Local(firstRecordHeader + 8u,
                    reverseDirectoryAndPayload ? 0xa18u : 0xa17u);
        putU32(directFixture.bytes, firstRecordHeader + 16u, 0u);
        const std::size_t secondRecordHeader =
            firstRecordHeader + DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE;
        putU32(directFixture.bytes, secondRecordHeader,
               DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE);
        putU64Local(secondRecordHeader + 8u,
                    reverseDirectoryAndPayload ? 0xa17u : 0xa18u);
        putU32(directFixture.bytes, secondRecordHeader + 16u,
               static_cast<std::uint32_t>(secondRecordLocalOffset));

        const std::size_t dataRegionOffset =
            dataOffset + dataRegionRelativeOffset;
        directFixture.dataRegionOffset = dataRegionOffset;
        const std::size_t packedHeadersEnd = dataOffset + 48u + 2u * 20u;
        std::fill(directFixture.bytes.begin()
                      + static_cast<std::ptrdiff_t>(packedHeadersEnd),
                  directFixture.bytes.begin()
                      + static_cast<std::ptrdiff_t>(dataRegionOffset),
                  0x62u);
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
        DRW_DataStorageConst::DATA_RECORD_INLINE_MAX_SIZE - 1u, 96u, false);
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
        DRW_DataStorageConst::DATA_RECORD_INLINE_MAX_SIZE, 96u, false);
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
        DRW_DataStorageConst::DATA_RECORD_INLINE_MAX_SIZE + 1u, 96u, false);
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

    const DirectFixture offsetAnchoredFixture = makeDirectFixture(37u, 112u,
                                                                   true);
    const DRW_DataStorageSection offsetAnchored = DRW_parseDataStorage(
        offsetAnchoredFixture.bytes, DRW::AC1027);
    const DRW_DataStorageRecord* offsetAnchoredFirst =
        offsetAnchored.findRecordByHandle(0xa17u);
    const DRW_DataStorageRecord* offsetAnchoredSecond =
        offsetAnchored.findRecordByHandle(0xa18u);
    std::vector<std::uint8_t> offsetAnchoredPayload(37u);
    for (std::size_t byte = 0; byte < offsetAnchoredPayload.size(); ++byte)
        offsetAnchoredPayload[byte] =
            static_cast<std::uint8_t>(byte % 251u + 1u);
    t.expect(offsetAnchored.structurallyValid && offsetAnchored.replayAllowed
                 && offsetAnchored.records.size() == 2u
                 && offsetAnchored.records[0].handle == 0xa17u
                 && offsetAnchored.records[0].recordLocalOffset == 41u
                 && offsetAnchored.records[1].handle == 0xa18u
                 && offsetAnchored.records[1].recordLocalOffset == 0u
                 && offsetAnchoredFirst != nullptr
                 && offsetAnchoredFirst->payload
                        == offsetAnchoredFixture.suffix
                 && offsetAnchoredSecond != nullptr
                 && offsetAnchoredSecond->recordOffset
                        == offsetAnchoredFixture.dataRegionOffset
                 && offsetAnchoredSecond->payload == offsetAnchoredPayload,
             "DataStorage follows object-data anchor despite extra padding and unsorted record offsets");

    const auto rejectsInvalidObjectDataAnchor = [&](
        DirectFixture changed, std::uint32_t anchorUnits,
        const char* label) {
        putU32(changed.bytes, changed.dataSegmentOffset + 36u, anchorUnits);
        const DRW_DataStorageSection rejected =
            DRW_parseDataStorage(changed.bytes, DRW::AC1027);
        const bool hasAnchorDiagnostic = std::any_of(
            rejected.diagnostics.cbegin(), rejected.diagnostics.cend(),
            [](const DRW_DataStorageDiagnostic& diagnostic) {
                return diagnostic.code
                    == "datastorage-object-data-alignment-offset-invalid";
            });
        t.expect(!rejected.structurallyValid && !rejected.replayAllowed
                     && rejected.records.empty() && hasAnchorDiagnostic,
                 label);
    };
    const DirectFixture invalidAnchorFixture =
        makeDirectFixture(37u, 112u, false);
    rejectsInvalidObjectDataAnchor(
        invalidAnchorFixture, 2u,
        "DataStorage rejects an object-data anchor before record headers");
    rejectsInvalidObjectDataAnchor(
        invalidAnchorFixture,
        static_cast<std::uint32_t>(
            invalidAnchorFixture.dataSegmentSize / 16u + 1u),
        "DataStorage rejects an object-data anchor beyond its segment");

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

    std::vector<std::uint8_t> anchoredReaderBacking{0u};
    dwgRW anchoredParent(nullptr);
    DataStorageLinkTestReader anchoredReader(
        std::unique_ptr<dwgBuffer>(new dwgBuffer(
            anchoredReaderBacking.data(), anchoredReaderBacking.size())),
        &anchoredParent);
    anchoredReader.setFormatVersion(DRW::AC1027);
    anchoredReader.m_dataStorageSections.push_back(offsetAnchored);
    DRW_ModelerGeometry anchoredBody(DRW::E3DSOLID);
    anchoredBody.handle = 0xb1au;
    anchoredBody.setHasDataStorageBinaryData(true);
    anchoredBody.dataStorageHandle = 0xa18u;
    anchoredBody.dataStorageHandleKey = "A18";
    anchoredReader.linkDataStorage(anchoredBody);
    anchoredReader.finalizeDataStorageLinks();
    t.expect(anchoredBody.hasDataStorageRecord
                 && anchoredBody.dataStorageHandle == 0xa18u
                 && anchoredBody.dataStorageHandleKey == "A18"
                 && anchoredBody.dataStorageData == offsetAnchoredPayload
                 && anchoredReader.m_dataStorageLinkFailures == 0u
                 && anchoredReader.m_dataStorageLinkedRecords.size() == 1u
                 && anchoredReader.m_dataStorageOrphanRecords == 1u,
             "header-anchored DataStorage payload resolves to its entity handle");

    std::vector<std::uint8_t> unqualifiedBodyReaderBacking{0u};
    dwgRW unqualifiedBodyParent(nullptr);
    DataStorageLinkTestReader unqualifiedBodyReader(
        std::unique_ptr<dwgBuffer>(new dwgBuffer(
            unqualifiedBodyReaderBacking.data(),
            unqualifiedBodyReaderBacking.size())),
        &unqualifiedBodyParent);
    unqualifiedBodyReader.setFormatVersion(DRW::AC1027);
    unqualifiedBodyReader.m_dataStorageSections.push_back(
        DRW_parseDataStorage(fixture.bytes, DRW::AC1027));
    DRW_ModelerGeometry unqualifiedBody(DRW::BODY);
    unqualifiedBody.handle = 0xb19u;
    unqualifiedBody.setHasDataStorageBinaryData(true);
    unqualifiedBody.dataStorageHandle = 0xa17u;
    unqualifiedBody.dataStorageHandleKey = "A17";
    unqualifiedBody.m_modelerVersion = 168u;
    unqualifiedBodyReader.linkDataStorage(unqualifiedBody);
    unqualifiedBodyReader.finalizeDataStorageLinks();
    t.expect(unqualifiedBody.hasDataStorageRecord
                 && unqualifiedBody.dataStorageData == payload
                 && unqualifiedBody.dataStorageHandle == 0xa17u
                 && unqualifiedBody.m_dwgSourceVersion == DRW::UNKNOWNV
                 && unqualifiedBody.m_modelerVersion == 168u
                 && !unqualifiedBody.m_hasModelerData
                 && unqualifiedBodyReader.m_dataStorageLinkFailures == 0u
                 && unqualifiedBodyReader.m_dataStorageOrphanRecords == 1u,
             "BODY keeps exact AcDs bytes opaque without REGION/3DSOLID SAB normalization");

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

void testDataStorageRecordCountAlignmentAnchors(TestContext& t) {
    struct WireRecord {
        std::uint64_t handle;
        std::string payload;
    };

    const auto roundUp = [](std::size_t value, std::size_t alignment) {
        return (value + alignment - 1u) & ~(alignment - 1u);
    };
    const auto checkProfile = [&t, &roundUp](
        const std::string& label, const std::vector<WireRecord>& records,
        const std::vector<std::size_t>& dataIndexOrder,
        std::uint32_t dataRegionAlignmentUnits) {
        constexpr std::size_t segmentIndexOffset = 64u;
        constexpr std::size_t segmentIndexSize = 128u;
        const std::size_t dataIndexEntriesEnd = 56u
            + records.size() * DRW_DataStorageConst::DATA_INDEX_ENTRY_SIZE;
        const std::size_t dataIndexSegmentSize = roundUp(dataIndexEntriesEnd, 64u);
        const std::size_t dataIndexOffset =
            segmentIndexOffset + segmentIndexSize;
        const std::size_t dataOffset = dataIndexOffset + dataIndexSegmentSize;
        const std::size_t dataRegionRelativeOffset =
            static_cast<std::size_t>(dataRegionAlignmentUnits) * 16u;
        const std::size_t minimumHeaderEnd =
            DRW_DataStorageConst::SEGMENT_HEADER_SIZE
            + records.size() * DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE;
        const std::size_t countDerivedAlignedBase =
            roundUp(minimumHeaderEnd, 16u);

        std::vector<std::size_t> payloadOffsets(records.size(), 0u);
        std::size_t payloadBytes = 0u;
        for (std::size_t index = 0; index < records.size(); ++index) {
            payloadOffsets[index] = payloadBytes;
            payloadBytes += 4u + records[index].payload.size();
        }
        const std::size_t dataSegmentSize =
            roundUp(dataRegionRelativeOffset + payloadBytes + 64u, 64u);
        const std::size_t blobOffset = dataOffset + dataSegmentSize;
        constexpr std::size_t blobSegmentSize = 192u;
        const std::size_t sectionSize = blobOffset + blobSegmentSize;
        const bool orderIsValid = dataIndexOrder.size() == records.size()
            && std::all_of(records.cbegin(), records.cend(),
                           [](const WireRecord& record) {
                               return !record.payload.empty();
                           });
        std::vector<bool> orderSeen(records.size(), false);
        for (const std::size_t recordIndex : dataIndexOrder) {
            if (recordIndex >= records.size() || orderSeen[recordIndex])
                continue;
            orderSeen[recordIndex] = true;
        }
        const bool orderIsPermutation = orderIsValid
            && std::all_of(orderSeen.cbegin(), orderSeen.cend(),
                           [](bool seen) { return seen; });
        if (!orderIsPermutation
            || dataRegionRelativeOffset <= countDerivedAlignedBase
            || dataIndexSegmentSize < dataIndexEntriesEnd) {
            t.expect(false, label + " has a distinct, bounded explicit anchor");
            return;
        }

        std::vector<std::uint8_t> bytes(sectionSize, 0u);
        const auto putU16 = [&bytes](std::size_t offset, std::uint16_t value) {
            bytes[offset] = static_cast<std::uint8_t>(value & 0xffu);
            bytes[offset + 1u] = static_cast<std::uint8_t>(value >> 8u);
        };
        const auto putU32 = [&bytes](std::size_t offset, std::uint32_t value) {
            for (unsigned byte = 0; byte < 4u; ++byte)
                bytes[offset + byte] =
                    static_cast<std::uint8_t>(value >> (byte * 8u));
        };
        const auto putU64 = [&putU32](std::size_t offset, std::uint64_t value) {
            putU32(offset, static_cast<std::uint32_t>(value));
            putU32(offset + 4u, static_cast<std::uint32_t>(value >> 32u));
        };
        const auto putSegment = [&bytes, &putU16, &putU32](
            std::size_t offset, const char* name, std::uint32_t index,
            std::size_t size, std::uint32_t objectDataAlignmentOffset) {
            putU16(offset, DRW_DataStorageConst::SEGMENT_SIGNATURE);
            for (std::size_t byte = 0; byte < 6u; ++byte)
                bytes[offset + 2u + byte] =
                    static_cast<std::uint8_t>(name[byte]);
            putU32(offset + 8u, index);
            putU32(offset + 16u, static_cast<std::uint32_t>(size));
            putU32(offset + 24u, 1u);
            putU32(offset + 36u, objectDataAlignmentOffset);
        };

        putU32(4u, DRW_DataStorageConst::HEADER_SIZE);
        putU32(12u, 1u);
        putU32(20u, 1u);
        putU32(24u, static_cast<std::uint32_t>(segmentIndexOffset));
        putU32(32u, 4u);
        putU32(36u, UINT32_MAX);
        putU32(40u, 1u);
        putU32(44u, UINT32_MAX);
        putU32(48u, UINT32_MAX);
        putU32(52u, static_cast<std::uint32_t>(sectionSize));

        const std::array<std::tuple<std::size_t, const char*, std::size_t>, 4>
            segments{{
                {segmentIndexOffset, "segidx", segmentIndexSize},
                {dataIndexOffset, "datidx", dataIndexSegmentSize},
                {dataOffset, "_data_", dataSegmentSize},
                {blobOffset, "blob01", blobSegmentSize}}};
        for (std::size_t index = 0; index < segments.size(); ++index) {
            const std::size_t offset = std::get<0>(segments[index]);
            const std::size_t size = std::get<2>(segments[index]);
            putSegment(offset, std::get<1>(segments[index]),
                       static_cast<std::uint32_t>(index), size,
                       index == 2u ? dataRegionAlignmentUnits : 0u);
            const std::size_t segmentIndexEntry = segmentIndexOffset
                + DRW_DataStorageConst::SEGMENT_HEADER_SIZE
                + index * DRW_DataStorageConst::SEGMENT_INDEX_ENTRY_SIZE;
            putU64(segmentIndexEntry, offset);
            putU32(segmentIndexEntry + 8u, static_cast<std::uint32_t>(size));
        }

        const std::size_t dataIndexHeader = dataIndexOffset
            + DRW_DataStorageConst::SEGMENT_HEADER_SIZE;
        putU32(dataIndexHeader, static_cast<std::uint32_t>(records.size()));
        for (std::size_t row = 0; row < dataIndexOrder.size(); ++row) {
            const std::size_t recordIndex = dataIndexOrder[row];
            const std::size_t entry = dataIndexHeader + 8u
                + row * DRW_DataStorageConst::DATA_INDEX_ENTRY_SIZE;
            putU32(entry, 2u);
            putU32(entry + 4u, static_cast<std::uint32_t>(
                recordIndex * DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE));
            putU32(entry + 8u, 0u);
        }

        const std::size_t recordHeaders = dataOffset
            + DRW_DataStorageConst::SEGMENT_HEADER_SIZE;
        const std::size_t dataRegion =
            dataOffset + dataRegionRelativeOffset;
        std::fill(bytes.begin() + static_cast<std::ptrdiff_t>(
                      recordHeaders + records.size()
                          * DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE),
                  bytes.begin() + static_cast<std::ptrdiff_t>(dataRegion), 0xe7u);
        for (std::size_t index = 0; index < records.size(); ++index) {
            const std::size_t header = recordHeaders
                + index * DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE;
            putU32(header, DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE);
            putU64(header + 8u, records[index].handle);
            putU32(header + 16u,
                   static_cast<std::uint32_t>(payloadOffsets[index]));

            const std::size_t payload = dataRegion + payloadOffsets[index];
            putU32(payload,
                   static_cast<std::uint32_t>(records[index].payload.size()));
            std::copy(records[index].payload.begin(),
                      records[index].payload.end(),
                      bytes.begin() + static_cast<std::ptrdiff_t>(payload + 4u));
        }

        const DRW_DataStorageSection parsed =
            DRW_parseDataStorage(bytes, DRW::AC1027);
        const bool exactAnchor = parsed.segments.size() == 4u
            && parsed.segments[2].objectDataAlignmentOffset
                == dataRegionAlignmentUnits
            && parsed.segments[2].offset + dataRegionRelativeOffset
                == dataRegion;
        bool exactAssociations = parsed.records.size() == records.size();
        for (const WireRecord& expected : records) {
            const DRW_DataStorageRecord* actual =
                parsed.findRecordByHandle(expected.handle);
            const std::vector<std::uint8_t> expectedPayload(
                expected.payload.begin(), expected.payload.end());
            exactAssociations = exactAssociations && actual != nullptr
                && !actual->isBlobReference && actual->payload == expectedPayload;
        }
        bool exactEntityLinks = records.empty();
        if (!records.empty()) {
            std::vector<std::uint8_t> readerBacking{0u};
            dwgRW parent(nullptr);
            DataStorageLinkTestReader reader(
                std::unique_ptr<dwgBuffer>(new dwgBuffer(
                    readerBacking.data(), readerBacking.size())),
                &parent);
            reader.setFormatVersion(DRW::AC1027);
            reader.m_dataStorageSections.push_back(parsed);

            std::vector<std::unique_ptr<DRW_ModelerGeometry>> modelers;
            modelers.reserve(records.size());
            for (std::size_t index = 0; index < records.size(); ++index) {
                const DRW_DataStorageRecord* record =
                    parsed.findRecordByHandle(records[index].handle);
                if (record == nullptr)
                    continue;
                std::unique_ptr<DRW_ModelerGeometry> modeler(
                    new DRW_ModelerGeometry(DRW::E3DSOLID));
                modeler->handle = 0xb000u + index;
                modeler->setHasDataStorageBinaryData(true);
                modeler->dataStorageHandle = records[index].handle;
                modeler->dataStorageHandleKey = record->handleKey;
                reader.linkDataStorage(*modeler);
                modelers.push_back(std::move(modeler));
            }
            reader.finalizeDataStorageLinks();
            exactEntityLinks = modelers.size() == records.size()
                && reader.m_dataStorageLinkFailures == 0u
                && reader.m_dataStorageLinkedRecords.size() == records.size()
                && reader.m_dataStorageOrphanRecords == 0u;
            for (std::size_t index = 0; index < modelers.size(); ++index) {
                const std::vector<std::uint8_t> expectedPayload(
                    records[index].payload.begin(), records[index].payload.end());
                exactEntityLinks = exactEntityLinks
                    && modelers[index]->hasDataStorageRecord
                    && modelers[index]->dataStorageHandle
                        == records[index].handle
                    && modelers[index]->dataStorageData == expectedPayload;
            }
        }
        t.expect(exactAnchor
                     && countDerivedAlignedBase < dataRegionRelativeOffset,
                 label + " uses the explicit anchor beyond count-derived alignment");
        t.expect(!parsed.parseFailed && parsed.structurallyValid
                     && parsed.replayAllowed && exactAssociations,
                 label + " preserves every handle/payload association");
        t.expect(exactEntityLinks,
                 label + " links every modeler entity to its exact payload");

        if (records.size() == 4u) {
            putU32(40u, 0u); // Point the header's DataIndex slot at segidx.
            const DRW_DataStorageSection rejectedDataIndexKind =
                DRW_parseDataStorage(bytes, DRW::AC1027);
            const bool hasDataIndexKindDiagnostic = std::any_of(
                rejectedDataIndexKind.diagnostics.cbegin(),
                rejectedDataIndexKind.diagnostics.cend(),
                [](const DRW_DataStorageDiagnostic& diagnostic) {
                    return diagnostic.code
                        == "datastorage-data-index-kind-invalid";
                });
            t.expect(!rejectedDataIndexKind.structurallyValid
                         && !rejectedDataIndexKind.replayAllowed
                         && rejectedDataIndexKind.dataIndexEntries.empty()
                         && rejectedDataIndexKind.records.empty()
                         && hasDataIndexKindDiagnostic,
                     "DataStorage rejects a header DataIndex reference to the segment-index segment");
            putU32(40u, 1u);

            // A DataIndex row must target a data-record segment, not a Blob01
            // page. Give the Blob01 segment bytes that are plausible as a
            // record header/payload so the role check—not incidental bounds—
            // is what rejects the association.
            constexpr std::size_t invalidRow = 2u;
            const std::size_t invalidEntry = dataIndexHeader + 8u
                + invalidRow * DRW_DataStorageConst::DATA_INDEX_ENTRY_SIZE;
            const std::uint64_t rejectedBlobHandle =
                records[dataIndexOrder[invalidRow]].handle;
            const std::size_t blobRecordHeader = blobOffset
                + DRW_DataStorageConst::SEGMENT_HEADER_SIZE;
            const std::size_t blobRecordPayload = blobOffset + 128u;
            putU32(blobOffset + 36u, 8u);
            putU32(invalidEntry, 3u);
            putU32(invalidEntry + 4u, 0u);
            putU32(blobRecordHeader,
                   DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE);
            putU32(blobRecordHeader + 4u, 1u);
            putU64(blobRecordHeader + 8u, 0xbad01u);
            putU32(blobRecordHeader + 16u, 0u);
            putU32(blobRecordPayload, 4u);
            putU32(blobRecordPayload + 4u, 0xa1b2c3d4u);
            const DRW_DataStorageSection rejectedBlobTarget =
                DRW_parseDataStorage(bytes, DRW::AC1027);
            const bool hasBlobTargetDiagnostic = std::any_of(
                rejectedBlobTarget.diagnostics.cbegin(),
                rejectedBlobTarget.diagnostics.cend(),
                [](const DRW_DataStorageDiagnostic& diagnostic) {
                    return diagnostic.code
                        == "datastorage-record-segment-kind-invalid";
                });
            bool validBlobNeighborsRetained =
                rejectedBlobTarget.records.size() == records.size() - 1u
                && rejectedBlobTarget.findRecordByHandle(rejectedBlobHandle)
                    == nullptr;
            for (std::size_t index = 0;
                 validBlobNeighborsRetained && index < records.size(); ++index) {
                if (index == dataIndexOrder[invalidRow])
                    continue;
                const DRW_DataStorageRecord* retained =
                    rejectedBlobTarget.findRecordByHandle(records[index].handle);
                const std::vector<std::uint8_t> expectedPayload(
                    records[index].payload.begin(), records[index].payload.end());
                validBlobNeighborsRetained = retained != nullptr
                    && retained->payload == expectedPayload;
            }
            t.expect(!rejectedBlobTarget.structurallyValid
                         && !rejectedBlobTarget.replayAllowed
                         && hasBlobTargetDiagnostic
                         && validBlobNeighborsRetained,
                     "DataStorage rejects DataIndex rows targeting Blob01 while retaining valid data records");
            putU32(blobOffset + 36u, 0u);
            putU32(invalidEntry, 2u);
            putU32(invalidEntry + 4u, static_cast<std::uint32_t>(
                dataIndexOrder[invalidRow]
                * DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE));

            // ODA §24.2.2.2 reserves segment index zero for a stub row. It
            // remains ignorable even when its unused offset/schema fields are
            // nonzero; the other valid rows should still be parsed.
            const std::uint64_t stubbedHandle =
                records[dataIndexOrder[invalidRow]].handle;
            putU32(invalidEntry, 0u);
            putU32(invalidEntry + 4u, UINT32_MAX);
            putU32(invalidEntry + 8u, 0x77u);
            const DRW_DataStorageSection ignoredStubRow =
                DRW_parseDataStorage(bytes, DRW::AC1027);
            const bool hasUnexpectedStubDiagnostic = std::any_of(
                ignoredStubRow.diagnostics.cbegin(),
                ignoredStubRow.diagnostics.cend(),
                [](const DRW_DataStorageDiagnostic& diagnostic) {
                    return diagnostic.code
                        == "datastorage-record-segment-kind-invalid";
                });
            bool validStubNeighborsRetained =
                !ignoredStubRow.parseFailed
                && ignoredStubRow.structurallyValid
                && ignoredStubRow.replayAllowed
                && ignoredStubRow.records.size() == records.size() - 1u
                && ignoredStubRow.findRecordByHandle(stubbedHandle) == nullptr
                && !hasUnexpectedStubDiagnostic;
            for (std::size_t index = 0;
                 validStubNeighborsRetained && index < records.size(); ++index) {
                if (index == dataIndexOrder[invalidRow])
                    continue;
                const DRW_DataStorageRecord* retained =
                    ignoredStubRow.findRecordByHandle(records[index].handle);
                const std::vector<std::uint8_t> expectedPayload(
                    records[index].payload.begin(), records[index].payload.end());
                validStubNeighborsRetained = retained != nullptr
                    && retained->payload == expectedPayload;
            }
            t.expect(validStubNeighborsRetained,
                     "DataStorage ignores a nonzero segment-index-zero stub row and retains valid data records");
            putU32(invalidEntry, 2u);
            putU32(invalidEntry + 4u, static_cast<std::uint32_t>(
                dataIndexOrder[invalidRow]
                * DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE));
            putU32(invalidEntry + 8u, 0u);

            // Point one DataIndex row at the object-data region itself. This
            // is physically inside the segment but is not a record-header
            // location; it must not be decoded as a header or payload anchor.
            putU32(invalidEntry + 4u, static_cast<std::uint32_t>(
                dataRegionRelativeOffset
                - DRW_DataStorageConst::SEGMENT_HEADER_SIZE));
            const DRW_DataStorageSection rejectedHeaderReference =
                DRW_parseDataStorage(bytes, DRW::AC1027);
            const bool hasHeaderBoundsDiagnostic = std::any_of(
                rejectedHeaderReference.diagnostics.cbegin(),
                rejectedHeaderReference.diagnostics.cend(),
                [](const DRW_DataStorageDiagnostic& diagnostic) {
                    return diagnostic.code
                        == "datastorage-record-header-outside-data-region";
                });
            const std::uint64_t rejectedHandle =
                records[dataIndexOrder[invalidRow]].handle;
            bool validNeighborsRetained =
                rejectedHeaderReference.records.size() == records.size() - 1u
                && rejectedHeaderReference.findRecordByHandle(rejectedHandle)
                    == nullptr;
            for (std::size_t index = 0;
                 validNeighborsRetained && index < records.size(); ++index) {
                if (index == dataIndexOrder[invalidRow])
                    continue;
                const DRW_DataStorageRecord* retained =
                    rejectedHeaderReference.findRecordByHandle(
                        records[index].handle);
                const std::vector<std::uint8_t> expectedPayload(
                    records[index].payload.begin(), records[index].payload.end());
                validNeighborsRetained = retained != nullptr
                    && retained->payload == expectedPayload;
            }
            t.expect(!rejectedHeaderReference.structurallyValid
                         && !rejectedHeaderReference.replayAllowed
                         && hasHeaderBoundsDiagnostic
                         && validNeighborsRetained,
                     "DataIndex row into payload region is rejected without losing adjacent valid records");

            // An in-range DataIndex header can still claim a payload start
            // inside a different record. Keep the declared boundary, reject
            // the overlapping records, and do not reinterpret either byte
            // range as a plausible record. The independently bounded records
            // on either side remain available for diagnostics only.
            const std::size_t falseRecordHeader = recordHeaders
                + records.size()
                    * DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE;
            putU32(falseRecordHeader,
                   DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE);
            putU32(falseRecordHeader + 4u, 1u);
            putU64(falseRecordHeader + 8u, 0xfadeu);
            putU32(falseRecordHeader + 16u,
                   static_cast<std::uint32_t>(payloadOffsets[1] + 4u));
            putU32(invalidEntry + 4u, static_cast<std::uint32_t>(
                falseRecordHeader
                - dataOffset
                - DRW_DataStorageConst::SEGMENT_HEADER_SIZE));
            const DRW_DataStorageSection overlappingRecordBoundary =
                DRW_parseDataStorage(bytes, DRW::AC1027);
            const DRW_DataStorageRecord* firstUnambiguous =
                overlappingRecordBoundary.findRecordByHandle(records[0].handle);
            const DRW_DataStorageRecord* secondUnambiguous =
                overlappingRecordBoundary.findRecordByHandle(records[2].handle);
            const bool hasOverlappingRecordDiagnostic = std::any_of(
                overlappingRecordBoundary.diagnostics.cbegin(),
                overlappingRecordBoundary.diagnostics.cend(),
                [](const DRW_DataStorageDiagnostic& diagnostic) {
                    return diagnostic.code
                        == "datastorage-record-payload-out-of-record";
                });
            const bool hasFalseRecordDiagnostic = std::any_of(
                overlappingRecordBoundary.diagnostics.cbegin(),
                overlappingRecordBoundary.diagnostics.cend(),
                [](const DRW_DataStorageDiagnostic& diagnostic) {
                    return diagnostic.code
                        == "datastorage-record-inline-size-over-limit";
                });
            t.expect(!overlappingRecordBoundary.structurallyValid
                         && !overlappingRecordBoundary.replayAllowed
                         && overlappingRecordBoundary.records.size() == 2u
                         && firstUnambiguous != nullptr
                         && firstUnambiguous->payload
                             == std::vector<std::uint8_t>(
                                 records[0].payload.begin(),
                                 records[0].payload.end())
                         && secondUnambiguous != nullptr
                         && secondUnambiguous->payload
                             == std::vector<std::uint8_t>(
                                 records[2].payload.begin(),
                                 records[2].payload.end())
                         && overlappingRecordBoundary.findRecordByHandle(
                                records[1].handle) == nullptr
                         && overlappingRecordBoundary.findRecordByHandle(
                                records[3].handle) == nullptr
                         && overlappingRecordBoundary.findRecordByHandle(
                                0xfadeu) == nullptr
                         && hasOverlappingRecordDiagnostic
                         && hasFalseRecordDiagnostic,
                     "DataStorage refuses an in-range overlapping record boundary and retains only independently bounded neighbors");
        }
    };

    const std::vector<WireRecord> noRecords;
    const std::vector<WireRecord> oneRecord{{0xa401u, "NEXT_RECORD_SENTINEL"}};
    const std::vector<WireRecord> fourRecords{{
        {0xa501u, "first-record"},
        {0xa502u, "second-record"},
        {0xa503u, "third-record"},
        {0xa504u, "NEXT_RECORD_SENTINEL"}}};
    std::vector<WireRecord> twelveRecords;
    for (std::size_t index = 0; index < 12u; ++index) {
        twelveRecords.push_back({
            0xa600u + index,
            index == 11u ? "NEXT_RECORD_SENTINEL"
                         : "record-" + std::to_string(index)});
    }

    checkProfile("empty DataStorage record index", noRecords, {}, 4u);
    checkProfile("single-record DataStorage index", oneRecord, {0u}, 6u);
    checkProfile("four-record DataStorage index", fourRecords,
                 {2u, 0u, 3u, 1u}, 12u);
    checkProfile("twelve-record DataStorage index", twelveRecords,
                 {7u, 0u, 11u, 3u, 8u, 1u, 10u, 5u, 2u, 9u, 4u, 6u},
                 20u);
}

void testDataStorageDefaultSchemaOnlySection(TestContext& t) {
    constexpr std::size_t segmentIndexOffset = 64u;
    constexpr std::size_t segmentIndexSize = 128u;
    constexpr std::size_t schemaIndexOffset =
        segmentIndexOffset + segmentIndexSize;
    constexpr std::size_t schemaIndexSize = 192u;
    constexpr std::size_t schemaDataOffset =
        schemaIndexOffset + schemaIndexSize;
    constexpr std::size_t schemaDataSize = 256u;
    constexpr std::size_t searchOffset = schemaDataOffset + schemaDataSize;
    constexpr std::size_t searchSize = 128u;
    constexpr std::size_t sectionSize = searchOffset + searchSize;
    constexpr std::size_t schemaPayloadOffset =
        schemaDataOffset + DRW_DataStorageConst::SEGMENT_HEADER_SIZE;
    constexpr std::size_t schemaNamesRelativeOffset = 224u;

    std::vector<std::uint8_t> bytes(sectionSize, 0u);
    const auto putU16 = [&bytes](std::size_t offset, std::uint16_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value & 0xffu);
        bytes[offset + 1u] = static_cast<std::uint8_t>(value >> 8u);
    };
    const auto putU32 = [&bytes](std::size_t offset, std::uint32_t value) {
        for (unsigned byte = 0; byte < 4u; ++byte)
            bytes[offset + byte] =
                static_cast<std::uint8_t>(value >> (byte * 8u));
    };
    const auto putU64 = [&putU32](std::size_t offset, std::uint64_t value) {
        putU32(offset, static_cast<std::uint32_t>(value));
        putU32(offset + 4u, static_cast<std::uint32_t>(value >> 32u));
    };
    const auto putSegment = [&bytes, &putU16, &putU32](
        std::size_t offset, const char* name, std::uint32_t index,
        std::size_t size, std::uint32_t systemDataAlignmentOffset) {
        putU16(offset, DRW_DataStorageConst::SEGMENT_SIGNATURE);
        for (std::size_t byte = 0; byte < 6u; ++byte)
            bytes[offset + 2u + byte] =
                static_cast<std::uint8_t>(name[byte]);
        putU32(offset + 8u, index);
        putU32(offset + 16u, static_cast<std::uint32_t>(size));
        putU32(offset + 24u, 1u);
        putU32(offset + 32u, systemDataAlignmentOffset);
    };

    putU32(0u, 1u);
    putU32(4u, DRW_DataStorageConst::HEADER_SIZE);
    putU32(12u, 1u);
    putU32(20u, 1u);
    putU32(24u, static_cast<std::uint32_t>(segmentIndexOffset));
    putU32(32u, 4u);
    putU32(36u, 1u); // schidx
    putU32(40u, UINT32_MAX); // no datidx/data records
    putU32(44u, 3u); // search
    putU32(48u, UINT32_MAX); // no previous-save segment
    putU32(52u, static_cast<std::uint32_t>(sectionSize));

    const std::array<std::pair<std::size_t, std::size_t>, 4> segments{{
        {segmentIndexOffset, segmentIndexSize},
        {schemaIndexOffset, schemaIndexSize},
        {schemaDataOffset, schemaDataSize},
        {searchOffset, searchSize}}};
    const std::array<const char*, 4> names{{
        "segidx", "schidx", "schdat", "search"}};
    for (std::size_t index = 0; index < segments.size(); ++index) {
        putSegment(segments[index].first, names[index],
                   static_cast<std::uint32_t>(index), segments[index].second,
                   index == 2u
                       ? static_cast<std::uint32_t>(
                             schemaNamesRelativeOffset / 16u)
                       : 0u);
        const std::size_t entry = segmentIndexOffset
            + DRW_DataStorageConst::SEGMENT_HEADER_SIZE
            + index * DRW_DataStorageConst::SEGMENT_INDEX_ENTRY_SIZE;
        putU64(entry, segments[index].first);
        putU32(entry + 8u, static_cast<std::uint32_t>(segments[index].second));
    }

    // ODA §24.3's default schema-data segment starts with four opaque
    // eight-byte property headers before the five schema entries.
    const std::array<std::uint32_t, 4> unknownPropertyFlags{{1u, 1u, 1u, 0u}};
    for (std::size_t index = 0; index < unknownPropertyFlags.size(); ++index) {
        const std::size_t property = schemaPayloadOffset + index * 8u;
        putU32(property, 8u);
        putU32(property + 4u, unknownPropertyFlags[index]);
    }

    const std::size_t schemaIndexPayload =
        schemaIndexOffset + DRW_DataStorageConst::SEGMENT_HEADER_SIZE;
    putU32(schemaIndexPayload, 4u);
    putU32(schemaIndexPayload + 4u, 0u);
    for (std::size_t index = 0; index < unknownPropertyFlags.size(); ++index) {
        const std::size_t entry = schemaIndexPayload + 8u + index * 12u;
        putU32(entry, static_cast<std::uint32_t>(index));
        putU32(entry + 4u, 2u); // schdat segment index
        putU32(entry + 8u, static_cast<std::uint32_t>(index * 8u));
    }
    const std::size_t schemaPropertiesHeader =
        schemaIndexPayload + 8u
        + unknownPropertyFlags.size()
            * DRW_DataStorageConst::SCHEMA_INDEX_ENTRY_SIZE;
    putU64(schemaPropertiesHeader, 0x00000000000af10cull);
    putU32(schemaPropertiesHeader + 8u, 5u);
    putU32(schemaPropertiesHeader + 12u, 0u);

    std::array<std::uint32_t, 5> schemaOffsets{};
    std::vector<std::size_t> schemaPropertyTypeOffsets;
    std::size_t schemaCursor = 32u;
    const auto beginSchema = [&](std::size_t index, std::uint16_t indexCount) {
        schemaOffsets[index] = static_cast<std::uint32_t>(schemaCursor);
        putU16(schemaPayloadOffset + schemaCursor, indexCount);
        schemaCursor += 2u;
    };
    const auto writeProperty = [&](std::uint32_t flags,
                                   std::uint32_t type,
                                   std::uint32_t unknown2,
                                   const std::vector<std::uint64_t>& values) {
        const std::size_t property = schemaPayloadOffset + schemaCursor;
        putU32(property, flags);
        putU32(property + 4u, DRW_DataStorageConst::SCHEMA_PROPERTY_NO_NAME);
        putU32(property + 8u, type);
        schemaPropertyTypeOffsets.push_back(property + 8u);
        schemaCursor += 12u;
        if (flags == 8u) {
            putU32(schemaPayloadOffset + schemaCursor, unknown2);
            schemaCursor += 4u;
        }
        putU16(schemaPayloadOffset + schemaCursor,
               static_cast<std::uint16_t>(values.size()));
        schemaCursor += 2u;
        for (const std::uint64_t value : values) {
            if (type == 10u) {
                putU64(schemaPayloadOffset + schemaCursor, value);
                schemaCursor += 8u;
            } else {
                bytes[schemaPayloadOffset + schemaCursor] =
                    static_cast<std::uint8_t>(value);
                ++schemaCursor;
            }
        }
    };

    beginSchema(0u, 2u);
    putU64(schemaPayloadOffset + schemaCursor, 0u);
    putU64(schemaPayloadOffset + schemaCursor + 8u, 1u);
    schemaCursor += 16u;
    putU16(schemaPayloadOffset + schemaCursor, 2u);
    schemaCursor += 2u;
    writeProperty(0u, 10u, 0u, {2u, 3u}); // AcDbDs::ID values
    writeProperty(0u, 15u, 0u, {}); // ASM_Data has no typed value

    for (std::size_t index = 1u; index <= 3u; ++index) {
        beginSchema(index, 0u);
        putU16(schemaPayloadOffset + schemaCursor, 1u);
        schemaCursor += 2u;
        writeProperty(0u, 1u, 0u, {});
    }
    beginSchema(4u, 0u);
    putU16(schemaPayloadOffset + schemaCursor, 1u);
    schemaCursor += 2u;
    writeProperty(8u, 7u, 1u, {0u}); // HandleAttribute schema

    putU32(schemaDataOffset + schemaNamesRelativeOffset, 0u);
    for (std::size_t index = 0; index < schemaOffsets.size(); ++index) {
        const std::size_t entry = schemaPropertiesHeader + 16u + index * 12u;
        putU32(entry, 2u); // schdat segment index
        putU32(entry + 4u, schemaOffsets[index]);
        putU32(entry + 8u, static_cast<std::uint32_t>(index));
    }

    // The default search section has one empty schema-search record.
    const std::size_t searchPayload =
        searchOffset + DRW_DataStorageConst::SEGMENT_HEADER_SIZE;
    putU32(searchPayload, 1u);
    putU32(searchPayload + 4u, 0u);
    putU64(searchPayload + 8u, 0u);
    putU32(searchPayload + 16u, 0u);

    const DRW_DataStorageSection parsed =
        DRW_parseDataStorage(bytes, DRW::AC1027);
    const std::vector<std::uint8_t> idTwo{
        2u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};
    const std::vector<std::uint8_t> idThree{
        3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};
    const bool segmentProfile = parsed.segments.size() == 4u
        && parsed.segments[1].kind == DRW_DataStorageSegmentKind::SchemaIndex
        && parsed.segments[2].kind == DRW_DataStorageSegmentKind::SchemaData
        && parsed.segments[3].kind == DRW_DataStorageSegmentKind::Search
        && parsed.searchSegmentIndex == 3
        && parsed.dataIndexSegmentIndex == -1;
    const bool solidSchema = parsed.schemas.size() == 5u
        && parsed.schemas[0].index == 0u
        && parsed.schemas[0].indexes == std::vector<std::uint64_t>{0u, 1u}
        && parsed.schemas[0].properties.size() == 2u
        && parsed.schemas[0].properties[0].type == 10u
        && parsed.schemas[0].properties[0].typeSize == 8u
        && parsed.schemas[0].properties[0].values
            == std::vector<std::vector<std::uint8_t>>{idTwo, idThree}
        && parsed.schemas[0].properties[1].type == 15u
        && parsed.schemas[0].properties[1].typeSize == 0u
        && parsed.schemas[0].properties[1].values.empty();
    const bool markerSchemas = parsed.schemas[1].properties.size() == 1u
        && parsed.schemas[1].properties[0].type == 1u
        && parsed.schemas[2].properties.size() == 1u
        && parsed.schemas[2].properties[0].type == 1u
        && parsed.schemas[3].properties.size() == 1u
        && parsed.schemas[3].properties[0].type == 1u
        && parsed.schemas[4].properties.size() == 1u
        && parsed.schemas[4].properties[0].flags == 8u
        && parsed.schemas[4].properties[0].type == 7u
        && parsed.schemas[4].properties[0].unknown2 == 1u
        && parsed.schemas[4].properties[0].values
            == std::vector<std::vector<std::uint8_t>>{{0u}};
    const bool unknownPropertyHeaders =
        parsed.schemaUnknownProperties.size() == 4u
        && parsed.schemaUnknownProperties[0].dataSize == 8u
        && parsed.schemaUnknownProperties[0].flags == 1u
        && parsed.schemaUnknownProperties[1].flags == 1u
        && parsed.schemaUnknownProperties[2].flags == 1u
        && parsed.schemaUnknownProperties[3].flags == 0u;
    t.expect(!parsed.parseFailed && parsed.structurallyValid
                 && parsed.replayAllowed,
             "DataStorage default schema-only profile is structurally replayable");
    t.expect(segmentProfile && parsed.schemaCount == 5u,
             "DataStorage default profile locates its schema/search segments");
    t.expect(solidSchema,
             "DataStorage default profile preserves the ASM schema indexes and properties");
    t.expect(markerSchemas,
             "DataStorage default profile preserves the four marker schemas");
    t.expect(unknownPropertyHeaders,
             "DataStorage default profile preserves schema unknown-property headers");
    t.expect(parsed.records.empty() && parsed.dataIndexEntries.empty(),
             "DataStorage default schema-only profile contains no data records");

    const auto reportsCountCap = [](
        const DRW_DataStorageSection& section) {
        return std::any_of(
            section.diagnostics.cbegin(), section.diagnostics.cend(),
            [](const DRW_DataStorageDiagnostic& diagnostic) {
                return diagnostic.code == "datastorage-count-capped";
            });
    };
    const auto overwriteU32 = [](std::vector<std::uint8_t>& target,
                                 std::size_t offset,
                                 std::uint32_t value) {
        for (unsigned byte = 0; byte < 4u; ++byte)
            target[offset + byte] =
                static_cast<std::uint8_t>(value >> (byte * 8u));
    };
    std::vector<std::uint8_t> oversizedUnknownPropertyCount = bytes;
    overwriteU32(oversizedUnknownPropertyCount, schemaIndexPayload,
                 std::numeric_limits<std::uint32_t>::max());
    const DRW_DataStorageSection boundedUnknownProperties =
        DRW_parseDataStorage(oversizedUnknownPropertyCount, DRW::AC1027);
    t.expect(!boundedUnknownProperties.structurallyValid
                 && !boundedUnknownProperties.replayAllowed
                 && reportsCountCap(boundedUnknownProperties)
                 && boundedUnknownProperties.schemaPropertyEntries.size()
                     <= (schemaIndexSize - 56u) / 12u,
             "DataStorage bounds an oversized first schidx table count");

    std::vector<std::uint8_t> oversizedSchemaEntryCount = bytes;
    overwriteU32(oversizedSchemaEntryCount, schemaPropertiesHeader + 8u,
                 std::numeric_limits<std::uint32_t>::max());
    const DRW_DataStorageSection boundedSchemas =
        DRW_parseDataStorage(oversizedSchemaEntryCount, DRW::AC1027);
    t.expect(!boundedSchemas.structurallyValid
                 && !boundedSchemas.replayAllowed
                 && reportsCountCap(boundedSchemas)
                 && boundedSchemas.schemaIndexEntries.size()
                     <= (schemaIndexSize - 120u) / 12u,
             "DataStorage bounds an oversized second schidx table count");

    const auto reportsInvalidSchemaType = [](
        const DRW_DataStorageSection& section, std::uint64_t offset) {
        return std::any_of(
            section.diagnostics.cbegin(), section.diagnostics.cend(),
            [offset](const DRW_DataStorageDiagnostic& diagnostic) {
                return diagnostic.code
                           == "datastorage-schema-property-type-invalid"
                    && diagnostic.hasOffset && diagnostic.offset == offset;
            });
    };
    t.expect(schemaPropertyTypeOffsets.size() == 6u
                 && bytes[schemaPropertyTypeOffsets[1]]
                        == static_cast<std::uint8_t>(15u),
             "DataStorage generated profile identifies legal type-15 boundary");
    if (schemaPropertyTypeOffsets.size() == 6u) {
        const std::array<std::uint32_t, 2> invalidTypes{{
            16u, std::numeric_limits<std::uint32_t>::max()}};
        for (const std::uint32_t invalidType : invalidTypes) {
            std::vector<std::uint8_t> invalidSchemaType = bytes;
            overwriteU32(invalidSchemaType, schemaPropertyTypeOffsets[1],
                         invalidType);
            const DRW_DataStorageSection rejectedType =
                DRW_parseDataStorage(invalidSchemaType, DRW::AC1027);
            const bool laterSchemaRetained =
                rejectedType.schemas.size() == 4u
                && rejectedType.schemas.back().index == 4u
                && rejectedType.schemas.back().properties.size() == 1u
                && rejectedType.schemas.back().properties[0].type == 7u;
            t.expect(!rejectedType.structurallyValid
                         && !rejectedType.replayAllowed
                         && reportsInvalidSchemaType(
                             rejectedType,
                             schemaPropertyTypeOffsets[1])
                         && laterSchemaRetained,
                     "DataStorage rejects out-of-range schema property type "
                     + std::to_string(invalidType)
                     + " before type-size lookup and continues bounded parsing");
        }
    }
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

    const std::vector<std::uint8_t> asmBytes = minimalSab("ASM BinaryFile4");
    DRW_AcisBrep asmWire;
    DRW_SabHeader asmHeader;
    DRW_SabParseOutcome asmOutcome;
    t.expect(drw_decodeAcisWireframeWithHeader(
                 std::vector<unsigned char>(asmBytes.begin(), asmBytes.end()),
                 asmWire, asmHeader, asmOutcome)
                 && asmOutcome.complete
                 && asmHeader.signature == "ASM BinaryFile4"
                 && asmWire.empty(),
             "linear SAB signature scan retains ASM header decoding");

    const std::vector<std::uint8_t> shiftedAsmBytes =
        shiftSabBits(asmBytes, 5u);
    DRW_AcisBrep shiftedAsmWire;
    DRW_SabHeader shiftedAsmHeader;
    DRW_SabParseOutcome shiftedAsmOutcome;
    t.expect(drw_decodeAcisWireframeWithHeader(
                 std::vector<unsigned char>(shiftedAsmBytes.begin(),
                                            shiftedAsmBytes.end()),
                 shiftedAsmWire, shiftedAsmHeader, shiftedAsmOutcome)
                 && shiftedAsmOutcome.complete
                 && shiftedAsmHeader.signature == "ASM BinaryFile4",
             "linear SAB signature scan retains bit-shifted ASM decoding");

    std::vector<unsigned char> mixedSignatures {
        'A', 'S', 'M', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 'F', 'i', 'l', 'e', '4', 0
    };
    mixedSignatures.insert(mixedSignatures.end(), bytes.begin(), bytes.end());
    DRW_AcisBrep preferredWire;
    DRW_SabHeader preferredHeader;
    DRW_SabParseOutcome preferredOutcome;
    t.expect(drw_decodeAcisWireframeWithHeader(
                 mixedSignatures, preferredWire, preferredHeader,
                 preferredOutcome)
                 && preferredOutcome.complete
                 && preferredHeader.signature == "ACIS BinaryFile"
                 && preferredOutcome.sourceByteCount == bytes.size(),
             "SAB signature precedence prefers ACIS over an earlier ASM marker");
}

void testAcisWireframeHeaderProjection(TestContext& t) {
    const std::vector<std::uint8_t> mmBytes = vertexSab(21800u, 0u, 0u, 1.0);
    const std::vector<std::uint8_t> inchBytes =
        vertexSab(21800u, 0u, 0u, 25.4);
    DRW_AcisBrep mmProjection;
    DRW_AcisBrep inchProjection;
    DRW_SabHeader mmHeader;
    DRW_SabHeader inchHeader;
    DRW_SabParseOutcome mmOutcome;
    DRW_SabParseOutcome inchOutcome;
    const DRW_SabParseLimits limits;
    const bool decodedMm = drw_decodeAcisWireframeWithHeader(
        std::vector<unsigned char>(mmBytes.begin(), mmBytes.end()),
        mmProjection, mmHeader, mmOutcome, limits);
    const bool decodedInch = drw_decodeAcisWireframeWithHeader(
        std::vector<unsigned char>(inchBytes.begin(), inchBytes.end()),
        inchProjection, inchHeader, inchOutcome, limits);
    t.expect(decodedMm && decodedInch && mmOutcome.complete
                 && inchOutcome.complete
                 && mmHeader.signature == "ACIS BinaryFile"
                 && inchHeader.signature == "ACIS BinaryFile"
                 && mmHeader.version == 21800
                 && inchHeader.version == 21800
                 && mmHeader.unitsInMm == 1.0
                 && inchHeader.unitsInMm == 25.4,
             "ACIS wireframe header API returns the exact source unit metadata");
    t.expect(mmProjection.vertices.size() == 1u
                 && inchProjection.vertices.size() == 1u
                 && mmProjection.vertices[0].valid
                 && inchProjection.vertices[0].valid
                 && mmProjection.vertices[0].point.x == 1.0
                 && mmProjection.vertices[0].point.y == 2.0
                 && mmProjection.vertices[0].point.z == 3.0
                 && inchProjection.vertices[0].point.x == 1.0
                 && inchProjection.vertices[0].point.y == 2.0
                 && inchProjection.vertices[0].point.z == 3.0,
             "ACIS wireframe projection does not scale coordinates from header metadata");

    std::vector<unsigned char> truncated(mmBytes.begin(), mmBytes.end());
    truncated.resize(truncated.size() - 2u);
    DRW_AcisBrep rejectedProjection;
    DRW_SabHeader rejectedHeader;
    rejectedHeader.productId = "stale";
    rejectedHeader.unitsInMm = 25.4;
    DRW_SabParseOutcome rejectedOutcome;
    t.expect(!drw_decodeAcisWireframeWithHeader(
                 truncated, rejectedProjection, rejectedHeader,
                 rejectedOutcome, limits)
                 && rejectedProjection.empty()
                 && rejectedHeader.signature.empty()
                 && rejectedHeader.productId.empty()
                 && rejectedHeader.unitsInMm == 1.0,
             "ACIS header API clears both outputs after malformed input");
}

void testAcis21800ProfileFraming(TestContext& t) {
    const std::vector<std::uint8_t> bytes = vertexSab(21800u, 0u);
    DRW_SabData sab;
    const bool parsed = drw_parseSab(bytes.data(), bytes.size(), sab);
    t.expect(parsed && sab.header.signature == "ACIS BinaryFile"
                 && sab.header.version == 21800
                 && sab.header.numRecords == 0
                 && sab.records.size() == 3u
                 && sab.records.back().type == "End-of-ASM-data",
             "ACIS 21800 count-zero stream is bounded by its terminal record");
    DRW_SabData completeCountless;
    DRW_SabParseOutcome completeOutcome;
    const bool parsedWithOutcome = drw_parseSabWithOutcome(
        bytes.data(), bytes.size(), completeCountless, completeOutcome);
    t.expect(parsedWithOutcome && completeOutcome.complete
                 && completeOutcome.sourceByteCount == bytes.size()
                 && completeOutcome.consumedByteCount == bytes.size()
                 && completeOutcome.trailingByteCount == 0u
                 && completeOutcome.terminalRecordSeen
                 && !completeOutcome.recordCountKnown
                 && !completeOutcome.recordCountMatched,
             "SAB outcome reports complete zero-count stream span");

    DRW_AcisBrep projection;
    t.expect(parsed
                 && drw_decodeAcisWireframe(bytes, projection)
                 && projection.vertices.size() == 1u
                 && projection.vertices.front().valid
                 && acisCoordNear(projection.vertices.front().point, 1, 2, 3),
             "ACIS 21800 count-zero stream reaches the finite local vertex projection");

    const std::vector<std::uint8_t> countedBytes = vertexSab(21800u, 3u);
    DRW_SabData counted;
    t.expect(drw_parseSab(countedBytes.data(), countedBytes.size(), counted)
                 && counted.records.size() == 3u
                 && counted.records.back().type == "End-of-ASM-data",
             "ACIS 21800 positive count matches records through terminal");

    const auto rejects = [&t](const std::vector<std::uint8_t>& malformed,
                              const char* label) {
        DRW_SabData rejected;
        rejected.header.version = 42;
        rejected.records.push_back(acisRecord("stale", {}));
        t.expect(!drw_parseSab(malformed.data(), malformed.size(), rejected)
                     && rejected.records.empty()
                     && rejected.header.signature.empty(),
                 label);
    };
    rejects(vertexSab(21700u, 0u),
            "SAB count-zero exception is not inferred for another profile");
    rejects(vertexSab(21800u, UINT32_MAX),
            "SAB negative record count is rejected");
    rejects(vertexSab(21800u, 2u),
            "SAB nonzero record count must match all parsed records");

    std::vector<std::uint8_t> asmCountZero = vertexSab(21800u, 0u);
    const std::string asmSignature = "ASM BinaryFile4";
    std::copy(asmSignature.begin(), asmSignature.end(), asmCountZero.begin());
    rejects(asmCountZero,
            "ACIS 21800 zero-count exception is not inferred for ASM signature");

    // Remove the complete terminal entity-type record for this negative.
    const std::string terminalMarker = "End-of-ASM-data";
    const std::size_t terminalRecordSize =
        2u + terminalMarker.size() + 1u;
    std::vector<std::uint8_t> unterminated = bytes;
    unterminated.resize(unterminated.size() - terminalRecordSize);
    rejects(unterminated,
            "SAB count-zero profile requires an explicit terminal record");
    DRW_SabData missingTerminalPrefix;
    DRW_SabParseOutcome missingTerminalOutcome;
    t.expect(drw_parseSabWithOutcome(
                 unterminated.data(), unterminated.size(),
                 missingTerminalPrefix, missingTerminalOutcome)
                 && !missingTerminalOutcome.complete
                 && !missingTerminalOutcome.terminalRecordSeen
                 && missingTerminalOutcome.consumedByteCount
                        == unterminated.size()
                 && missingTerminalOutcome.trailingByteCount == 0u
                 && missingTerminalPrefix.records.size() == 2u,
             "SAB prefix API marks unknown-count EOF without terminal incomplete");

    std::vector<std::uint8_t> countedWithoutTerminal =
        vertexSab(21800u, 2u);
    countedWithoutTerminal.resize(
        countedWithoutTerminal.size() - terminalRecordSize);
    DRW_SabData countedWithoutMarker;
    DRW_SabParseOutcome countedWithoutMarkerOutcome;
    t.expect(drw_parseSabWithOutcome(
                 countedWithoutTerminal.data(),
                 countedWithoutTerminal.size(), countedWithoutMarker,
                 countedWithoutMarkerOutcome)
                 && countedWithoutMarkerOutcome.complete
                 && countedWithoutMarkerOutcome.recordCountKnown
                 && countedWithoutMarkerOutcome.recordCountMatched
                 && !countedWithoutMarkerOutcome.terminalRecordSeen,
             "positive exact-count EOF remains admitted pending profile evidence");

    std::vector<std::uint8_t> trailing = bytes;
    trailing.push_back(0xFFu);
    DRW_SabData trailingPrefix;
    DRW_SabParseOutcome trailingOutcome;
    const bool trailingParsed = drw_parseSabWithOutcome(
        trailing.data(), trailing.size(), trailingPrefix, trailingOutcome);
    t.expect(trailingParsed && !trailingOutcome.complete
                 && trailingOutcome.sourceByteCount == trailing.size()
                 && trailingOutcome.consumedByteCount == bytes.size()
                 && trailingOutcome.trailingByteCount == 1u
                 && trailingOutcome.terminalRecordSeen
                 && !trailingOutcome.recordCountKnown
                 && !trailingOutcome.recordCountMatched
                 && trailingPrefix.records.size() == 3u,
             "SAB prefix API reports terminal and exact suffix span");
    rejects(trailing,
            "SAB count-zero profile rejects bytes after its terminal record");
    DRW_AcisBrep trailingProjection;
    DRW_SabParseOutcome projectionOutcome;
    t.expect(drw_decodeAcisWireframeWithOutcome(
                 trailing, trailingProjection, projectionOutcome)
                 && !projectionOutcome.complete
                 && projectionOutcome.trailingByteCount == 1u
                 && trailingProjection.vertices.size() == 1u,
             "SAB prefix projection carries an explicit incomplete span outcome");
    DRW_AcisBrep strictTrailingProjection;
    t.expect(!drw_decodeAcisWireframe(trailing, strictTrailingProjection)
                 && strictTrailingProjection.empty(),
             "legacy SAB projection refuses an unconsumed suffix");

    std::vector<std::uint8_t> countedTrailing = countedBytes;
    countedTrailing.push_back(0xFFu);
    DRW_SabData countedTrailingPrefix;
    DRW_SabParseOutcome countedTrailingOutcome;
    t.expect(drw_parseSabWithOutcome(
                 countedTrailing.data(), countedTrailing.size(),
                 countedTrailingPrefix, countedTrailingOutcome)
                 && !countedTrailingOutcome.complete
                 && countedTrailingOutcome.recordCountKnown
                 && countedTrailingOutcome.recordCountMatched
                 && countedTrailingOutcome.terminalRecordSeen
                 && countedTrailingOutcome.consumedByteCount
                        == countedBytes.size()
                 && countedTrailingOutcome.trailingByteCount == 1u,
             "positive-count SAB reports an unconsumed terminal suffix");
    rejects(countedTrailing,
            "ACIS 21800 positive-count profile rejects trailing bytes");

    std::vector<std::uint8_t> missingRecordEnd = bytes;
    missingRecordEnd.pop_back();
    DRW_SabData terminalAtEof;
    t.expect(drw_parseSab(missingRecordEnd.data(), missingRecordEnd.size(),
                          terminalAtEof)
                 && terminalAtEof.records.size() == 3u
                 && terminalAtEof.records.back().type == "End-of-ASM-data",
             "SAB count-zero profile accepts terminal marker at exact EOF");

    std::vector<std::uint8_t> countedMissingRecordEnd = countedBytes;
    countedMissingRecordEnd.pop_back();
    DRW_SabData countedTerminalAtEof;
    t.expect(drw_parseSab(countedMissingRecordEnd.data(),
                          countedMissingRecordEnd.size(),
                          countedTerminalAtEof)
                 && countedTerminalAtEof.records.size() == 3u
                 && countedTerminalAtEof.records.back().type
                        == "End-of-ASM-data",
             "ACIS 21800 positive-count profile accepts terminal at exact EOF");
}

void testSab21800RecordHeaderAlignment(TestContext& t) {
    const std::vector<std::uint8_t> nullAttributeBytes =
        vertexSab(21800u, 3u);
    DRW_SabData nullAttributeSab;
    const bool nullAttributeParsed = drw_parseSab(
        nullAttributeBytes.data(), nullAttributeBytes.size(),
        nullAttributeSab);
    const DRW_AcisModel nullAttributeModel =
        drw_buildAcisModel(nullAttributeSab);
    DRW_AcisBrep nullAttributeWireframe;
    t.expect(nullAttributeParsed && nullAttributeSab.records.size() == 3u
                 && nullAttributeSab.records[0].tokens.size() == 4u
                 && nullAttributeSab.records[0].tokens[1].tag
                        == DRW_SabTag::Pointer
                 && nullAttributeSab.records[0].tokens[1].ival == -1
                 && nullAttributeSab.records[0].tokens[2].ival == 101
                 && nullAttributeModel.nodes.size() == 3u
                 && nullAttributeModel.nodes[0].refs.empty()
                 && nullAttributeModel.nodes[1].refs
                        == std::vector<int>({0})
                 && drw_extractAcisWireframe(
                        nullAttributeModel, nullAttributeWireframe)
                 && nullAttributeWireframe.vertices.size() == 1u
                 && acisCoordNear(
                        nullAttributeWireframe.vertices[0].point, 1, 2, 3),
             "SAB 21800 null attribute pointer preserves semantic offset and graph refs");

    const std::vector<std::uint8_t> bytes =
        vertexSab(21800u, 3u, 0u, 1.0, 77, 88);
    DRW_SabData sab;
    const bool parsed = drw_parseSab(bytes.data(), bytes.size(), sab);
    t.expect(parsed && sab.records.size() == 3u
                 && sab.records[0].tokens.size() == 4u
                 && sab.records[0].tokens[0].tag == DRW_SabTag::EntityType
                 && sab.records[0].tokens[0].sval == "point"
                 && sab.records[0].tokens[1].tag == DRW_SabTag::Pointer
                 && sab.records[0].tokens[1].ival == 77
                 && sab.records[0].tokens[2].tag == DRW_SabTag::Int
                 && sab.records[0].tokens[2].ival == 101
                 && sab.records[1].tokens[1].ival == 88
                 && sab.records[1].tokens[2].ival == 202,
             "SAB 21800 parser preserves type, attribute pointer, ID and data offset");

    const DRW_AcisModel model = drw_buildAcisModel(sab);
    t.expect(model.nodes.size() == 3u
                 && model.nodes[0].refs.empty()
                 && model.nodes[1].refs == std::vector<int>({0})
                 && model.nodes[1].record.tokens[1].ival == 88,
             "SAB 21800 model excludes attribute pointer from semantic refs");
    DRW_AcisBrep wireframe;
    t.expect(drw_extractAcisWireframe(model, wireframe)
                 && wireframe.vertices.size() == 1u
                 && wireframe.vertices.front().valid
                 && acisCoordNear(wireframe.vertices.front().point, 1, 2, 3),
             "SAB 21800 wire projection resolves point after shared record prefix");

    const std::vector<std::uint8_t> bodyBytes = rootedVertexBodySab();
    DRW_SabData bodySab;
    const bool bodyParsed = drw_parseSab(
        bodyBytes.data(), bodyBytes.size(), bodySab);
    const DRW_AcisModel bodyModel = drw_buildAcisModel(bodySab);
    std::vector<DRW_AcisBodyProjection> bodies;
    DRW_AcisRootedProjectionReport report;
    const bool projected = drw_extractAcisBodyWireframes(
        bodyModel, bodies, report);
    const std::array<double, 12> expectedBodyTransformComponents = {{
        1.0, 0.0, 0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 1.0, 0.0, 0.0, 0.0
    }};
    t.expect(bodyParsed && projected && bodies.size() == 1u
                 && report.bodyCount == 1u
                 && bodies.front().bodyNodeIndex == 0
                 && bodies.front().bodyGraphProjected
                 && bodies.front().wireframe.vertices.size() == 2u
                 && bodies.front().bodyTransformNodeIndex == 13
                 && bodies.front().transform.rawTokens.size() == 4u
                 && bodies.front().transform.rawTokens[0].tag
                        == DRW_SabTag::EntityType
                 && bodies.front().transform.rawTokens[0].sval == "transform"
                 && bodies.front().transform.rawTokens[1].ival == -1
                 && bodies.front().transform.rawTokens[2].tag
                        == DRW_SabTag::Int
                 && bodies.front().transform.rawTokens[2].ival == 14
                 && bodies.front().transform.rawLiteral
                        == "1 0 0 0 1 0 0 0 1 0 0 0"
                 && bodies.front().transform.rawLiteralTokenIndex == 3u
                 && bodies.front().transform.candidateComponentsPresent
                 && bodies.front().transform.candidateComponents
                        == expectedBodyTransformComponents
                 && bodies.front().transform.state
                        == DRW_AcisTransformState::ParsedUnapplied
                 && acisCoordNear(
                        bodies.front().wireframe.vertices[0].point,
                        1, 2, 3)
                 && acisCoordNear(
                        bodies.front().wireframe.vertices[1].point,
                        4, 5, 6),
             "SAB 21800 rooted body uses type-specific reference ordinals after shared prefix");

    const std::vector<std::uint8_t> twoBodyBytes = rootedVertexBodySab(2u);
    DRW_SabData twoBodySab;
    const bool twoBodyParsed = drw_parseSab(
        twoBodyBytes.data(), twoBodyBytes.size(), twoBodySab);
    const DRW_AcisModel twoBodyModel = drw_buildAcisModel(twoBodySab);
    std::vector<DRW_AcisBodyProjection> twoBodies;
    DRW_AcisRootedProjectionReport twoBodyReport;
    const bool twoBodiesProjected = drw_extractAcisBodyWireframes(
        twoBodyModel, twoBodies, twoBodyReport);
    t.expect(twoBodyParsed && twoBodiesProjected
                 && twoBodies.size() == 2u
                 && twoBodyReport.bodyCount == 2u
                 && twoBodyReport.multiplyRootedNodeCount == 0u
                 && twoBodies[0].bodyNodeIndex == 0u
                 && twoBodies[1].bodyNodeIndex == 14u
                 && twoBodies[0].bodyTransformNodeIndex == 13u
                 && twoBodies[1].bodyTransformNodeIndex == 27u
                 && twoBodies[0].bodyGraphProjected
                 && twoBodies[1].bodyGraphProjected
                 && twoBodies[0].wireframe.vertices.size() == 2u
                 && twoBodies[1].wireframe.vertices.size() == 2u
                 && acisCoordNear(
                        twoBodies[0].wireframe.vertices[0].point, 1, 2, 3)
                 && acisCoordNear(
                        twoBodies[1].wireframe.vertices[0].point, 101, 2, 3)
                 && twoBodies[0].transform.state
                        == DRW_AcisTransformState::ParsedUnapplied
                 && twoBodies[1].transform.state
                        == DRW_AcisTransformState::ParsedUnapplied
                 && twoBodies[0].transform.rawLiteralTokenIndex == 3u
                 && twoBodies[1].transform.rawLiteralTokenIndex == 3u
                 && twoBodies[0].transform.rawLiteral
                        == "1 0 0 0 1 0 0 0 1 0 0 0"
                 && twoBodies[1].transform.rawLiteral
                        == "1 0 0 0 1 0 0 0 1 100 0 0"
                 && twoBodies[0].transform.candidateComponentsPresent
                 && twoBodies[1].transform.candidateComponentsPresent
                 && twoBodies[0].transform.candidateComponents[9] == 0.0
                 && twoBodies[1].transform.candidateComponents[9] == 100.0
                 && !twoBodies[0].report.bodyTransformsApplied
                 && !twoBodies[1].report.bodyTransformsApplied
                 && !twoBodies[0].report.sourceUnitScaleApplied
                 && !twoBodies[1].report.sourceUnitScaleApplied,
             "SAB 21800 wire projection isolates two rooted bodies and preserves independent unapplied transforms");

    const std::vector<std::uint8_t> sharedBodyBytes =
        rootedVertexBodySab(2u, true);
    DRW_SabData sharedBodySab;
    const bool sharedBodyParsed = drw_parseSab(
        sharedBodyBytes.data(), sharedBodyBytes.size(), sharedBodySab);
    const DRW_AcisModel sharedBodyModel = drw_buildAcisModel(sharedBodySab);
    std::vector<DRW_AcisBodyProjection> sharedBodies;
    DRW_AcisRootedProjectionReport sharedBodyReport;
    const bool sharedBodiesProjected = drw_extractAcisBodyWireframes(
        sharedBodyModel, sharedBodies, sharedBodyReport);
    t.expect(sharedBodyParsed && sharedBodiesProjected
                 && sharedBodies.size() == 2u
                 && sharedBodyReport.bodyCount == 2u
                 && sharedBodyReport.multiplyRootedNodeCount == 1u
                 && sharedBodies[0].bodyGraphProjected
                 && sharedBodies[1].bodyGraphProjected
                 && sharedBodies[1].wireframe.vertices.size() == 2u
                 && acisCoordNear(
                        sharedBodies[0].wireframe.vertices[0].point,
                        1, 2, 3)
                 && acisCoordNear(
                        sharedBodies[1].wireframe.vertices[0].point,
                        1, 2, 3)
                 && std::find(sharedBodies[1].reachableNodeIndices.begin(),
                              sharedBodies[1].reachableNodeIndices.end(), 11)
                        != sharedBodies[1].reachableNodeIndices.end()
                 && sharedBodyReport.failure
                        == DRW_AcisProjectionFailure::NoFailure,
             "SAB 21800 parser-fed projection counts one point shared by two body roots");

    std::vector<std::uint8_t> malformed = bytes;
    const std::array<std::uint8_t, 7> pointType {{
        static_cast<std::uint8_t>(DRW_SabTag::EntityType), 5u,
        'p', 'o', 'i', 'n', 't'
    }};
    const auto point = std::search(malformed.begin(), malformed.end(),
                                   pointType.begin(), pointType.end());
    if (point != malformed.end())
        *(point + static_cast<std::ptrdiff_t>(pointType.size())) =
            static_cast<std::uint8_t>(DRW_SabTag::Int);
    DRW_SabData rejected;
    rejected.header.signature = "STALE";
    rejected.records.push_back(acisRecord("stale-record", {}));
    t.expect(point != malformed.end()
                 && !drw_parseSab(malformed.data(), malformed.size(), rejected)
                 && rejected.records.empty()
                 && rejected.header.signature.empty(),
             "SAB 21800 parser rejects malformed shared-header tag without guessing an offset");

    std::vector<std::uint8_t> malformedEntityId = bytes;
    const std::array<std::uint8_t, 8> pointAndAttributeTag {{
        static_cast<std::uint8_t>(DRW_SabTag::EntityType), 5u,
        'p', 'o', 'i', 'n', 't',
        static_cast<std::uint8_t>(DRW_SabTag::Pointer)
    }};
    const auto pointWithAttribute = std::search(
        malformedEntityId.begin(), malformedEntityId.end(),
        pointAndAttributeTag.begin(), pointAndAttributeTag.end());
    if (pointWithAttribute != malformedEntityId.end())
        *(pointWithAttribute + static_cast<std::ptrdiff_t>(
              pointAndAttributeTag.size() + sizeof(std::int32_t))) =
            static_cast<std::uint8_t>(DRW_SabTag::Pointer);
    DRW_SabData rejectedEntityId;
    rejectedEntityId.header.signature = "STALE";
    rejectedEntityId.records.push_back(acisRecord("stale-record", {}));
    t.expect(pointWithAttribute != malformedEntityId.end()
                 && !drw_parseSab(malformedEntityId.data(),
                                  malformedEntityId.size(), rejectedEntityId)
                 && rejectedEntityId.records.empty()
                 && rejectedEntityId.header.signature.empty(),
             "SAB 21800 parser rejects malformed entity-ID tag without guessing an offset");

    DRW_SabData callerBuilt;
    callerBuilt.header.signature = "ACIS BinaryFile";
    callerBuilt.header.version = 21800;
    callerBuilt.records.push_back(acisRecord("vertex", {acisPointer(1)}));
    callerBuilt.records.push_back(
        acisRecord("point", {acisLocation(9, 8, 7)}));
    const DRW_AcisModel callerBuiltModel = drw_buildAcisModel(callerBuilt);
    t.expect(callerBuiltModel.nodes.size() == 2u
                 && callerBuiltModel.nodes[0].refs == std::vector<int>({1}),
             "caller-built ACIS records retain the compatibility default offset zero");
}

void testAcisParseBudgets(TestContext& t) {
    const std::vector<std::uint8_t> bytes = vertexSab(21800u, 3u);
    DRW_SabData baselineData;
    DRW_SabParseOutcome baseline;
    t.expect(drw_parseSabWithOutcome(bytes.data(), bytes.size(),
                                     baselineData, baseline)
                 && baseline.complete
                 && baseline.recordsProcessed == 3u
                 && baseline.tagsProcessed > 0u
                 && baseline.stringBytesAccounted > 0u
                 && baseline.tokenCapacityItemsAccounted > 0u,
             "SAB default parse reports deterministic budget counters");

    DRW_AcisModel graphBaselineModel;
    DRW_SabParseOutcome graphBaseline;
    DRW_SabParseLimits graphDefaults;
    t.expect(drw_buildAcisModelWithLimits(
                 baselineData, graphBaselineModel, graphBaseline,
                 graphDefaults)
                 && graphBaselineModel.nodes.size()
                        == baselineData.records.size()
                 && graphBaseline.graphWorkUnitsProcessed > 0u
                 && graphBaseline.graphItemsAccounted > 0u
                 && graphBaseline.stringBytesAccounted > 0u,
             "SAB graph builder reports deterministic copy and reference budgets");

    std::size_t expectedGraphItems = baselineData.records.size() * 2u;
    for (const DRW_SabRecord& record : baselineData.records) {
        expectedGraphItems += record.tokens.size();
        for (const DRW_SabToken& token : record.tokens) {
            if (token.tag == DRW_SabTag::Pointer)
                ++expectedGraphItems;
        }
    }
    t.expect(graphBaseline.graphItemsAccounted == expectedGraphItems,
             "SAB graph budget includes reference-count planner slots");

    DRW_SabParseLimits exactGraphLimits;
    exactGraphLimits.maxGraphWorkUnits =
        graphBaseline.graphWorkUnitsProcessed;
    exactGraphLimits.maxGraphItems = graphBaseline.graphItemsAccounted;
    exactGraphLimits.maxCumulativeStringBytes =
        graphBaseline.stringBytesAccounted;
    DRW_AcisModel exactGraphModel;
    DRW_SabParseOutcome exactGraphOutcome;
    t.expect(drw_buildAcisModelWithLimits(
                 baselineData, exactGraphModel, exactGraphOutcome,
                 exactGraphLimits)
                 && exactGraphOutcome.graphWorkUnitsProcessed
                        == exactGraphLimits.maxGraphWorkUnits
                 && exactGraphOutcome.graphItemsAccounted
                        == exactGraphLimits.maxGraphItems
                 && exactGraphOutcome.stringBytesAccounted
                        == exactGraphLimits.maxCumulativeStringBytes,
             "SAB graph builder admits exact work, item, and string budgets");

    DRW_SabParseLimits graphWorkOver = exactGraphLimits;
    --graphWorkOver.maxGraphWorkUnits;
    DRW_AcisModel rejectedGraph;
    DRW_SabParseOutcome graphWorkOutcome;
    t.expect(!drw_buildAcisModelWithLimits(
                 baselineData, rejectedGraph, graphWorkOutcome,
                 graphWorkOver)
                 && rejectedGraph.nodes.empty()
                 && graphWorkOutcome.resourceLimitExceeded,
             "SAB graph builder rejects work beyond its configured cap");

    DRW_SabParseLimits graphItemsOver = exactGraphLimits;
    --graphItemsOver.maxGraphItems;
    DRW_SabParseOutcome graphItemsOutcome;
    t.expect(!drw_buildAcisModelWithLimits(
                 baselineData, rejectedGraph, graphItemsOutcome,
                 graphItemsOver)
                 && rejectedGraph.nodes.empty()
                 && graphItemsOutcome.resourceLimitExceeded,
             "SAB graph builder rejects nodes, tokens, or refs beyond its cap");

    DRW_SabParseLimits graphStringOver = exactGraphLimits;
    --graphStringOver.maxCumulativeStringBytes;
    DRW_SabParseOutcome graphStringOutcome;
    t.expect(!drw_buildAcisModelWithLimits(
                 baselineData, rejectedGraph, graphStringOutcome,
                 graphStringOver)
                 && rejectedGraph.nodes.empty()
                 && graphStringOutcome.resourceLimitExceeded,
             "SAB graph builder rejects copied strings beyond their aggregate cap");

    DRW_SabParseLimits exactLimits;
    exactLimits.maxInputBytes = bytes.size();
    exactLimits.maxRecords = baseline.recordsProcessed;
    exactLimits.maxTags = baseline.tagsProcessed;
    exactLimits.maxStringBytes = 16u;
    exactLimits.maxCumulativeStringBytes = baseline.stringBytesAccounted;
    exactLimits.maxTokenCapacityItems =
        baseline.tokenCapacityItemsAccounted;
    DRW_SabData exactData;
    DRW_SabParseOutcome exactOutcome;
    t.expect(drw_parseSabWithLimits(bytes.data(), bytes.size(), exactData,
                                    exactOutcome, exactLimits)
                 && exactOutcome.complete
                 && exactOutcome.recordsProcessed == exactLimits.maxRecords
                 && exactOutcome.tagsProcessed == exactLimits.maxTags
                 && exactOutcome.stringBytesAccounted
                        == exactLimits.maxCumulativeStringBytes
                 && exactOutcome.tokenCapacityItemsAccounted
                        == exactLimits.maxTokenCapacityItems,
             "SAB parser admits exact input, record, tag, string, and token-capacity budgets");

    DRW_SabParseLimits tokenCapacityOver = exactLimits;
    --tokenCapacityOver.maxTokenCapacityItems;
    DRW_SabData tokenCapacityRejected;
    DRW_SabParseOutcome tokenCapacityOutcome;
    t.expect(!drw_parseSabWithLimits(bytes.data(), bytes.size(),
                                    tokenCapacityRejected,
                                    tokenCapacityOutcome,
                                    tokenCapacityOver)
                 && tokenCapacityRejected.records.empty()
                 && tokenCapacityOutcome.resourceLimitExceeded
                 && tokenCapacityOutcome.tokenCapacityItemsAccounted
                        <= tokenCapacityOver.maxTokenCapacityItems,
             "SAB parser refuses token-vector reserve beyond its aggregate cap");

    const std::vector<std::uint8_t> extendedTypeBytes =
        extendedEntityTypeSab();
    DRW_SabData extendedTypeData;
    DRW_SabParseOutcome extendedTypeOutcome;
    t.expect(!extendedTypeBytes.empty()
                 && drw_parseSabWithOutcome(
                        extendedTypeBytes.data(), extendedTypeBytes.size(),
                        extendedTypeData, extendedTypeOutcome)
                 && extendedTypeData.records[0].type == "poi-nt"
                 && extendedTypeOutcome.typePartCapacityPeakItems == 2u,
             "SAB extended entity names report bounded temporary part capacity");

    DRW_SabParseLimits extendedTypeExact = exactLimits;
    extendedTypeExact.maxInputBytes = extendedTypeBytes.size();
    extendedTypeExact.maxRecords = extendedTypeOutcome.recordsProcessed;
    extendedTypeExact.maxTags = extendedTypeOutcome.tagsProcessed;
    extendedTypeExact.maxCumulativeStringBytes =
        extendedTypeOutcome.stringBytesAccounted;
    extendedTypeExact.maxTokenCapacityItems =
        extendedTypeOutcome.tokenCapacityItemsAccounted;
    extendedTypeExact.maxTypePartCapacityItems =
        extendedTypeOutcome.typePartCapacityPeakItems;
    DRW_SabData extendedExactData;
    DRW_SabParseOutcome extendedExactOutcome;
    t.expect(drw_parseSabWithLimits(
                 extendedTypeBytes.data(), extendedTypeBytes.size(),
                 extendedExactData, extendedExactOutcome,
                 extendedTypeExact)
                 && extendedExactOutcome.typePartCapacityPeakItems
                        == extendedTypeExact.maxTypePartCapacityItems,
             "SAB parser admits exact extended-type temporary capacity");

    DRW_SabParseLimits extendedTypeOver = extendedTypeExact;
    --extendedTypeOver.maxTypePartCapacityItems;
    DRW_SabData extendedRejected;
    DRW_SabParseOutcome extendedRejectedOutcome;
    t.expect(!drw_parseSabWithLimits(
                 extendedTypeBytes.data(), extendedTypeBytes.size(),
                 extendedRejected, extendedRejectedOutcome,
                 extendedTypeOver)
                 && extendedRejected.records.empty()
                 && extendedRejectedOutcome.resourceLimitExceeded,
             "SAB parser refuses extended-type temporary capacity beyond cap");

    DRW_SabParseLimits inputOver = exactLimits;
    --inputOver.maxInputBytes;
    DRW_SabData rejected;
    DRW_SabParseOutcome inputOutcome;
    t.expect(!drw_parseSabWithLimits(bytes.data(), bytes.size(), rejected,
                                     inputOutcome, inputOver)
                 && rejected.records.empty()
                 && inputOutcome.resourceLimitExceeded
                 && inputOutcome.recordsProcessed == 0u,
             "SAB parser rejects input one byte over its cap before allocation");

    DRW_SabParseLimits recordOver = exactLimits;
    --recordOver.maxRecords;
    DRW_SabParseOutcome recordOutcome;
    t.expect(!drw_parseSabWithLimits(bytes.data(), bytes.size(), rejected,
                                     recordOutcome, recordOver)
                 && rejected.records.empty()
                 && recordOutcome.resourceLimitExceeded
                 && recordOutcome.recordsProcessed
                        == recordOver.maxRecords,
             "SAB parser rejects a record beyond the configured record cap");

    DRW_SabParseLimits tagOver = exactLimits;
    --tagOver.maxTags;
    DRW_SabParseOutcome tagOutcome;
    t.expect(!drw_parseSabWithLimits(bytes.data(), bytes.size(), rejected,
                                     tagOutcome, tagOver)
                 && rejected.records.empty()
                 && tagOutcome.resourceLimitExceeded
                 && tagOutcome.tagsProcessed == tagOver.maxTags,
             "SAB parser rejects a tag beyond the configured work cap");

    DRW_SabParseLimits stringOver = exactLimits;
    stringOver.maxStringBytes = 15u;
    DRW_SabParseOutcome stringOutcome;
    t.expect(!drw_parseSabWithLimits(bytes.data(), bytes.size(), rejected,
                                     stringOutcome, stringOver)
                 && rejected.records.empty()
                 && stringOutcome.resourceLimitExceeded,
             "SAB parser rejects one wire string beyond its per-string cap");

    DRW_SabParseLimits aggregateStringOver = exactLimits;
    --aggregateStringOver.maxCumulativeStringBytes;
    DRW_SabParseOutcome aggregateStringOutcome;
    t.expect(!drw_parseSabWithLimits(bytes.data(), bytes.size(), rejected,
                                     aggregateStringOutcome,
                                     aggregateStringOver)
                 && rejected.records.empty()
                 && aggregateStringOutcome.resourceLimitExceeded,
             "SAB parser rejects aggregate retained strings beyond their cap");

    std::vector<std::uint8_t> nested = bytes;
    const std::array<std::uint8_t, 7> pointType {{
        static_cast<std::uint8_t>(DRW_SabTag::EntityType), 5u,
        'p', 'o', 'i', 'n', 't'
    }};
    const auto typeAt = std::search(nested.begin(), nested.end(),
                                    pointType.begin(), pointType.end());
    t.expect(typeAt != nested.end(), "SAB subtype vector locates point record");
    if (typeAt != nested.end()) {
        const auto endAt = std::find(typeAt + pointType.size(), nested.end(),
                                     static_cast<std::uint8_t>(
                                         DRW_SabTag::RecordEnd));
        t.expect(endAt != nested.end(),
                 "SAB subtype vector locates point record boundary");
        if (endAt != nested.end()) {
            const auto endIndex = static_cast<std::size_t>(
                std::distance(nested.begin(), endAt));
            nested.insert(nested.begin() + endIndex,
                          {static_cast<std::uint8_t>(
                               DRW_SabTag::SubtypeStart),
                           static_cast<std::uint8_t>(
                               DRW_SabTag::SubtypeEnd)});
            DRW_SabParseLimits oneLevel;
            oneLevel.maxSubtypeDepth = 1u;
            DRW_SabData nestedData;
            DRW_SabParseOutcome nestedOutcome;
            t.expect(drw_parseSabWithLimits(
                         nested.data(), nested.size(), nestedData,
                         nestedOutcome, oneLevel)
                         && nestedOutcome.complete,
                     "SAB parser accepts a subtype at its exact depth cap");

            oneLevel.maxSubtypeDepth = 0u;
            DRW_SabParseOutcome depthOutcome;
            t.expect(!drw_parseSabWithLimits(
                         nested.data(), nested.size(), rejected,
                         depthOutcome, oneLevel)
                         && rejected.records.empty()
                         && depthOutcome.resourceLimitExceeded,
                     "SAB parser rejects subtype nesting beyond its depth cap");
        }
    }

    DRW_AcisBrep decoded;
    DRW_SabParseOutcome decodedOutcome;
    DRW_AcisBrep decodeBaselineBrep;
    DRW_SabParseOutcome decodeBaseline;
    DRW_SabParseLimits decodeDefaults;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(bytes.begin(), bytes.end()),
                 decodeBaselineBrep, decodeBaseline, decodeDefaults)
                 && decodeBaselineBrep.vertices.size() == 1u
                 && decodeBaseline.extractionWorkUnitsProcessed > 0u
                 && decodeBaseline.outputItemsAccounted > 0u,
             "SAB decoder reports bounded extraction and output counters");

    DRW_SabParseLimits decodeLimits = exactLimits;
    decodeLimits.maxStringBytes = 1024u * 1024u;
    decodeLimits.maxCumulativeStringBytes = decodeBaseline.stringBytesAccounted;
    decodeLimits.maxGraphWorkUnits = graphBaseline.graphWorkUnitsProcessed;
    decodeLimits.maxGraphItems = graphBaseline.graphItemsAccounted;
    decodeLimits.maxExtractionWorkUnits =
        decodeBaseline.extractionWorkUnitsProcessed;
    decodeLimits.maxOutputItems = decodeBaseline.outputItemsAccounted;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(bytes.begin(), bytes.end()),
                 decoded, decodedOutcome, decodeLimits)
                 && decodedOutcome.complete
                 && decodedOutcome.graphWorkUnitsProcessed
                        == graphBaseline.graphWorkUnitsProcessed
                 && decodedOutcome.graphItemsAccounted
                        == graphBaseline.graphItemsAccounted
                 && decodedOutcome.stringBytesAccounted
                        == decodeLimits.maxCumulativeStringBytes
                 && decodedOutcome.extractionWorkUnitsProcessed
                        == decodeLimits.maxExtractionWorkUnits
                 && decodedOutcome.outputItemsAccounted
                        == decodeLimits.maxOutputItems
                 && decoded.vertices.size() == 1u,
             "SAB convenience decoder shares exact parser, graph, and projection budgets");

    std::vector<unsigned char> prefixed {0xAAu, 0xBBu, 0xCCu};
    prefixed.insert(prefixed.end(), bytes.begin(), bytes.end());
    DRW_SabParseLimits prefixedLimits = decodeLimits;
    prefixedLimits.maxInputBytes = prefixed.size();
    DRW_SabParseOutcome prefixedOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 prefixed, decoded, prefixedOutcome, prefixedLimits)
                 && prefixedOutcome.complete
                 && prefixedOutcome.sourceByteCount == bytes.size()
                 && prefixedOutcome.graphWorkUnitsProcessed
                        == graphBaseline.graphWorkUnitsProcessed
                 && prefixedOutcome.outputItemsAccounted
                        == decodeBaseline.outputItemsAccounted
                 && decoded.vertices.size() == 1u,
             "SAB signature prefix is excluded from candidate parse accounting");

    const std::vector<std::uint8_t> shiftedBytes = shiftSabBits(bytes, 3u);
    DRW_AcisBrep shiftedDecoded;
    DRW_SabParseOutcome shiftedOutcome;
    DRW_SabParseLimits shiftedLimits = decodeDefaults;
    shiftedLimits.maxInputBytes = shiftedBytes.size();
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(shiftedBytes.begin(),
                                            shiftedBytes.end()),
                 shiftedDecoded, shiftedOutcome, shiftedLimits)
                 && shiftedOutcome.complete
                 && shiftedOutcome.sourceByteCount == bytes.size()
                 && shiftedOutcome.consumedByteCount == bytes.size()
                 && shiftedOutcome.trailingByteCount == 0u
                 && shiftedDecoded.vertices.size() == 1u
                 && shiftedDecoded.vertices[0].valid
                 && shiftedDecoded.vertices[0].point.x == 1.0
                 && shiftedDecoded.vertices[0].point.y == 2.0
                 && shiftedDecoded.vertices[0].point.z == 3.0,
             "SAB bit-shifted stream parses without a realigned candidate copy");

    const std::vector<std::uint8_t> loopCycleBytes =
        repeatedLoopCycleSab(4u, 4u, 4u);
    DRW_AcisBrep loopCycleBrep;
    DRW_SabParseOutcome loopCycleOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(loopCycleBytes.begin(),
                                            loopCycleBytes.end()),
                 loopCycleBrep, loopCycleOutcome, decodeDefaults)
                 && loopCycleBrep.faces.size() == 4u
                 && loopCycleBrep.faces[0].loops.size() == 4u
                 && loopCycleBrep.faces[1].loops.size() == 4u
                 && loopCycleBrep.faces[2].loops.size() == 4u
                 && loopCycleBrep.faces[3].loops.size() == 4u
                 && loopCycleBrep.faces[0].loops[0].nodeIndex
                        == loopCycleBrep.faces[3].loops[0].nodeIndex
                 && loopCycleBrep.faces[0].loops[0].coedgeCount == 4
                 && loopCycleBrep.faces[0].loops[0].coedgeNodeIndices
                        == std::vector<int>{8, 9, 10, 11}
                 && loopCycleBrep.faces[0].loops[0].coedgeEdgeNodeIndices
                        == std::vector<int>{-1, -1, -1, -1}
                 && loopCycleBrep.faces[0].loops[3].coedgeCount == 4
                 && loopCycleOutcome.extractionWorkUnitsProcessed > 0u,
             "SAB extraction preserves shared 4x4 cyclic output across faces");
    bool sharedLoopOutputPreserved = loopCycleBrep.faces.size() == 4u;
    for (const DRW_AcisFace& face : loopCycleBrep.faces) {
        sharedLoopOutputPreserved = sharedLoopOutputPreserved
            && face.loops.size() == 4u;
        for (std::size_t i = 0; i < face.loops.size(); ++i) {
            sharedLoopOutputPreserved = sharedLoopOutputPreserved
                && face.loops[i].nodeIndex
                    == loopCycleBrep.faces.front().loops[i].nodeIndex
                && face.loops[i].coedgeCount == 4
                && face.loops[i].coedgeNodeIndices
                    == loopCycleBrep.faces.front().loops[i]
                           .coedgeNodeIndices
                && face.loops[i].coedgeEdgeNodeIndices
                    == loopCycleBrep.faces.front().loops[i]
                           .coedgeEdgeNodeIndices;
        }
    }
    t.expect(sharedLoopOutputPreserved,
             "SAB shared-loop memoization preserves each face's ordered loop projection");

    const std::vector<std::uint8_t> oneLoopCycleBytes =
        repeatedLoopCycleSab(1u, 4u, 4u);
    DRW_AcisBrep oneLoopCycleBrep;
    DRW_SabParseOutcome oneLoopCycleOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(oneLoopCycleBytes.begin(),
                                            oneLoopCycleBytes.end()),
                 oneLoopCycleBrep, oneLoopCycleOutcome, decodeDefaults)
                 && oneLoopCycleBrep.faces.size() == 1u
                 && loopCycleOutcome.extractionWorkUnitsProcessed
                        > oneLoopCycleOutcome.extractionWorkUnitsProcessed,
             "SAB shared-cycle work counter includes per-face loop-chain visits");
    t.expect(loopCycleOutcome.extractionWorkUnitsProcessed
                 < oneLoopCycleOutcome.extractionWorkUnitsProcessed * 4u,
             "SAB shared-loop ring and next-link scans are cached without deduplicating face output");

    const std::vector<std::uint8_t> oneSharedVertexEdgeBytes =
        sharedVertexEdgesSab(1u);
    DRW_AcisBrep oneSharedVertexEdgeBrep;
    DRW_SabParseOutcome oneSharedVertexEdgeOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(oneSharedVertexEdgeBytes.begin(),
                                            oneSharedVertexEdgeBytes.end()),
                 oneSharedVertexEdgeBrep, oneSharedVertexEdgeOutcome,
                 decodeDefaults)
                 && oneSharedVertexEdgeBrep.vertices.size() == 1u
                 && oneSharedVertexEdgeBrep.edges.size() == 1u
                 && oneSharedVertexEdgeBrep.edges[0].hasStart
                 && oneSharedVertexEdgeBrep.edges[0].hasEnd
                 && acisCoordNear(oneSharedVertexEdgeBrep.edges[0].start,
                                  1.0, 2.0, 3.0)
                 && acisCoordNear(oneSharedVertexEdgeBrep.edges[0].end,
                                  1.0, 2.0, 3.0),
             "SAB shared-vertex baseline resolves both edge endpoints");

    const std::vector<std::uint8_t> sharedVertexEdges =
        sharedVertexEdgesSab(8u);
    DRW_AcisBrep sharedVertexEdgesBrep;
    DRW_SabParseOutcome sharedVertexEdgesOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(sharedVertexEdges.begin(),
                                            sharedVertexEdges.end()),
                 sharedVertexEdgesBrep, sharedVertexEdgesOutcome,
                 decodeDefaults)
                 && sharedVertexEdgesBrep.vertices.size() == 1u
                 && sharedVertexEdgesBrep.edges.size() == 8u
                 && std::all_of(
                     sharedVertexEdgesBrep.edges.begin(),
                     sharedVertexEdgesBrep.edges.end(),
                     [](const DRW_AcisEdge& edge) {
                         return edge.hasStart && edge.hasEnd
                             && acisCoordNear(edge.start, 1.0, 2.0, 3.0)
                             && acisCoordNear(edge.end, 1.0, 2.0, 3.0);
                     })
                 && sharedVertexEdgesOutcome.extractionWorkUnitsProcessed
                        > oneSharedVertexEdgeOutcome
                              .extractionWorkUnitsProcessed
                 && sharedVertexEdgesOutcome.extractionWorkUnitsProcessed
                        < oneSharedVertexEdgeOutcome
                                  .extractionWorkUnitsProcessed * 8u,
             "SAB shared-vertex projection retains eight edges without rescanning point tokens per endpoint");

    DRW_SabParseLimits sharedVertexExact;
    sharedVertexExact.maxExtractionWorkUnits =
        sharedVertexEdgesOutcome.extractionWorkUnitsProcessed;
    sharedVertexExact.maxOutputItems =
        sharedVertexEdgesOutcome.outputItemsAccounted;
    DRW_AcisBrep sharedVertexExactBrep;
    DRW_SabParseOutcome sharedVertexExactOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(sharedVertexEdges.begin(),
                                            sharedVertexEdges.end()),
                 sharedVertexExactBrep, sharedVertexExactOutcome,
                 sharedVertexExact)
                 && sharedVertexExactOutcome.extractionWorkUnitsProcessed
                        == sharedVertexExact.maxExtractionWorkUnits
                 && sharedVertexExactOutcome.outputItemsAccounted
                        == sharedVertexExact.maxOutputItems,
             "SAB shared-vertex cache admits exact extraction and cache-item budgets");

    DRW_SabParseLimits sharedVertexOver = sharedVertexExact;
    --sharedVertexOver.maxExtractionWorkUnits;
    DRW_AcisBrep rejectedSharedVertexBrep;
    DRW_SabParseOutcome rejectedSharedVertexOutcome;
    t.expect(!drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(sharedVertexEdges.begin(),
                                            sharedVertexEdges.end()),
                 rejectedSharedVertexBrep, rejectedSharedVertexOutcome,
                 sharedVertexOver)
                 && rejectedSharedVertexBrep.empty()
                 && rejectedSharedVertexOutcome.resourceLimitExceeded,
             "SAB shared-vertex projection refuses one work unit below its measured bound");

    const std::vector<std::uint8_t> oneSharedSurfaceFaceBytes =
        sharedSurfaceFacesSab(1u);
    DRW_AcisBrep oneSharedSurfaceFaceBrep;
    DRW_SabParseOutcome oneSharedSurfaceFaceOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(oneSharedSurfaceFaceBytes.begin(),
                                            oneSharedSurfaceFaceBytes.end()),
                 oneSharedSurfaceFaceBrep, oneSharedSurfaceFaceOutcome,
                 decodeDefaults)
                 && oneSharedSurfaceFaceBrep.faces.size() == 1u
                 && oneSharedSurfaceFaceBrep.faces[0].surfaceType
                        == DRW_AcisSurface::Plane
                 && oneSharedSurfaceFaceBrep.faces[0].hasSurface
                 && oneSharedSurfaceFaceBrep.faces[0].hasUDir
                 && acisCoordNear(oneSharedSurfaceFaceBrep.faces[0].p0,
                                  1.0, 2.0, 3.0)
                 && acisCoordNear(oneSharedSurfaceFaceBrep.faces[0].p1,
                                  0.0, 0.0, 1.0)
                 && acisCoordNear(oneSharedSurfaceFaceBrep.faces[0].p2,
                                  1.0, 0.0, 0.0),
             "SAB shared-surface baseline projects complete plane parameters");

    const std::vector<std::uint8_t> sharedSurfaceFaces =
        sharedSurfaceFacesSab(8u);
    DRW_AcisBrep sharedSurfaceFacesBrep;
    DRW_SabParseOutcome sharedSurfaceFacesOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(sharedSurfaceFaces.begin(),
                                            sharedSurfaceFaces.end()),
                 sharedSurfaceFacesBrep, sharedSurfaceFacesOutcome,
                 decodeDefaults)
                 && sharedSurfaceFacesBrep.faces.size() == 8u
                 && std::all_of(
                     sharedSurfaceFacesBrep.faces.begin(),
                     sharedSurfaceFacesBrep.faces.end(),
                     [](const DRW_AcisFace& face) {
                         return face.surfaceType == DRW_AcisSurface::Plane
                             && face.hasSurface && face.hasUDir
                             && acisCoordNear(face.p0, 1.0, 2.0, 3.0)
                             && acisCoordNear(face.p1, 0.0, 0.0, 1.0)
                             && acisCoordNear(face.p2, 1.0, 0.0, 0.0);
                     })
                 && sharedSurfaceFacesOutcome.extractionWorkUnitsProcessed
                        > oneSharedSurfaceFaceOutcome
                              .extractionWorkUnitsProcessed
                 && sharedSurfaceFacesOutcome.extractionWorkUnitsProcessed
                        < oneSharedSurfaceFaceOutcome
                                  .extractionWorkUnitsProcessed * 8u,
             "SAB shared-surface memoization preserves each face projection without rescanning surface tokens");

    DRW_SabParseLimits sharedSurfaceExact;
    sharedSurfaceExact.maxExtractionWorkUnits =
        sharedSurfaceFacesOutcome.extractionWorkUnitsProcessed;
    sharedSurfaceExact.maxOutputItems =
        sharedSurfaceFacesOutcome.outputItemsAccounted;
    DRW_AcisBrep sharedSurfaceExactBrep;
    DRW_SabParseOutcome sharedSurfaceExactOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(sharedSurfaceFaces.begin(),
                                            sharedSurfaceFaces.end()),
                 sharedSurfaceExactBrep, sharedSurfaceExactOutcome,
                 sharedSurfaceExact)
                 && sharedSurfaceExactOutcome.extractionWorkUnitsProcessed
                        == sharedSurfaceExact.maxExtractionWorkUnits
                 && sharedSurfaceExactOutcome.outputItemsAccounted
                        == sharedSurfaceExact.maxOutputItems,
             "SAB shared-surface cache admits exact extraction and cache-item budgets");

    DRW_SabParseLimits sharedSurfaceOver = sharedSurfaceExact;
    --sharedSurfaceOver.maxExtractionWorkUnits;
    DRW_AcisBrep rejectedSharedSurfaceBrep;
    DRW_SabParseOutcome rejectedSharedSurfaceOutcome;
    t.expect(!drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(sharedSurfaceFaces.begin(),
                                            sharedSurfaceFaces.end()),
                 rejectedSharedSurfaceBrep, rejectedSharedSurfaceOutcome,
                 sharedSurfaceOver)
                 && rejectedSharedSurfaceBrep.empty()
                 && rejectedSharedSurfaceOutcome.resourceLimitExceeded,
             "SAB shared-surface projection refuses one work unit below its measured bound");

    const std::vector<std::uint8_t> oneOrderedFaceBytes =
        orderedFaceLinksSab(1u);
    DRW_AcisBrep oneOrderedFaceBrep;
    DRW_SabParseOutcome oneOrderedFaceOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(oneOrderedFaceBytes.begin(),
                                            oneOrderedFaceBytes.end()),
                 oneOrderedFaceBrep, oneOrderedFaceOutcome, decodeDefaults)
                 && oneOrderedFaceBrep.faces.size() == 1u
                 && oneOrderedFaceBrep.faces[0].surfaceType
                        == DRW_AcisSurface::Plane
                 && oneOrderedFaceBrep.faces[0].hasSurface
                 && oneOrderedFaceBrep.faces[0].hasUDir
                 && acisCoordNear(oneOrderedFaceBrep.faces[0].p0,
                                  1.0, 2.0, 3.0)
                 && acisCoordNear(oneOrderedFaceBrep.faces[0].p1,
                                  0.0, 0.0, 1.0)
                 && acisCoordNear(oneOrderedFaceBrep.faces[0].p2,
                                  1.0, 0.0, 0.0)
                 && oneOrderedFaceBrep.faces[0].loops.size() == 1u
                 && oneOrderedFaceBrep.faces[0].loops[0].nodeIndex == 2
                 && oneOrderedFaceBrep.faces[0].loops[0].coedgeCount == 0u,
             "SAB ordered face-link baseline preserves first matching surface and loop");

    const std::vector<std::uint8_t> orderedFaces = orderedFaceLinksSab(8u);
    DRW_AcisBrep orderedFacesBrep;
    DRW_SabParseOutcome orderedFacesOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(orderedFaces.begin(),
                                            orderedFaces.end()),
                 orderedFacesBrep, orderedFacesOutcome, decodeDefaults)
                 && orderedFacesBrep.faces.size() == 8u
                 && std::all_of(
                     orderedFacesBrep.faces.begin(),
                     orderedFacesBrep.faces.end(),
                     [](const DRW_AcisFace& face) {
                         return face.surfaceType == DRW_AcisSurface::Plane
                             && face.hasSurface && face.hasUDir
                             && acisCoordNear(face.p0, 1.0, 2.0, 3.0)
                             && acisCoordNear(face.p1, 0.0, 0.0, 1.0)
                             && acisCoordNear(face.p2, 1.0, 0.0, 0.0)
                             && face.loops.size() == 1u
                             && face.loops[0].nodeIndex == 2
                             && face.loops[0].coedgeCount == 0u;
                     })
                 && orderedFacesOutcome.extractionWorkUnitsProcessed
                        == oneOrderedFaceOutcome.extractionWorkUnitsProcessed
                               + 49u,
             "SAB combined face-reference scan preserves first matches at exactly seven work units per added face");

    DRW_SabParseLimits orderedFacesExact;
    orderedFacesExact.maxExtractionWorkUnits =
        orderedFacesOutcome.extractionWorkUnitsProcessed;
    orderedFacesExact.maxOutputItems = orderedFacesOutcome.outputItemsAccounted;
    DRW_AcisBrep orderedFacesExactBrep;
    DRW_SabParseOutcome orderedFacesExactOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(orderedFaces.begin(),
                                            orderedFaces.end()),
                 orderedFacesExactBrep, orderedFacesExactOutcome,
                 orderedFacesExact)
                 && orderedFacesExactOutcome.extractionWorkUnitsProcessed
                        == orderedFacesExact.maxExtractionWorkUnits
                 && orderedFacesExactOutcome.outputItemsAccounted
                        == orderedFacesExact.maxOutputItems,
             "SAB combined face-reference scan admits exact work and output-item limits");

    DRW_SabParseLimits orderedFacesUnder = orderedFacesExact;
    --orderedFacesUnder.maxExtractionWorkUnits;
    DRW_AcisBrep rejectedOrderedFacesBrep;
    DRW_SabParseOutcome rejectedOrderedFacesOutcome;
    t.expect(!drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(orderedFaces.begin(),
                                            orderedFaces.end()),
                 rejectedOrderedFacesBrep, rejectedOrderedFacesOutcome,
                 orderedFacesUnder)
                 && rejectedOrderedFacesBrep.empty()
                 && rejectedOrderedFacesOutcome.resourceLimitExceeded,
             "SAB combined face-reference scan refuses one work unit below its measured bound");

    const std::vector<std::uint8_t> oneSharedCurveEdgeBytes =
        sharedCurveEdgesSab(1u);
    DRW_AcisBrep oneSharedCurveEdgeBrep;
    DRW_SabParseOutcome oneSharedCurveEdgeOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(oneSharedCurveEdgeBytes.begin(),
                                            oneSharedCurveEdgeBytes.end()),
                 oneSharedCurveEdgeBrep, oneSharedCurveEdgeOutcome,
                 decodeDefaults)
                 && oneSharedCurveEdgeBrep.edges.size() == 1u
                 && oneSharedCurveEdgeBrep.edges[0].curveType
                        == DRW_AcisCurve::Straight
                 && oneSharedCurveEdgeBrep.edges[0].hasCurve
                 && acisCoordNear(oneSharedCurveEdgeBrep.edges[0].p0,
                                  10.0, 20.0, 30.0)
                 && acisCoordNear(oneSharedCurveEdgeBrep.edges[0].p1,
                                  1.0, 0.0, 0.0),
             "SAB shared-curve baseline projects scalar straight-curve parameters");

    const std::vector<std::uint8_t> sharedCurveEdges =
        sharedCurveEdgesSab(8u);
    DRW_AcisBrep sharedCurveEdgesBrep;
    DRW_SabParseOutcome sharedCurveEdgesOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(sharedCurveEdges.begin(),
                                            sharedCurveEdges.end()),
                 sharedCurveEdgesBrep, sharedCurveEdgesOutcome,
                 decodeDefaults)
                 && sharedCurveEdgesBrep.edges.size() == 8u
                 && std::all_of(
                     sharedCurveEdgesBrep.edges.begin(),
                     sharedCurveEdgesBrep.edges.end(),
                     [](const DRW_AcisEdge& edge) {
                         return edge.curveType == DRW_AcisCurve::Straight
                             && edge.hasCurve
                             && acisCoordNear(edge.p0, 10.0, 20.0, 30.0)
                             && acisCoordNear(edge.p1, 1.0, 0.0, 0.0);
                     })
                 && sharedCurveEdgesOutcome.extractionWorkUnitsProcessed
                        > oneSharedCurveEdgeOutcome
                              .extractionWorkUnitsProcessed
                 && sharedCurveEdgesOutcome.extractionWorkUnitsProcessed
                        < oneSharedCurveEdgeOutcome
                                  .extractionWorkUnitsProcessed * 8u,
             "SAB shared scalar curve memoization preserves each edge without rescanning curve tokens");

    DRW_SabParseLimits sharedCurveExact;
    sharedCurveExact.maxExtractionWorkUnits =
        sharedCurveEdgesOutcome.extractionWorkUnitsProcessed;
    sharedCurveExact.maxOutputItems =
        sharedCurveEdgesOutcome.outputItemsAccounted;
    DRW_AcisBrep sharedCurveExactBrep;
    DRW_SabParseOutcome sharedCurveExactOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(sharedCurveEdges.begin(),
                                            sharedCurveEdges.end()),
                 sharedCurveExactBrep, sharedCurveExactOutcome,
                 sharedCurveExact)
                 && sharedCurveExactOutcome.extractionWorkUnitsProcessed
                        == sharedCurveExact.maxExtractionWorkUnits
                 && sharedCurveExactOutcome.outputItemsAccounted
                        == sharedCurveExact.maxOutputItems,
             "SAB shared scalar curve cache admits exact extraction and cache-item budgets");

    DRW_SabParseLimits sharedCurveOver = sharedCurveExact;
    --sharedCurveOver.maxExtractionWorkUnits;
    DRW_AcisBrep rejectedSharedCurveBrep;
    DRW_SabParseOutcome rejectedSharedCurveOutcome;
    t.expect(!drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(sharedCurveEdges.begin(),
                                            sharedCurveEdges.end()),
                 rejectedSharedCurveBrep, rejectedSharedCurveOutcome,
                 sharedCurveOver)
                 && rejectedSharedCurveBrep.empty()
                 && rejectedSharedCurveOutcome.resourceLimitExceeded,
             "SAB shared scalar curve projection refuses one work unit below its measured bound");

    const std::vector<std::uint8_t> oneSharedEdgeReferenceBytes =
        sharedEndpointCurveEdgesSab(1u);
    DRW_AcisBrep oneSharedEdgeReferenceBrep;
    DRW_SabParseOutcome oneSharedEdgeReferenceOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(oneSharedEdgeReferenceBytes.begin(),
                                            oneSharedEdgeReferenceBytes.end()),
                 oneSharedEdgeReferenceBrep, oneSharedEdgeReferenceOutcome,
                 decodeDefaults)
                 && oneSharedEdgeReferenceBrep.edges.size() == 1u
                 && oneSharedEdgeReferenceBrep.edges[0].hasStart
                 && oneSharedEdgeReferenceBrep.edges[0].hasEnd
                 && acisCoordNear(oneSharedEdgeReferenceBrep.edges[0].start,
                                  1.0, 2.0, 3.0)
                 && acisCoordNear(oneSharedEdgeReferenceBrep.edges[0].end,
                                  4.0, 5.0, 6.0)
                 && oneSharedEdgeReferenceBrep.edges[0].curveType
                        == DRW_AcisCurve::Straight
                 && oneSharedEdgeReferenceBrep.edges[0].hasCurve
                 && acisCoordNear(oneSharedEdgeReferenceBrep.edges[0].p0,
                                  10.0, 20.0, 30.0)
                 && acisCoordNear(oneSharedEdgeReferenceBrep.edges[0].p1,
                                  1.0, 0.0, 0.0),
             "SAB edge-reference baseline preserves endpoint and curve selection");

    const std::vector<std::uint8_t> sharedEdgeReferenceBytes =
        sharedEndpointCurveEdgesSab(8u);
    DRW_AcisBrep sharedEdgeReferenceBrep;
    DRW_SabParseOutcome sharedEdgeReferenceOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(sharedEdgeReferenceBytes.begin(),
                                            sharedEdgeReferenceBytes.end()),
                 sharedEdgeReferenceBrep, sharedEdgeReferenceOutcome,
                 decodeDefaults)
                 && sharedEdgeReferenceBrep.edges.size() == 8u
                 && std::all_of(
                     sharedEdgeReferenceBrep.edges.begin(),
                     sharedEdgeReferenceBrep.edges.end(),
                     [](const DRW_AcisEdge& edge) {
                         return edge.hasStart && edge.hasEnd
                             && acisCoordNear(edge.start, 1.0, 2.0, 3.0)
                             && acisCoordNear(edge.end, 4.0, 5.0, 6.0)
                             && edge.curveType == DRW_AcisCurve::Straight
                             && edge.hasCurve
                             && acisCoordNear(edge.p0, 10.0, 20.0, 30.0)
                             && acisCoordNear(edge.p1, 1.0, 0.0, 0.0);
                     })
                 && sharedEdgeReferenceOutcome.extractionWorkUnitsProcessed
                        - oneSharedEdgeReferenceOutcome
                              .extractionWorkUnitsProcessed
                        == 7u * 6u,
             "SAB edge endpoint/curve projection selects first references in one pass");

    DRW_SabParseLimits sharedEdgeReferenceExact;
    sharedEdgeReferenceExact.maxExtractionWorkUnits =
        sharedEdgeReferenceOutcome.extractionWorkUnitsProcessed;
    sharedEdgeReferenceExact.maxOutputItems =
        sharedEdgeReferenceOutcome.outputItemsAccounted;
    DRW_AcisBrep exactSharedEdgeReferenceBrep;
    DRW_SabParseOutcome exactSharedEdgeReferenceOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(sharedEdgeReferenceBytes.begin(),
                                            sharedEdgeReferenceBytes.end()),
                 exactSharedEdgeReferenceBrep,
                 exactSharedEdgeReferenceOutcome, sharedEdgeReferenceExact)
                 && exactSharedEdgeReferenceOutcome.extractionWorkUnitsProcessed
                        == sharedEdgeReferenceExact.maxExtractionWorkUnits
                 && exactSharedEdgeReferenceOutcome.outputItemsAccounted
                        == sharedEdgeReferenceExact.maxOutputItems,
             "SAB combined edge-reference scan admits exact work/output limits");

    DRW_SabParseLimits sharedEdgeReferenceOver = sharedEdgeReferenceExact;
    --sharedEdgeReferenceOver.maxExtractionWorkUnits;
    DRW_AcisBrep rejectedSharedEdgeReferenceBrep;
    DRW_SabParseOutcome rejectedSharedEdgeReferenceOutcome;
    t.expect(!drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(sharedEdgeReferenceBytes.begin(),
                                            sharedEdgeReferenceBytes.end()),
                 rejectedSharedEdgeReferenceBrep,
                 rejectedSharedEdgeReferenceOutcome, sharedEdgeReferenceOver)
                 && rejectedSharedEdgeReferenceBrep.empty()
                 && rejectedSharedEdgeReferenceOutcome.resourceLimitExceeded,
             "SAB combined edge-reference scan refuses one unit below measured work");

    const std::vector<std::uint8_t> oneSharedIntcurveEdgeBytes =
        sharedIntcurveEdgesSab(1u);
    DRW_AcisBrep oneSharedIntcurveEdgeBrep;
    DRW_SabParseOutcome oneSharedIntcurveEdgeOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(oneSharedIntcurveEdgeBytes.begin(),
                                            oneSharedIntcurveEdgeBytes.end()),
                 oneSharedIntcurveEdgeBrep, oneSharedIntcurveEdgeOutcome,
                 decodeDefaults)
                 && oneSharedIntcurveEdgeBrep.edges.size() == 1u
                 && oneSharedIntcurveEdgeBrep.edges[0].curveType
                        == DRW_AcisCurve::Intcurve
                 && oneSharedIntcurveEdgeBrep.edges[0].hasCurve
                 && oneSharedIntcurveEdgeBrep.edges[0].controlPoints.size() == 2u
                 && acisCoordNear(
                     oneSharedIntcurveEdgeBrep.edges[0].controlPoints[0],
                     7.0, 8.0, 9.0)
                 && acisCoordNear(
                     oneSharedIntcurveEdgeBrep.edges[0].controlPoints[1],
                     10.0, 11.0, 12.0),
             "SAB shared-intcurve baseline recovers its control polygon");

    const std::vector<std::uint8_t> sharedIntcurveEdges =
        sharedIntcurveEdgesSab(8u);
    DRW_AcisBrep sharedIntcurveEdgesBrep;
    DRW_SabParseOutcome sharedIntcurveEdgesOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(sharedIntcurveEdges.begin(),
                                            sharedIntcurveEdges.end()),
                 sharedIntcurveEdgesBrep, sharedIntcurveEdgesOutcome,
                 decodeDefaults)
                 && sharedIntcurveEdgesBrep.edges.size() == 8u
                 && std::all_of(
                     sharedIntcurveEdgesBrep.edges.begin(),
                     sharedIntcurveEdgesBrep.edges.end(),
                     [](const DRW_AcisEdge& edge) {
                         return edge.curveType == DRW_AcisCurve::Intcurve
                             && edge.hasCurve
                             && edge.controlPoints.size() == 2u
                             && acisCoordNear(edge.controlPoints[0],
                                              7.0, 8.0, 9.0)
                             && acisCoordNear(edge.controlPoints[1],
                                              10.0, 11.0, 12.0);
                     })
                 && sharedIntcurveEdgesBrep.edges[0].controlPoints.data()
                        != sharedIntcurveEdgesBrep.edges[1].controlPoints.data()
                 && sharedIntcurveEdgesOutcome.extractionWorkUnitsProcessed
                        > oneSharedIntcurveEdgeOutcome
                              .extractionWorkUnitsProcessed
                 && sharedIntcurveEdgesOutcome.extractionWorkUnitsProcessed
                        < oneSharedIntcurveEdgeOutcome
                                  .extractionWorkUnitsProcessed * 8u,
             "SAB shared intcurve memoization preserves separate edge vectors without rescanning subtype knots");

    DRW_SabParseLimits sharedIntcurveExact;
    sharedIntcurveExact.maxExtractionWorkUnits =
        sharedIntcurveEdgesOutcome.extractionWorkUnitsProcessed;
    sharedIntcurveExact.maxOutputItems =
        sharedIntcurveEdgesOutcome.outputItemsAccounted;
    DRW_AcisBrep sharedIntcurveExactBrep;
    DRW_SabParseOutcome sharedIntcurveExactOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(sharedIntcurveEdges.begin(),
                                            sharedIntcurveEdges.end()),
                 sharedIntcurveExactBrep, sharedIntcurveExactOutcome,
                 sharedIntcurveExact)
                 && sharedIntcurveExactOutcome.extractionWorkUnitsProcessed
                        == sharedIntcurveExact.maxExtractionWorkUnits
                 && sharedIntcurveExactOutcome.outputItemsAccounted
                        == sharedIntcurveExact.maxOutputItems,
             "SAB shared intcurve cache admits exact extraction and output-item budgets");

    DRW_SabParseLimits sharedIntcurveOver = sharedIntcurveExact;
    --sharedIntcurveOver.maxExtractionWorkUnits;
    DRW_AcisBrep rejectedSharedIntcurveBrep;
    DRW_SabParseOutcome rejectedSharedIntcurveOutcome;
    t.expect(!drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(sharedIntcurveEdges.begin(),
                                            sharedIntcurveEdges.end()),
                 rejectedSharedIntcurveBrep, rejectedSharedIntcurveOutcome,
                 sharedIntcurveOver)
                 && rejectedSharedIntcurveBrep.empty()
                 && rejectedSharedIntcurveOutcome.resourceLimitExceeded,
             "SAB shared intcurve projection refuses one work unit below its measured bound");

    const std::vector<std::uint8_t> oneUnprojectedRecord =
        unprojectedRecordsSab(1u);
    DRW_AcisBrep oneUnprojectedBrep;
    DRW_SabParseOutcome oneUnprojectedOutcome;
    const std::vector<std::uint8_t> eightUnprojectedRecords =
        unprojectedRecordsSab(8u);
    DRW_AcisBrep eightUnprojectedBrep;
    DRW_SabParseOutcome eightUnprojectedOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(oneUnprojectedRecord.begin(),
                                            oneUnprojectedRecord.end()),
                 oneUnprojectedBrep, oneUnprojectedOutcome, decodeDefaults)
                 && drw_decodeAcisWireframeWithLimits(
                     std::vector<unsigned char>(eightUnprojectedRecords.begin(),
                                                eightUnprojectedRecords.end()),
                     eightUnprojectedBrep, eightUnprojectedOutcome,
                     decodeDefaults)
                 && oneUnprojectedBrep.empty()
                 && eightUnprojectedBrep.empty()
                 && eightUnprojectedOutcome.extractionWorkUnitsProcessed
                        - oneUnprojectedOutcome.extractionWorkUnitsProcessed
                        == 7u,
             "SAB unprojected nodes incur one candidate scan each");

    DRW_SabParseLimits unprojectedExact;
    unprojectedExact.maxExtractionWorkUnits =
        eightUnprojectedOutcome.extractionWorkUnitsProcessed;
    unprojectedExact.maxOutputItems =
        eightUnprojectedOutcome.outputItemsAccounted;
    DRW_AcisBrep unprojectedExactBrep;
    DRW_SabParseOutcome unprojectedExactOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(eightUnprojectedRecords.begin(),
                                            eightUnprojectedRecords.end()),
                 unprojectedExactBrep, unprojectedExactOutcome,
                 unprojectedExact)
                 && unprojectedExactOutcome.extractionWorkUnitsProcessed
                        == unprojectedExact.maxExtractionWorkUnits
                 && unprojectedExactOutcome.outputItemsAccounted
                        == unprojectedExact.maxOutputItems,
             "SAB single-pass candidate scan admits exact work/output budgets");

    DRW_SabParseLimits unprojectedOver = unprojectedExact;
    --unprojectedOver.maxExtractionWorkUnits;
    DRW_AcisBrep rejectedUnprojectedBrep;
    DRW_SabParseOutcome rejectedUnprojectedOutcome;
    t.expect(!drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(eightUnprojectedRecords.begin(),
                                            eightUnprojectedRecords.end()),
                 rejectedUnprojectedBrep, rejectedUnprojectedOutcome,
                 unprojectedOver)
                 && rejectedUnprojectedBrep.empty()
                 && rejectedUnprojectedOutcome.resourceLimitExceeded,
             "SAB single-pass candidate scan refuses one work unit below its bound");

    const std::vector<std::uint8_t> largerLoopCycleBytes =
        repeatedLoopCycleSab(8u, 8u, 8u);
    DRW_AcisBrep largerLoopCycleBrep;
    DRW_SabParseOutcome largerLoopCycleOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(largerLoopCycleBytes.begin(),
                                            largerLoopCycleBytes.end()),
                 largerLoopCycleBrep, largerLoopCycleOutcome, decodeDefaults)
                 && largerLoopCycleBrep.faces.size() == 8u
                 && largerLoopCycleBrep.faces[0].loops.size() == 8u
                 && largerLoopCycleBrep.faces[7].loops.size() == 8u
                 && largerLoopCycleBrep.faces[0].loops[7].coedgeCount == 8
                 && largerLoopCycleOutcome.extractionWorkUnitsProcessed
                        > loopCycleOutcome.extractionWorkUnitsProcessed,
             "SAB extraction bounds the generated shared 8x8 cycle workload");

    DRW_SabParseLimits loopCycleExact = decodeDefaults;
    loopCycleExact.maxInputBytes = loopCycleBytes.size();
    loopCycleExact.maxRecords = loopCycleOutcome.recordsProcessed;
    loopCycleExact.maxTags = loopCycleOutcome.tagsProcessed;
    loopCycleExact.maxCumulativeStringBytes =
        loopCycleOutcome.stringBytesAccounted;
    loopCycleExact.maxGraphWorkUnits =
        loopCycleOutcome.graphWorkUnitsProcessed;
    loopCycleExact.maxGraphItems = loopCycleOutcome.graphItemsAccounted;
    loopCycleExact.maxExtractionWorkUnits =
        loopCycleOutcome.extractionWorkUnitsProcessed;
    loopCycleExact.maxOutputItems = loopCycleOutcome.outputItemsAccounted;
    loopCycleExact.maxTokenCapacityItems =
        loopCycleOutcome.tokenCapacityItemsAccounted;
    loopCycleExact.maxTypePartCapacityItems =
        loopCycleOutcome.typePartCapacityPeakItems;
    DRW_AcisBrep exactLoopCycleBrep;
    DRW_SabParseOutcome exactLoopCycleOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(loopCycleBytes.begin(),
                                            loopCycleBytes.end()),
                 exactLoopCycleBrep, exactLoopCycleOutcome, loopCycleExact)
                 && exactLoopCycleBrep.faces.size() == 4u
                 && exactLoopCycleOutcome.extractionWorkUnitsProcessed
                        == loopCycleExact.maxExtractionWorkUnits,
             "SAB cyclic shared-reference extraction admits exact work budget");

    DRW_SabParseLimits loopCycleOver = loopCycleExact;
    --loopCycleOver.maxExtractionWorkUnits;
    DRW_AcisBrep rejectedLoopCycleBrep;
    DRW_SabParseOutcome rejectedLoopCycleOutcome;
    t.expect(!drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(loopCycleBytes.begin(),
                                            loopCycleBytes.end()),
                 rejectedLoopCycleBrep, rejectedLoopCycleOutcome,
                 loopCycleOver)
                 && rejectedLoopCycleBrep.empty()
                 && rejectedLoopCycleOutcome.resourceLimitExceeded
                 && rejectedLoopCycleOutcome.complete,
             "SAB cyclic revisits stop at one under the shared work budget");

    DRW_SabParseLimits graphDecodeOver = decodeLimits;
    graphDecodeOver.maxGraphItems = 0u;
    DRW_SabParseOutcome graphDecodeOutcome;
    t.expect(!drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(bytes.begin(), bytes.end()),
                 decoded, graphDecodeOutcome, graphDecodeOver)
                 && decoded.empty()
                 && graphDecodeOutcome.resourceLimitExceeded
                 && graphDecodeOutcome.complete,
             "SAB convenience decoder reports graph-cap failure after valid framing");

    DRW_SabParseLimits extractionDecodeOver = decodeLimits;
    --extractionDecodeOver.maxExtractionWorkUnits;
    DRW_SabParseOutcome extractionDecodeOutcome;
    t.expect(!drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(bytes.begin(), bytes.end()),
                 decoded, extractionDecodeOutcome, extractionDecodeOver)
                 && decoded.empty()
                 && extractionDecodeOutcome.resourceLimitExceeded
                 && extractionDecodeOutcome.complete,
             "SAB decoder rejects extraction revisits beyond their cap");

    DRW_SabParseLimits outputDecodeOver = decodeLimits;
    --outputDecodeOver.maxOutputItems;
    DRW_SabParseOutcome outputDecodeOutcome;
    t.expect(!drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(bytes.begin(), bytes.end()),
                 decoded, outputDecodeOutcome, outputDecodeOver)
                 && decoded.empty()
                 && outputDecodeOutcome.resourceLimitExceeded
                 && outputDecodeOutcome.complete,
             "SAB decoder rejects projected output beyond its cap");

    const std::vector<std::uint8_t> unresolvedVertexSab =
        vertexSab(21800u, 3u, 99u);
    DRW_AcisBrep unresolvedVertex;
    DRW_SabParseOutcome unresolvedOutcome;
    DRW_SabParseLimits unresolvedLimits;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(unresolvedVertexSab.begin(),
                                            unresolvedVertexSab.end()),
                 unresolvedVertex, unresolvedOutcome, unresolvedLimits)
                 && unresolvedVertex.vertices.size() == 1u
                 && unresolvedVertex.diagnostics.size() == 1u,
             "SAB projection keeps the bounded missing-point diagnostic");

    const std::vector<std::uint8_t> resolvedVertexSab =
        vertexSab(21800u, 3u, 0u);
    DRW_AcisBrep resolvedVertex;
    DRW_SabParseOutcome resolvedOutcome;
    t.expect(drw_decodeAcisWireframeWithOutcome(
                 std::vector<unsigned char>(resolvedVertexSab.begin(),
                                            resolvedVertexSab.end()),
                 resolvedVertex, resolvedOutcome)
                 && resolvedVertex.diagnostics.empty()
                 && unresolvedOutcome.outputItemsAccounted
                        >= resolvedOutcome.outputItemsAccounted
                 && unresolvedOutcome.outputItemsAccounted
                            - resolvedOutcome.outputItemsAccounted
                        == 2u,
             "SAB diagnostic budget counts staging and published result slots");

    DRW_SabParseLimits diagnosticExactLimits = unresolvedLimits;
    diagnosticExactLimits.maxInputBytes = unresolvedVertexSab.size();
    diagnosticExactLimits.maxRecords = unresolvedOutcome.recordsProcessed;
    diagnosticExactLimits.maxTags = unresolvedOutcome.tagsProcessed;
    diagnosticExactLimits.maxStringBytes = 29u;
    diagnosticExactLimits.maxCumulativeStringBytes =
        unresolvedOutcome.stringBytesAccounted;
    diagnosticExactLimits.maxGraphWorkUnits =
        unresolvedOutcome.graphWorkUnitsProcessed;
    diagnosticExactLimits.maxGraphItems = unresolvedOutcome.graphItemsAccounted;
    diagnosticExactLimits.maxExtractionWorkUnits =
        unresolvedOutcome.extractionWorkUnitsProcessed;
    diagnosticExactLimits.maxOutputItems = unresolvedOutcome.outputItemsAccounted;
    DRW_AcisBrep diagnosticExactOutput;
    DRW_SabParseOutcome diagnosticExactOutcome;
    t.expect(drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(unresolvedVertexSab.begin(),
                                            unresolvedVertexSab.end()),
                 diagnosticExactOutput, diagnosticExactOutcome,
                 diagnosticExactLimits)
                 && diagnosticExactOutput.diagnostics.size() == 1u
                 && diagnosticExactOutcome.stringBytesAccounted
                        == diagnosticExactLimits.maxCumulativeStringBytes
                 && diagnosticExactOutcome.outputItemsAccounted
                        == diagnosticExactLimits.maxOutputItems,
             "SAB diagnostic construction admits exact work and string caps");

    DRW_SabParseLimits diagnosticStringOver;
    diagnosticStringOver.maxStringBytes = 28u;
    DRW_SabParseOutcome diagnosticStringOutcome;
    t.expect(!drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(unresolvedVertexSab.begin(),
                                            unresolvedVertexSab.end()),
                 decoded, diagnosticStringOutcome, diagnosticStringOver)
                 && decoded.empty()
                 && diagnosticStringOutcome.resourceLimitExceeded
                 && diagnosticStringOutcome.complete,
             "SAB decoder pre-admits generated diagnostic string limits");

    DRW_SabParseLimits diagnosticOutputOver = unresolvedLimits;
    diagnosticOutputOver.maxOutputItems =
        unresolvedOutcome.outputItemsAccounted - 1u;
    DRW_SabParseOutcome diagnosticOutputOutcome;
    t.expect(!drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(unresolvedVertexSab.begin(),
                                            unresolvedVertexSab.end()),
                 decoded, diagnosticOutputOutcome, diagnosticOutputOver)
                 && decoded.empty()
                 && diagnosticOutputOutcome.resourceLimitExceeded
                 && diagnosticOutputOutcome.complete,
             "SAB decoder pre-admits diagnostic and geometry vector growth");

    DRW_SabParseLimits decodeOver = exactLimits;
    --decodeOver.maxInputBytes;
    DRW_SabParseOutcome decodeOutcome;
    t.expect(!drw_decodeAcisWireframeWithLimits(
                 std::vector<unsigned char>(bytes.begin(), bytes.end()),
                 decoded, decodeOutcome, decodeOver)
                 && decoded.empty()
                 && decodeOutcome.resourceLimitExceeded,
             "SAB convenience decoder refuses an over-budget raw carrier");
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

void appendProxyChunk(std::vector<std::uint8_t>& stream,
                      std::uint32_t opcode,
                      const std::vector<std::uint8_t>& payload) {
    appendU32(stream, static_cast<std::uint32_t>(payload.size() + 8u));
    appendU32(stream, opcode);
    stream.insert(stream.end(), payload.begin(), payload.end());
}

void testProxyShellMultiFaceFraming(TestContext& t) {
    // A proxy stream has an eight-byte header followed by independently sized
    // chunks. The shell face-entry count is a count of longs, not faces.
    std::vector<std::uint8_t> stream(8u, 0u);
    std::vector<std::uint8_t> shell;
    appendU32(shell, 4u); // vertices
    appendDouble(shell, 0.0); appendDouble(shell, 0.0); appendDouble(shell, 0.0);
    appendDouble(shell, 1.0); appendDouble(shell, 0.0); appendDouble(shell, 0.0);
    appendDouble(shell, 1.0); appendDouble(shell, 1.0); appendDouble(shell, 0.0);
    appendDouble(shell, 0.0); appendDouble(shell, 1.0); appendDouble(shell, 0.0);
    appendU32(shell, 8u); // two faces, four longs each
    const std::uint32_t faceEntries[] = {3u, 0u, 1u, 2u, 3u, 0u, 2u, 3u};
    for (const std::uint32_t value : faceEntries)
        appendU32(shell, value);
    appendProxyChunk(stream, 9u, shell); // SHELL

    std::vector<std::uint8_t> circle;
    appendDouble(circle, 123.5); appendDouble(circle, -67.25);
    appendDouble(circle, 9.0); // center: unique next-chunk sentinel
    appendDouble(circle, 2.25); // radius
    appendDouble(circle, 0.0); appendDouble(circle, 0.0);
    appendDouble(circle, 1.0); // normal
    appendProxyChunk(stream, 2u, circle); // CIRCLE after the shell

    const std::string encoded(
        reinterpret_cast<const char*>(stream.data()), stream.size());
    DRW_Point parent;
    ProxyGraphicCapture capture;
    const DRW_ProxyGraphicDecodeResult decoded = DRW_ProxyGraphicDecoder::decode(
        encoded, DRW::AC1027, capture, parent);

    t.expect(decoded.completed()
                 && decoded.consumedByteCount == encoded.size()
                 && decoded.recognizedPrimitiveChunkCount == 2u
                 && decoded.emittedPrimitiveCount == 2u,
             "multi-face proxy shell consumes exactly its chunk and next primitive");
    t.expect(capture.meshes.size() == 1u
                 && capture.meshes[0].vertices.size() == 4u
                 && capture.meshes[0].faces.size() == 2u
                 && capture.meshes[0].faces[0]
                        == std::vector<std::int32_t>({0, 1, 2})
                 && capture.meshes[0].faces[1]
                        == std::vector<std::int32_t>({0, 2, 3}),
             "multi-face proxy shell preserves both counted face lists");
    t.expect(capture.circles.size() == 1u
                 && capture.circles[0].basePoint.x == 123.5
                 && capture.circles[0].basePoint.y == -67.25
                 && capture.circles[0].basePoint.z == 9.0
                 && capture.circles[0].radious == 2.25,
             "following proxy circle sentinel retains its identity and payload");
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
    testDataStorageRecordCountAlignmentAnchors(context);
    testDataStorageDefaultSchemaOnlySection(context);
    testAcisBoundaries(context);
    testAcisWireframeHeaderProjection(context);
    testAcis21800ProfileFraming(context);
    testSab21800RecordHeaderAlignment(context);
    testAcisWireframeGraph(context);
    testAcisRootedBodyProjection(context);
    testAcisOrientationFlagsRequireExactPointerPrefix(context);
    testAcisRootedBodyProjectionFailures(context);
    testModelerLazyDecode(context);
    testProxyBounds(context);
    testProxyShellMultiFaceFraming(context);
    testDataStorageCapabilities(context);
    if (context.failures != 0) {
        std::cerr << context.failures
                  << " graph/preservation assertion(s) failed\n";
        return 1;
    }
    std::cout << "Graph/preservation tests: PASS\n";
    return 0;
}
