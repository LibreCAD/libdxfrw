/******************************************************************************
**  dwg2dxf - Program to convert dwg/dxf to dxf(ascii & binary)              **
**                                                                           **
**  Copyright (C) 2015 José F. Soriano, rallazz@gmail.com                    **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include <iostream>
#include <algorithm>
#include <charconv>
#include <array>
#include <cctype>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include "dx_iface.h"
#include "libdwgr.h"
#include "libdxfrw.h"
#include "intern/dxfparserlimits.h"
#include "intern/dxfcode.h"

namespace {

// A fixed buffer independently bounds the formatter, including future added
// fields. Reserve a footer so truncation never masquerades as a full report.
class DwgReadReportBuffer {
public:
    void append(const char* text) noexcept {
        const std::size_t count = std::char_traits<char>::length(text);
        const std::size_t copied = std::min(count, contentLimit - used);
        std::copy_n(text, copied, bytes.begin() + used);
        used += copied;
        truncated = truncated || copied != count;
    }
    template<typename Integer> void integer(Integer value) noexcept {
        std::array<char, 32> text{};
        const auto result = std::to_chars(text.data(), text.data() + text.size() - 1,
                                          value);
        if (result.ec == std::errc{}) {
            *result.ptr = '\0';
            append(text.data());
        }
    }
    template<typename Integer> void field(const char* name, Integer value) noexcept {
        append(name); append("="); integer(static_cast<std::uint64_t>(value)); append(" ");
    }
    void prefix(const DwgReadBytePrefix& value) noexcept {
        static constexpr char hex[] = "0123456789abcdef";
        append("raw-prefix=\"");
        // Values from the public struct may be caller-constructed. Clamp the
        // length even though the core extractor always obeys this invariant.
        const std::size_t renderedBytes = std::min(value.length, value.bytes.size());
        for (std::size_t index = 0; index < renderedBytes;
             ++index) {
            const unsigned byte = value.bytes[index];
            const char escaped[] = {'\\', 'x', hex[byte >> 4], hex[byte & 15], '\0'};
            append(escaped);
        }
        append("\" ");
        field("prefixBytes", renderedBytes);
        field("sourceBytes", value.sourceBytes);
        append(renderedBytes != value.sourceBytes || value.length > value.bytes.size()
            ? "nameTruncated=true " : "nameTruncated=false ");
    }
    void write(std::ostream& output) const {
        output.write(bytes.data(), static_cast<std::streamsize>(used));
        if (truncated)
            output.write(footer, sizeof(footer) - 1);
    }
private:
    static constexpr char footer[] =
        "\nDWG read report: truncated=true (report-byte-limit)\n";
    static constexpr std::size_t contentLimit =
        dx_iface::DwgReadReportMaxBytes - (sizeof(footer) - 1);
    std::array<char, dx_iface::DwgReadReportMaxBytes> bytes{};
    std::size_t used{0};
    bool truncated{false};
};

bool observedDwgSemanticGaps(const DwgReadObservations& value) noexcept {
    return value.entityParseFailures != 0 || value.objectParseFailures != 0
        || value.skippedCustomClasses.sourceNames != 0
        || value.skippedUnsupportedObjects.sourceNames != 0
        || value.dataStorageLinkFailures != 0;
}

bool hasDwgReadWarning(const dx_dwgReadOutcome& outcome) noexcept {
    const DwgReadObservations& value = outcome.observations;
    return !outcome.operationSucceeded || observedDwgSemanticGaps(value)
        || value.classesCrcMismatch != 0 || value.reader21CrcMismatch != 0
        || value.reader18HeaderCrcMismatch != 0
        || value.dataStorageOrphanRecords != 0
        || value.integrity.sourceRecorded != 0
        || value.integrity.reportedUpstreamDropped != 0
        || value.frames.status == DRW_DwgFrameCoverageStatus::FinalizedPartial;
}

bool visualStyleRawIdentityMatches(const DRW_RawDxfObject& object) {
    if (object.handle == 0 || object.handle == DRW::NoHandle
        || object.parentHandle == 0 || object.parentHandle == DRW::NoHandle
        || (object.hasRawValues
            && object.rawValues.size() != object.groups.size()))
        return false;

    const auto parseHandle = [](const std::string& text,
                                std::uint32_t& handle) {
        if (text.empty() || text.size() > 16u)
            return false;
        std::uint64_t parsed = 0;
        const char* begin = text.data();
        const char* end = begin + text.size();
        const std::from_chars_result result =
            std::from_chars(begin, end, parsed, 16);
        if (result.ec != std::errc{} || result.ptr != end || parsed == 0
            || parsed > std::numeric_limits<std::uint32_t>::max())
            return false;
        handle = static_cast<std::uint32_t>(parsed);
        return true;
    };

    std::size_t selfHandleCount = 0;
    std::size_t ownerHandleCount = 0;
    int applicationDepth = 0;
    for (std::size_t index = 0; index < object.groups.size(); ++index) {
        const DRW_Variant& group = object.groups[index];
        if (group.code() == 102) {
            if (group.type() != DRW_Variant::STRING
                || group.c_str() == nullptr)
                return false;
            const std::string marker(group.c_str());
            if (marker.size() > 1u && marker.front() == '{') {
                ++applicationDepth;
            } else if (marker == "}") {
                if (applicationDepth == 0)
                    return false;
                --applicationDepth;
            } else {
                return false;
            }
            continue;
        }
        if (applicationDepth != 0
            || (group.code() != 5 && group.code() != 330))
            continue;
        if (group.type() != DRW_Variant::STRING || group.c_str() == nullptr)
            return false;

        std::uint32_t decodedHandle = 0;
        const std::uint32_t expectedHandle =
            group.code() == 5 ? object.handle : object.parentHandle;
        std::size_t& count = group.code() == 5
            ? selfHandleCount : ownerHandleCount;
        if (++count != 1u
            || !parseHandle(group.c_str(), decodedHandle)
            || decodedHandle != expectedHandle)
            return false;
        if (object.hasRawValues) {
            std::uint32_t sourceHandle = 0;
            if (!parseHandle(object.rawValues[index], sourceHandle)
                || sourceHandle != expectedHandle)
                return false;
        }
    }
    return applicationDepth == 0 && selfHandleCount == 1u
        && ownerHandleCount == 1u;
}

const char* canonicalDwgReadVersion(DRW::Version version) noexcept {
    switch (version) {
    case DRW::MC00: return "MC0.0";
    case DRW::AC12: return "AC1.2";
    case DRW::AC14: return "AC1.40";
    case DRW::AC150: return "AC1.50";
    case DRW::AC210: return "AC2.10";
    case DRW::AC1002: return "AC1002";
    case DRW::AC1003: return "AC1003";
    case DRW::AC1004: return "AC1004";
    case DRW::AC1006: return "AC1006";
    case DRW::AC1009: return "AC1009";
    case DRW::AC1012: return "AC1012";
    case DRW::AC1014: return "AC1014";
    case DRW::AC1015: return "AC1015";
    case DRW::AC1018: return "AC1018";
    case DRW::AC1021: return "AC1021";
    case DRW::AC1024: return "AC1024";
    case DRW::AC1027: return "AC1027";
    case DRW::AC1032: return "AC1032";
    default: return "unknown";
    }
}

void formatNamedDwgObservations(DwgReadReportBuffer& buffer, const char* label,
                                 const DwgReadNamedObservations& value) noexcept {
    buffer.append(label); buffer.append(": ");
    buffer.field("sourceNames", value.sourceNames);
    buffer.field("observedInstances", value.instances);
    buffer.field("retainedRows", value.retained);
    buffer.field("omittedNames", value.omittedNames);
    buffer.field("omittedRowInstances", value.omittedInstances);
    buffer.field("truncatedRetainedNames", value.truncatedRetainedNames);
    buffer.field("omittedNameBytes", value.omittedNameBytes);
    buffer.append(value.aggregateSaturated ? "aggregateSaturated=true\n"
                                          : "aggregateSaturated=false\n");
    for (std::size_t index = 0; index < std::min(value.retained, value.rows.size());
         ++index) {
        buffer.append(label); buffer.append(" row: ");
        buffer.prefix(value.rows[index].name);
        buffer.field("instances", value.rows[index].instances);
        buffer.append("\n");
    }
}

DRW::Version sourceVersionFromDxfHeader(
    const DRW_Header& header, DRW::Version fallback) {
    const DRW::Version accepted = header.dxfSourceVersion();
    return accepted == DRW::UNKNOWNV ? fallback : accepted;
}

void addAcdsSchemaHeader(DRW_RawDxfSection& section, int index,
                         const char* schemaName, const char* payloadName) {
    section.m_groups.emplace_back(0, "ACDSSCHEMA");
    section.m_groups.emplace_back(90, index);
    section.m_groups.emplace_back(1, schemaName);
    section.m_groups.emplace_back(2, "AcDbDs::ID");
    section.m_groups.emplace_back(280, 10);
    section.m_groups.emplace_back(91, 8);
    section.m_groups.emplace_back(2, payloadName);
    section.m_groups.emplace_back(280, 15);
    section.m_groups.emplace_back(91, 0);
}

void addAcdsSchemaAttributes(DRW_RawDxfSection& section, int index) {
    section.m_groups.emplace_back(101, "ACDSRECORD");
    section.m_groups.emplace_back(95, index);
    section.m_groups.emplace_back(90, 2);
    section.m_groups.emplace_back(2, "AcDbDs::TreatedAsObjectData");
    section.m_groups.emplace_back(280, 1);
    section.m_groups.emplace_back(291, 1);

    section.m_groups.emplace_back(101, "ACDSRECORD");
    section.m_groups.emplace_back(95, index);
    section.m_groups.emplace_back(90, 3);
    section.m_groups.emplace_back(2, "AcDbDs::Legacy");
    section.m_groups.emplace_back(280, 1);
    section.m_groups.emplace_back(291, 1);

    section.m_groups.emplace_back(101, "ACDSRECORD");
    section.m_groups.emplace_back(1, "AcDbDs::ID");
    section.m_groups.emplace_back(90, 4);
    section.m_groups.emplace_back(2, "AcDs:Indexable");
    section.m_groups.emplace_back(280, 1);
    section.m_groups.emplace_back(291, 1);

    section.m_groups.emplace_back(101, "ACDSRECORD");
    section.m_groups.emplace_back(1, "AcDbDs::ID");
    section.m_groups.emplace_back(90, 5);
    section.m_groups.emplace_back(2, "AcDbDs::HandleAttribute");
    section.m_groups.emplace_back(280, 7);
    section.m_groups.emplace_back(282, 1);
}

void addAcdsMarkerSchema(DRW_RawDxfSection& section, int index,
                         const char* name, const char* member) {
    section.m_groups.emplace_back(0, "ACDSSCHEMA");
    section.m_groups.emplace_back(90, index);
    section.m_groups.emplace_back(1, name);
    section.m_groups.emplace_back(2, member);
    section.m_groups.emplace_back(280, 1);
    section.m_groups.emplace_back(91, 0);
}

bool hasSabSignature(const std::vector<std::uint8_t>& payload) {
    static constexpr char signature[] = "ACIS BinaryFile";
    return payload.size() >= sizeof(signature) - 1u
        && std::equal(std::begin(signature), std::end(signature) - 1,
                      payload.begin(), [](char expected, std::uint8_t actual) {
                          return static_cast<std::uint8_t>(expected) == actual;
                      });
}

bool hasPngSignature(const std::vector<std::uint8_t>& payload) {
    static constexpr std::uint8_t signature[] = {
        0x89u, 'P', 'N', 'G', 0x0Du, 0x0Au, 0x1Au, 0x0Au};
    return payload.size() >= sizeof(signature)
        && std::equal(std::begin(signature), std::end(signature),
                      payload.begin());
}

bool matchesDataStorageSchemaProperty(
    const DRW_DataStorageSchemaProperty& property, std::uint32_t nameIndex,
    const char* name, std::uint32_t type, std::uint32_t flags,
    std::uint16_t valueCount,
    const std::vector<std::vector<std::uint8_t>>& values) {
    return name != nullptr && property.nameIndex == nameIndex
        && property.name == name && property.type == type
        && property.flags == flags && property.valueCount == valueCount
        && property.values == values;
}

// The DXF ACDSSCHEMA writer below reconstructs one observed AC1027 table.
// Restrict this bridge to the matching DWG DataStorage schema fingerprint;
// a six-schema count alone must never relabel a different property layout.
// This is a sample-qualified guard, not a general DataStorage schema codec.
bool matchesQualifiedOdaAc1027AcdsSchemas(
    const DRW_DataStorageSection& storage) {
    try {
    static const std::array<const char*, 7> expectedPropertyNames = {
        "AcDbDs::ID", "Thumbnail_Data", "ASM_Data",
        "AcDbDs::TreatedAsObjectData", "AcDbDs::Legacy",
        "AcDs:Indexable", "AcDbDs::HandleAttribute"};
    if (storage.schemaCount != 6u || storage.schemas.size() != 6u
        || storage.schemaPropertyNameCount != expectedPropertyNames.size()
        || storage.schemaPropertyNames.size() != expectedPropertyNames.size()) {
        return false;
    }
    for (std::size_t index = 0; index < expectedPropertyNames.size(); ++index) {
        if (storage.schemaPropertyNames[index] != expectedPropertyNames[index])
            return false;
    }

    std::array<const DRW_DataStorageSchema*, 6> schemasByIndex{};
    for (const DRW_DataStorageSchema& schema : storage.schemas) {
        if (schema.index >= schemasByIndex.size()
            || schemasByIndex[schema.index] != nullptr) {
            return false;
        }
        schemasByIndex[schema.index] = &schema;
    }
    if (!std::all_of(schemasByIndex.begin(), schemasByIndex.end(),
                     [](const DRW_DataStorageSchema* schema) {
                         return schema != nullptr;
                     })) {
        return false;
    }
    const auto propertyMatches = [](const DRW_DataStorageSchema& schema,
                                    std::size_t propertyIndex,
                                    std::uint32_t nameIndex,
                                    const char* name,
                                    std::uint32_t type,
                                    std::uint32_t flags,
                                    std::uint16_t valueCount,
                                    const std::vector<std::vector<std::uint8_t>>&
                                        values) {
        return propertyIndex < schema.properties.size()
            && matchesDataStorageSchemaProperty(
                schema.properties[propertyIndex], nameIndex, name, type, flags,
                valueCount, values);
    };
    const std::vector<std::vector<std::uint8_t>> idValues0 = {
        {6u, 0u, 0u, 0u, 0u, 0u, 0u, 0u},
        {7u, 0u, 0u, 0u, 0u, 0u, 0u, 0u}};
    const std::vector<std::vector<std::uint8_t>> idValues1 = {
        {2u, 0u, 0u, 0u, 0u, 0u, 0u, 0u},
        {3u, 0u, 0u, 0u, 0u, 0u, 0u, 0u}};
    const std::vector<std::vector<std::uint8_t>> handleValue = {{0u}};
    const auto& thumbnail = *schemasByIndex[0];
    const auto& asmData = *schemasByIndex[1];
    const auto& treatedAsObjectData = *schemasByIndex[2];
    const auto& legacy = *schemasByIndex[3];
    const auto& indexable = *schemasByIndex[4];
    const auto& handleAttribute = *schemasByIndex[5];
    return thumbnail.indexes == std::vector<std::uint64_t>{4u, 5u}
        && thumbnail.properties.size() == 2u
        && propertyMatches(thumbnail, 0u, 0u, "AcDbDs::ID", 10u, 0u, 2u,
                           idValues0)
        && propertyMatches(thumbnail, 1u, 1u, "Thumbnail_Data", 15u, 0u, 0u,
                           {})
        && asmData.indexes == std::vector<std::uint64_t>{0u, 1u}
        && asmData.properties.size() == 2u
        && propertyMatches(asmData, 0u, 0u, "AcDbDs::ID", 10u, 0u, 2u,
                           idValues1)
        && propertyMatches(asmData, 1u, 2u, "ASM_Data", 15u, 0u, 0u, {})
        && treatedAsObjectData.indexes.empty()
        && treatedAsObjectData.properties.size() == 1u
        && propertyMatches(treatedAsObjectData, 0u, 3u,
                           "AcDbDs::TreatedAsObjectData", 1u, 0u, 0u, {})
        && legacy.indexes.empty() && legacy.properties.size() == 1u
        && propertyMatches(legacy, 0u, 4u, "AcDbDs::Legacy", 1u, 0u, 0u, {})
        && indexable.indexes.empty() && indexable.properties.size() == 1u
        && propertyMatches(indexable, 0u, 5u, "AcDs:Indexable", 1u, 0u, 0u,
                           {})
        && handleAttribute.indexes.empty()
        && handleAttribute.properties.size() == 1u
        && propertyMatches(handleAttribute, 0u, 6u,
                           "AcDbDs::HandleAttribute", 7u, 8u, 1u,
                           handleValue);
    } catch (...) {
        return false;
    }
}

struct AcdsOutputSchemaRoles {
    std::uint32_t thumbnailSchemaIndex = 0;
    std::uint32_t modelerSchemaIndex = 0;
};

bool matchesQualifiedAcadSharpAc1027AcdsSchemas(
    const DRW_DataStorageSection& storage,
    AcdsOutputSchemaRoles& outputRoles) {
    try {
        static const std::array<const char*, 8> expectedPropertyNames = {
            "AcDbDs::ID", "Thumbnail_Data", "AcDbDs::TreatedAsObjectData",
            "AcDbDs::Legacy", "AcDs:Indexable", "AcDbDs::HandleAttribute",
            "AcDbDs::ID", "ASM_Data"};
        if (storage.schemaCount != 6u || storage.schemas.size() != 6u
            || storage.schemaPropertyNameCount != 2u
            || storage.schemaPropertyNames.size() != expectedPropertyNames.size()) {
            return false;
        }
        for (std::size_t index = 0; index < expectedPropertyNames.size();
             ++index) {
            if (storage.schemaPropertyNames[index]
                != expectedPropertyNames[index]) {
                return false;
            }
        }

        std::array<const DRW_DataStorageSchema*, 6> schemasByIndex{};
        for (const DRW_DataStorageSchema& schema : storage.schemas) {
            if (schema.index >= schemasByIndex.size()
                || schemasByIndex[schema.index] != nullptr) {
                return false;
            }
            schemasByIndex[schema.index] = &schema;
        }
        if (!std::all_of(schemasByIndex.begin(), schemasByIndex.end(),
                         [](const DRW_DataStorageSchema* schema) {
                             return schema != nullptr;
                         })) {
            return false;
        }
        const auto propertyMatches = [](
            const DRW_DataStorageSchema& schema, std::size_t propertyIndex,
            std::uint32_t nameIndex, const char* name, std::uint32_t type,
            std::uint32_t flags, std::uint16_t valueCount,
            std::size_t valueSize) {
            if (propertyIndex >= schema.properties.size())
                return false;
            const DRW_DataStorageSchemaProperty& property =
                schema.properties[propertyIndex];
            if (property.nameIndex != nameIndex || property.name != name
                || property.type != type || property.flags != flags
                || property.valueCount != valueCount
                || (valueCount == 0u && !property.values.empty())
                || (valueCount != 0u
                    && property.values.size() != valueCount)) {
                return false;
            }
            return std::all_of(
                property.values.begin(), property.values.end(),
                [valueSize](const std::vector<std::uint8_t>& value) {
                    return value.size() == valueSize;
                });
        };
        const auto idPropertyMatches = [&propertyMatches](
            const DRW_DataStorageSchema& schema, std::size_t propertyIndex) {
            return propertyMatches(schema, propertyIndex, 0u, "AcDbDs::ID",
                                   10u, 0u, 2u, 8u);
        };
        const auto& thumbnail = *schemasByIndex[0];
        const auto& treated = *schemasByIndex[1];
        const auto& legacy = *schemasByIndex[2];
        const auto& indexable = *schemasByIndex[3];
        const auto& handleAttribute = *schemasByIndex[4];
        const auto& modeler = *schemasByIndex[5];
        const bool profileMatches =
            thumbnail.indexes == std::vector<std::uint64_t>{0u, 1u}
            && thumbnail.properties.size() == 2u
            && idPropertyMatches(thumbnail, 0u)
            && propertyMatches(thumbnail, 1u, 1u, "Thumbnail_Data", 15u,
                               0u, 0u, 0u)
            && treated.indexes.empty() && treated.properties.size() == 1u
            && propertyMatches(treated, 0u, 2u,
                               "AcDbDs::TreatedAsObjectData", 1u, 0u, 0u,
                               0u)
            && legacy.indexes.empty() && legacy.properties.size() == 1u
            && propertyMatches(legacy, 0u, 3u, "AcDbDs::Legacy", 1u, 0u,
                               0u, 0u)
            && indexable.indexes.empty() && indexable.properties.size() == 1u
            && propertyMatches(indexable, 0u, 4u, "AcDs:Indexable", 1u,
                               0u, 0u, 0u)
            && handleAttribute.indexes.empty()
            && handleAttribute.properties.size() == 1u
            && propertyMatches(handleAttribute, 0u, 5u,
                               "AcDbDs::HandleAttribute", 7u, 8u, 1u, 1u)
            && modeler.indexes == std::vector<std::uint64_t>{6u, 4u}
            && modeler.properties.size() == 2u
            && idPropertyMatches(modeler, 0u)
            && propertyMatches(modeler, 1u, 1u, "ASM_Data", 15u, 0u, 0u,
                               0u)
            && thumbnail.segmentIndex == treated.segmentIndex
            && thumbnail.segmentIndex == legacy.segmentIndex
            && thumbnail.segmentIndex == indexable.segmentIndex
            && thumbnail.segmentIndex == handleAttribute.segmentIndex
            && thumbnail.segmentIndex != modeler.segmentIndex;
        if (!profileMatches)
            return false;

        outputRoles.thumbnailSchemaIndex = 0u;
        outputRoles.modelerSchemaIndex = 5u;
        return true;
    } catch (...) {
        return false;
    }
}

bool matchesQualifiedAc1027AcdsSchemas(
    const DRW_DataStorageSection& storage,
    AcdsOutputSchemaRoles& outputRoles) {
    if (matchesQualifiedOdaAc1027AcdsSchemas(storage)) {
        outputRoles.thumbnailSchemaIndex = 0u;
        outputRoles.modelerSchemaIndex = 1u;
        return true;
    }
    return matchesQualifiedAcadSharpAc1027AcdsSchemas(storage, outputRoles);
}

std::string proxyHandleString(std::uint64_t handle) {
    std::array<char, 17> digits{};
    const auto converted = std::to_chars(digits.data(), digits.data() + 16,
                                         handle, 16);
    if (converted.ec != std::errc())
        return {};
    std::string value(digits.data(), converted.ptr);
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::toupper(ch));
                   });
    return value;
}

bool appendProxyBinaryGroups(DRW_RawDxfObject& raw, int code,
                             const std::vector<std::uint8_t>& bytes) {
    constexpr std::size_t bytesPerChunk = 127u;
    for (std::size_t offset = 0; offset < bytes.size();
         offset += bytesPerChunk) {
        const std::size_t count = std::min(bytesPerChunk,
                                           bytes.size() - offset);
        static constexpr char hex[] = "0123456789ABCDEF";
        std::string chunk;
        chunk.reserve(count * 2u);
        for (std::size_t index = 0; index < count; ++index) {
            const std::uint8_t byte = bytes[offset + index];
            chunk.push_back(hex[byte >> 4u]);
            chunk.push_back(hex[byte & 0x0Fu]);
        }
        raw.groups.emplace_back(code, std::move(chunk));
    }
    return true;
}

bool makeRawProxyObject(const DRW_ProxyObject& proxy,
                        std::int32_t outputClassId,
                        DRW_RawDxfObject& raw) {
    const auto expectedBytes = [](std::uint64_t bits) {
        return bits / 8u + ((bits & 7u) != 0 ? 1u : 0u);
    };
    if (proxy.handle == 0 || proxy.parentHandle == 0
        || !proxy.m_hasProxyCarrierId || !proxy.m_hasProxyClassId
        || !proxy.m_hasProxyDrawingFormat
        || !proxy.m_hasObjectDataBitSize
        || proxy.m_objectDataBitSize
               > DRW::kMaxDxfBinaryPayloadBytes * 8u
        || expectedBytes(proxy.m_objectDataBitSize)
               != proxy.m_objectData.size()
        || (proxy.m_hasProxyGraphicsByteSize
            && (proxy.m_proxyGraphicsByteSize
                    != proxy.m_binaryData.size()
                || proxy.m_proxyGraphicsByteSize
                       > DRW::kMaxDxfBinaryPayloadBytes))
        || (!proxy.m_hasProxyGraphicsByteSize
            && !proxy.m_binaryData.empty())
        || (proxy.m_hasUnknownDataByteSize
            && (proxy.m_unknownDataByteSize
                    != proxy.m_unknownData.size()
                || proxy.m_unknownDataByteSize
                       > DRW::kMaxDxfBinaryPayloadBytes))
        || (!proxy.m_hasUnknownDataByteSize
            && !proxy.m_unknownData.empty())) {
        return false;
    }
    const std::uint32_t significantLastBits =
        proxy.m_objectDataBitSize & 7u;
    if (significantLastBits != 0u
        && (proxy.m_objectData.back()
            & ((1u << (8u - significantLastBits)) - 1u)) != 0u) {
        return false;
    }

    raw = DRW_RawDxfObject{};
    raw.name = "ACAD_PROXY_OBJECT";
    raw.handle = proxy.handle;
    raw.parentHandle = proxy.parentHandle;
    raw.groups.emplace_back(5, proxyHandleString(proxy.handle));
    raw.groups.emplace_back(330, proxyHandleString(proxy.parentHandle));
    if (!proxy.reactorHandles.empty()) {
        raw.groups.emplace_back(102, "{ACAD_REACTORS");
        for (std::uint32_t reactor : proxy.reactorHandles) {
            if (reactor == 0)
                return false;
            raw.groups.emplace_back(330, proxyHandleString(reactor));
        }
        raw.groups.emplace_back(102, "}");
    }
    if (proxy.xDictHandle != 0) {
        raw.groups.emplace_back(102, "{ACAD_XDICTIONARY");
        raw.groups.emplace_back(360, proxyHandleString(proxy.xDictHandle));
        raw.groups.emplace_back(102, "}");
    }
    raw.groups.emplace_back(100, "AcDbProxyObject");
    raw.groups.emplace_back(90, proxy.m_proxyCarrierId);
    // DXF proxy class IDs are the 500-based ordinal positions of CLASS
    // records in this output, not the source DWG's class IDs.
    raw.groups.emplace_back(91, outputClassId);
    if (proxy.m_hasProxyGraphicsByteSize) {
        if (proxy.m_proxyGraphicsByteSize
            > static_cast<std::uint64_t>(
                  std::numeric_limits<std::int32_t>::max())) {
            return false;
        }
        raw.groups.emplace_back(92, static_cast<std::int32_t>(
            proxy.m_proxyGraphicsByteSize));
        if (!appendProxyBinaryGroups(raw, 310, proxy.m_binaryData))
            return false;
    }
    if (proxy.m_proxyDrawingFormat
        > static_cast<std::uint32_t>(
              std::numeric_limits<std::int32_t>::max())) {
        return false;
    }
    raw.groups.emplace_back(95, static_cast<std::int32_t>(
        proxy.m_proxyDrawingFormat));
    raw.groups.emplace_back(70,
        proxy.m_hasFromDxf && proxy.m_fromDxf ? 1 : 0);
    raw.groups.emplace_back(161,
        static_cast<std::int64_t>(proxy.m_objectDataBitSize));
    if (!appendProxyBinaryGroups(raw, 310, proxy.m_objectData))
        return false;
    if (proxy.m_hasUnknownDataByteSize) {
        raw.groups.emplace_back(162, static_cast<std::int64_t>(
            proxy.m_unknownDataByteSize));
        if (!appendProxyBinaryGroups(raw, 310, proxy.m_unknownData))
            return false;
    }
    for (const DRW_ProxyObjectIdRef& reference : proxy.m_objectIdRefs) {
        if ((reference.m_dxfCode != 330 && reference.m_dxfCode != 340
             && reference.m_dxfCode != 350 && reference.m_dxfCode != 360)
            || reference.m_handle == 0) {
            return false;
        }
        raw.groups.emplace_back(reference.m_dxfCode,
                                proxyHandleString(reference.m_handle));
    }
    raw.groups.emplace_back(94, 0);
    return true;
}

bool collectAcdsHistoryProxyGraph(
    const dx_data& data, const DRW_ModelerGeometry& modeler,
    std::vector<DRW_RawDxfObject>& output,
    std::vector<std::uint32_t>& materialHandles, bool debug) {
    const auto reject = [&](const char* reason, std::uint32_t handle = 0,
                            std::size_t depth = 0) {
        if (debug) {
            std::cerr << "ACDS history graph rejected: modeler="
                      << proxyHandleString(modeler.handle)
                      << " history="
                      << proxyHandleString(modeler.m_historyHandle)
                      << " handle=" << proxyHandleString(handle)
                      << " depth=" << depth << " reason=" << reason << '\n';
        }
        return false;
    };
    if (modeler.m_historyHandle == 0 || modeler.handle == 0)
        return reject("missing modeler or history handle");

    // A DWG class number is assigned per drawing, not fixed by the object
    // type. Bind every proxy body to the independently parsed CLASSES record
    // instead of accepting a familiar numeric class-ID triple by coincidence.
    if (!data.dwgClassCoverage.m_complete
        || data.dwgClassCoverage.m_status
               != DRW_DwgClassCoverageStatus::FinalizedComplete) {
        return reject("source CLASSES coverage is incomplete");
    }

    struct ProxyClass {
        const char* recordName;
        const char* className;
        const char* subclass;
    };
    const auto proxyClassForSubclass = [](const std::string& subclass,
                                          ProxyClass& result) {
        if (subclass == "cn:AcDbShHistory") {
            result = {"ACSH_HISTORY_CLASS", "AcDbShHistory",
                      "cn:AcDbShHistory"};
            return true;
        }
        if (subclass == "cn:AcDbEvalGraph") {
            result = {"ACAD_EVALUATION_GRAPH", "AcDbEvalGraph",
                      "cn:AcDbEvalGraph"};
            return true;
        }
        if (subclass == "cn:AcDbShCone") {
            result = {"ACSH_CONE_CLASS", "AcDbShCone", "cn:AcDbShCone"};
            return true;
        }
        if (subclass == "cn:AcDbShBox") {
            result = {"ACSH_BOX_CLASS", "AcDbShBox", "cn:AcDbShBox"};
            return true;
        }
        if (subclass == "cn:AcDbShExtrusion") {
            result = {"ACSH_EXTRUSION_CLASS", "AcDbShExtrusion",
                      "cn:AcDbShExtrusion"};
            return true;
        }
        return false;
    };
    const auto sourceClassIdFor = [&data](const ProxyClass& proxyClass,
                                          std::int32_t& sourceClassId) {
        sourceClassId = 0;
        for (const DRW_DwgClassCoverageEntry& entry :
             data.dwgClassCoverage.m_entries) {
            if (entry.m_recordName != proxyClass.recordName
                || entry.m_className != proxyClass.className) {
                continue;
            }
            if (sourceClassId != 0
                || entry.m_state != DRW_DwgClassCoverageState::Published
                || entry.m_entityFlagRaw != 0x1F3
                || entry.m_classNumber < 500) {
                return false;
            }
            sourceClassId = entry.m_classNumber;
        }
        return sourceClassId != 0;
    };

    const std::size_t outputStart = output.size();
    ProxyClass expectedClasses[3] = {
        {"ACSH_HISTORY_CLASS", "AcDbShHistory", "cn:AcDbShHistory"},
        {"ACAD_EVALUATION_GRAPH", "AcDbEvalGraph", "cn:AcDbEvalGraph"},
        {nullptr, nullptr, nullptr}};
    std::int32_t sourceClassIds[3] = {0, 0, 0};
    if (!sourceClassIdFor(expectedClasses[0], sourceClassIds[0])
        || !sourceClassIdFor(expectedClasses[1], sourceClassIds[1])) {
        return reject("history/evaluation source class metadata is unqualified");
    }

    std::unordered_map<std::uint32_t, std::vector<const DRW_ProxyObject*>> byHandle;
    for (const DRW_ProxyObject& proxy : data.proxyObjects) {
        if (proxy.handle == 0
            || !byHandle.emplace(proxy.handle,
                                 std::vector<const DRW_ProxyObject*>{}).second) {
            return reject("duplicate or zero source proxy handle", proxy.handle);
        }
        byHandle[proxy.handle].push_back(&proxy);
    }

    std::vector<std::pair<std::uint32_t, std::uint32_t>> pending;
    std::unordered_map<std::uint32_t, std::uint32_t> expectedOwner;
    std::unordered_set<std::uint32_t> visited;
    pending.emplace_back(modeler.m_historyHandle, modeler.handle);
    try {
        while (!pending.empty()) {
            if (output.size() - outputStart >= 3u)
                return reject("history closure exceeds three objects",
                              pending.back().first, output.size() - outputStart);
            const auto current = pending.back();
            pending.pop_back();
            const std::uint32_t handle = current.first;
            const auto priorOwner = expectedOwner.emplace(handle, current.second);
            if (!priorOwner.second && priorOwner.first->second != current.second)
                return reject("proxy has conflicting expected owners", handle,
                              output.size() - outputStart);
            if (!visited.insert(handle).second)
                continue;

            const auto found = byHandle.find(handle);
            if (found == byHandle.end() || found->second.size() != 1u
                || found->second.front()->parentHandle != current.second) {
                if (debug && found != byHandle.end()) {
                    std::cerr << "  proxy candidates for "
                              << proxyHandleString(handle) << " count="
                              << found->second.size() << " expectedOwner="
                              << proxyHandleString(current.second);
                    for (const DRW_ProxyObject* candidate : found->second) {
                        std::cerr << " [owner="
                                  << proxyHandleString(candidate->parentHandle)
                                  << " class=" << candidate->m_proxyClassId
                                  << " subclass=" << candidate->m_proxySubclass
                                  << " refs=" << candidate->m_objectIdRefs.size();
                        for (const DRW_ProxyObjectIdRef& ref :
                             candidate->m_objectIdRefs) {
                            std::cerr << ',' << ref.m_dxfCode << ':'
                                      << proxyHandleString(ref.m_handle);
                        }
                        std::cerr << ']';
                    }
                    std::cerr << '\n';
                }
                return reject("proxy is missing, duplicated, or has wrong owner",
                              handle, output.size() - outputStart);
            }
            const DRW_ProxyObject& proxy = *found->second.front();
            const std::size_t depth = output.size() - outputStart;
            if (depth == 2u
                && !proxyClassForSubclass(proxy.m_proxySubclass,
                                          expectedClasses[2])) {
                return reject("terminal history class is unsupported", handle,
                              depth);
            }
            if (depth >= 3u
                || !proxy.m_hasProxyClassId
                || !sourceClassIdFor(expectedClasses[depth],
                                     sourceClassIds[depth])
                || (depth == 2u
                    && (sourceClassIds[2] == sourceClassIds[0]
                        || sourceClassIds[2] == sourceClassIds[1]))
                || proxy.m_proxyClassId != sourceClassIds[depth]
                || proxy.m_proxySubclass != expectedClasses[depth].subclass
                || proxy.m_objectIdRefs.size() != 1u) {
                return reject("proxy identity, ordinal, or reference shape is unsupported",
                              handle, depth);
            }

            DRW_RawDxfObject raw;
            if (!makeRawProxyObject(proxy, proxy.m_proxyClassId, raw))
                return reject("proxy object fields cannot be re-emitted",
                              handle, depth);
            output.push_back(std::move(raw));

            const DRW_ProxyObjectIdRef& reference =
                proxy.m_objectIdRefs.front();
            if (reference.m_handle == 0
                || reference.m_handle
                       > std::numeric_limits<std::uint32_t>::max()) {
                return reject("history object reference is invalid", handle,
                              depth);
            }
            const std::uint32_t target =
                static_cast<std::uint32_t>(reference.m_handle);
            if (depth < 2u) {
                if (reference.m_dxfCode != 360
                    || byHandle.find(target) == byHandle.end()) {
                    return reject("history link does not target a proxy object",
                                  handle, depth);
                }
                pending.emplace_back(target, handle);
                continue;
            }

            if (reference.m_dxfCode != 340
                || byHandle.find(target) != byHandle.end())
                return reject("terminal link is not a material reference",
                              handle, depth);
            std::size_t materialMatches = 0;
            for (const DRW_Material& material : data.materials) {
                if (material.handle == target)
                    ++materialMatches;
            }
            if (materialMatches != 1u)
                return reject("terminal material target is missing or ambiguous",
                              handle, depth);
            if (std::find(materialHandles.begin(), materialHandles.end(),
                          target) == materialHandles.end()) {
                materialHandles.push_back(target);
            }
        }
    } catch (...) {
        return reject("exception while validating proxy graph");
    }
    if (output.size() - outputStart != 3u)
        return reject("proxy graph has an incomplete closure",
                      modeler.m_historyHandle, output.size() - outputStart);
    return true;
}

bool configureAcdsMaterialDictionary(
    const dx_data& data, const std::vector<std::uint32_t>& referencedMaterials,
    std::vector<DRW_Dictionary>& namedDictionaries,
    std::vector<std::pair<std::string, std::string>>& rootEntries,
    std::vector<std::uint32_t>& materialHandles) {
    if (referencedMaterials.empty())
        return true;
    std::unordered_set<std::uint32_t> required(referencedMaterials.begin(),
                                                referencedMaterials.end());
    const DRW_Dictionary* materialDictionary = nullptr;
    for (const DRW_Dictionary& dictionary : data.dictionaries) {
        // R2010+ DWG readers do not publish the implicit NamedObjectsDictionary
        // body (handle C). Reconstruct only ACAD_MATERIAL when one complete
        // root-owned dictionary has entries that all resolve uniquely to
        // typed MATERIAL objects and includes every history reference.
        if (dictionary.handle == 0 || dictionary.handle == 0xCu
            || dictionary.handle == 0xDu || dictionary.parentHandle != 0xCu
            || !dictionary.hasCompleteDwgEntries()
            || dictionary.m_entries.empty()) {
            continue;
        }

        std::unordered_set<std::uint32_t> entries;
        bool matchesMaterials = true;
        for (const DRW_Dictionary::Entry& entry : dictionary.m_entries) {
            if (entry.m_handle == 0 || !entries.insert(entry.m_handle).second) {
                matchesMaterials = false;
                break;
            }
            std::size_t matches = 0;
            for (const DRW_Material& material : data.materials) {
                if (material.handle == entry.m_handle)
                    ++matches;
            }
            if (matches != 1u) {
                matchesMaterials = false;
                break;
            }
        }
        if (!matchesMaterials
            || !std::all_of(required.begin(), required.end(),
                            [&entries](std::uint32_t handle) {
                                return entries.count(handle) != 0;
                            })) {
            continue;
        }
        if (materialDictionary != nullptr)
            return false;
        materialDictionary = &dictionary;
    }
    if (materialDictionary == nullptr)
        return false;

    for (const DRW_Dictionary::Entry& entry : materialDictionary->m_entries)
        materialHandles.push_back(entry.m_handle);
    namedDictionaries.push_back(*materialDictionary);
    rootEntries.emplace_back("ACAD_MATERIAL",
                             proxyHandleString(materialDictionary->handle));
    return true;
}

bool appendAcdsDataSection(
    const dx_data& data, DRW::Version outputVersion,
    std::vector<DRW_RawDxfSection>& sections, bool debug) {
    bool hasExistingAcdsData = false;
    for (const DRW_RawDxfSection& section : sections) {
        if (section.m_name == "ACDSDATA"
            || section.m_name == "acdsdata") {
            hasExistingAcdsData = true;
            break;
        }
    }

    std::vector<const DRW_ModelerGeometry*> linkedModelers;
    std::unordered_set<std::uint32_t> linkedHandles;
    const auto inspectBlock = [&](const dx_ifaceBlock* block) {
        if (block == nullptr)
            return true;
        for (const DRW_Entity* entity : block->ent) {
            if (entity == nullptr)
                continue;
            const bool carriesDataStorage = entity->hasDataStorageRecord
                || entity->hasDataStorageBinaryData()
                || !entity->dataStorageData.empty();
            if (!carriesDataStorage)
                continue;
            if (entity->eType != DRW::E3DSOLID
                && entity->eType != DRW::REGION) {
                return false;
            }
            const auto* modeler =
                static_cast<const DRW_ModelerGeometry*>(entity);
            if (modeler->handle == 0
                || !linkedHandles.insert(modeler->handle).second) {
                return false;
            }
            linkedModelers.push_back(modeler);
        }
        return true;
    };

    if (!inspectBlock(data.mBlock))
        return false;
    for (const dx_ifaceBlock* block : data.blocks) {
        if (block == data.mBlock)
            continue;
        if (!inspectBlock(block))
            return false;
    }

    if (data.dataStorageSections.empty())
        return linkedModelers.empty();
    if (data.dataStorageSections.size() != 1u)
        return false;
    const DRW_DataStorageSection& storage = data.dataStorageSections.front();
    if (storage.records.empty() && linkedModelers.empty())
        return true;
    AcdsOutputSchemaRoles schemaRoles;
    const bool schemasQualified =
        matchesQualifiedAc1027AcdsSchemas(storage, schemaRoles);
    if (debug && !schemasQualified) {
        std::cerr << "ACDS schema fingerprint details: declared="
                  << storage.schemaCount << " decoded=" << storage.schemas.size()
                  << " propertyNames=" << storage.schemaPropertyNameCount
                  << '/' << storage.schemaPropertyNames.size() << '\n';
        for (std::size_t index = 0;
             index < storage.schemaPropertyNames.size(); ++index) {
            std::cerr << "  property-name[" << index << "]="
                      << storage.schemaPropertyNames[index] << '\n';
        }
        for (const DRW_DataStorageSchema& schema : storage.schemas) {
            std::cerr << "  schema[" << schema.index << "] segment="
                      << schema.segmentIndex << " indexes=";
            for (std::uint64_t value : schema.indexes)
                std::cerr << value << ',';
            std::cerr << " properties=";
            for (const DRW_DataStorageSchemaProperty& property :
                 schema.properties) {
                std::cerr << '[' << property.name << " index="
                          << property.nameIndex << " type=" << property.type
                          << " flags=" << property.flags << " unknown="
                          << property.unknown1 << '/' << property.unknown2
                          << " count=" << property.valueCount << " values=";
                for (const std::vector<std::uint8_t>& value : property.values) {
                    std::cerr << '{';
                    for (std::uint8_t byte : value)
                        std::cerr << static_cast<unsigned>(byte) << ',';
                    std::cerr << '}';
                }
                std::cerr << ']';
            }
            std::cerr << '\n';
        }
        for (const DRW_DataStorageRecord& record : storage.records) {
            std::cerr << "  record handle=" << record.handleKey
                      << " schemaIndex=" << record.schemaIndex
                      << " bytes=" << record.payload.size()
                      << " marker=" << record.hasPayloadMarker
                      << " markerSection=" << record.payloadMarkerSection
                      << '\n';
        }
    }
    if (hasExistingAcdsData || outputVersion != DRW::AC1027
        || storage.m_name != "AcDb:AcDsPrototype_1b"
        || storage.m_version != DRW::AC1027 || storage.parseFailed
        || !storage.structurallyValid || !storage.replayAllowed
        || !storage.payloadsRetained
        || !schemasQualified
        || !storage.duplicateRecordHandleKeys.empty()
        || storage.records.size() < linkedModelers.size()) {
        if (debug) {
            std::cerr << "ACDS projection rejected: output="
                      << static_cast<int>(outputVersion)
                      << " existingSection=" << hasExistingAcdsData
                      << " storageName=" << storage.m_name
                      << " storageVersion="
                      << static_cast<int>(storage.m_version)
                      << " parsed=" << !storage.parseFailed
                      << " structurallyValid=" << storage.structurallyValid
                      << " replayAllowed=" << storage.replayAllowed
                      << " payloadsRetained=" << storage.payloadsRetained
                      << " schemasQualified=" << schemasQualified
                      << " duplicateKeys="
                      << storage.duplicateRecordHandleKeys.size()
                      << " records=" << storage.records.size()
                      << " linkedModelers=" << linkedModelers.size()
                      << '\n';
        }
        return false;
    }

    std::unordered_map<std::uint32_t, const DRW_ModelerGeometry*> byHandle;
    for (const DRW_ModelerGeometry* modeler : linkedModelers) {
        if (modeler == nullptr || !modeler->hasDataStorageBinaryData()
            || !modeler->hasDataStorageRecord || modeler->m_isEmpty
            || !modeler->m_hasModelerData
            || !modeler->m_dwgAcisPayload.empty()
            || modeler->dataStorageHandle != modeler->handle
            || modeler->dataStorageHandleKey.empty()
            || modeler->dataStorageData.empty()
            || modeler->dataStorageData.size()
                   > DRW_DataStorageConst::PAYLOAD_BLOB_SECTION_CAP
            || !hasSabSignature(modeler->dataStorageData)
            || !byHandle.emplace(modeler->handle, modeler).second) {
            return false;
        }
    }

    std::unordered_set<std::string> recordKeys;
    std::unordered_set<std::uint32_t> matchedModelers;
    std::size_t thumbnailCount = 0;
    for (const DRW_DataStorageRecord& record : storage.records) {
        if (!record.isHandleSafe || record.isBlobReference
            || record.handle == 0 || record.handleKey.empty()
            || !recordKeys.insert(record.handleKey).second
            || record.payload.empty()
            || record.payload.size() > DRW_DataStorageConst::PAYLOAD_BLOB_SECTION_CAP
            || record.dataByteLength != record.payload.size()) {
            return false;
        }
        if (record.schemaIndex == schemaRoles.thumbnailSchemaIndex
            && !record.hasPayloadMarker
            && hasPngSignature(record.payload)) {
            // DataStorage schema 0 is the independently verified
            // Thumbnail_Data property. Preserve layout thumbnails as
            // unowned ACDSDATA records; their handles do not name modelers.
            ++thumbnailCount;
            continue;
        }
        if (record.schemaIndex != schemaRoles.modelerSchemaIndex)
            return false;
        if (!record.hasPayloadMarker
            || record.payloadMarkerLength == 0u
            || record.payloadMarkerOffset > record.payload.size()
            || record.payloadMarkerLength
                   > record.payload.size() - record.payloadMarkerOffset) {
            return false;
        }
        if (record.handle > std::numeric_limits<std::uint32_t>::max())
            return false;
        const auto linked = byHandle.find(static_cast<std::uint32_t>(record.handle));
        if (linked == byHandle.end()
            || linked->second->dataStorageHandleKey != record.handleKey
            || linked->second->dataStorageData != record.payload
            || !hasSabSignature(record.payload)
            || !matchedModelers.insert(linked->first).second) {
            return false;
        }
    }
    if (matchedModelers.size() != linkedModelers.size()
        || storage.orphanRecordCount != thumbnailCount
        || storage.records.size() != linkedModelers.size() + thumbnailCount)
        return false;

    DRW_RawDxfSection section;
    section.m_name = "ACDSDATA";
    section.m_version = DRW::AC1027;
    section.m_groups.emplace_back(70, 2);
    section.m_groups.emplace_back(71, 2);

    addAcdsSchemaHeader(section, 0, "AcDb_Thumbnail_Schema",
                        "Thumbnail_Data");
    addAcdsSchemaAttributes(section, 0);
    addAcdsSchemaHeader(section, 1, "AcDb3DSolid_ASM_Data", "ASM_Data");
    addAcdsSchemaAttributes(section, 1);
    addAcdsMarkerSchema(section, 2,
                        "AcDbDs::TreatedAsObjectDataSchema",
                        "AcDbDs::TreatedAsObjectData");
    addAcdsMarkerSchema(section, 3, "AcDbDs::LegacySchema",
                        "AcDbDs::Legacy");
    addAcdsMarkerSchema(section, 4, "AcDbDs::IndexedPropertySchema",
                        "AcDs:Indexable");
    section.m_groups.emplace_back(0, "ACDSSCHEMA");
    section.m_groups.emplace_back(90, 5);
    section.m_groups.emplace_back(1, "AcDbDs::HandleAttributeSchema");
    section.m_groups.emplace_back(2, "AcDbDs::HandleAttribute");
    section.m_groups.emplace_back(280, 7);
    section.m_groups.emplace_back(91, 1);
    section.m_groups.emplace_back(284, 1);

    static constexpr char hexDigits[] = "0123456789ABCDEF";
    constexpr std::size_t bytesPerChunk = 127u;
    for (const DRW_DataStorageRecord& record : storage.records) {
        if (record.payload.size() > static_cast<std::size_t>(
                std::numeric_limits<std::int32_t>::max())) {
            return false;
        }
        section.m_groups.emplace_back(0, "ACDSRECORD");
        const bool isThumbnail =
            record.schemaIndex == schemaRoles.thumbnailSchemaIndex
            && !record.hasPayloadMarker;
        section.m_groups.emplace_back(90, isThumbnail ? 0 : 1);
        section.m_groups.emplace_back(2, "AcDbDs::ID");
        section.m_groups.emplace_back(280, 10);
        section.m_groups.emplace_back(320, record.handleKey);
        section.m_groups.emplace_back(2,
            isThumbnail ? "Thumbnail_Data" : "ASM_Data");
        section.m_groups.emplace_back(280, 15);
        section.m_groups.emplace_back(94,
            static_cast<std::int32_t>(record.payload.size()));
        for (std::size_t offset = 0; offset < record.payload.size();
             offset += bytesPerChunk) {
            const std::size_t count = std::min(
                bytesPerChunk, record.payload.size() - offset);
            std::string hex;
            hex.reserve(count * 2u);
            for (std::size_t index = 0; index < count; ++index) {
                const std::uint8_t value = record.payload[offset + index];
                hex.push_back(hexDigits[value >> 4u]);
                hex.push_back(hexDigits[value & 0x0fu]);
            }
            section.m_groups.emplace_back(310, std::move(hex));
        }
    }
    sections.push_back(std::move(section));
    return true;
}

} // namespace

bool dx_iface::collectAcdsHistoryProxyObjects(
    const dx_data& data, std::vector<DRW_RawDxfObject>& objects,
    std::vector<std::uint32_t>& materialHandles, bool debug) {
    if (data.dataStorageSections.empty())
        return true;
    std::vector<const DRW_ModelerGeometry*> linkedModelers;
    const auto inspect = [&](const dx_ifaceBlock* block) {
        if (block == nullptr)
            return true;
        for (const DRW_Entity* entity : block->ent) {
            if (entity == nullptr
                || (!entity->hasDataStorageRecord
                    && !entity->hasDataStorageBinaryData()
                    && entity->dataStorageData.empty())) {
                continue;
            }
            if (entity->eType != DRW::E3DSOLID
                && entity->eType != DRW::REGION) {
                return false;
            }
            linkedModelers.push_back(
                static_cast<const DRW_ModelerGeometry*>(entity));
        }
        return true;
    };
    if (!inspect(data.mBlock))
        return false;
    for (const dx_ifaceBlock* block : data.blocks) {
        if (block != data.mBlock && !inspect(block))
            return false;
    }

    const std::size_t startingObjectCount = objects.size();
    std::unordered_set<std::uint32_t> modelerHandles;
    for (const DRW_ModelerGeometry* modeler : linkedModelers) {
        if (modeler == nullptr || modeler->handle == 0
            || !modelerHandles.insert(modeler->handle).second) {
            return false;
        }
        if (modeler->m_historyHandle == 0) {
            // The independently witnessed REGION has no history reference.
            // Do not synthesize an operation graph for that entity.
            if (modeler->eType != DRW::REGION)
                return false;
            continue;
        }
        if (modeler->eType != DRW::E3DSOLID
            || !collectAcdsHistoryProxyGraph(data, *modeler, objects,
                                             materialHandles, debug)) {
            return false;
        }
    }

    // This converter slice only emits proxy records that participate in the
    // witnessed modeler closures. Refuse to silently drop any other proxy.
    return objects.size() - startingObjectCount == data.proxyObjects.size();
}

bool dx_iface::collectAcdsTypedBoxHistoryObjects(
    const dx_data& data, std::vector<DRW_RawDxfObject>& objects,
    std::vector<DRW_EvaluationGraph>& graphs,
    std::vector<std::uint32_t>& materialHandles, bool debug) {
    const auto reject = [&](const char* reason, std::uint32_t handle = 0) {
        if (debug) {
            std::cerr << "ACDS typed BOX closure rejected: handle="
                      << proxyHandleString(handle) << " reason=" << reason
                      << '\n';
        }
        return false;
    };
    if (data.dataStorageSections.empty() || !data.proxyObjects.empty()
        || !data.rawProxyObjects.empty()) {
        return reject("typed lane requires DataStorage and no proxy records");
    }
    if (!data.dwgClassCoverage.m_complete
        || data.dwgClassCoverage.m_status
               != DRW_DwgClassCoverageStatus::FinalizedComplete) {
        return reject("source CLASSES coverage is incomplete");
    }

    const auto hasOnePublishedClass = [&data](const char* recordName,
                                               const char* className) {
        std::size_t matches = 0;
        std::uint16_t classNumber = 0;
        for (const DRW_DwgClassCoverageEntry& entry :
             data.dwgClassCoverage.m_entries) {
            if (entry.m_recordName != recordName
                || entry.m_className != className) {
                continue;
            }
            ++matches;
            classNumber = entry.m_classNumber;
            if (entry.m_state != DRW_DwgClassCoverageState::Published
                || entry.m_classNumber < 500u
                || entry.m_entityFlagRaw != 0x1F3u) {
                return false;
            }
        }
        return matches == 1u && classNumber >= 500u;
    };
    if (!hasOnePublishedClass("ACSH_HISTORY_CLASS", "AcDbShHistory")
        || !hasOnePublishedClass("ACAD_EVALUATION_GRAPH", "AcDbEvalGraph")
        || !hasOnePublishedClass("ACSH_BOX_CLASS", "AcDbShBox")) {
        return reject("required source custom-class metadata is not unique");
    }

    const auto hasCompletePrefixes = [](
        const DRW_AcShHistoryObject& object,
        std::initializer_list<DRW_AssociativePrefixStatus::Kind> kinds) {
        if (object.m_prefixStatuses.size() != kinds.size())
            return false;
        auto expected = kinds.begin();
        for (const DRW_AssociativePrefixStatus& status :
             object.m_prefixStatuses) {
            if (expected == kinds.end() || status.m_kind != *expected
                || status.m_status
                       != DRW_AssociativePrefixStatus::ParseStatus::Complete) {
                return false;
            }
            ++expected;
        }
        return expected == kinds.end();
    };
    const auto commonLinksAreEmpty = [](const DRW_TableEntry& object) {
        return object.extData.empty() && object.appData.empty()
            && object.reactorHandles.empty() && object.xDictHandle == 0
            && object.extensionDictionaryFlag() == 0;
    };

    std::unordered_map<std::uint32_t, const DRW_AcShHistoryObject*> historyByHandle;
    for (const DRW_AcShHistoryObject& object : data.acshHistoryObjects) {
        if (object.handle == 0
            || !historyByHandle.emplace(object.handle, &object).second) {
            return reject("duplicate or zero typed ACSH handle", object.handle);
        }
    }
    std::unordered_map<std::uint32_t, const DRW_EvaluationGraph*> graphByHandle;
    for (const DRW_EvaluationGraph& graph : data.evaluationGraphs) {
        if (graph.handle == 0
            || !graphByHandle.emplace(graph.handle, &graph).second) {
            return reject("duplicate or zero EvaluationGraph handle",
                          graph.handle);
        }
    }

    std::vector<const DRW_ModelerGeometry*> modelers;
    const auto inspect = [&modelers](const dx_ifaceBlock* block) {
        if (block == nullptr)
            return true;
        for (const DRW_Entity* entity : block->ent) {
            if (entity == nullptr
                || (!entity->hasDataStorageRecord
                    && !entity->hasDataStorageBinaryData()
                    && entity->dataStorageData.empty())) {
                continue;
            }
            if (entity->eType != DRW::E3DSOLID
                && entity->eType != DRW::REGION) {
                return false;
            }
            modelers.push_back(
                static_cast<const DRW_ModelerGeometry*>(entity));
        }
        return true;
    };
    if (!inspect(data.mBlock))
        return reject("DataStorage owner is not a modeler entity");
    for (const dx_ifaceBlock* block : data.blocks) {
        if (block != data.mBlock && !inspect(block))
            return reject("DataStorage owner is not a modeler entity");
    }

    std::vector<DRW_RawDxfObject> typedObjects;
    std::vector<DRW_EvaluationGraph> typedGraphs;
    std::vector<std::uint32_t> typedMaterials;
    std::unordered_set<std::uint32_t> modelerHandles;
    std::unordered_set<std::uint32_t> usedHistoryHandles;
    std::unordered_set<std::uint32_t> usedGraphHandles;
    std::unordered_set<std::uint32_t> usedShapeHandles;

    const auto addString = [](DRW_RawDxfObject& object, int code,
                              const std::string& value) {
        object.groups.emplace_back(code, value);
    };
    const auto addInt = [](DRW_RawDxfObject& object, int code,
                           std::int32_t value) {
        object.groups.emplace_back(code, value);
    };
    const auto buildHistoryObject = [&](const DRW_AcShHistoryObject& history,
                                        DRW_RawDxfObject& output) {
        output.name = "ACSH_HISTORY_CLASS";
        output.handle = history.handle;
        output.parentHandle = history.parentHandle;
        addString(output, 5, proxyHandleString(history.handle));
        addString(output, 330, proxyHandleString(history.parentHandle));
        addString(output, 100, "AcDbShHistory");
        addInt(output, 90, history.m_major);
        addInt(output, 91, history.m_minor);
        addString(output, 360, proxyHandleString(history.m_ownerHandle));
        addInt(output, 92, history.m_historyNodeId);
        addInt(output, 280, history.m_showHistory ? 1 : 0);
        addInt(output, 281, history.m_recordHistory ? 1 : 0);
        return true;
    };
    const auto buildBoxObject = [&](const DRW_AcShHistoryObject& box,
                                    DRW_RawDxfObject& output) {
        output.name = "ACSH_BOX_CLASS";
        output.handle = box.handle;
        output.parentHandle = box.parentHandle;
        addString(output, 5, proxyHandleString(box.handle));
        addString(output, 330, proxyHandleString(box.parentHandle));
        addString(output, 100, "AcDbEvalExpr");
        addInt(output, 90, box.m_evalExprPrefix.m_id);
        addInt(output, 98, box.m_evalExprPrefix.m_value98);
        addInt(output, 99, box.m_evalExprPrefix.m_value99);
        addString(output, 100, "AcDbShHistoryNode");
        addInt(output, 90, box.m_historyNodePrefix.m_major);
        addInt(output, 91, box.m_historyNodePrefix.m_minor);
        int code = 40;
        for (double value : box.m_historyNodePrefix.m_transform)
            output.groups.emplace_back(code++, value);
        addInt(output, 62, static_cast<std::int32_t>(
            box.m_historyNodePrefix.m_colorIndex));
        addInt(output, 92, box.m_historyNodePrefix.m_nodeValue);
        addString(output, 347,
                  proxyHandleString(box.m_historyNodePrefix.m_handle));
        addString(output, 100, "AcDbShPrimitive");
        addString(output, 100, "AcDbShBox");
        addInt(output, 90, box.m_major);
        addInt(output, 91, box.m_minor);
        for (std::size_t index = 0; index < box.m_shapeParams.size(); ++index)
            output.groups.emplace_back(40 + static_cast<int>(index),
                                       box.m_shapeParams[index]);
        return true;
    };

    for (const DRW_ModelerGeometry* modeler : modelers) {
        if (modeler == nullptr || modeler->handle == 0
            || !modelerHandles.insert(modeler->handle).second) {
            return reject("duplicate or zero DataStorage modeler handle");
        }
        if (modeler->m_historyHandle == 0) {
            if (modeler->eType != DRW::REGION)
                return reject("3DSOLID has no history owner", modeler->handle);
            continue;
        }
        if (modeler->eType != DRW::E3DSOLID)
            return reject("only 3DSOLID may own the BOX history profile",
                          modeler->handle);
        const auto historyIt = historyByHandle.find(modeler->m_historyHandle);
        if (historyIt == historyByHandle.end())
            return reject("modeler history handle is not typed",
                          modeler->m_historyHandle);
        const DRW_AcShHistoryObject& history = *historyIt->second;
        if (history.m_recordName != "ACSH_HISTORY_CLASS"
            || history.parentHandle != modeler->handle
            || history.m_ownerHandle == 0
            || history.m_major > static_cast<std::uint32_t>(
                   std::numeric_limits<std::int32_t>::max())
            || history.m_minor > static_cast<std::uint32_t>(
                   std::numeric_limits<std::int32_t>::max())
            || history.m_historyNodeId > static_cast<std::uint32_t>(
                   std::numeric_limits<std::int32_t>::max())
            || !commonLinksAreEmpty(history)
            || !hasCompletePrefixes(history, {
                DRW_AssociativePrefixStatus::Kind::AcDbShHistoryNode})) {
            return reject("history object owner or body is incomplete",
                          history.handle);
        }
        const auto graphIt = graphByHandle.find(history.m_ownerHandle);
        if (graphIt == graphByHandle.end())
            return reject("history graph handle is not typed",
                          history.m_ownerHandle);
        const DRW_EvaluationGraph& graph = *graphIt->second;
        if (graph.parentHandle != history.handle || graph.m_value96 != 1
            || graph.m_value97 != 1 || graph.m_nodes.size() != 1u
            || !graph.m_edges.empty() || !commonLinksAreEmpty(graph)) {
            return reject("EvaluationGraph shape is outside the BOX profile",
                          graph.handle);
        }
        const DRW_EvaluationGraphNode& node = graph.m_nodes.front();
        if (node.m_index != 0 || node.m_flags != 32
            || node.m_nextNodeIndex != 1 || node.m_data1 != -1
            || node.m_data2 != -1 || node.m_data3 != -1
            || node.m_data4 != -1 || node.m_expressionHandle == 0) {
            return reject("EvaluationGraph node is outside the BOX profile",
                          graph.handle);
        }
        const auto boxIt = historyByHandle.find(node.m_expressionHandle);
        if (boxIt == historyByHandle.end())
            return reject("graph expression handle is not typed",
                          node.m_expressionHandle);
        const DRW_AcShHistoryObject& box = *boxIt->second;
        if (box.m_recordName != "ACSH_BOX_CLASS"
            || box.parentHandle != graph.handle
            || !commonLinksAreEmpty(box)
            || !hasCompletePrefixes(box, {
                DRW_AssociativePrefixStatus::Kind::AcDbEvalExpr,
                DRW_AssociativePrefixStatus::Kind::AcDbShHistoryNode,
                DRW_AssociativePrefixStatus::Kind::AcShActionBody})
            || !box.m_evalExprPrefix.m_complete
            || box.m_evalExprPrefix.m_hasEvaluatedValue
            || box.m_evalExprPrefix.m_valueCode > 0
            || box.m_evalExprPrefix.m_id <= 0
            || !box.m_historyNodePrefix.m_complete
            || box.m_historyNodePrefix.m_major < 0
            || box.m_historyNodePrefix.m_minor < 0
            || static_cast<std::uint32_t>(
                   box.m_historyNodePrefix.m_major) != history.m_major
            || static_cast<std::uint32_t>(
                   box.m_historyNodePrefix.m_minor) != history.m_minor
            || box.m_historyNodePrefix.m_nodeValue != 3
            || box.m_historyNodePrefix.m_handle == 0
            || box.m_historyNodePrefix.m_hasRgbColor
            || !box.m_historyNodePrefix.m_colorName.empty()
            || !box.m_historyNodePrefix.m_colorBookName.empty()
            || box.m_major != static_cast<std::uint32_t>(
                   box.m_historyNodePrefix.m_major)
            || box.m_minor != static_cast<std::uint32_t>(
                   box.m_historyNodePrefix.m_minor)
            || box.m_shapeParams.size() != 3u
            || std::any_of(box.m_shapeParams.begin(), box.m_shapeParams.end(),
                           [](double value) {
                               return !std::isfinite(value) || value <= 0.0;
                           })
            || std::any_of(box.m_historyNodePrefix.m_transform.begin(),
                           box.m_historyNodePrefix.m_transform.end(),
                           [](double value) {
                               return !std::isfinite(value);
                           })) {
            return reject("BOX class body or prefix is incomplete",
                          box.handle);
        }
        std::size_t materialMatches = 0;
        for (const DRW_Material& material : data.materials) {
            if (material.handle == box.m_historyNodePrefix.m_handle)
                ++materialMatches;
        }
        if (materialMatches != 1u)
            return reject("BOX material link is missing or ambiguous",
                          box.handle);

        if (!usedHistoryHandles.insert(history.handle).second
            || !usedGraphHandles.insert(graph.handle).second
            || !usedShapeHandles.insert(box.handle).second) {
            return reject("history closure is shared or ambiguous",
                          modeler->handle);
        }
        DRW_RawDxfObject historyOutput;
        DRW_RawDxfObject boxOutput;
        buildHistoryObject(history, historyOutput);
        buildBoxObject(box, boxOutput);
        typedObjects.push_back(std::move(historyOutput));
        typedObjects.push_back(std::move(boxOutput));
        typedGraphs.push_back(graph);
        if (std::find(typedMaterials.begin(), typedMaterials.end(),
                      box.m_historyNodePrefix.m_handle) == typedMaterials.end()) {
            typedMaterials.push_back(box.m_historyNodePrefix.m_handle);
        }
    }

    if (usedHistoryHandles.size() * 2u != data.acshHistoryObjects.size()
        || usedGraphHandles.size() != data.evaluationGraphs.size()
        || usedHistoryHandles.empty()) {
        return reject("unmatched ACSH objects or EvaluationGraphs remain");
    }
    objects.swap(typedObjects);
    graphs.swap(typedGraphs);
    materialHandles.insert(materialHandles.end(), typedMaterials.begin(),
                           typedMaterials.end());
    return true;
}

bool dx_iface::prepareExtensionObjectGraph(dx_data* data,
                                           bool includeNamedObjectRoot) {
    m_extensionDictionaryHandles.clear();
    m_extensionXRecordHandles.clear();
    m_extensionVisualStyleHandles.clear();
    m_extensionGraphStats = {};
    m_extensionGraphFailure = ExtensionGraphFailure::None;
    const auto reject = [this](ExtensionGraphFailure reason = ExtensionGraphFailure::InvalidGraph) {
        // No partial traversal result may leak into a later export attempt.
        m_extensionDictionaryHandles.clear();
        m_extensionXRecordHandles.clear();
        m_extensionVisualStyleHandles.clear();
        if (m_extensionGraphFailure == ExtensionGraphFailure::None)
            m_extensionGraphFailure = reason;
        return false;
    };
    const auto examineEdge = [this, &reject]() {
        if (m_extensionGraphStats.examinedEdges >= m_extensionGraphLimits.maxEdges)
            return reject(ExtensionGraphFailure::EdgeLimit);
        ++m_extensionGraphStats.examinedEdges;
        return true;
    };
    const auto examineSourceRecord = [this, &reject]() {
        if (m_extensionGraphStats.sourceRecords >= m_extensionGraphLimits.maxSourceRecords)
            return reject(ExtensionGraphFailure::SourceRecordLimit);
        ++m_extensionGraphStats.sourceRecords;
        return true;
    };
    if (data == nullptr)
        return reject();

    try {
        std::unordered_map<std::uint32_t, const DRW_Dictionary*> dictionaries;
        std::unordered_map<std::uint32_t, const DRW_XRecord*> xrecords;
        std::unordered_map<std::uint32_t, const DRW_VisualStyle*> visualStyles;
        std::unordered_map<std::uint32_t, const DRW_RawDxfObject*> rawVisualStyles;
        std::unordered_set<std::uint32_t> sourceHandles;
        std::unordered_set<std::uint32_t> linetypeHandles;
        if (includeNamedObjectRoot && data->hasNamedObjectDictionary) {
            const std::uint32_t rootHandle =
                data->namedObjectDictionary.handle;
            if (rootHandle == 0 || rootHandle == DRW::NoHandle
                || !sourceHandles.insert(rootHandle).second)
                return reject();
        }
        for (const DRW_LType& lineType : data->lineTypes) {
            if (!examineSourceRecord())
                return false;
            if (lineType.handle == 0)
                continue;
            if (!sourceHandles.insert(lineType.handle).second)
                return reject();
            linetypeHandles.insert(lineType.handle);
        }
        for (const DRW_Dictionary& dictionary : data->dictionaries) {
            if (!examineSourceRecord())
                return false;
            if (dictionary.handle == 0)
                continue;
            if (!sourceHandles.insert(dictionary.handle).second
                || !dictionaries.emplace(dictionary.handle, &dictionary).second)
                return reject();
        }
        for (const DRW_XRecord& record : data->xRecords) {
            if (!examineSourceRecord())
                return false;
            if (record.handle == 0)
                continue;
            if (!sourceHandles.insert(record.handle).second
                || !xrecords.emplace(record.handle, &record).second)
                return reject();
        }
        for (const DRW_VisualStyle& style : data->visualStyles) {
            if (!examineSourceRecord())
                return false;
            if (style.handle == 0 || style.handle == DRW::NoHandle
                || !sourceHandles.insert(style.handle).second
                || !visualStyles.emplace(style.handle, &style).second)
                return reject();
        }
        for (const DRW_RawDxfObject& object : data->rawVisualStyleObjects) {
            if (!examineSourceRecord())
                return false;
            const auto typed = visualStyles.find(object.handle);
            if (object.name != "VISUALSTYLE" || object.handle == 0
                || object.handle == DRW::NoHandle
                || !visualStyleRawIdentityMatches(object)
                || typed == visualStyles.end()
                || typed->second->parentHandle != object.parentHandle
                || !rawVisualStyles.emplace(object.handle, &object).second)
                return reject();
        }
        if (rawVisualStyles.size() != visualStyles.size())
            return reject();
        for (const DRW_EvaluationGraph& graph : data->evaluationGraphs) {
            if (!examineSourceRecord())
                return false;
            if (graph.handle == 0
                || !sourceHandles.insert(graph.handle).second)
                return reject();
        }

        std::unordered_set<std::uint32_t> visiting;
        std::unordered_set<std::uint32_t> visitedDictionaries;
        std::unordered_set<std::uint32_t> visitedXRecords;
        std::unordered_set<std::uint32_t> visitedVisualStyles;
        enum class Phase { Enter, ExtensionDictionary, Reactors, Entries, Leave };
        struct Frame {
            std::uint32_t handle;
            const DRW_Dictionary* dictionary{nullptr};
            const DRW_XRecord* record{nullptr};
            const DRW_VisualStyle* visualStyle{nullptr};
            Phase phase{Phase::Enter};
            std::size_t index{0};
        };
        std::vector<Frame> frames;
        const auto queueTarget = [&](std::uint32_t handle, bool requireDictionary) {
            const auto dictionary = dictionaries.find(handle);
            if (dictionary != dictionaries.end()) {
                if (handle == 0xCu || handle == 0xDu)
                    return reject();
                frames.push_back({handle, dictionary->second, nullptr, nullptr,
                                  Phase::Enter, 0});
                return true;
            }
            const auto record = xrecords.find(handle);
            if (record != xrecords.end()) {
                if (requireDictionary)
                    return reject();
                frames.push_back({handle, nullptr, record->second, nullptr,
                                  Phase::Enter, 0});
                return true;
            }
            const auto visualStyle = visualStyles.find(handle);
            if (requireDictionary || visualStyle == visualStyles.end())
                return reject();
            frames.push_back({handle, nullptr, nullptr, visualStyle->second,
                              Phase::Enter, 0});
            return true;
        };
        const auto visit = [&](std::uint32_t handle, bool requireDictionary) {
            if (!queueTarget(handle, requireDictionary))
                return false;
            while (!frames.empty()) {
                Frame& frame = frames.back();
                const DRW_TableEntry& entry = frame.dictionary != nullptr
                    ? static_cast<const DRW_TableEntry&>(*frame.dictionary)
                    : frame.record != nullptr
                        ? static_cast<const DRW_TableEntry&>(*frame.record)
                        : static_cast<const DRW_TableEntry&>(*frame.visualStyle);
                switch (frame.phase) {
                case Phase::Enter: {
                    if (visiting.count(frame.handle) != 0)
                        return reject(); // Containment/extension edge, not a reactor.
                    const bool visited = frame.dictionary != nullptr
                        ? visitedDictionaries.count(frame.handle) != 0
                        : frame.record != nullptr
                            ? visitedXRecords.count(frame.handle) != 0
                            : visitedVisualStyles.count(frame.handle) != 0;
                    if (visited) {
                        frames.pop_back();
                        break;
                    }
                    if (m_extensionGraphStats.visitedNodes >= m_extensionGraphLimits.maxNodes)
                        return reject(ExtensionGraphFailure::NodeLimit);
                    ++m_extensionGraphStats.visitedNodes;
                    if (frame.dictionary != nullptr) {
                        // Completeness checks inspect every entry before the
                        // DFS consumes its edge. Bound that preliminary scan.
                        if (frame.dictionary->m_entries.size()
                            > m_extensionGraphLimits.maxEdges - m_extensionGraphStats.examinedEdges)
                            return reject(ExtensionGraphFailure::EdgeLimit);
                        if (!frame.dictionary->hasCompleteDwgEntries()
                            && !frame.dictionary->hasCompleteDxfEntries())
                            return reject();
                    } else if (frame.record != nullptr) {
                        if (frame.record->m_rawDataValid)
                            return reject();
                        for (const auto& value : frame.record->m_handleValues) {
                            if (!examineEdge())
                                return false;
                            if (value.first == 0)
                                return reject();
                        }
                    } else {
                        const auto raw = rawVisualStyles.find(frame.handle);
                        if (raw == rawVisualStyles.end()
                            || raw->second->parentHandle != entry.parentHandle)
                            return reject();
                    }
                    if (m_extensionGraphStats.stagedHandles >= m_extensionGraphLimits.maxStagedHandles)
                        return reject(ExtensionGraphFailure::StagedHandleLimit);
                    ++m_extensionGraphStats.stagedHandles;
                    visiting.insert(frame.handle);
                    if (frame.dictionary != nullptr)
                        m_extensionDictionaryHandles.push_back(frame.handle);
                    else if (frame.record != nullptr)
                        m_extensionXRecordHandles.push_back(frame.handle);
                    else
                        m_extensionVisualStyleHandles.push_back(frame.handle);
                    frame.phase = Phase::ExtensionDictionary;
                    break;
                }
                case Phase::ExtensionDictionary:
                    frame.phase = Phase::Reactors;
                    if (entry.xDictHandle != 0
                        && (!examineEdge() || !queueTarget(entry.xDictHandle, true)))
                        return false;
                    break;
                case Phase::Reactors:
                    if (frame.index < entry.reactorHandles.size()) {
                        const std::uint32_t reactor = entry.reactorHandles[frame.index++];
                        if (!examineEdge())
                            return false;
                        if (reactor == 0)
                            return reject();
                        // Non-owning back-edges may refer to gray containment
                        // nodes. Both soft/hard dictionary entries remain owning.
                        if (linetypeHandles.count(reactor) == 0 && visiting.count(reactor) == 0
                            && !queueTarget(reactor, false))
                            return false;
                    } else {
                        frame.index = 0;
                        frame.phase = Phase::Entries;
                    }
                    break;
                case Phase::Entries:
                    if (frame.dictionary != nullptr
                        && frame.index < frame.dictionary->m_entries.size()) {
                        const std::uint32_t child = frame.dictionary->m_entries[frame.index++].m_handle;
                        if (!examineEdge() || !queueTarget(child, false))
                            return false;
                    } else {
                        frame.phase = Phase::Leave;
                    }
                    break;
                case Phase::Leave:
                    visiting.erase(frame.handle);
                    if (frame.dictionary != nullptr)
                        visitedDictionaries.insert(frame.handle);
                    else if (frame.record != nullptr)
                        visitedXRecords.insert(frame.handle);
                    else
                        visitedVisualStyles.insert(frame.handle);
                    frames.pop_back();
                    break;
                }
            }
            return true;
        };

        for (const DRW_LType& lineType : data->lineTypes) {
            if (lineType.xDictHandle != 0
                && (!examineEdge() || !visit(lineType.xDictHandle, true)))
                return false;
            for (const std::uint32_t reactor : lineType.reactorHandles) {
                if (!examineEdge())
                    return false;
                if (reactor == 0)
                    return reject();
                if (linetypeHandles.count(reactor) != 0
                    || visiting.count(reactor) != 0)
                    continue;
                if (!visit(reactor, false))
                    return false;
            }
        }

        const auto hasParent = [&dictionaries, &visitedDictionaries,
                                &visitedXRecords, &visitedVisualStyles,
                                &linetypeHandles, data,
                                includeNamedObjectRoot](
                                   std::uint32_t parent) {
            return parent == 0 || parent == 0xCu || parent == 0xDu
                || (includeNamedObjectRoot
                    && data->hasNamedObjectDictionary
                    && parent == data->namedObjectDictionary.handle)
                || linetypeHandles.count(parent) != 0
                || (dictionaries.count(parent) != 0
                    && visitedDictionaries.count(parent) != 0)
                || visitedXRecords.count(parent) != 0
                || visitedVisualStyles.count(parent) != 0;
        };

        if (includeNamedObjectRoot && data->hasNamedObjectDictionary) {
            for (const DRW_Dictionary::Entry& entry :
                 data->namedObjectDictionary.m_entries) {
                std::string key = entry.m_name;
                for (char& character : key) {
                    if (character >= 'a' && character <= 'z')
                        character = static_cast<char>(
                            character - 'a' + 'A');
                }
                if (key == "ACAD_LAYOUT" || key == "ACAD_GROUP"
                    || (key == "ACAD_IMAGE_DICT"
                        && !data->images.empty())
                    || (key == "ACAD_MATERIAL"
                        && !data->dataStorageSections.empty()))
                    continue;
                if (entry.m_handle == 0 || entry.m_handle == DRW::NoHandle
                    || !examineEdge())
                    return reject();
                std::size_t matches = 0;
                if (dictionaries.count(entry.m_handle) != 0)
                    ++matches;
                if (xrecords.count(entry.m_handle) != 0)
                    ++matches;
                if (visualStyles.count(entry.m_handle) != 0)
                    ++matches;
                if (matches != 1u || !visit(entry.m_handle, false))
                    return false;
            }
        }
        for (std::uint32_t handle : m_extensionDictionaryHandles) {
            const DRW_Dictionary* dictionary = dictionaries.at(handle);
            if (!examineEdge())
                return false;
            if (!hasParent(dictionary->parentHandle))
                return reject();
        }
        for (std::uint32_t handle : m_extensionXRecordHandles) {
            const DRW_XRecord* record = xrecords.at(handle);
            if (!examineEdge())
                return false;
            if (!hasParent(record->parentHandle))
                return reject();
            const auto isHandleCode = [](int code) {
                return isDxfTranslatedHandleCode(code);
            };
            const auto validateHandleTarget =
                [&isHandleCode, &linetypeHandles, &visitedDictionaries,
                 &visitedXRecords, &visitedVisualStyles](
                    int code, std::uint64_t target) {
                    if (!isHandleCode(code))
                        return true;
                    if (target > std::numeric_limits<std::uint32_t>::max())
                        return false;
                    const std::uint32_t targetHandle =
                        static_cast<std::uint32_t>(target);
                    return targetHandle == 0
                        || linetypeHandles.count(targetHandle) != 0
                        || visitedDictionaries.count(targetHandle) != 0
                        || visitedXRecords.count(targetHandle) != 0
                        || visitedVisualStyles.count(targetHandle) != 0;
                };
            const auto validateHandleValue =
                [&isHandleCode, &validateHandleTarget, &examineEdge](
                    const DRW_Variant& value) {
                    const int code = value.code();
                    if (!isHandleCode(code))
                        return true;
                    if (!examineEdge())
                        return false;
                    std::uint64_t target = 0;
                    if (value.type() == DRW_Variant::INTEGER) {
                        if (value.i_val() < 0)
                            return false;
                        target = static_cast<std::uint32_t>(value.i_val());
                    } else if (value.type() == DRW_Variant::INTEGER64) {
                        if (value.i64_val() < 0)
                            return false;
                        target = static_cast<std::uint64_t>(value.i64_val());
                    } else if (value.type() == DRW_Variant::STRING) {
                        const char* text = value.c_str();
                        if (text == nullptr || *text == '\0')
                            return false;
                        const char* end = text
                            + std::char_traits<char>::length(text);
                        const auto parsed = std::from_chars(text, end, target, 16);
                        if (parsed.ec != std::errc{} || parsed.ptr != end)
                            return false;
                    } else {
                        return false;
                    }
                    return validateHandleTarget(code, target);
                };
            if (!record->m_dataEntries.empty()) {
                for (const DRW_Variant& value : record->m_dataEntries) {
                    if (!validateHandleValue(value))
                        return reject();
                }
            } else {
                for (const DRW_Variant& value : record->m_values) {
                    if (!validateHandleValue(value))
                        return reject();
                }
                for (const auto& value : record->m_handleValues) {
                    // Already counted during the bounded entry validation.
                    if (!validateHandleTarget(value.first, value.second))
                        return reject();
                }
            }
        }
        for (std::uint32_t handle : m_extensionVisualStyleHandles) {
            const DRW_VisualStyle* style = visualStyles.at(handle);
            if (!examineEdge() || !hasParent(style->parentHandle))
                return reject();
        }
        return true;
    } catch (...) {
        return reject();
    }
}

bool dx_iface::validateVisualStyleReferences(const dx_data& data,
                                              const char** failure) const {
    const auto reject = [failure](const char* reason) {
        if (failure != nullptr)
            *failure = reason;
        return false;
    };
    if (failure != nullptr)
        *failure = nullptr;
    const auto isVisualStyleKey = [](const std::string& name) {
        static constexpr char expected[] = "ACAD_VISUALSTYLE";
        if (name.size() != sizeof(expected) - 1u)
            return false;
        for (std::size_t i = 0; i < name.size(); ++i) {
            char character = name[i];
            if (character >= 'a' && character <= 'z')
                character = static_cast<char>(character - 'a' + 'A');
            if (character != expected[i])
                return false;
        }
        return true;
    };

    bool hasVisualStyleReference = false;
    for (const DRW_Vport& vport : data.VPorts)
        hasVisualStyleReference |= vport.visualStyleHandle != 0;
    for (const DRW_View& view : data.views)
        hasVisualStyleReference |= view.m_visualStyleHandle != 0;
    const auto hasViewportReference = [&hasVisualStyleReference](
                                          const std::list<DRW_Entity*>& entities) {
        for (const DRW_Entity* entity : entities) {
            const auto* viewport = dynamic_cast<const DRW_Viewport*>(entity);
            if (viewport != nullptr && viewport->visualStyleHandle != 0)
                hasVisualStyleReference = true;
        }
    };
    if (data.mBlock != nullptr)
        hasViewportReference(data.mBlock->ent);
    for (const dx_ifaceBlock* block : data.blocks) {
        if (block != nullptr)
            hasViewportReference(block->ent);
    }

    if (!data.hasNamedObjectDictionary) {
        if (!data.visualStyles.empty() || !data.rawVisualStyleObjects.empty()
            || hasVisualStyleReference)
            return reject("VISUALSTYLE data or code-348 edge has no NOD root");
        return true;
    }

    const DRW_Dictionary* styleDictionary = nullptr;
    std::size_t styleDictionaryRootEntries = 0;
    for (const DRW_Dictionary::Entry& entry :
         data.namedObjectDictionary.m_entries) {
        if (!isVisualStyleKey(entry.m_name))
            continue;
        ++styleDictionaryRootEntries;
        for (const DRW_Dictionary& dictionary : data.dictionaries) {
            if (dictionary.handle != entry.m_handle)
                continue;
            if (styleDictionary != nullptr)
                return reject("ACAD_VISUALSTYLE root target is ambiguous");
            styleDictionary = &dictionary;
        }
    }
    if (styleDictionaryRootEntries == 0) {
        if (!data.visualStyles.empty() || !data.rawVisualStyleObjects.empty()
            || hasVisualStyleReference)
            return reject("VISUALSTYLE data or code-348 edge is not rooted");
        return true;
    }
    if (styleDictionaryRootEntries != 1u || styleDictionary == nullptr
        || styleDictionary->parentHandle
            != data.namedObjectDictionary.handle
        || std::find(m_extensionDictionaryHandles.begin(),
                     m_extensionDictionaryHandles.end(),
                     styleDictionary->handle)
            == m_extensionDictionaryHandles.end())
        return reject("ACAD_VISUALSTYLE dictionary is missing or not closed");

    std::unordered_set<std::uint32_t> rootedStyleHandles;
    for (const DRW_Dictionary::Entry& entry : styleDictionary->m_entries) {
        std::size_t typedMatches = 0;
        const DRW_VisualStyle* style = nullptr;
        for (const DRW_VisualStyle& candidate : data.visualStyles) {
            if (candidate.handle != entry.m_handle)
                continue;
            style = &candidate;
            ++typedMatches;
        }
        std::size_t rawMatches = 0;
        for (const DRW_RawDxfObject& candidate : data.rawVisualStyleObjects) {
            if (candidate.handle == entry.m_handle
                && candidate.name == "VISUALSTYLE"
                && candidate.parentHandle == styleDictionary->handle)
                ++rawMatches;
        }
        if (typedMatches != 1u || rawMatches != 1u || style == nullptr
            || style->parentHandle != styleDictionary->handle
            || !rootedStyleHandles.insert(entry.m_handle).second
            || std::find(m_extensionVisualStyleHandles.begin(),
                         m_extensionVisualStyleHandles.end(), entry.m_handle)
                == m_extensionVisualStyleHandles.end())
            return reject("ACAD_VISUALSTYLE entry is not a unique closed style");
    }
    if (rootedStyleHandles.size() != data.visualStyles.size()
        || rootedStyleHandles.size() != data.rawVisualStyleObjects.size())
        return reject("unrooted or unmatched VISUALSTYLE carrier remains");

    const auto targetIsRooted = [&rootedStyleHandles](std::uint32_t handle) {
        return handle == 0 || rootedStyleHandles.count(handle) != 0;
    };
    for (const DRW_Vport& vport : data.VPorts) {
        if (!targetIsRooted(vport.visualStyleHandle))
            return reject("VPORT code-348 target is missing or wrongly typed");
    }
    for (const DRW_View& view : data.views) {
        if (!targetIsRooted(view.m_visualStyleHandle))
            return reject("VIEW code-348 target is missing or wrongly typed");
    }
    const auto validateViewportTargets = [&targetIsRooted](
                                             const std::list<DRW_Entity*>& entities) {
        for (const DRW_Entity* entity : entities) {
            const auto* viewport = dynamic_cast<const DRW_Viewport*>(entity);
            if (viewport != nullptr
                && !targetIsRooted(viewport->visualStyleHandle))
                return false;
        }
        return true;
    };
    if ((data.mBlock != nullptr
         && !validateViewportTargets(data.mBlock->ent)))
        return reject("VIEWPORT code-348 target is missing or wrongly typed");
    for (const dx_ifaceBlock* block : data.blocks) {
        if (block != nullptr && !validateViewportTargets(block->ent))
            return reject("VIEWPORT code-348 target is missing or wrongly typed");
    }
    return true;
}


void dx_iface::reportDwgReadOutcome(std::ostream& output,
                                   const dx_dwgReadOutcome& outcome) try {
    if (!outcome.attempted)
        return;
    DwgReadReportBuffer buffer;
    buffer.append("DWG read observations: schema=1 ");
    buffer.append(outcome.operationSucceeded ? "operationSucceeded=true "
                                             : "operationSucceeded=false ");
    buffer.append("sourceVersion=");
    buffer.append(canonicalDwgReadVersion(outcome.sourceVersion));
    buffer.append(" sourceVersionEnum="); buffer.integer(static_cast<int>(outcome.sourceVersion));
    buffer.append(" legacyError="); buffer.integer(static_cast<int>(outcome.error));
    buffer.append(observedDwgSemanticGaps(outcome.observations)
        ? " semanticCoverage=partial-observed\n" : " semanticCoverage=unknown\n");
    buffer.append("Observations overlap; decodedRecordTotal=unknown "
                  "uniqueSkippedRecordTotal=unknown carrierLossTotal=unknown "
                  "exportCompleteness=unknown\n");
    buffer.append("Raw name prefixes can collide and are not complete identities.\n");
    const DwgReadObservations& value = outcome.observations;
    buffer.field("entityParseFailures", value.entityParseFailures);
    buffer.field("objectParseFailures", value.objectParseFailures);
    buffer.field("classesCrcMismatch", value.classesCrcMismatch);
    buffer.field("reader21Ac1021CrcMismatch", value.reader21CrcMismatch);
    buffer.field("reader18FamilyHeaderCrcMismatch", value.reader18HeaderCrcMismatch);
    buffer.field("dataStorageLinkFailures", value.dataStorageLinkFailures);
    buffer.field("dataStorageOrphanRecords", value.dataStorageOrphanRecords);
    buffer.field("decodedProxyPrimitives", value.decodedProxyPrimitives);
    buffer.append("\n");
    formatNamedDwgObservations(buffer, "skippedCustomClasses", value.skippedCustomClasses);
    formatNamedDwgObservations(buffer, "skippedUnsupportedObjects", value.skippedUnsupportedObjects);
    buffer.field("frameCoverageStatus", static_cast<std::size_t>(value.frames.status));
    buffer.append(value.frames.status == DRW_DwgFrameCoverageStatus::NotAvailable
        ? "frameSourceDenominator=unknown " : "frameSourceDenominator=observed ");
    buffer.field("sourceFrames", value.frames.sourceFrames);
    static constexpr std::array<const char*, 7> dispositions{{
        "pending", "deferred", "staged", "published", "quarantined", "failed", "unresolved"}};
    for (std::size_t index = 0; index < dispositions.size(); ++index)
        buffer.field(dispositions[index], value.frames.dispositions[index]);
    buffer.field("unrecognizedDispositions", value.frames.unrecognizedDispositions);
    buffer.append("\nObserved receipt callbacks (not terminal delivery): ");
    static constexpr std::array<const char*, 4> carriers{{
        "typed", "raw", "typedAndRaw", "control"}};
    for (std::size_t index = 0; index < carriers.size(); ++index)
        buffer.field(carriers[index], outcome.observedReceiptCarriers[index]);
    buffer.append("\nOpaque callbacks are not CLI retention/export: ");
    buffer.field("opaqueViewCallbacks", outcome.opaqueViewCallbacks);
    buffer.field("opaqueFramesRetainedForCorrelation", outcome.opaqueFramesRetainedByAdapter);
    buffer.append(outcome.callbackCountsSaturated ? "callbackCountsSaturated=true\n"
                                                : "callbackCountsSaturated=false\n");
    buffer.append("Integrity observations: ");
    buffer.field("sourceRecorded", value.integrity.sourceRecorded);
    buffer.field("retainedRows", value.integrity.retained);
    buffer.field("extractionOmitted", value.integrity.extractionOmitted);
    buffer.field("reportedUpstreamDroppedLowerBound", value.integrity.reportedUpstreamDropped);
    buffer.field("truncatedRetainedNames", value.integrity.truncatedRetainedNames);
    buffer.field("omittedNameBytes", value.integrity.omittedNameBytes);
    buffer.append(value.integrity.aggregateSaturated ? "aggregateSaturated=true\n"
                                                    : "aggregateSaturated=false\n");
    for (std::size_t index = 0;
         index < std::min(value.integrity.retained, value.integrity.rows.size()); ++index) {
        const DwgReadIntegrityObservation& row = value.integrity.rows[index];
        buffer.append("integrity row: version="); buffer.integer(static_cast<int>(row.version));
        buffer.append(" ");
        buffer.field("severity", static_cast<std::size_t>(row.severity));
        buffer.field("phase", static_cast<std::size_t>(row.phase));
        buffer.field("kind", static_cast<std::size_t>(row.kind));
        buffer.field("offsetSpace", static_cast<std::size_t>(row.offsetSpace));
        buffer.append("logicalSectionId="); buffer.integer(row.logicalSectionId);
        buffer.append(" sectionDescriptorId="); buffer.integer(row.sectionDescriptorId);
        buffer.append(" "); buffer.prefix(row.sectionName);
        buffer.field("hasPageId", row.hasPageId); buffer.field("pageId", row.pageId);
        buffer.field("hasFileOffset", row.hasFileOffset); buffer.field("fileOffset", row.fileOffset);
        buffer.field("hasLogicalHandle", row.hasLogicalHandle); buffer.field("logicalHandle", row.logicalHandle);
        buffer.field("hasExpected", row.hasExpected); buffer.field("expected", row.expected);
        buffer.field("hasObserved", row.hasObserved); buffer.field("observed", row.observed);
        buffer.append("\n");
    }
    buffer.write(output);
} catch (...) {
    // Optional reporting must not change the legacy import Boolean/error or
    // output-publication policy, even when the caller's stream throws.
}

bool dx_iface::fileImport(const std::string& fileI, dx_data *fData, bool debug){
    fData->dwgReadOutcome = {};
    fData->sourceFormat = dx_source_format::Unknown;
    unsigned int found = fileI.find_last_of(".");
    std::string fileExt = fileI.substr(found+1);
    std::transform(fileExt.begin(), fileExt.end(),fileExt.begin(), ::toupper);
    cData = fData;
    currentBlock = cData->mBlock;

    if (fileExt == "DXF"){
        fData->sourceFormat = dx_source_format::DXF;
        //loads dxf
        dxfRW* dxf = new dxfRW(fileI.c_str());
        if (debug) {
            dxf->setDebug(DRW::DebugLevel::Debug);
        }
        bool success = dxf->read(this, false);
        if (success)
            fData->sourceVersion = sourceVersionFromDxfHeader(
                fData->headerC, dxf->getVersion());
        if (!success) {
            std::cout << "DXF file error: format " << dxf->getVersion() << " error " << dxf->getError() << std::endl;
        }
        delete dxf;
        return success;
    } else if (fileExt == "DWG"){
        fData->sourceFormat = dx_source_format::DWG;
        //loads dwg
        dwgRW dwg(fileI.c_str());
        fData->dwgReadOutcome.attempted = true;
        if (debug) {
            dwg.setDebug(DRW::DebugLevel::Debug);
        }
        const bool success = dwg.read(this, false);
        fData->dwgReadOutcome.operationSucceeded = success;
        fData->dwgReadOutcome.error = dwg.getError();
        fData->dwgReadOutcome.sourceVersion = dwg.getVersion();
        fData->dwgReadOutcome.observations = dwg.getReadObservations();
        if (success)
            fData->sourceVersion = dwg.getVersion();
        if (!success) {
            std::cout << "DWG file error: format " << dwg.getVersion() << " error " << dwg.getError() << std::endl;
        }
        if (debug || hasDwgReadWarning(fData->dwgReadOutcome))
            reportDwgReadOutcome(std::cerr, fData->dwgReadOutcome);
        return success;
    }
    std::cout << "file extension can be dxf or dwg" << std::endl;
    return false;
}

namespace {

std::string layoutSymbolKey(std::string value) {
    for (char& character : value) {
        const unsigned char byte = static_cast<unsigned char>(character);
        if (byte >= 'a' && byte <= 'z')
            character = static_cast<char>(byte - 'a' + 'A');
    }
    return value;
}

bool validateLayoutGraph(const dx_data& data, const char*& failure) {
    failure = "layout graph is incomplete or ambiguous";
    const auto validateUcsTargets = [&data](
            std::unordered_set<std::uint32_t>* structuralHandles) {
        std::unordered_map<std::uint32_t, const DRW_UCS*> ucsByHandle;
        std::unordered_set<std::string> ucsNames;
        for (const DRW_UCS& ucs : data.ucsList) {
            const std::string key = layoutSymbolKey(ucs.name);
            if (ucs.handle == 0 || ucs.handle == DRW::NoHandle
                || ucs.namedUcsHandle.ref != 0 || key.empty()
                || !ucsNames.insert(key).second
                || !ucsByHandle.emplace(ucs.handle, &ucs).second
                || (structuralHandles != nullptr
                    && !structuralHandles->insert(ucs.handle).second))
                return false;
        }

        std::unordered_set<std::uint32_t> entityHandles;
        const auto collectEntityHandles = [&entityHandles](
                const std::list<DRW_Entity*>& entities) {
            for (const DRW_Entity* entity : entities) {
                if (entity != nullptr && entity->handle != 0
                    && entity->handle != DRW::NoHandle)
                    entityHandles.insert(entity->handle);
            }
        };
        if (data.mBlock != nullptr)
            collectEntityHandles(data.mBlock->ent);
        for (const dx_ifaceBlock* block : data.blocks) {
            if (block != nullptr)
                collectEntityHandles(block->ent);
        }
        for (const auto& ucs : ucsByHandle) {
            if (entityHandles.count(ucs.first) != 0)
                return false;
        }

        for (const DRW_UCS& ucs : data.ucsList) {
            if (ucs.baseUcsHandle.ref != 0
                && ucsByHandle.count(ucs.baseUcsHandle.ref) != 1u)
                return false;
        }
        for (const DRW_Layout& layout : data.layouts) {
            if (layout.orthoViewType < 0 || layout.orthoViewType > 6
                || (layout.baseUcsHandle.ref != 0
                    && layout.orthoViewType == 0))
                return false;
            const std::uint32_t refs[] = {
                layout.namedUcsHandle.ref, layout.baseUcsHandle.ref
            };
            for (const std::uint32_t handle : refs) {
                if (handle != 0 && ucsByHandle.count(handle) != 1u)
                    return false;
            }
        }
        return true;
    };
    // DWG space ownership is encoded through DWG header/control references,
    // not the DXF NOD/ACAD_LAYOUT dictionary. Keep that route on its own
    // decoder/adapter lane unless it explicitly supplies the DXF graph.
    if (data.sourceFormat == dx_source_format::DWG
        && !data.hasNamedObjectDictionary)
        return validateUcsTargets(nullptr);
    const bool hasLayoutBacklink = std::any_of(
        data.blockRecords.begin(), data.blockRecords.end(),
        [](const DRW_Block_Record& record) { return record.layoutHandle != 0; });
    bool hasRootEntry = false;
    if (data.hasNamedObjectDictionary) {
        hasRootEntry = std::any_of(
            data.namedObjectDictionary.m_entries.begin(),
            data.namedObjectDictionary.m_entries.end(),
            [](const DRW_Dictionary::Entry& entry) {
                return layoutSymbolKey(entry.m_name) == "ACAD_LAYOUT";
            });
    }
    if (data.layouts.empty() && !hasLayoutBacklink && !hasRootEntry)
        return validateUcsTargets(nullptr);

    if (!data.hasNamedObjectDictionary
        || data.namedObjectDictionary.handle == 0
        || data.layouts.empty())
        return false;

    std::unordered_set<std::uint32_t> structuralHandles;
    if (!structuralHandles.insert(data.namedObjectDictionary.handle).second)
        return false;

    std::size_t rootLayoutEntryCount = 0;
    std::uint32_t layoutDictionaryHandle = 0;
    std::unordered_set<std::string> rootNames;
    for (const DRW_Dictionary::Entry& entry :
         data.namedObjectDictionary.m_entries) {
        const std::string key = layoutSymbolKey(entry.m_name);
        if (key.empty() || !rootNames.insert(key).second)
            return false;
        if (key == "ACAD_LAYOUT") {
            ++rootLayoutEntryCount;
            layoutDictionaryHandle = entry.m_handle;
        }
    }
    if (rootLayoutEntryCount != 1 || layoutDictionaryHandle == 0)
        return false;

    const DRW_Dictionary* layoutDictionary = nullptr;
    std::unordered_set<std::uint32_t> dictionaryHandles;
    for (const DRW_Dictionary& dictionary : data.dictionaries) {
        if (dictionary.handle == 0
            || !dictionaryHandles.insert(dictionary.handle).second
            || !structuralHandles.insert(dictionary.handle).second)
            return false;
        if (dictionary.handle == layoutDictionaryHandle)
            layoutDictionary = &dictionary;
    }
    if (layoutDictionary == nullptr
        || layoutDictionary->parentHandle != data.namedObjectDictionary.handle)
        return false;

    std::unordered_map<std::uint32_t, const DRW_Layout*> layoutsByHandle;
    std::unordered_map<std::string, std::uint32_t> layoutsByName;
    for (const DRW_Layout& layout : data.layouts) {
        const std::string key = layoutSymbolKey(layout.name);
        if (layout.handle == 0 || key.empty()
            || layout.parentHandle != layoutDictionaryHandle
            || !structuralHandles.insert(layout.handle).second
            || !layoutsByHandle.emplace(layout.handle, &layout).second
            || !layoutsByName.emplace(key, layout.handle).second)
            return false;
    }
    if (layoutDictionary->m_entries.size() != data.layouts.size())
        return false;
    std::unordered_set<std::uint32_t> dictionaryTargets;
    std::unordered_set<std::string> dictionaryNames;
    for (const DRW_Dictionary::Entry& entry : layoutDictionary->m_entries) {
        const std::string key = layoutSymbolKey(entry.m_name);
        const auto layout = layoutsByHandle.find(entry.m_handle);
        if (key.empty() || layout == layoutsByHandle.end()
            || layoutSymbolKey(layout->second->name) != key
            || !dictionaryTargets.insert(entry.m_handle).second
            || !dictionaryNames.insert(key).second)
            return false;
    }

    std::unordered_map<std::uint32_t, const DRW_Block_Record*> recordsByHandle;
    std::unordered_map<std::string, std::uint32_t> recordsByName;
    for (const DRW_Block_Record& record : data.blockRecords) {
        if (record.handle == 0 || record.name.empty()
            || !structuralHandles.insert(record.handle).second
            || !recordsByHandle.emplace(record.handle, &record).second
            || !recordsByName.emplace(layoutSymbolKey(record.name),
                                      record.handle).second)
            return false;
    }
    if (!validateUcsTargets(&structuralHandles))
        return false;
    std::unordered_set<std::uint32_t> associatedRecords;
    for (const DRW_Layout& layout : data.layouts) {
        const std::uint32_t blockRecordHandle =
            layout.paperSpaceBlockRecordHandle.ref;
        const auto record = recordsByHandle.find(blockRecordHandle);
        if (blockRecordHandle == 0 || record == recordsByHandle.end()
            || record->second->layoutHandle != layout.handle
            || !associatedRecords.insert(blockRecordHandle).second)
            return false;
    }

    // A non-null LAYOUT 331 is a last-active paper-space VIEWPORT entity,
    // not a VPORT table row or an arbitrary existing handle. Resolve it only
    // after the layout's BLOCK_RECORD association is known, and require the
    // viewport's logical owner/space to agree with that association. Entities
    // in the active paperspace arrive through ENTITIES (mBlock); inactive
    // paperspaces arrive in their *Paper_Spacennnn block containers.
    const auto hasOwnedViewport = [&data](std::uint32_t handle,
                                          std::uint32_t blockRecordHandle) {
        const DRW_Entity* target = nullptr;
        std::size_t matches = 0;
        const auto inspect = [handle, &target, &matches](
                                 const std::list<DRW_Entity*>& entities) {
            for (const DRW_Entity* entity : entities) {
                if (entity != nullptr && entity->handle == handle) {
                    target = entity;
                    ++matches;
                }
            }
        };
        if (data.mBlock != nullptr)
            inspect(data.mBlock->ent);
        for (const dx_ifaceBlock* block : data.blocks) {
            if (block != nullptr)
                inspect(block->ent);
        }
        const auto* viewport = dynamic_cast<const DRW_Viewport*>(target);
        return matches == 1 && viewport != nullptr
            && viewport->space == DRW::PaperSpace
            && viewport->parentHandle == blockRecordHandle;
    };
    for (const DRW_Block_Record& record : data.blockRecords) {
        if (record.layoutHandle == 0)
            continue;
        const auto layout = layoutsByHandle.find(record.layoutHandle);
        if (layout == layoutsByHandle.end()
            || layout->second->paperSpaceBlockRecordHandle.ref != record.handle)
            return false;
    }
    for (const DRW_Layout& layout : data.layouts) {
        const std::uint32_t viewportHandle =
            layout.lastActiveViewportHandle.ref;
        if (viewportHandle != 0
            && !hasOwnedViewport(
                viewportHandle,
                layout.paperSpaceBlockRecordHandle.ref))
            return false;
    }
    return associatedRecords.size() == data.layouts.size();
}

} // namespace

bool dx_iface::validateHatchBoundaryGraph(const dx_data& data) {
    m_hatchGraphStats = {};
    m_hatchGraphFailure = ExtensionGraphFailure::None;
    m_hatchSourceHandles.clear();
    m_hatchTargets.clear();
    m_hatchOwnerAliases.clear();
    m_hatchOwnerBlocks.clear();
    const auto reject = [this](ExtensionGraphFailure reason = ExtensionGraphFailure::InvalidGraph) {
        if (m_hatchGraphFailure == ExtensionGraphFailure::None)
            m_hatchGraphFailure = reason;
        m_hatchSourceHandles.clear();
        m_hatchTargets.clear();
        m_hatchOwnerAliases.clear();
        m_hatchOwnerBlocks.clear();
        return false;
    };
    const auto sourceRecord = [this, &reject]() {
        if (m_hatchGraphStats.sourceRecords >= m_hatchGraphLimits.maxSourceRecords)
            return reject(ExtensionGraphFailure::SourceRecordLimit);
        ++m_hatchGraphStats.sourceRecords;
        return true;
    };
    const auto edge = [this, &reject]() {
        if (m_hatchGraphStats.examinedEdges >= m_hatchGraphLimits.maxEdges)
            return reject(ExtensionGraphFailure::EdgeLimit);
        ++m_hatchGraphStats.examinedEdges;
        return true;
    };
    // Never retain borrowed entity pointers after this preflight returns.
    struct Candidate {
        const DRW_Entity* entity{nullptr};
        std::string owner;
    };
    try {
        std::unordered_map<std::uint32_t, Candidate> selected;
        const auto select = [this, &selected, &reject](std::uint32_t handle) {
            if (handle == DRW::NoHandle)
                return reject();
            if (selected.count(handle) != 0)
                return true;
            if (m_hatchGraphStats.visitedNodes >= m_hatchGraphLimits.maxNodes)
                return reject(ExtensionGraphFailure::NodeLimit);
            if (m_hatchGraphStats.stagedHandles >= m_hatchGraphLimits.maxStagedHandles)
                return reject(ExtensionGraphFailure::StagedHandleLimit);
            selected.emplace(handle, Candidate{});
            m_hatchSourceHandles.push_back(handle);
            ++m_hatchGraphStats.visitedNodes;
            ++m_hatchGraphStats.stagedHandles;
            return true;
        };
        const auto inspectBlocks = [&data, &sourceRecord](const auto& inspect) {
            if (data.mBlock == nullptr || !sourceRecord() || !inspect(*data.mBlock, true))
                return false;
            for (const dx_ifaceBlock* block : data.blocks) {
                if (!sourceRecord() || block == nullptr || !inspect(*block, false))
                    return false;
            }
            return true;
        };
        const auto gather = [&sourceRecord, &edge, &select, &reject](
                                const dx_ifaceBlock& block, bool) {
            for (const DRW_Entity* entity : block.ent) {
                if (!sourceRecord() || entity == nullptr)
                    return reject();
                const auto* hatch = dynamic_cast<const DRW_Hatch*>(entity);
                if (hatch == nullptr || entity->eType != DRW::HATCH)
                    continue;
                for (const auto& loop : hatch->looplist) {
                    if (!sourceRecord() || !loop)
                        return reject();
                    for (std::uint32_t handle : loop->m_boundaryHandles) {
                        if (!edge() || !select(hatch->handle) || !select(handle))
                            return false;
                    }
                }
            }
            return true;
        };
        if (!inspectBlocks(gather))
            return m_hatchGraphFailure == ExtensionGraphFailure::None ? reject() : false;
        if (selected.empty())
            return true; // Detached/no-reference HATCH and unrelated drawings.

        // Match the codec's source BLOCK_RECORD aliases and ASCII symbol key.
        // Explicit entity parentHandle wins over physical container/space.
        std::unordered_map<std::uint32_t, std::string> owners;
        std::unordered_map<std::string, std::uint32_t> ownerNames;
        std::unordered_set<std::string> emittedBlocks{"*MODEL_SPACE", "*PAPER_SPACE"};
        const auto ownerRecord = [&](std::uint32_t handle, const std::string& name) {
            if (handle == DRW::NoHandle || name.empty())
                return reject();
            const std::string key = layoutSymbolKey(name);
            const auto identity = owners.emplace(handle, key);
            const auto symbol = ownerNames.emplace(key, handle);
            if ((!identity.second && identity.first->second != key)
                || (!symbol.second && symbol.first->second != handle)
                || selected.count(handle) != 0)
                return reject();
            return true;
        };
        for (const DRW_Block_Record& record : data.blockRecords) {
            if (!sourceRecord() || !ownerRecord(record.handle, record.name))
                return false;
        }
        for (const dx_ifaceBlock* block : data.blocks) {
            if (!sourceRecord())
                return false;
            if (!isFixedSpaceBlock(block))
                emittedBlocks.insert(layoutSymbolKey(block->name));
            if (selected.count(block->handle) != 0
                || (block->parentHandle != DRW::NoHandle
                    && !ownerRecord(block->parentHandle, block->name)))
                return reject();
        }
        const auto otherIdentity = [&](std::uint32_t handle) {
            return sourceRecord() && (selected.count(handle) == 0 || reject());
        };
        const auto otherEntries = [&otherIdentity](const auto& entries) {
            for (const auto& entry : entries)
                if (!otherIdentity(entry.handle))
                    return false;
            return true;
        };
        if (!otherEntries(data.lineTypes) || !otherEntries(data.layers)
            || !otherEntries(data.dimStyles) || !otherEntries(data.VPorts)
            || !otherEntries(data.views) || !otherEntries(data.ucsList)
            || !otherEntries(data.textStyles) || !otherEntries(data.appIds)
            || !otherEntries(data.layouts) || !otherEntries(data.dictionaries)
            || !otherEntries(data.xRecords) || !otherEntries(data.materials)
            || !otherEntries(data.visualStyles) || !otherEntries(data.rawVisualStyleObjects)
            || !otherEntries(data.proxyObjects) || !otherEntries(data.rawProxyObjects)
            || !otherEntries(data.evaluationGraphs) || !otherEntries(data.acshHistoryObjects)
            || !otherEntries(data.dynamicBlockObjects)
            || (data.hasNamedObjectDictionary
                && !otherIdentity(data.namedObjectDictionary.handle)))
            return false;

        const auto inventory = [&](const dx_ifaceBlock& block, bool mainContainer) {
            for (const DRW_Entity* entity : block.ent) {
                if (!sourceRecord())
                    return false;
                const auto found = selected.find(entity->handle);
                if (found != selected.end()) {
                    Candidate& candidate = found->second;
                    if (candidate.entity != nullptr)
                        return reject(); // Duplicate identity or publication.
                    if (entity->parentHandle != DRW::NoHandle) {
                        const auto owner = owners.find(entity->parentHandle);
                        if (owner == owners.end() || emittedBlocks.count(owner->second) == 0)
                            return reject();
                        candidate.owner = owner->second;
                    } else if (!mainContainer && !isFixedSpaceBlock(&block)) {
                        candidate.owner = layoutSymbolKey(block.name);
                    } else {
                        candidate.owner = entity->space == DRW::PaperSpace
                            ? "*PAPER_SPACE" : "*MODEL_SPACE";
                    }
                    candidate.entity = entity;
                }
                // Children/terminators are not independently dispatched top-
                // level HATCH boundaries. Also catch collisions with parents.
                if (const auto* polyline = dynamic_cast<const DRW_Polyline*>(entity)) {
                    for (const auto& vertex : polyline->vertlist)
                        if (!vertex || !otherIdentity(vertex->handle))
                            return reject();
                    if (!otherIdentity(polyline->dwgSeqEndHandle()))
                        return false;
                }
                if (const auto* insert = dynamic_cast<const DRW_Insert*>(entity)) {
                    for (const auto& attribute : insert->attlist)
                        if (!attribute || !otherIdentity(attribute->handle))
                            return reject();
                    if (!otherIdentity(insert->seqendH.ref))
                        return false;
                }
            }
            return true;
        };
        if (!inspectBlocks(inventory))
            return m_hatchGraphFailure == ExtensionGraphFailure::None ? reject() : false;
        for (const auto& entry : selected) {
            if (entry.second.entity == nullptr)
                return reject();
            const auto* hatch = dynamic_cast<const DRW_Hatch*>(entry.second.entity);
            if (hatch == nullptr || hatch->eType != DRW::HATCH)
                continue;
            for (const auto& loop : hatch->looplist) {
                if (!sourceRecord())
                    return false;
                for (std::uint32_t handle : loop->m_boundaryHandles) {
                    const auto target = selected.find(handle);
                    if (target == selected.end() || target->second.entity == nullptr
                        || target->second.owner != entry.second.owner
                        || target->second.entity == hatch)
                        return reject();
                }
            }
        }
        for (const auto& entry : selected) {
            const DRW_Entity& entity = *entry.second.entity;
            HatchPublication target;
            target.type = entity.eType;
            target.owner = entry.second.owner;
            m_hatchTargets.emplace(entry.first, std::move(target));
            if (entity.parentHandle != DRW::NoHandle)
                m_hatchOwnerAliases.emplace(entity.parentHandle, entry.second.owner);
            if (entry.second.owner != "*MODEL_SPACE"
                && entry.second.owner != "*PAPER_SPACE")
                m_hatchOwnerBlocks.emplace(entry.second.owner, false);
        }
        return true;
    } catch (...) {
        return reject();
    }
}

void dx_iface::failHatchPublication() noexcept {
    m_hatchPlanningFailed = true;
    if (m_hatchGraphFailure == ExtensionGraphFailure::None)
        m_hatchGraphFailure = ExtensionGraphFailure::InvalidGraph;
}

void dx_iface::recordHatchBlockPublication(const std::string& name, bool written) {
    if (m_hatchOwnerBlocks.empty())
        return;
    const auto block = m_hatchOwnerBlocks.find(layoutSymbolKey(name));
    if (block == m_hatchOwnerBlocks.end())
        return;
    if (!written || block->second)
        failHatchPublication();
    else
        block->second = true;
}

bool dx_iface::finalizeDxfWrite() {
    if (m_hatchPlanningFailed)
        return false;
    const auto* context = dxfW != nullptr ? dxfW->getWritingContext() : nullptr;
    if (!m_hatchTargets.empty() && context == nullptr) {
        failHatchPublication();
        return false;
    }
    for (const auto& target : m_hatchTargets) {
        const auto binding = context->sourceHandleToMintedMap.find(target.first);
        if (target.second.emitted == 0 || !target.second.published
            || context->writtenSourceEntityHandles.count(target.first) != 1
            || context->ambiguousSourceHandles.count(target.first) != 0
            || binding == context->sourceHandleToMintedMap.end()
            || binding->second != target.second.emitted) {
            failHatchPublication();
            return false;
        }
    }
    for (const auto& block : m_hatchOwnerBlocks) {
        if (!block.second) {
            failHatchPublication();
            return false;
        }
    }
    return true;
}

bool dx_iface::fileExport(const std::string& file, DRW::Version v, bool binary,
                          dx_data *fData, bool debug, bool requireResolvedOwners){
    m_hatchSourceHandles.clear();
    m_hatchTargets.clear();
    m_hatchOwnerAliases.clear();
    m_hatchOwnerBlocks.clear();
    m_hatchOwnerContext.clear();
    m_hatchPlanningFailed = false;
    struct HatchPlanScope {
        std::vector<std::uint32_t>& handles;
        std::unordered_map<std::uint32_t, HatchPublication>& targets;
        std::unordered_map<std::uint32_t, std::string>& aliases;
        std::unordered_map<std::string, bool>& blocks;
        std::string& context;
        bool& failed;
        ~HatchPlanScope() {
            handles.clear(); targets.clear(); aliases.clear(); blocks.clear();
            context.clear(); failed = false;
        }
    } hatchPlanScope{m_hatchSourceHandles, m_hatchTargets, m_hatchOwnerAliases,
                     m_hatchOwnerBlocks, m_hatchOwnerContext, m_hatchPlanningFailed};
    cData = fData;
    m_acdsHistoryObjects.clear();
    m_acdsTypedHistoryObjects.clear();
    m_acdsTypedEvaluationGraphs.clear();
    m_acdsMaterialHandles.clear();
    const auto reportPreflightFailure = [debug](const char* stage) {
        if (debug) {
            std::cerr << "DXF export preflight failed: " << stage << '\n';
        }
    };
    if (cData == nullptr) {
        m_hatchGraphStats = {};
        m_hatchGraphFailure = ExtensionGraphFailure::InvalidGraph;
        reportPreflightFailure("HATCH graph source drawing is null");
        return false;
    }
    if (!validateHatchBoundaryGraph(*cData)) {
        reportPreflightFailure("HATCH boundary identity/owner graph validation");
        return false;
    }
    const char* layoutGraphFailure = nullptr;
    if (!validateLayoutGraph(*cData, layoutGraphFailure)) {
        reportPreflightFailure(layoutGraphFailure);
        return false;
    }
    if (v < DRW::AC1015
        && (!cData->layouts.empty()
            || std::any_of(cData->blockRecords.begin(),
                           cData->blockRecords.end(),
                           [](const DRW_Block_Record& record) {
                               return record.layoutHandle != 0;
                           }))) {
        reportPreflightFailure(
            "LAYOUT graph cannot be represented by a pre-R2000 DXF target");
        return false;
    }
    if (!cData->dynamicBlockObjects.empty()) {
        reportPreflightFailure(
            "dynamic-block objects have no qualified DXF serializer");
        return false;
    }
    const auto hasVisualStyleEdges = [this]() {
        for (const DRW_Vport& vport : cData->VPorts) {
            if (vport.visualStyleHandle != 0)
                return true;
        }
        for (const DRW_View& view : cData->views) {
            if (view.m_visualStyleHandle != 0)
                return true;
        }
        const auto hasViewportEdge = [](const std::list<DRW_Entity*>& entities) {
            for (const DRW_Entity* entity : entities) {
                const auto* viewport = dynamic_cast<const DRW_Viewport*>(entity);
                if (viewport != nullptr && viewport->visualStyleHandle != 0)
                    return true;
            }
            return false;
        };
        if (cData->mBlock != nullptr
            && hasViewportEdge(cData->mBlock->ent))
            return true;
        for (const dx_ifaceBlock* block : cData->blocks) {
            if (block != nullptr && hasViewportEdge(block->ent))
                return true;
        }
        return false;
    };
    const bool hasVisualStyleData = !cData->visualStyles.empty()
        || !cData->rawVisualStyleObjects.empty();
    const bool validateVisualStyles = hasVisualStyleData
        || hasVisualStyleEdges();
    // A VisualStyle carrier is only safe to emit with its rooted dictionary
    // graph. Tolerant callers still get fail-closed behavior for this feature
    // until there is an explicit, observable omission policy.
    const bool includeNamedObjectRoot = requireResolvedOwners
        || validateVisualStyles;
    if (!prepareExtensionObjectGraph(cData, includeNamedObjectRoot)) {
        reportPreflightFailure("extension-object graph validation");
        if (debug) {
            std::cerr << "Extension graph diagnostic: " << extensionGraphFailureName()
                      << " nodes=" << m_extensionGraphStats.visitedNodes
                      << " edges=" << m_extensionGraphStats.examinedEdges
                      << " staged-handles=" << m_extensionGraphStats.stagedHandles
                      << " source-records=" << m_extensionGraphStats.sourceRecords << '\n';
        }
        return false;
    }
    if (includeNamedObjectRoot) {
        const char* visualStyleFailure = nullptr;
        if (!validateVisualStyleReferences(*cData, &visualStyleFailure)) {
            reportPreflightFailure(visualStyleFailure != nullptr
                                       ? visualStyleFailure
                                       : "VISUALSTYLE reference closure");
            return false;
        }
    }
    if (!cData->dataStorageSections.empty()
        && (!cData->rawProxyObjects.empty()
            || !cData->dxfClasses.empty())) {
        // The narrow DWG DataStorage projection owns its four CLASS ordinal
        // slots. Do not silently discard unrelated custom classes/proxies.
        reportPreflightFailure("mixed source DXF classes/proxies with ACDS data");
        return false;
    }
    dxfW = new dxfRW(file.c_str());
    std::vector<DRW_Block_Record> sourceBlockRecords = cData->blockRecords;
    for (const dx_ifaceBlock* block : cData->blocks) {
        if (block == nullptr || block->parentHandle == DRW::NoHandle)
            continue;
        // DWG currently delivers the record identity on BLOCK.parentHandle.
        // Keep that structural alias without inventing missing table metadata.
        DRW_Block_Record identity;
        identity.handle = block->parentHandle;
        identity.name = block->name;
        sourceBlockRecords.push_back(std::move(identity));
    }
    dxfW->setSourceBlockRecords(sourceBlockRecords, requireResolvedOwners);
    std::vector<DRW_LType> lineTypes(cData->lineTypes.begin(),
                                     cData->lineTypes.end());
    dxfW->setCanonicalLineTypeMetadata(lineTypes);
    std::vector<DRW_RawDxfSection> rawDxfSections = cData->rawDxfSections;
    if (!appendAcdsDataSection(*cData, v, rawDxfSections, debug)) {
        reportPreflightFailure("ACDS DataStorage projection");
        delete dxfW;
        dxfW = nullptr;
        return false;
    }
    std::vector<std::uint32_t> referencedMaterials;
    std::vector<DRW_Dictionary> materialDictionaries;
    std::vector<std::pair<std::string, std::string>> materialRootEntries;
    if (!cData->dataStorageSections.empty()) {
        const auto failAcdsExport = [&]() {
            delete dxfW;
            dxfW = nullptr;
            return false;
        };
        const bool hasTypedAcdsObjects =
            !cData->acshHistoryObjects.empty()
            || !cData->evaluationGraphs.empty();
        if (hasTypedAcdsObjects) {
            if (!collectAcdsTypedBoxHistoryObjects(
                    *cData, m_acdsTypedHistoryObjects,
                    m_acdsTypedEvaluationGraphs, referencedMaterials,
                    debug)) {
                reportPreflightFailure("ACDS typed BOX history closure");
                delete dxfW;
                dxfW = nullptr;
                return false;
            }
        } else if (!collectAcdsHistoryProxyObjects(
                       *cData, m_acdsHistoryObjects, referencedMaterials,
                       debug)) {
            reportPreflightFailure("ACDS modeler/history proxy closure");
            delete dxfW;
            dxfW = nullptr;
            return false;
        }
        if (!configureAcdsMaterialDictionary(
                *cData, referencedMaterials, materialDictionaries,
                materialRootEntries, m_acdsMaterialHandles)) {
            reportPreflightFailure("ACDS material dictionary closure");
            delete dxfW;
            dxfW = nullptr;
            return false;
        }
        if (!m_acdsTypedHistoryObjects.empty()) {
            // Native custom objects use their record names directly. They do
            // not carry the proxy-only group-91 class ordinal.
            std::map<std::string, int> classInstanceCounts;
            for (const DRW_RawDxfObject& object :
                 m_acdsTypedHistoryObjects) {
                if (object.name.empty() || object.handle == 0) {
                    reportPreflightFailure("ACDS typed object class metadata");
                    return failAcdsExport();
                }
                ++classInstanceCounts[object.name];
            }
            if (m_acdsTypedEvaluationGraphs.size()
                > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
                return failAcdsExport();
            }
            if (!m_acdsTypedEvaluationGraphs.empty()) {
                classInstanceCounts["ACAD_EVALUATION_GRAPH"] =
                    static_cast<int>(m_acdsTypedEvaluationGraphs.size());
            }
            if (m_acdsMaterialHandles.size()
                > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
                return failAcdsExport();
            }
            if (!m_acdsMaterialHandles.empty())
                classInstanceCounts["MATERIAL"] =
                    static_cast<int>(m_acdsMaterialHandles.size());

            std::vector<DRW_Class> classes;
            static const char* supportedTypedClasses[] = {
                "ACSH_HISTORY_CLASS", "ACAD_EVALUATION_GRAPH",
                "ACSH_BOX_CLASS", "MATERIAL"};
            for (const char* name : supportedTypedClasses) {
                const auto count = classInstanceCounts.find(name);
                if (count == classInstanceCounts.end())
                    continue;
                DRW_Class cls;
                if (!dxfRW::dxfClassForRecordName(name, cls))
                    return failAcdsExport();
                cls.instanceCount = count->second;
                cls.wasaProxyFlag = 0;
                classes.push_back(std::move(cls));
            }
            dxfW->setDxfClasses(classes);
        } else {
        // DXF proxy group 91 indexes CLASSES from 500; ODA discards proxies
        // if those ordinal references do not resolve. Rebuild the class table
        // from the exact source DWG class records used by the validated proxy
        // graphs, then assign output ordinals independent of source numbering.
        static const char* supportedProxyClasses[] = {
            "ACSH_HISTORY_CLASS", "ACAD_EVALUATION_GRAPH",
            "ACSH_CONE_CLASS", "ACSH_BOX_CLASS",
            "ACSH_EXTRUSION_CLASS"};
        std::unordered_map<std::uint32_t, std::string> sourceClassNames;
        std::map<std::string, int> classInstanceCounts;
        for (DRW_RawDxfObject& object : m_acdsHistoryObjects) {
            std::size_t classIdCount = 0;
            std::uint32_t sourceClassId = 0;
            std::size_t classIdGroupIndex = 0;
            for (std::size_t index = 0; index < object.groups.size(); ++index) {
                const DRW_Variant& group = object.groups[index];
                if (group.code() != 91)
                    continue;
                ++classIdCount;
                classIdGroupIndex = index;
                if (group.type() != DRW_Variant::INTEGER
                || group.i_val() < 0) {
                    return failAcdsExport();
                }
                sourceClassId = static_cast<std::uint32_t>(group.i_val());
            }
            if (classIdCount != 1u)
                return failAcdsExport();
            const DRW_DwgClassCoverageEntry* classEntry = nullptr;
            for (const DRW_DwgClassCoverageEntry& entry :
                 cData->dwgClassCoverage.m_entries) {
                if (entry.m_classNumber != sourceClassId)
                    continue;
                if (classEntry != nullptr) {
                    classEntry = nullptr;
                    break;
                }
                classEntry = &entry;
            }
            if (classEntry == nullptr
                || std::find(std::begin(supportedProxyClasses),
                             std::end(supportedProxyClasses),
                             classEntry->m_recordName)
                       == std::end(supportedProxyClasses)) {
                return failAcdsExport();
            }
            const auto inserted = sourceClassNames.emplace(
                sourceClassId, classEntry->m_recordName);
            if (!inserted.second
                && inserted.first->second != classEntry->m_recordName) {
                return failAcdsExport();
            }
            ++classInstanceCounts[classEntry->m_recordName];
            object.groups[classIdGroupIndex].addInt(91, 0);
        }

        std::vector<DRW_Class> classes;
        std::unordered_map<std::string, std::int32_t> outputClassIds;
        for (const char* name : supportedProxyClasses) {
            const auto count = classInstanceCounts.find(name);
            if (count == classInstanceCounts.end())
                continue;
            DRW_Class cls;
            if (!dxfRW::dxfClassForRecordName(name, cls)) {
                delete dxfW;
                dxfW = nullptr;
                return false;
            }
            cls.instanceCount = count->second;
            cls.wasaProxyFlag = 1;
            outputClassIds.emplace(name,
                static_cast<std::int32_t>(500u + classes.size()));
            classes.push_back(std::move(cls));
        }
        if (!m_acdsMaterialHandles.empty()) {
            DRW_Class cls;
            if (!dxfRW::dxfClassForRecordName("MATERIAL", cls)) {
                delete dxfW;
                dxfW = nullptr;
                return false;
            }
            cls.instanceCount = static_cast<int>(m_acdsMaterialHandles.size());
            cls.wasaProxyFlag = 0;
            classes.push_back(std::move(cls));
        }
        for (DRW_RawDxfObject& object : m_acdsHistoryObjects) {
            std::size_t classIdGroupIndex = object.groups.size();
            std::uint32_t sourceClassId = 0;
            for (std::size_t index = 0; index < object.groups.size(); ++index) {
                const DRW_Variant& group = object.groups[index];
                if (group.code() == 91) {
                    classIdGroupIndex = index;
                    break;
                }
            }
            if (classIdGroupIndex == object.groups.size())
                return failAcdsExport();
            // The temporary value was zeroed above; recover the exact class
            // name from the proxy subclass and the source coverage report.
            const DRW_ProxyObject* sourceProxy = nullptr;
            for (const DRW_ProxyObject& proxy : cData->proxyObjects) {
                if (proxy.handle != object.handle)
                    continue;
                if (sourceProxy != nullptr)
                    return failAcdsExport();
                sourceProxy = &proxy;
            }
            if (sourceProxy == nullptr || !sourceProxy->m_hasProxyClassId)
                return failAcdsExport();
            sourceClassId = static_cast<std::uint32_t>(
                sourceProxy->m_proxyClassId);
            const auto sourceName = sourceClassNames.find(sourceClassId);
            if (sourceName == sourceClassNames.end())
                return failAcdsExport();
            const auto outputId = outputClassIds.find(sourceName->second);
            if (outputId == outputClassIds.end())
                return failAcdsExport();
            object.groups[classIdGroupIndex].addInt(91, outputId->second);
        }
        dxfW->setDxfClasses(classes);
        }
    } else {
        std::vector<DRW_Class> classes = cData->dxfClasses;
        if (!cData->evaluationGraphs.empty()) {
            const auto isEvaluationGraphClass = [](const DRW_Class& cls) {
                return cls.recName == "EVALUATION_GRAPH"
                    || cls.recName == "EVALUATIONGRAPH"
                    || cls.recName == "ACDBEVALGRAPH"
                    || cls.recName == "ACAD_EVALUATION_GRAPH";
            };
            classes.erase(
                std::remove_if(classes.begin(), classes.end(),
                               isEvaluationGraphClass),
                classes.end());
            DRW_Class evaluationGraphClass;
            if (!dxfRW::dxfClassForRecordName(
                    "ACAD_EVALUATION_GRAPH", evaluationGraphClass)) {
                delete dxfW;
                dxfW = nullptr;
                return false;
            }
            if (cData->evaluationGraphs.size()
                > static_cast<std::size_t>(
                    std::numeric_limits<int>::max())) {
                delete dxfW;
                dxfW = nullptr;
                return false;
            }
            evaluationGraphClass.instanceCount = static_cast<int>(
                cData->evaluationGraphs.size());
            classes.push_back(std::move(evaluationGraphClass));
        }
        dxfW->setDxfClasses(classes);
    }
    dxfW->setRawDxfSections(rawDxfSections);
    if (debug) {
        dxfW->setDebug(DRW::DebugLevel::Debug);
    }
    const std::set<std::uint32_t> fixedHandles = {
        0x1u, 0x2u, 0x3u, 0x5u, 0x6u, 0x7u, 0x8u, 0x9u, 0xAu,
        0xCu, 0xDu, 0x10u, 0x12u, 0x14u, 0x15u, 0x16u,
        0x1Cu, 0x1Du, 0x1Eu, 0x1Fu, 0x20u, 0x21u};
    for (const DRW_Dictionary& dictionary : cData->dictionaries) {
        if (dictionary.handle != 0 && !dxfW->reserveHandle(dictionary.handle)) {
            delete dxfW;
            dxfW = nullptr;
            return false;
        }
    }
    for (const DRW_Layout& layout : cData->layouts) {
        if (layout.handle != 0 && !dxfW->reserveHandle(layout.handle)) {
            delete dxfW;
            dxfW = nullptr;
            return false;
        }
    }
    for (const DRW_UCS& ucs : cData->ucsList) {
        if (ucs.handle != 0 && ucs.handle != DRW::NoHandle
            && !dxfW->reserveHandle(ucs.handle)) {
            delete dxfW;
            dxfW = nullptr;
            return false;
        }
    }
    for (const DRW_XRecord& record : cData->xRecords) {
        if (record.handle != 0 && !dxfW->reserveHandle(record.handle)) {
            delete dxfW;
            dxfW = nullptr;
            return false;
        }
    }
    for (const DRW_VisualStyle& style : cData->visualStyles) {
        if (!dxfW->reserveHandle(style.handle)) {
            delete dxfW;
            dxfW = nullptr;
            return false;
        }
    }
    std::unordered_set<std::uint32_t> proxyHandles;
    for (const DRW_RawDxfObject& object : cData->rawProxyObjects) {
        if (object.handle == 0 || !proxyHandles.insert(object.handle).second
            || !dxfW->reserveHandle(object.handle)) {
            delete dxfW;
            dxfW = nullptr;
            return false;
        }
    }
    for (const DRW_EvaluationGraph& graph : cData->evaluationGraphs) {
        if (cData->dataStorageSections.empty()
            && (graph.handle == 0
                || !proxyHandles.insert(graph.handle).second
                || !dxfW->reserveHandle(graph.handle))) {
            delete dxfW;
            dxfW = nullptr;
            return false;
        }
    }
    if (!cData->dataStorageSections.empty()) {
        for (const DRW_RawDxfObject& object : m_acdsHistoryObjects) {
            if (!proxyHandles.insert(object.handle).second
                || !dxfW->reserveHandle(object.handle)) {
                delete dxfW;
                dxfW = nullptr;
                return false;
            }
        }
        for (const DRW_RawDxfObject& object : m_acdsTypedHistoryObjects) {
            if (object.handle == 0
                || !proxyHandles.insert(object.handle).second
                || !dxfW->reserveHandle(object.handle)) {
                delete dxfW;
                dxfW = nullptr;
                return false;
            }
        }
        for (const DRW_EvaluationGraph& graph :
             m_acdsTypedEvaluationGraphs) {
            if (graph.handle == 0
                || !proxyHandles.insert(graph.handle).second
                || !dxfW->reserveHandle(graph.handle)) {
                delete dxfW;
                dxfW = nullptr;
                return false;
            }
        }
        for (const std::uint32_t handle : m_acdsMaterialHandles) {
            if (!dxfW->reserveHandle(handle)) {
                delete dxfW;
                dxfW = nullptr;
                return false;
            }
        }
    }
    std::map<std::uint32_t, std::uint32_t> handleRemap;
    const auto registerRemap = [&handleRemap](std::uint32_t source,
                                               std::uint32_t emitted) {
        if (source == 0 || emitted == 0)
            return false;
        const auto inserted = handleRemap.emplace(source, emitted);
        return inserted.second || inserted.first->second == emitted;
    };
    if (cData->hasNamedObjectDictionary
        && cData->namedObjectDictionary.handle != 0
        && cData->namedObjectDictionary.handle != 0xCu
        && !registerRemap(cData->namedObjectDictionary.handle, 0xCu)) {
        delete dxfW;
        dxfW = nullptr;
        return false;
    }
    const auto remapFixedCollision = [&](std::uint32_t handle) {
        if (fixedHandles.count(handle) == 0)
            return true;
        const auto existing = handleRemap.find(handle);
        if (existing != handleRemap.end())
            return true;
        const std::uint32_t replacement = dxfW->allocHandle();
        if (replacement == 0 || !dxfW->reserveHandle(replacement)
            || !registerRemap(handle, replacement))
            return false;
        return true;
    };
    for (const DRW_Dictionary& dictionary : cData->dictionaries) {
        if (!remapFixedCollision(dictionary.handle)) {
            delete dxfW;
            dxfW = nullptr;
            return false;
        }
    }
    for (const DRW_Layout& layout : cData->layouts) {
        if (!remapFixedCollision(layout.handle)) {
            delete dxfW;
            dxfW = nullptr;
            return false;
        }
    }
    for (const DRW_UCS& ucs : cData->ucsList) {
        if (!remapFixedCollision(ucs.handle)) {
            delete dxfW;
            dxfW = nullptr;
            return false;
        }
    }
    for (const std::uint32_t handle : m_extensionDictionaryHandles) {
        if (!remapFixedCollision(handle)) {
            delete dxfW;
            dxfW = nullptr;
            return false;
        }
    }
    for (const std::uint32_t handle : m_extensionXRecordHandles) {
        if (!remapFixedCollision(handle)) {
            delete dxfW;
            dxfW = nullptr;
            return false;
        }
    }
    for (const DRW_VisualStyle& style : cData->visualStyles) {
        if (!remapFixedCollision(style.handle)) {
            delete dxfW;
            dxfW = nullptr;
            return false;
        }
    }
    dxfW->setHandleRemap(handleRemap);
    std::vector<std::pair<std::string, std::string>> rootEntries =
        materialRootEntries;
    std::unordered_set<std::string> rootEntryNames;
    for (const auto& entry : rootEntries) {
        if (!rootEntryNames.insert(layoutSymbolKey(entry.first)).second) {
            delete dxfW;
            dxfW = nullptr;
            return false;
        }
    }
    if (cData->hasNamedObjectDictionary) {
        for (const DRW_Dictionary::Entry& sourceEntry :
             cData->namedObjectDictionary.m_entries) {
            const std::string key = layoutSymbolKey(sourceEntry.m_name);
            if (key == "ACAD_LAYOUT" || key == "ACAD_GROUP"
                || (key == "ACAD_IMAGE_DICT" && !cData->images.empty()))
                continue;

            if (key == "ACAD_MATERIAL"
                && rootEntryNames.count(key) != 0) {
                const auto material = std::find_if(
                    materialDictionaries.begin(), materialDictionaries.end(),
                    [&sourceEntry](const DRW_Dictionary& dictionary) {
                        return dictionary.handle == sourceEntry.m_handle;
                    });
                if (material == materialDictionaries.end()) {
                    delete dxfW;
                    dxfW = nullptr;
                    return false;
                }
                continue;
            }

            if (key.empty() || !rootEntryNames.insert(key).second
                || cData->namedObjectDictionary.handle == 0
                || cData->namedObjectDictionary.handle == DRW::NoHandle
                || sourceEntry.m_handle == 0
                || sourceEntry.m_handle == DRW::NoHandle) {
                delete dxfW;
                dxfW = nullptr;
                return false;
            }

            const DRW_Dictionary* targetDictionary = nullptr;
            const DRW_XRecord* targetXRecord = nullptr;
            std::size_t targetMatches = 0;
            for (const DRW_Dictionary& dictionary : cData->dictionaries) {
                if (dictionary.handle == sourceEntry.m_handle) {
                    targetDictionary = &dictionary;
                    ++targetMatches;
                }
            }
            for (const DRW_XRecord& record : cData->xRecords) {
                if (record.handle == sourceEntry.m_handle) {
                    targetXRecord = &record;
                    ++targetMatches;
                }
            }
            const bool dictionaryQueued = std::find(
                m_extensionDictionaryHandles.begin(),
                m_extensionDictionaryHandles.end(), sourceEntry.m_handle)
                    != m_extensionDictionaryHandles.end();
            const bool xRecordQueued = std::find(
                m_extensionXRecordHandles.begin(),
                m_extensionXRecordHandles.end(), sourceEntry.m_handle)
                    != m_extensionXRecordHandles.end();
            const bool closedDictionary = targetDictionary != nullptr
                && targetDictionary->parentHandle
                    == cData->namedObjectDictionary.handle
                && (dictionaryQueued
                    || (!requireResolvedOwners
                        && targetDictionary->m_entries.empty()));
            const bool closedXRecord = targetXRecord != nullptr
                && targetXRecord->parentHandle
                    == cData->namedObjectDictionary.handle
                && xRecordQueued;
            if (targetMatches != 1u
                || (!closedDictionary && !closedXRecord)) {
                if (requireResolvedOwners) {
                    delete dxfW;
                    dxfW = nullptr;
                    return false;
                }
                continue;
            }

            if (closedDictionary && !dictionaryQueued) {
                const bool alreadyQueued = std::any_of(
                    materialDictionaries.begin(), materialDictionaries.end(),
                    [sourceEntry](const DRW_Dictionary& dictionary) {
                        return dictionary.handle == sourceEntry.m_handle;
                    });
                if (!alreadyQueued)
                    materialDictionaries.push_back(*targetDictionary);
            }

            std::uint32_t emittedHandle = sourceEntry.m_handle;
            const auto remapped = handleRemap.find(emittedHandle);
            if (remapped != handleRemap.end())
                emittedHandle = remapped->second;
            rootEntries.emplace_back(
                sourceEntry.m_name, dxfW->toHexStrHandle(emittedHandle));
        }
    }
    if (cData->hasNamedObjectDictionary && !cData->layouts.empty()) {
        const auto layoutEntry = std::find_if(
            cData->namedObjectDictionary.m_entries.begin(),
            cData->namedObjectDictionary.m_entries.end(),
            [](const DRW_Dictionary::Entry& entry) {
                return layoutSymbolKey(entry.m_name) == "ACAD_LAYOUT";
            });
        if (layoutEntry == cData->namedObjectDictionary.m_entries.end()) {
            delete dxfW;
            dxfW = nullptr;
            return false;
        }
        std::uint32_t emittedHandle = layoutEntry->m_handle;
        const auto remapped = handleRemap.find(emittedHandle);
        if (remapped != handleRemap.end())
            emittedHandle = remapped->second;
        rootEntries.emplace_back("ACAD_LAYOUT",
                                 dxfW->toHexStrHandle(emittedHandle));
    }
    dxfW->setNamedDictObjects(materialDictionaries);
    dxfW->setRootDictEntries(rootEntries);
    bool success = dxfW->write(this, v, binary);
    if (!success && (debug || requireResolvedOwners)) {
        const DRW_OperationDiagnostic diagnostic = dxfW->getLastDiagnostic();
        std::cerr << "DXF export failed: " << diagnostic.code << ": "
                  << diagnostic.message << '\n';
    }
    delete dxfW;
    dxfW = nullptr;
    return success;
}

void dx_iface::writeEntity(DRW_Entity* e){
    if (e == nullptr) {
        failHatchPublication();
        return;
    }
    const auto selected = m_hatchTargets.find(e->handle);
    const std::uint32_t sourceHandle = e->handle;
    if (selected != m_hatchTargets.end()) {
        std::string owner;
        if (e->parentHandle != DRW::NoHandle) {
            const auto alias = m_hatchOwnerAliases.find(e->parentHandle);
            if (alias != m_hatchOwnerAliases.end()) owner = alias->second;
        } else if (!m_hatchOwnerContext.empty()) {
            owner = layoutSymbolKey(m_hatchOwnerContext);
        } else {
            owner = e->space == DRW::PaperSpace ? "*PAPER_SPACE" : "*MODEL_SPACE";
        }
        const HatchPublication& target = selected->second;
        if (target.published || target.emitted == 0 || target.type != e->eType
            || target.owner != owner
            || dxfW->getWritingContext()->writtenSourceEntityHandles.count(sourceHandle) != 0) {
            failHatchPublication();
            return;
        }
    }
    bool written = false;
    switch (e->eType) {
    case DRW::POINT:
        written = dxfW->writePoint(static_cast<DRW_Point*>(e));
        break;
    case DRW::LINE:
        written = dxfW->writeLine(static_cast<DRW_Line*>(e));
        break;
    case DRW::THREEDLINE:
        written = dxfW->write3DLine(static_cast<DRW_3DLine*>(e));
        break;
    case DRW::RAY:
        written = dxfW->writeRay(static_cast<DRW_Ray*>(e));
        break;
    case DRW::XLINE:
        written = dxfW->writeXline(static_cast<DRW_Xline*>(e));
        break;
    case DRW::CIRCLE:
        written = dxfW->writeCircle(static_cast<DRW_Circle*>(e));
        break;
    case DRW::ARC:
        written = dxfW->writeArc(static_cast<DRW_Arc*>(e));
        break;
    case DRW::SOLID:
        written = dxfW->writeSolid(static_cast<DRW_Solid*>(e));
        break;
    case DRW::DXF_TRACE:
        written = dxfW->writeTrace(static_cast<DRW_Trace*>(e));
        break;
    case DRW::E3DFACE:
        written = dxfW->write3dface(static_cast<DRW_3Dface*>(e));
        break;
    case DRW::ELLIPSE:
        written = dxfW->writeEllipse(static_cast<DRW_Ellipse*>(e));
        break;
    case DRW::LWPOLYLINE:
        written = dxfW->writeLWPolyline(static_cast<DRW_LWPolyline*>(e));
        break;
    case DRW::POLYLINE:
        written = dxfW->writePolyline(static_cast<DRW_Polyline*>(e));
        break;
    case DRW::MESH:
        written = dxfW->writeMesh(static_cast<DRW_Mesh*>(e));
        break;
    case DRW::HELIX:
        written = dxfW->writeHelix(static_cast<DRW_Helix*>(e));
        break;
    case DRW::SPLINE:
        written = dxfW->writeSpline(static_cast<DRW_Spline*>(e));
        break;
//    case RS2::EntitySplinePoints:
//        writeSplinePoints(static_cast<DRW_Point*>(e));
//        break;
//    case RS2::EntityVertex:
//        break;
    case DRW::INSERT:
        written = dxfW->writeInsert(static_cast<DRW_Insert*>(e));
        break;
    case DRW::MTEXT:
        written = dxfW->writeMText(static_cast<DRW_MText*>(e));
        break;
    case DRW::TEXT:
        if (auto* rtext = dynamic_cast<DRW_RText*>(e))
            written = dxfW->writeRText(rtext);
        else if (auto* arcText = dynamic_cast<DRW_ArcAlignedText*>(e))
            written = dxfW->writeArcAlignedText(arcText);
        else
            written = dxfW->writeText(static_cast<DRW_Text*>(e));
        break;
    case DRW::DIMLINEAR:
    case DRW::DIMALIGNED:
    case DRW::DIMANGULAR:
    case DRW::DIMANGULAR3P:
    case DRW::DIMRADIAL:
    case DRW::DIMDIAMETRIC:
    case DRW::DIMORDINATE:
    case DRW::DIMARC:
        written = dxfW->writeDimension(static_cast<DRW_Dimension*>(e));
        break;
    case DRW::LEADER:
        written = dxfW->writeLeader(static_cast<DRW_Leader*>(e));
        break;
    case DRW::HATCH:
        written = dxfW->writeHatch(static_cast<DRW_Hatch*>(e));
        break;
    case DRW::MPOLYGON:
        written = dxfW->writeMPolygon(static_cast<DRW_MPolygon*>(e));
        break;
    case DRW::VIEWPORT:
        written = dxfW->writeViewport(static_cast<DRW_Viewport*>(e));
        break;
    case DRW::IMAGE:
        written = dxfW->writeImage(static_cast<DRW_Image*>(e), static_cast<dx_ifaceImg*>(e)->path);
        break;
    case DRW::WIPEOUT:
        written = dxfW->writeWipeout(static_cast<DRW_Wipeout*>(e));
        break;
    case DRW::PLANESURFACE:
    case DRW::EXTRUDEDSURFACE:
    case DRW::REVOLVEDSURFACE:
    case DRW::SWEPTSURFACE:
    case DRW::LOFTEDSURFACE:
    case DRW::NURBSURFACE:
        written = dxfW->writeSurface(static_cast<DRW_Surface*>(e));
        break;
    case DRW::E3DSOLID:
    case DRW::REGION:
    case DRW::BODY:
        written = dxfW->writeModelerGeometry(static_cast<DRW_ModelerGeometry*>(e));
        break;
    default:
        break;
    }
    if (selected != m_hatchTargets.end()) {
        if (!written || e->handle != selected->second.emitted
            || dxfW->getWritingContext()->writtenSourceEntityHandles.count(sourceHandle) != 1)
            failHatchPublication();
        else
            selected->second.published = true;
    }
}
