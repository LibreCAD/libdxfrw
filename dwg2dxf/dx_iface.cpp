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
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include "dx_iface.h"
#include "libdwgr.h"
#include "libdxfrw.h"
#include "intern/dxfparserlimits.h"

namespace {

DRW::Version sourceVersionFromDxfHeader(
    const DRW_Header& header, DRW::Version fallback) {
    const auto acadVersion = header.vars.find("$ACADVER");
    if (acadVersion == header.vars.end() || acadVersion->second == nullptr
            || acadVersion->second->type() != DRW_Variant::STRING) {
        return fallback;
    }

    const std::string value = acadVersion->second->c_str();
    for (const auto& versionEntry : DRW::dwgVersionStrings) {
        if (versionEntry.first != nullptr && value == versionEntry.first)
            return versionEntry.second;
    }
    return fallback;
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

bool dx_iface::prepareExtensionObjectGraph(dx_data* data) {
    m_extensionDictionaryHandles.clear();
    m_extensionXRecordHandles.clear();
    if (data == nullptr)
        return false;

    try {
        std::unordered_map<std::uint32_t, const DRW_Dictionary*> dictionaries;
        std::unordered_map<std::uint32_t, const DRW_XRecord*> xrecords;
        std::unordered_set<std::uint32_t> sourceHandles;
        std::unordered_set<std::uint32_t> linetypeHandles;
        for (const DRW_LType& lineType : data->lineTypes) {
            if (lineType.handle == 0)
                continue;
            if (!sourceHandles.insert(lineType.handle).second)
                return false;
            linetypeHandles.insert(lineType.handle);
        }
        for (const DRW_Dictionary& dictionary : data->dictionaries) {
            if (dictionary.handle == 0)
                continue;
            if (!sourceHandles.insert(dictionary.handle).second
                || !dictionaries.emplace(dictionary.handle, &dictionary).second)
                return false;
        }
        for (const DRW_XRecord& record : data->xRecords) {
            if (record.handle == 0)
                continue;
            if (!sourceHandles.insert(record.handle).second
                || !xrecords.emplace(record.handle, &record).second)
                return false;
        }
        for (const DRW_EvaluationGraph& graph : data->evaluationGraphs) {
            if (graph.handle == 0
                || !sourceHandles.insert(graph.handle).second)
                return false;
        }

        std::unordered_set<std::uint32_t> visiting;
        std::unordered_set<std::uint32_t> visitedDictionaries;
        std::unordered_set<std::uint32_t> visitedXRecords;
        std::function<bool(std::uint32_t)> visitDictionary;
        std::function<bool(std::uint32_t)> visitXRecord;
        visitXRecord = [&](std::uint32_t handle) {
            const auto found = xrecords.find(handle);
            if (found == xrecords.end())
                return false;
            if (visiting.count(handle) != 0)
                return false;
            if (visitedXRecords.count(handle) != 0)
                return true;
            if (found->second->m_rawDataValid
                || (!found->second->m_handleValues.empty()
                    && std::any_of(found->second->m_handleValues.cbegin(),
                                   found->second->m_handleValues.cend(),
                                   [](const auto& value) {
                                       return value.first == 0;
                                   })))
                return false;
            visiting.insert(handle);
            m_extensionXRecordHandles.push_back(handle);
            if (found->second->xDictHandle != 0
                && !visitDictionary(found->second->xDictHandle)) {
                visiting.erase(handle);
                return false;
            }
            for (const std::uint32_t reactor : found->second->reactorHandles) {
                if (reactor == 0) {
                    visiting.erase(handle);
                    return false;
                }
                if (linetypeHandles.count(reactor) != 0)
                    continue;
                // Reactors are non-owning back-references; an object already
                // on the containment walk is present in the output closure.
                if (visiting.count(reactor) != 0)
                    continue;
                if (dictionaries.count(reactor) != 0) {
                    if (!visitDictionary(reactor)) {
                        visiting.erase(handle);
                        return false;
                    }
                } else if (xrecords.count(reactor) == 0
                           || !visitXRecord(reactor)) {
                    visiting.erase(handle);
                    return false;
                }
            }
            visiting.erase(handle);
            visitedXRecords.insert(handle);
            return true;
        };
        visitDictionary = [&](std::uint32_t handle) {
            const auto found = dictionaries.find(handle);
            if (found == dictionaries.end() || handle == 0xCu || handle == 0xDu)
                return false;
            if (visiting.count(handle) != 0)
                return false;
            if (visitedDictionaries.count(handle) != 0)
                return true;
            if (!visiting.insert(handle).second)
                return false;
            if (!found->second->hasCompleteDwgEntries()
                && !found->second->hasCompleteDxfEntries()) {
                visiting.erase(handle);
                return false;
            }
            m_extensionDictionaryHandles.push_back(handle);
            if (found->second->xDictHandle != 0
                && !visitDictionary(found->second->xDictHandle)) {
                visiting.erase(handle);
                return false;
            }
            for (const std::uint32_t reactor : found->second->reactorHandles) {
                if (reactor == 0) {
                    visiting.erase(handle);
                    return false;
                }
                if (linetypeHandles.count(reactor) != 0
                    || visiting.count(reactor) != 0)
                    continue;
                if (dictionaries.count(reactor) != 0) {
                    if (!visitDictionary(reactor)) {
                        visiting.erase(handle);
                        return false;
                    }
                } else if (xrecords.count(reactor) == 0
                           || !visitXRecord(reactor)) {
                    visiting.erase(handle);
                    return false;
                }
            }
            for (const DRW_Dictionary::Entry& entry : found->second->m_entries) {
                if (entry.m_handle == 0)
                    return false;
                if (dictionaries.count(entry.m_handle) != 0) {
                    if (!visitDictionary(entry.m_handle)) {
                        visiting.erase(handle);
                        return false;
                    }
                } else if (xrecords.count(entry.m_handle) != 0) {
                    if (!visitXRecord(entry.m_handle)) {
                        visiting.erase(handle);
                        return false;
                    }
                } else {
                    visiting.erase(handle);
                    return false;
                }
            }
            visiting.erase(handle);
            visitedDictionaries.insert(handle);
            return true;
        };

        for (const DRW_LType& lineType : data->lineTypes) {
            if (lineType.xDictHandle != 0
                && !visitDictionary(lineType.xDictHandle))
                return false;
            for (const std::uint32_t reactor : lineType.reactorHandles) {
                if (reactor == 0)
                    return false;
                if (linetypeHandles.count(reactor) != 0
                    || visiting.count(reactor) != 0)
                    continue;
                if (dictionaries.count(reactor) != 0) {
                    if (!visitDictionary(reactor))
                        return false;
                } else if (xrecords.count(reactor) == 0
                           || !visitXRecord(reactor)) {
                    return false;
                }
            }
        }

        const auto hasParent = [&dictionaries, &visitedDictionaries,
                                &visitedXRecords, &linetypeHandles](
                                   std::uint32_t parent) {
            return parent == 0 || parent == 0xCu || parent == 0xDu
                || linetypeHandles.count(parent) != 0
                || (dictionaries.count(parent) != 0
                    && visitedDictionaries.count(parent) != 0)
                || visitedXRecords.count(parent) != 0;
        };
        for (std::uint32_t handle : m_extensionDictionaryHandles) {
            const DRW_Dictionary* dictionary = dictionaries.at(handle);
            if (!hasParent(dictionary->parentHandle))
                return false;
        }
        for (std::uint32_t handle : m_extensionXRecordHandles) {
            const DRW_XRecord* record = xrecords.at(handle);
            if (!hasParent(record->parentHandle))
                return false;
            const auto isHandleCode = [](int code) {
                return (code >= 320 && code <= 369)
                    || (code >= 390 && code <= 399)
                    || (code >= 480 && code <= 481)
                    || code == 5 || code == 105
                    || (code >= 1005 && code <= 1009);
            };
            const auto validateHandleTarget =
                [&isHandleCode, &linetypeHandles, &visitedDictionaries,
                 &visitedXRecords](int code, std::uint64_t target) {
                    if (!isHandleCode(code))
                        return true;
                    if (target > std::numeric_limits<std::uint32_t>::max())
                        return false;
                    const std::uint32_t targetHandle =
                        static_cast<std::uint32_t>(target);
                    return targetHandle == 0
                        || linetypeHandles.count(targetHandle) != 0
                        || visitedDictionaries.count(targetHandle) != 0
                        || visitedXRecords.count(targetHandle) != 0;
                };
            const auto validateHandleValue =
                [&isHandleCode, &validateHandleTarget](
                    const DRW_Variant& value) {
                    const int code = value.code();
                    if (!isHandleCode(code))
                        return true;
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
                        return false;
                }
            } else {
                for (const DRW_Variant& value : record->m_values) {
                    if (!validateHandleValue(value))
                        return false;
                }
                for (const auto& value : record->m_handleValues) {
                    if (!validateHandleTarget(value.first, value.second))
                        return false;
                }
            }
        }
        return true;
    } catch (...) {
        m_extensionDictionaryHandles.clear();
        m_extensionXRecordHandles.clear();
        return false;
    }
}


bool dx_iface::fileImport(const std::string& fileI, dx_data *fData, bool debug){
    unsigned int found = fileI.find_last_of(".");
    std::string fileExt = fileI.substr(found+1);
    std::transform(fileExt.begin(), fileExt.end(),fileExt.begin(), ::toupper);
    cData = fData;
    currentBlock = cData->mBlock;

    if (fileExt == "DXF"){
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
        //loads dwg
        dwgR* dwg = new dwgR(fileI.c_str());
        if (debug) {
            dwg->setDebug(DRW::DebugLevel::Debug);
        }
        bool success = dwg->read(this, false);
        if (success)
            fData->sourceVersion = dwg->getVersion();
        if (!success) {
            std::cout << "DWG file error: format " << dwg->getVersion() << " error " << dwg->getError() << std::endl;
        }
        delete dwg;
        return success;
    }
    std::cout << "file extension can be dxf or dwg" << std::endl;
    return false;
}

bool dx_iface::fileExport(const std::string& file, DRW::Version v, bool binary, dx_data *fData, bool debug){
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
    if (!cData->dynamicBlockObjects.empty()) {
        reportPreflightFailure(
            "dynamic-block objects have no qualified DXF serializer");
        return false;
    }
    if (!prepareExtensionObjectGraph(cData)) {
        reportPreflightFailure("extension-object graph validation");
        return false;
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
        dxfW->setNamedDictObjects(materialDictionaries);
        dxfW->setRootDictEntries(materialRootEntries);

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
    for (const DRW_XRecord& record : cData->xRecords) {
        if (record.handle != 0 && !dxfW->reserveHandle(record.handle)) {
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
    for (std::uint32_t handle : m_extensionDictionaryHandles) {
        if (fixedHandles.count(handle) == 0)
            continue;
        const std::uint32_t replacement = dxfW->allocHandle();
        if (replacement == 0 || !dxfW->reserveHandle(replacement)) {
            delete dxfW;
            dxfW = nullptr;
            return false;
        }
        handleRemap.emplace(handle, replacement);
    }
    for (std::uint32_t handle : m_extensionXRecordHandles) {
        if (fixedHandles.count(handle) == 0)
            continue;
        const std::uint32_t replacement = dxfW->allocHandle();
        if (replacement == 0 || !dxfW->reserveHandle(replacement)) {
            delete dxfW;
            dxfW = nullptr;
            return false;
        }
        handleRemap.emplace(handle, replacement);
    }
    dxfW->setHandleRemap(handleRemap);
    bool success = dxfW->write(this, v, binary);
    if (!success && debug) {
        const DRW_OperationDiagnostic diagnostic = dxfW->getLastDiagnostic();
        std::cerr << "DXF export failed: " << diagnostic.code << ": "
                  << diagnostic.message << '\n';
    }
    delete dxfW;
    dxfW = nullptr;
    return success;
}

void dx_iface::writeEntity(DRW_Entity* e){
    switch (e->eType) {
    case DRW::POINT:
        dxfW->writePoint(static_cast<DRW_Point*>(e));
        break;
    case DRW::LINE:
        dxfW->writeLine(static_cast<DRW_Line*>(e));
        break;
    case DRW::THREEDLINE:
        dxfW->write3DLine(static_cast<DRW_3DLine*>(e));
        break;
    case DRW::RAY:
        dxfW->writeRay(static_cast<DRW_Ray*>(e));
        break;
    case DRW::XLINE:
        dxfW->writeXline(static_cast<DRW_Xline*>(e));
        break;
    case DRW::CIRCLE:
        dxfW->writeCircle(static_cast<DRW_Circle*>(e));
        break;
    case DRW::ARC:
        dxfW->writeArc(static_cast<DRW_Arc*>(e));
        break;
    case DRW::SOLID:
        dxfW->writeSolid(static_cast<DRW_Solid*>(e));
        break;
    case DRW::DXF_TRACE:
        dxfW->writeTrace(static_cast<DRW_Trace*>(e));
        break;
    case DRW::E3DFACE:
        dxfW->write3dface(static_cast<DRW_3Dface*>(e));
        break;
    case DRW::ELLIPSE:
        dxfW->writeEllipse(static_cast<DRW_Ellipse*>(e));
        break;
    case DRW::LWPOLYLINE:
        dxfW->writeLWPolyline(static_cast<DRW_LWPolyline*>(e));
        break;
    case DRW::POLYLINE:
        dxfW->writePolyline(static_cast<DRW_Polyline*>(e));
        break;
    case DRW::MESH:
        dxfW->writeMesh(static_cast<DRW_Mesh*>(e));
        break;
    case DRW::HELIX:
        dxfW->writeHelix(static_cast<DRW_Helix*>(e));
        break;
    case DRW::SPLINE:
        dxfW->writeSpline(static_cast<DRW_Spline*>(e));
        break;
//    case RS2::EntitySplinePoints:
//        writeSplinePoints(static_cast<DRW_Point*>(e));
//        break;
//    case RS2::EntityVertex:
//        break;
    case DRW::INSERT:
        dxfW->writeInsert(static_cast<DRW_Insert*>(e));
        break;
    case DRW::MTEXT:
        dxfW->writeMText(static_cast<DRW_MText*>(e));
        break;
    case DRW::TEXT:
        if (auto* rtext = dynamic_cast<DRW_RText*>(e))
            dxfW->writeRText(rtext);
        else if (auto* arcText = dynamic_cast<DRW_ArcAlignedText*>(e))
            dxfW->writeArcAlignedText(arcText);
        else
            dxfW->writeText(static_cast<DRW_Text*>(e));
        break;
    case DRW::DIMLINEAR:
    case DRW::DIMALIGNED:
    case DRW::DIMANGULAR:
    case DRW::DIMANGULAR3P:
    case DRW::DIMRADIAL:
    case DRW::DIMDIAMETRIC:
    case DRW::DIMORDINATE:
        dxfW->writeDimension(static_cast<DRW_Dimension*>(e));
        break;
    case DRW::LEADER:
        dxfW->writeLeader(static_cast<DRW_Leader*>(e));
        break;
    case DRW::HATCH:
        dxfW->writeHatch(static_cast<DRW_Hatch*>(e));
        break;
    case DRW::MPOLYGON:
        dxfW->writeMPolygon(static_cast<DRW_MPolygon*>(e));
        break;
    case DRW::VIEWPORT:
        dxfW->writeViewport(static_cast<DRW_Viewport*>(e));
        break;
    case DRW::IMAGE:
        dxfW->writeImage(static_cast<DRW_Image*>(e), static_cast<dx_ifaceImg*>(e)->path);
        break;
    case DRW::PLANESURFACE:
    case DRW::EXTRUDEDSURFACE:
    case DRW::REVOLVEDSURFACE:
    case DRW::SWEPTSURFACE:
    case DRW::LOFTEDSURFACE:
    case DRW::NURBSURFACE:
        dxfW->writeSurface(static_cast<DRW_Surface*>(e));
        break;
    case DRW::E3DSOLID:
    case DRW::REGION:
    case DRW::BODY:
        dxfW->writeModelerGeometry(static_cast<DRW_ModelerGeometry*>(e));
        break;
    default:
        break;
    }
}
