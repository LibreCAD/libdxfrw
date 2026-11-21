/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**  Copyright (C) 2026 Dongxu Li (github.com/dxli)                            **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include "dwgwriter15.h"
#include "dwgtemplate.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <map>
#include <new>
#include <type_traits>
#include <typeinfo>
#include <set>

#include "../drw_base.h"
#include "../drw_datastorage.h"
#include "../drw_entities.h"
#include "../drw_objects.h"
#include "drw_dbg.h"
#include "drw_reserve.h"
#include "dwgbuffer.h"
#include "dwgbufferw.h"
#include "dwg_header_time.h"
#include "dwg_default_material.h"
#include "dwg_fixed_handles.h"
#include "dwgobjectframe.h"
#include "dwgreader.h"
#include "dwgsafety.h"
#include "dwgutil.h"
#include "sabheader.h"

bool drwHasEncodableDwgVportAngles(const DRW_Vport& entity) noexcept;
bool drwHasEncodableDwgViewTwist(const DRW_View& entity) noexcept;

namespace {

constexpr std::uint32_t kMaxDwgBlockRecordPreviewBytes = 0xA00000u;
constexpr int kDwgBlockRecordFlagsMask = 0x7Fu;

template <std::size_t N>
bool canonicalHandleTailMatches(const DRW::DwgObjectFrameReceipt& frame,
    DRW::Version version, std::uint32_t handle, std::uint16_t objectType,
    const std::array<std::pair<std::uint8_t, std::uint64_t>, N>& expected) {
    if (!frame.valid || (version != DRW::AC1021 && version != DRW::AC1024
                         && version != DRW::AC1027)
        || frame.version != version || frame.objectHandle != handle
        || frame.classNumber != objectType)
        return false;
    // R2007 LAYER handles are merged at their exact bit boundary into the
    // data segment; other selected records keep a detached handle segment.
    const bool mergedLayerHandles = version == DRW::AC1021
        && objectType == DRW::DwgLayerObjectType;
    const auto segment = mergedLayerHandles ? DRW::DwgObjectSerializedSegment::Data
                                            : DRW::DwgObjectSerializedSegment::Handles;
    const auto segmentBits = mergedLayerHandles ? frame.dataBitSize : frame.handleBitSize;
    std::size_t index = 0u;
    std::uint64_t priorEnd = 0u;
    for (const auto& occurrence : frame.occurrences) {
        if (occurrence.stream != DRW::DwgObjectHandleStream::Handles) continue;
        if (index >= expected.size() || occurrence.objectHandle != handle
            || occurrence.serializedSegment != segment
            || occurrence.token.code != expected[index].first
            || occurrence.token.reference != expected[index].second
            || occurrence.token.startBit < priorEnd
            || occurrence.token.endBit <= occurrence.token.startBit
            || occurrence.token.endBit > segmentBits) return false;
        priorEnd = occurrence.token.endBit;
        ++index;
    }
    return index == expected.size();
}

bool validAc1018TableBlockRecordReference(const dwgHandle& handle) {
    const bool normalized = handle.code == DRW::DwgNullReference
        && handle.size == 0u && handle.rawCode == 0u && handle.ref64 == 0u;
    if (normalized)
        return true;
    if (handle.code != DRW::DwgHardPointer
        || handle.rawCode != DRW::DwgHardPointer || handle.ref == 0u
        || handle.ref64 != handle.ref || handle.size == 0u
        || handle.size > sizeof(handle.ref))
        return false;

    std::uint32_t remaining = handle.ref;
    std::uint8_t minimalSize = 1u;
    while (remaining > 0xFFu) {
        remaining >>= 8u;
        ++minimalSize;
    }
    return handle.size == minimalSize;
}

// ODA 24.2.2.7.1: AutoCAD needs the schema-search entries to find
// DataStorage-backed entities. Validate the exact modeler associations while
// keeping unrelated opaque records available for raw section replay.
bool validateModelerDataStorageSearch(
    const DRW_DataStorageSection& section, const std::uint8_t* bytes,
    std::size_t byteLength,
    const std::set<std::pair<std::uint32_t, std::uint64_t>>& required) {
    if (required.empty())
        return true;
    if (bytes == nullptr || section.searchSegmentIndex < 0
        || static_cast<std::size_t>(section.searchSegmentIndex)
               >= section.segments.size())
        return false;
    const auto& segment = section.segments[section.searchSegmentIndex];
    if (segment.kind != DRW_DataStorageSegmentKind::Search
        || segment.payloadOffset > byteLength
        || segment.payloadByteLength > byteLength - segment.payloadOffset)
        return false;
    std::size_t cursor = static_cast<std::size_t>(segment.payloadOffset);
    const std::size_t end = cursor + segment.payloadByteLength;
    const auto read = [&](unsigned length, std::uint64_t& value) {
        if (length > end - cursor)
            return false;
        value = 0;
        for (unsigned n = 0; n < length; ++n)
            value |= static_cast<std::uint64_t>(bytes[cursor++]) << (8u * n);
        return true;
    };
    try {
        // ODA's sequential sorted-index profile is interpreted in data-index
        // order, per schema. Bind each row through its exact segment/local
        // offset rather than relying on record-vector order.
        using Location = std::pair<std::uint32_t, std::uint32_t>;
        using Expected = std::map<std::uint64_t, std::uint64_t>;
        std::map<std::uint32_t, Expected> expected;
        for (const auto& pair : required)
            expected.emplace(pair.first, Expected{});
        std::map<Location, const DRW_DataStorageRecord*> byLocation;
        for (const auto& record : section.records) {
            if (expected.count(record.schemaIndex) != 0
                && !byLocation.emplace(Location{record.segmentIndex, record.localOffset},
                                       &record).second)
                return false;
        }
        std::set<Location> usedLocations;
        for (const auto& entry : section.dataIndexEntries) {
            auto schema = expected.find(entry.schemaIndex);
            if (schema == expected.end() || entry.segmentIndex == 0u)
                continue;
            const Location location{entry.segmentIndex, entry.localOffset};
            const auto record = byLocation.find(location);
            if (record == byLocation.end() || !usedLocations.insert(location).second
                || record->second->schemaIndex != entry.schemaIndex
                || !schema->second.emplace(record->second->handle,
                                           schema->second.size()).second)
                return false;
        }
        if (usedLocations.size() != byLocation.size())
            return false;
        for (const auto& pair : required) {
            const auto schema = expected.find(pair.first);
            if (schema == expected.end() || schema->second.count(pair.second) != 1u)
                return false;
        }

        std::uint64_t schemaCount = 0;
        if (!read(4, schemaCount) || schemaCount > (end - cursor) / 16u)
            return false;
        std::set<std::uint32_t> seenSchemas;
        std::set<std::pair<std::uint32_t, std::uint64_t>> found;
        for (std::uint64_t schema = 0; schema < schemaCount; ++schema) {
            std::uint64_t name = 0, sortedCount = 0;
            if (!read(4, name) || !read(8, sortedCount)
                || sortedCount > (end - cursor) / 8u)
                return false;
            const auto schemaName = static_cast<std::uint32_t>(name);
            const auto wanted = expected.find(schemaName);
            if (wanted != expected.end()
                && (!seenSchemas.insert(schemaName).second
                    || sortedCount != wanted->second.size()))
                return false;
            for (std::uint64_t i = 0, index = 0; i < sortedCount; ++i) {
                if (!read(8, index) || (wanted != expected.end() && index != i))
                    return false;
            }
            std::uint64_t groups = 0;
            if (!read(4, groups) || groups > (end - cursor) / 4u)
                return false;
            if (groups != 0) {
                std::uint64_t unknown = 0;
                if (!read(4, unknown)) return false;
            }
            for (std::uint64_t group = 0; group < groups; ++group) {
                std::uint64_t entries = 0;
                if (!read(4, entries) || entries > (end - cursor) / 16u)
                    return false;
                for (std::uint64_t entry = 0; entry < entries; ++entry) {
                    std::uint64_t handle = 0, indexCount = 0;
                    if (!read(8, handle) || !read(8, indexCount)
                        || indexCount > (end - cursor) / 8u)
                        return false;
                    Expected::const_iterator record;
                    if (wanted != expected.end()) {
                        record = wanted->second.find(handle);
                        if (record == wanted->second.end() || indexCount != 1u
                            || !found.emplace(schemaName, handle).second)
                            return false;
                    }
                    for (std::uint64_t i = 0, index = 0; i < indexCount; ++i) {
                        if (!read(8, index)
                            || (wanted != expected.end() && index != record->second))
                            return false;
                    }
                }
            }
        }
        if (seenSchemas.size() != expected.size())
            return false;
        std::size_t expectedCount = 0;
        for (const auto& schema : expected)
            expectedCount += schema.second.size();
        return found.size() == expectedCount;
    } catch (...) {
        return false;
    }
}

struct DataStoragePayloadInspection {
    bool hasAcisMarker = false;
    std::uint32_t acisMarkerOffset = 0;
};

bool inspectDataStorageRecordPayload(
    const std::uint8_t* sectionBytes, std::size_t sectionByteLength,
    const DRW_DataStorageSection& section,
    const DRW_DataStorageRecord& record,
    const std::vector<std::uint8_t>* expectedPayload,
    DataStoragePayloadInspection& inspection) {
    static constexpr char marker[] = "ACIS BinaryFile";
    constexpr std::size_t markerLength = sizeof(marker) - 1u;
    constexpr std::uint64_t segmentHeaderSize =
        DRW_DataStorageConst::SEGMENT_HEADER_SIZE;
    constexpr std::uint64_t blobPageHeaderSize =
        DRW_DataStorageConst::DATA_BLOB_PAGE_HEADER_SIZE;

    const std::uint64_t expectedLength = record.isBlobReference
        ? record.blobTotalByteLength : record.dataByteLength;
    if (sectionBytes == nullptr || expectedLength
            > DRW_DataStorageConst::PAYLOAD_BLOB_SECTION_CAP
        || (expectedPayload != nullptr
            && expectedPayload->size() != expectedLength))
        return false;

    std::array<std::size_t, markerLength> prefix{};
    for (std::size_t index = 1u, matched = 0u; index < markerLength; ++index) {
        while (matched != 0u && marker[index] != marker[matched])
            matched = prefix[matched - 1u];
        if (marker[index] == marker[matched])
            ++matched;
        prefix[index] = matched;
    }

    std::uint64_t payloadOffset = 0;
    std::size_t expectedOffset = 0;
    std::size_t matchedMarkerBytes = 0;
    const auto consumeSpan = [&](std::uint64_t sourceOffset,
                                 std::uint64_t byteLength) {
        if (sourceOffset > sectionByteLength
            || byteLength > sectionByteLength - sourceOffset
            || payloadOffset > expectedLength
            || byteLength > expectedLength - payloadOffset
            || (expectedPayload != nullptr
                && (expectedOffset > expectedPayload->size()
                    || byteLength > expectedPayload->size() - expectedOffset)))
            return false;

        const std::uint8_t* source = sectionBytes
            + static_cast<std::size_t>(sourceOffset);
        if (expectedPayload != nullptr && byteLength != 0u
            && std::memcmp(source, expectedPayload->data() + expectedOffset,
                           static_cast<std::size_t>(byteLength)) != 0)
            return false;

        for (std::uint64_t index = 0; index < byteLength; ++index) {
            const char value = static_cast<char>(source[index]);
            while (matchedMarkerBytes != 0u
                   && value != marker[matchedMarkerBytes])
                matchedMarkerBytes = prefix[matchedMarkerBytes - 1u];
            if (value == marker[matchedMarkerBytes])
                ++matchedMarkerBytes;
            if (matchedMarkerBytes == markerLength) {
                if (!inspection.hasAcisMarker) {
                    const std::uint64_t markerStart = payloadOffset + index
                        + 1u - markerLength;
                    if (markerStart > UINT32_MAX)
                        return false;
                    inspection.hasAcisMarker = true;
                    inspection.acisMarkerOffset =
                        static_cast<std::uint32_t>(markerStart);
                }
                matchedMarkerBytes = prefix[matchedMarkerBytes - 1u];
            }
        }
        payloadOffset += byteLength;
        if (expectedPayload != nullptr)
            expectedOffset += static_cast<std::size_t>(byteLength);
        return true;
    };

    if (!record.isBlobReference) {
        if (static_cast<std::size_t>(record.segmentIndex)
                >= section.segments.size())
            return false;
        const DRW_DataStorageSegment& segment =
            section.segments[record.segmentIndex];
        if (segment.kind != DRW_DataStorageSegmentKind::Data
            || segment.offset > sectionByteLength
            || segment.size > sectionByteLength - segment.offset)
            return false;
        const std::uint64_t segmentEnd = segment.offset + segment.size;
        if (segment.offset > UINT64_MAX - segmentHeaderSize)
            return false;
        const std::uint64_t segmentPayloadStart =
            segment.offset + segmentHeaderSize;
        if (record.dataOffset < segmentPayloadStart
            || record.dataOffset > segmentEnd
            || record.dataByteLength > segmentEnd - record.dataOffset
            || !consumeSpan(record.dataOffset, record.dataByteLength))
            return false;
    } else {
        if (record.blobPageCount == 0u
            || record.blobPageCount != record.blobPages.size())
            return false;
        for (const DRW_DataStorageBlobPage& page : record.blobPages) {
            if (static_cast<std::size_t>(page.segmentIndex)
                    >= section.segments.size())
                return false;
            const DRW_DataStorageSegment& segment =
                section.segments[page.segmentIndex];
            if (segment.kind != DRW_DataStorageSegmentKind::Blob
                || segment.offset > sectionByteLength
                || segment.size > sectionByteLength - segment.offset
                || segment.offset
                       > UINT64_MAX - segmentHeaderSize - blobPageHeaderSize)
                return false;
            const std::uint64_t segmentEnd = segment.offset + segment.size;
            const std::uint64_t pageDataOffset = segment.offset
                + segmentHeaderSize + blobPageHeaderSize;
            if (pageDataOffset > segmentEnd
                || page.byteLength > segmentEnd - pageDataOffset
                || !consumeSpan(pageDataOffset, page.byteLength))
                return false;
        }
    }

    return payloadOffset == expectedLength
        && (expectedPayload == nullptr
            || expectedOffset == expectedPayload->size());
}

bool isCompoundEntityChild(const DRW_Entity& entity) {
    return dynamic_cast<const DRW_Attrib*>(&entity) != nullptr
        || dynamic_cast<const DRW_SeqEnd*>(&entity) != nullptr
        || dynamic_cast<const DRW_Vertex*>(&entity) != nullptr;
}

std::uint64_t blockInsertReferenceKey(std::uint32_t ownerHandle,
                                      std::uint32_t entityHandle) {
    return (static_cast<std::uint64_t>(ownerHandle) << 32u) | entityHandle;
}

/// File-header CRC seed-XOR adjustment (matches reader at
/// dwgreader15.cpp:94-106). Indexed by `num_sections`.
std::uint16_t seedXorForCount(std::uint8_t numSections) {
    switch (numSections) {
        case 3: return 0xA598;
        case 4: return 0x8101;
        case 5: return 0x3CC4;
        case 6: return 0x8461;
        default: return 0;
    }
}

/// File-format `recno` values (NOT the libdxfrw `secEnum::DWGSection`
/// enum indices, which sort alphabetically and put HEADER at 2,
/// HANDLES at 14, etc.).  Reader maps recno → secEnum at
/// dwgreader15.cpp:60-83.

/// Canonical reserved-handle table for R2000 control objects.  Matches
/// `HandleAllocator::seedReserved` and the Phase 3 sub-plan §"Reserved
/// (fixed) handles".  0x04 is intentionally unused.
namespace reservedHandle {
    constexpr std::uint32_t BLOCK_CONTROL              = DRW::DwgBlockControlHandle;
    constexpr std::uint32_t LAYER_CONTROL              = DRW::DwgLayerControlHandle;
    constexpr std::uint32_t STYLE_CONTROL              = DRW::DwgStyleControlHandle;
    constexpr std::uint32_t LTYPE_CONTROL              = DRW::DwgLtypeControlHandle;
    constexpr std::uint32_t VIEW_CONTROL               = DRW::DwgViewControlHandle;
    constexpr std::uint32_t UCS_CONTROL                = DRW::DwgUcsControlHandle;
    constexpr std::uint32_t VPORT_CONTROL              =
        DRW::DwgVportControlHandle;
    constexpr std::uint32_t APPID_CONTROL              = DRW::DwgAppIdControlHandle;
    constexpr std::uint32_t DIMSTYLE_CONTROL           = DRW::DwgDimstyleControlHandle;
    constexpr std::uint32_t VPORT_ENTITY_HEADER_CONTROL =
        DRW::DwgViewportEntityHeaderControlHandle;
    // Reserved-block / linetype handles emitted in the HEADER section's
    // trailing 5-handle block (and as phantom entries in
    // BLOCK_CONTROL / LTYPE_CONTROL).  Phase 3e populates the records.
    constexpr std::uint32_t LTYPE_BYBLOCK = DRW::DwgLtypeByBlockHandle;
    constexpr std::uint32_t LTYPE_BYLAYER = DRW::DwgLtypeByLayerHandle;
    constexpr std::uint32_t LTYPE_CONTINUOUS = DRW::DwgLtypeContinuousHandle;
    constexpr std::uint32_t BLOCK_MODEL_SPACE = DRW::DwgModelSpaceBlockRecordHandle;
    constexpr std::uint32_t BLOCK_PAPER_SPACE = DRW::DwgPaperSpaceBlockRecordHandle;
    constexpr std::uint32_t BLOCK_MODEL_ENTITY = DRW::DwgModelSpaceBlockEntityHandle;
    constexpr std::uint32_t BLOCK_MODEL_END = DRW::DwgModelSpaceEndBlockEntityHandle;
    constexpr std::uint32_t BLOCK_PAPER_ENTITY = DRW::DwgPaperSpaceBlockEntityHandle;
    constexpr std::uint32_t BLOCK_PAPER_END = DRW::DwgPaperSpaceEndBlockEntityHandle;
}

bool controlEntryCountUsesBitShort(std::uint16_t objectType) {
    switch (objectType) {
    case DRW::DwgLTypeControlObjectType:
    case DRW::DwgUcsControlObjectType:
    case DRW::DwgVPortControlObjectType:
    case DRW::DwgDimStyleControlObjectType:
        return true;
    default:
        return false;
    }
}

bool hasR2007DetachedControlHandles(std::uint16_t type) noexcept {
    switch (type) {
    case DRW::DwgLayerControlObjectType:
    case DRW::DwgStyleControlObjectType:
    case DRW::DwgLTypeControlObjectType:
    case DRW::DwgViewControlObjectType:
    case DRW::DwgUcsControlObjectType:
    case DRW::DwgVPortControlObjectType:
    case DRW::DwgAppIdControlObjectType:
    case DRW::DwgDimStyleControlObjectType:
        return true;
    default:
        return false;
    }
}

bool writeControlEntryCount(dwgBufferW& body, std::uint16_t objectType,
                            std::uint32_t count, DRW::Version version) {
    // ODA v5.4.1 §20.4.57 specifies BL for LTYPE_CONTROL.
    if ((version == DRW::AC1015 || version == DRW::AC1018
         || version == DRW::AC1021 || version == DRW::AC1024 || version == DRW::AC1027
         || version == DRW::AC1032)
        && objectType == DRW::DwgLTypeControlObjectType) {
        if (count > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()))
            return false;
        body.putBitLong(static_cast<std::int32_t>(count));
        return body.isGood();
    }
    if (controlEntryCountUsesBitShort(objectType)) {
        if (count > std::numeric_limits<std::uint16_t>::max())
            return false;
        body.putBitShort(static_cast<std::uint16_t>(count));
    } else {
        if (count > static_cast<std::uint32_t>(
                         std::numeric_limits<std::int32_t>::max()))
            return false;
        body.putBitLong(static_cast<std::int32_t>(count));
    }
    return body.isGood();
}

bool isReplayableFixedModelerRawEntity(const DRW_UnsupportedObject& object) {
    return object.m_isEntity && !object.m_isCustomClass
        && (object.m_objectType == 34   // VIEWPORT (paper-space; no typed RS entity)
            || object.m_objectType == 37 || object.m_objectType == 38
            || object.m_objectType == 39);
}

bool isReplayableFixedModelerBodyRawEntity(
    const DRW_UnsupportedObject& object) {
    return object.m_isEntity && !object.m_isCustomClass
        && (object.m_objectType == 37 || object.m_objectType == 38
            || object.m_objectType == 39);
}

bool isReplayableFixedEntityShellRawEntity(
    const DRW_UnsupportedObject& object) {
    return object.m_isEntity && !object.m_isCustomClass
        && DRW_UnsupportedObject::isFixedEntityShellType(object.m_objectType);
}

bool isReplayableSurfaceRawEntity(const DRW_UnsupportedObject& object) {
    if (!object.m_isEntity || !object.m_isCustomClass)
        return false;
    return object.m_recordName == "PLANESURFACE"
        || object.m_recordName == "EXTRUDEDSURFACE"
        || object.m_recordName == "REVOLVEDSURFACE"
        || object.m_recordName == "SWEPTSURFACE"
        || object.m_recordName == "LOFTEDSURFACE"
        || object.m_recordName == "NURBSURFACE";
}

bool isReplayableCustomRawEntity(const DRW_UnsupportedObject& object) {
    const std::uint32_t blockOwnerHandle =
        object.m_blockOwnerHandle != DRW::NoHandle
            ? object.m_blockOwnerHandle : object.m_parentHandle;
    return object.m_isEntity && object.m_isCustomClass
        && (blockOwnerHandle != DRW::NoHandle
            || isReplayableSurfaceRawEntity(object))
        && (!object.m_recordName.empty() || !object.m_className.empty());
}

std::uint32_t rawBlockOwnerHandle(const DRW_UnsupportedObject& object) {
    return object.m_blockOwnerHandle != DRW::NoHandle
        ? object.m_blockOwnerHandle : object.m_parentHandle;
}

std::uint16_t underlayDefinitionClassNum(
    const DRW_UnderlayDefinition& definition) {
    switch (definition.kind) {
    case DRW_UnderlayDefinition::DGN:
        return DRW_UnderlayDefinition::kDwgClassNumDgn;
    case DRW_UnderlayDefinition::DWF:
        return DRW_UnderlayDefinition::kDwgClassNumDwf;
    case DRW_UnderlayDefinition::PDF:
        return DRW_UnderlayDefinition::kDwgClassNumPdf;
    default:
        return 0;
    }
}

/// Build a soft-pointer (code 4) handle with the minimum-width ref.
dwgHandle makeSoftPointer(std::uint32_t ref) {
    dwgHandle h;
    h.code = 4;
    h.ref  = ref;
    h.size = 0;
    if (ref != 0) {
        std::uint32_t t = ref;
        while (t != 0) { t >>= 8; ++h.size; }
    }
    return h;
}

/// Build a soft-ownership (code 2) handle with the minimum-width ref.
dwgHandle makeSoftOwner(std::uint32_t ref) {
    dwgHandle h;
    h.code = 2;
    h.ref  = ref;
    h.size = 0;
    if (ref != 0) {
        std::uint32_t t = ref;
        while (t != 0) { t >>= 8; ++h.size; }
    }
    return h;
}

/// Build an own-handle entry (code 0) — the form used for an object's own handle.
dwgHandle makeOwnHandle(std::uint32_t ref) {
    dwgHandle h;
    h.code = 0;
    h.ref  = ref;
    h.size = 0;
    if (ref != 0) {
        std::uint32_t t = ref;
        while (t != 0) { t >>= 8; ++h.size; }
    }
    return h;
}

void putAuxRL(std::vector<std::uint8_t>& v, std::uint32_t x) {
    v.push_back(static_cast<std::uint8_t>(x));
    v.push_back(static_cast<std::uint8_t>(x >> 8));
    v.push_back(static_cast<std::uint8_t>(x >> 16));
    v.push_back(static_cast<std::uint8_t>(x >> 24));
}

void putAuxRS(std::vector<std::uint8_t>& v, std::uint16_t x) {
    v.push_back(static_cast<std::uint8_t>(x));
    v.push_back(static_cast<std::uint8_t>(x >> 8));
}

double headerDoubleVar(const DRW_Header *header, const std::string& key) {
    if (header == nullptr)
        return 0.0;
    auto it = header->vars.find(key);
    if (it == header->vars.end() || it->second == nullptr
        || it->second->type() != DRW_Variant::DOUBLE) {
        return 0.0;
    }
    return it->second->d_val();
}

bool putAuxDate(std::vector<std::uint8_t>& v, double stored) {
    std::int32_t day = 0;
    std::int32_t msec = 0;
    if (!dwgHeaderTime::split(stored, day, msec))
        return false;
    putAuxRL(v, static_cast<std::uint32_t>(day));
    putAuxRL(v, static_cast<std::uint32_t>(msec));
    return true;
}

std::vector<std::uint8_t> buildR2000AuxHeaderContent(const DRW_Header *header) {
    constexpr std::uint16_t rawVersion = 23;  // AC1015
    std::vector<std::uint8_t> v;
    v.reserve(111);
    v.push_back(0xff);
    v.push_back(0x77);
    v.push_back(0x01);
    putAuxRS(v, rawVersion);
    putAuxRS(v, 0);                     // maintenance version
    putAuxRL(v, 1);                     // number of saves
    putAuxRL(v, static_cast<std::uint32_t>(-1));
    putAuxRS(v, 1);                     // saves part 1
    putAuxRS(v, 0);                     // saves part 2
    putAuxRL(v, 0);
    putAuxRS(v, rawVersion);
    putAuxRS(v, 0);
    putAuxRS(v, rawVersion);
    putAuxRS(v, 0);
    putAuxRS(v, 0x0005);
    putAuxRS(v, 0x0893);
    putAuxRS(v, 0x0005);
    putAuxRS(v, 0x0893);
    putAuxRS(v, 0);
    putAuxRS(v, 1);
    for (int i = 0; i < 5; ++i)
        putAuxRL(v, 0);

    if (!putAuxDate(v, headerDoubleVar(header, "TDCREATE"))
        || !putAuxDate(v, headerDoubleVar(header, "TDUPDATE")))
        return {};

    const std::uint32_t handSeed = header ? header->getHandSeed() : 0;
    putAuxRL(v, handSeed <= 0x7fffffffu ? handSeed : static_cast<std::uint32_t>(-1));
    putAuxRL(v, 0);                     // educational plot stamp
    putAuxRS(v, 0);
    putAuxRS(v, 1);
    putAuxRL(v, 0);
    putAuxRL(v, 0);
    putAuxRL(v, 0);
    putAuxRL(v, 1);                     // number of saves
    putAuxRL(v, 0);
    putAuxRL(v, 0);
    putAuxRL(v, 0);
    putAuxRL(v, 0);
    return v;
}

/// Build a hard-ownership (code 3) handle — the form used for an
/// xdictionary and for objects that are required by their owner.
dwgHandle makeHardOwner(std::uint32_t ref) {
    dwgHandle h;
    h.code = 3;
    h.ref  = ref;
    h.size = 0;
    if (ref != 0) {
        std::uint32_t t = ref;
        while (t != 0) { t >>= 8; ++h.size; }
    }
    return h;
}

/// Build a hard-pointer (code 5) handle with the minimum-width ref.
dwgHandle makeHardPointer(std::uint32_t ref) {
    dwgHandle h;
    h.code = 5;
    h.ref  = ref;
    h.size = 0;
    if (ref != 0) {
        std::uint32_t t = ref;
        while (t != 0) { t >>= 8; ++h.size; }
    }
    return h;
}

/// Normalize string to upper-case for writing-context map lookups.
static std::string toUpperCase(const std::string& s) {
    std::string r(s);
    for (auto& c : r)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return r;
}

// ACAD_PLOTSTYLENAME is an ASCII-reserved NOD key. Do not change the
// collation policy for other (possibly Unicode) caller dictionary keys.
static bool isPlotStyleDictionaryKey(const std::string& name) noexcept {
    static constexpr char expected[] = "ACAD_PLOTSTYLENAME";
    if (name.size() != sizeof(expected) - 1u) return false;
    for (std::size_t i = 0; i < name.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(name[i]);
        const unsigned char folded = c >= 'a' && c <= 'z'
            ? static_cast<unsigned char>(c - ('a' - 'A')) : c;
        if (folded != static_cast<unsigned char>(expected[i])) return false;
    }
    return true;
}

} // namespace

bool dwgWriter15::canWriteDwgDataStorageOperation(
    DwgDataStorageWriterOperation operation) const {
    switch (operation) {
    case DwgDataStorageWriterOperation::WriteMLeaderStyle:
    case DwgDataStorageWriterOperation::WriteTableStyle:
    case DwgDataStorageWriterOperation::WriteMaterial:
    case DwgDataStorageWriterOperation::WriteLightList:
    case DwgDataStorageWriterOperation::WriteGroup:
    case DwgDataStorageWriterOperation::WriteDictionary:
    case DwgDataStorageWriterOperation::WriteXRecord:
    case DwgDataStorageWriterOperation::WriteLayout:
    case DwgDataStorageWriterOperation::WriteMLineStyle:
    case DwgDataStorageWriterOperation::WriteAcDbPlaceholder:
    case DwgDataStorageWriterOperation::WriteRasterVariables:
    case DwgDataStorageWriterOperation::WriteWipeoutVariables:
    case DwgDataStorageWriterOperation::WriteGeoData:
    case DwgDataStorageWriterOperation::WriteSpatialFilter:
    case DwgDataStorageWriterOperation::WriteScale:
    case DwgDataStorageWriterOperation::WriteIDBuffer:
    case DwgDataStorageWriterOperation::WriteLayerIndex:
    case DwgDataStorageWriterOperation::WriteSpatialIndex:
    case DwgDataStorageWriterOperation::WriteDictionaryVar:
    case DwgDataStorageWriterOperation::WriteDictionaryWithDefault:
    case DwgDataStorageWriterOperation::WriteSortEntsTable:
    case DwgDataStorageWriterOperation::WriteFieldList:
    case DwgDataStorageWriterOperation::WriteField:
    case DwgDataStorageWriterOperation::WriteUnderlayDefinition:
    case DwgDataStorageWriterOperation::WritePointCloudDef:
    case DwgDataStorageWriterOperation::WriteNavisworksModelDef:
    case DwgDataStorageWriterOperation::WritePointCloudColorMap:
    case DwgDataStorageWriterOperation::WriteDbColor:
    case DwgDataStorageWriterOperation::WriteDimensionAssociation:
    case DwgDataStorageWriterOperation::WriteEvaluationGraph:
    case DwgDataStorageWriterOperation::WriteTvDeviceProperties:
    case DwgDataStorageWriterOperation::WriteVxControl:
    case DwgDataStorageWriterOperation::WriteVxTableRecord:
        return true;
    case DwgDataStorageWriterOperation::None:
        return false;
    }
    return false;
}

bool dwgWriter15::writeFileHeaderStub() {
    std::uint16_t measurement = 0;
    if (!dwgTemplate::headerMeasurement(m_header, measurement))
        return false;
    // Bytes 0x00-0x05: version string "AC1015".
    for (int i = 0; i < 6; ++i)
        m_buf.putRawChar8(static_cast<std::uint8_t>(dwgVersionString::R2000[i]));

    // Bytes 0x06-0x0A: five zero bytes.
    for (int i = 0; i < 5; ++i)
        m_buf.putRawChar8(0);

    // Byte 0x0B: maintenance release version (R2000 = 0x0F per LibreDWG).
    m_buf.putRawChar8(0x0F);

    // Byte 0x0C: "zero_one_or_three" marker (1 for R13+ writes from scratch).
    m_buf.putRawChar8(0x01);

    // Bytes 0x0D-0x10: RL thumbnail/preview address (placeholder; back-patched
    // by finalize() once we know whether a PREVIEW section was emitted).
    m_buf.putRawLong32(0);

    // Byte 0x11: RC dwg_version (R2000 internal version = 0x17).
    m_buf.putRawChar8(0x17);

    // Byte 0x12: RC maint_version (0 by default; real files vary).
    m_buf.putRawChar8(0x00);

    // Bytes 0x13-0x14: RS codepage matching $DWGCODEPAGE.
    m_buf.putRawShort16(fileCodePageId());

    // Bytes 0x15-0x18: RL num_sections.
    m_buf.putRawLong32(m_numSections);

    // Section locator records start here. Record current offset so
    // finalize() can back-patch addresses + sizes.
    m_recordsOffset = static_cast<std::uint32_t>(m_buf.size());

    // Emit `m_numSections` stub records of (RC recno, RL address=0, RL size=0).
    // recno values are 0..(N-1) in canonical order.
    for (std::uint8_t recno = 0; recno < m_numSections; ++recno) {
        m_buf.putRawChar8(recno);
        m_buf.putRawLong32(0); // address placeholder
        m_buf.putRawLong32(0); // size placeholder
    }

    // CRC16 over bytes [0 .. end-of-records) with seed=0, then XOR'd
    // by the per-count constant.  Stored value patched in finalize()
    // after section addresses are filled in.
    m_buf.putRawShort16(0); // placeholder

    // 16-byte FILE_HEADER_END sentinel.
    m_buf.putBytes(dwgSentinels::FILE_HEADER_END, 16);

    return true;
}

size_t dwgWriter15::beginSentinelSection(const std::uint8_t (&beginSentinel)[16]) {
    // Caller must ensure cursor is byte-aligned before invoking.
    m_buf.alignToByte();
    m_buf.putBytes(beginSentinel, 16);
    // Reserve 4 bytes for RL section payload size — back-patched at end.
    size_t sizeOffset = m_buf.size();
    m_buf.putRawLong32(0);
    return sizeOffset;
}

void dwgWriter15::endSentinelSection(size_t sectionStart, size_t sizeOffset,
                                     const std::uint8_t (&endSentinel)[16],
                                     size_t unknownTailBytes) {
    // Bit-packed bodies (HEADER vars, CLASSES, etc.) typically leave the
    // cursor mid-byte.  Pad to the next byte boundary so the END sentinel
    // lands aligned and the recorded payload size is in whole bytes.
    m_buf.alignToByte();

    // Patch the payload size: bytes between (sizeOffset + 4) and current cursor.
    std::uint32_t payloadSize =
        static_cast<std::uint32_t>(m_buf.size()) - static_cast<std::uint32_t>(sizeOffset + 4);
    m_buf.patchRawLong32(sizeOffset, payloadSize);

    // CRC covers RL-size + data only — spec §9: "covers the stepper and the
    // data"; sectionStart+16 skips the 16-byte beginning sentinel.
    std::uint16_t crc = m_buf.crc16(0xC0C1, sectionStart + 16, m_buf.size());
    m_buf.putRawShort16(crc);        // CRC BEFORE end sentinel
    for (size_t i = 0; i < unknownTailBytes; ++i)
        m_buf.putRawChar8(0);
    m_buf.putBytes(endSentinel, 16); // end sentinel AFTER CRC
}

void dwgWriter15::initHeaderControlHandles() {
    if (m_header == nullptr) return;
    auto fillIfZero = [](std::uint32_t& slot, std::uint32_t value) {
        if (slot == 0) slot = value;
    };
    fillIfZero(m_header->blockCtrl,       reservedHandle::BLOCK_CONTROL);
    fillIfZero(m_header->layerCtrl,       reservedHandle::LAYER_CONTROL);
    fillIfZero(m_header->styleCtrl,       reservedHandle::STYLE_CONTROL);
    fillIfZero(m_header->linetypeCtrl,    reservedHandle::LTYPE_CONTROL);
    fillIfZero(m_header->viewCtrl,        reservedHandle::VIEW_CONTROL);
    fillIfZero(m_header->ucsCtrl,         reservedHandle::UCS_CONTROL);
    fillIfZero(m_header->vportCtrl,       reservedHandle::VPORT_CONTROL);
    fillIfZero(m_header->appidCtrl,       reservedHandle::APPID_CONTROL);
    fillIfZero(m_header->dimstyleCtrl,    reservedHandle::DIMSTYLE_CONTROL);
    if (m_version < DRW::AC1018)
        fillIfZero(m_header->vpEntHeaderCtrl,
                   reservedHandle::VPORT_ENTITY_HEADER_CONTROL);
}

bool dwgWriter15::planHeaderCurrentReferences(
        std::array<std::uint32_t, 4>& references) const {
    if (m_header == nullptr || !m_dwgTableRecordsComplete)
        return false;
    struct RequestedReference {
        const char* name;
        const char* defaultName;
        int group;
        const std::map<std::string, std::uint32_t>* table;
    };
    const std::array<RequestedReference, 4> requests{{
        {"CLAYER", "0", 8, &m_writingCtx.layerMap},
        {"TEXTSTYLE", "STANDARD", 7, &m_writingCtx.styleMap},
        {"CELTYPE", "BYLAYER", 6, &m_writingCtx.ltypeMap},
        {"DIMSTYLE", "STANDARD", 2, &m_writingCtx.dimstyleMap}}};
    std::array<std::uint32_t, 4> result{};
    for (std::size_t i = 0; i < requests.size(); ++i) {
        const auto& request = requests[i];
        const std::string canonicalBare(request.name);
        const std::string canonicalDollar = std::string("$") + request.name;
        for (const auto& candidate : m_header->vars) {
            const std::string& key = candidate.first;
            const std::string normalized = !key.empty() && key.front() == '$'
                ? toUpperCase(key.substr(1)) : toUpperCase(key);
            if (normalized == canonicalBare && key != canonicalBare
                && key != canonicalDollar)
                return false;
        }
        const auto bare = m_header->vars.find(canonicalBare);
        const auto dollar = m_header->vars.find(canonicalDollar);
        if (bare != m_header->vars.end() && dollar != m_header->vars.end())
            return false;
        const auto found = dollar != m_header->vars.end() ? dollar : bare;
        std::string name(request.defaultName);
        if (found != m_header->vars.end()) {
            const DRW_Variant* value = found->second;
            if (value == nullptr || value->type() != DRW_Variant::STRING
                || value->code() != request.group || value->content.s == nullptr)
                return false;
            name = *value->content.s;
            if (name.empty() || name.find('\0') != std::string::npos)
                return false;
        }
        const auto target = request.table->find(toUpperCase(name));
        if (target == request.table->end() || target->second == 0u
            || m_emittedDwgTableRecordHandles.find(target->second)
                == m_emittedDwgTableRecordHandles.end())
            return false;
        result[i] = target->second;
    }
    references = result;
    return true;
}

bool dwgWriter15::writeDwgHeader() {
    size_t sectionStart = m_buf.size();
    m_sectionOffsets[recno::HEADER] = static_cast<std::uint32_t>(sectionStart);

    size_t sizeOffset = beginSentinelSection(dwgSentinels::HEADER_BEGIN);

    // Phase 3d: populate the control-handle members on m_header so the
    // encoder's control-handle block (parseDwg:2162-2199) references the
    // canonical reserved handles.
    initHeaderControlHandles();

    // Phase 3a: full 282-var bit-packed emission via DRW_Header::encodeDwg.
    // For R2000 the handle stream is inline with the data stream — pass
    // the same accumulator for both buf and hBbuf.
    if (m_header != nullptr) {
        const std::uint64_t dataStartBit = m_buf.bitCount();
        std::array<std::uint64_t, 4> currentReferenceBits{};
        // Propagate encode failure: writeDwgHeader reports success as a bool.
        if (!m_header->encodeDwgForWriter(
                m_version, &m_buf, &m_buf, nullptr,
                DRW::DwgNamedObjectsDictionaryHandle,
                m_plotSettingsDictionaryHandle, &currentReferenceBits,
                DRW::DwgAcadGroupDictionaryHandle,
                m_defaultPlotStyleDictionaryHandle))
            return false;
        if (!recordHeaderHandseedOffset(dataStartBit)
            || !recordHeaderCurrentReferenceOffsets(0u, currentReferenceBits))
            return false;
    }

    endSentinelSection(sectionStart, sizeOffset, dwgSentinels::HEADER_END);

    m_sectionSizes[recno::HEADER] =
        static_cast<std::uint32_t>(m_buf.size() - sectionStart);
    return true;
}

bool dwgWriter15::writeDwgClasses() {
    if (hasDwgClassConflict() || !validateDwgClassManifest())
        return false;

    size_t sectionStart = m_buf.size();
    m_sectionOffsets[recno::CLASSES] = static_cast<std::uint32_t>(sectionStart);

    size_t sizeOffset = beginSentinelSection(dwgSentinels::CLASSES_BEGIN);

    // R2000 CLASSES is just the packed class entries between sentinels.
    // Field layout mirrors DRW_Class::parseDwg and ODA class section records.
    if (!emitDwgClassDefinitions(
            [this](const DwgClassDefinition& definition) {
                return writeDwgClassDefinition(definition, &m_buf, nullptr);
            }))
        return false;

    endSentinelSection(sectionStart, sizeOffset, dwgSentinels::CLASSES_END);

    m_sectionSizes[recno::CLASSES] =
        static_cast<std::uint32_t>(m_buf.size() - sectionStart);
    return true;
}

bool dwgWriter15::emitControlObject(
    std::uint16_t typeCode, std::uint32_t handle, std::uint32_t numEntries,
    std::initializer_list<std::uint32_t> childHandles)
{
    if (numEntries > static_cast<std::uint32_t>(
                         std::numeric_limits<std::int32_t>::max()))
        return false;

    const bool scopedEmptyControl = m_version == DRW::AC1027 && numEntries == 0u
        && childHandles.size() == 0u
        && ((handle == DRW::DwgViewControlHandle && typeCode == DRW::DwgViewControlObjectType)
            || (handle == DRW::DwgUcsControlHandle && typeCode == DRW::DwgUcsControlObjectType));
    const auto emit = [&]() -> bool {
    if (typeCode == DRW::DwgLTypeControlObjectType
        && (numEntries > childHandles.size()
            || childHandles.size() - numEntries != 2u))
        return false;
    dwgBufferW& body = beginObject(handle);

    // Common preamble (mirrors DRW_TableEntry::parseDwg):
    //   OT  oType  — R2010+ uses 2-bit code + RC/RS; R2000/R2004 uses BS
    //   RL  objSize — R2000/R2004 only; R2010+ computes it from buf.size()-bs
    //   H   handle  (the object's own handle)
    //   BS  extDataSize = 0
    //   BL  numReactors = 0
    //   B   xDictFlag = 0  (R2004+)
    body.putObjType(m_version, typeCode);
    if (m_version > DRW::AC1014 && m_version < DRW::AC1024) {
        body.putRawLong32(0);  // objSize placeholder (back-patched by finishObject)
    }
    body.putHandle(makeOwnHandle(handle));
    body.putBitShort(0);  // extDataSize
    body.putBitLong(0);   // numReactors
    if (m_version > DRW::AC1015) {
        body.putBit(0);   // xDictFlag=0 (R2004+)
    }
    if (m_version > DRW::AC1024) {
        body.putBit(0);   // Have binary data (AC1027+)
    }

    // DRW_ObjControl body (mirrors dwgreader.cpp DRW_ObjControl::parseDwg):
    //   BS/BL numEntries (the DWG type determines the field width)
    //   RC  unkData (DIMSTYLE only, R2000+)
    //   B   stringBit = 0  (R2007+: control objects never have strings)
    //   H   null handle
    //   H   XDic handle  (xDictFlag=0 → reader reads one)
    //   H   child offset handles × numEntries (+2 for BLOCK/LTYPE controls)
    if (!writeControlEntryCount(body, typeCode, numEntries, m_version))
        return false;
    if (typeCode == DRW::DwgDimStyleControlObjectType) {
        body.putRawChar8(0);
    }
    if (m_version > DRW::AC1018) {
        body.putBit(0);   // stringBit = 0: no strings in control objects (R2007+)
    }
    dwgBufferW *hdlBuf = &body;
    const bool useControlHandleScratch =
        (m_version > DRW::AC1014 && m_version < DRW::AC1021) ||
        (m_version == DRW::AC1021 && hasR2007DetachedControlHandles(typeCode)) ||
        m_version > DRW::AC1021;
    if (useControlHandleScratch) {
        m_objectHandles.reset();
        hdlBuf = &m_objectHandles;
    }
    hdlBuf->putHandle(makeSoftPointer(0));
    hdlBuf->putHandle(makeHardOwner(0));  // XDic null
    std::size_t childIndex = 0;
    for (std::uint32_t child : childHandles) {
        const bool ltypePhantom =
            typeCode == DRW::DwgLTypeControlObjectType
            && childIndex >= numEntries;
        hdlBuf->putHandle(ltypePhantom ? makeHardOwner(child)
                                       : makeSoftOwner(child));
        ++childIndex;
    }

    finishObject();
    if (objectWriteFailed())
        return false;
    m_emittedDwgTableControlHandles.insert(handle);
    return true;
    };
    if (!scopedEmptyControl)
        return emit();
    CompoundWriteCheckpoint checkpoint;
    const auto previousGeneration = m_nextDwgObjectFrameGeneration;
    try {
        checkpoint = checkpointCompoundWrite();
    } catch (...) {
        markObjectWriteFailure();
        return false;
    }
    try {
        if (emit())
            return true;
    } catch (...) {
        // Preserve the pre-emission transaction on a late allocation failure.
    }
    try {
        rollbackCompoundWrite(checkpoint);
    } catch (...) {
        m_nextDwgObjectFrameGeneration = previousGeneration;
        markObjectWriteFailure();
        return false;
    }
    m_nextDwgObjectFrameGeneration = previousGeneration;
    markObjectWriteFailure();
    return false;
}

bool dwgWriter15::emitControlObject(
    std::uint16_t typeCode, std::uint32_t handle, std::uint32_t numEntries,
    const std::vector<std::uint32_t>& childHandles)
{
    if (numEntries > static_cast<std::uint32_t>(
                         std::numeric_limits<std::int32_t>::max()))
        return false;

    const bool scopedEmptyControl = m_version == DRW::AC1027 && numEntries == 0u
        && childHandles.size() == 0u
        && ((handle == DRW::DwgViewControlHandle && typeCode == DRW::DwgViewControlObjectType)
            || (handle == DRW::DwgUcsControlHandle && typeCode == DRW::DwgUcsControlObjectType));
    const auto emit = [&]() -> bool {
    if (typeCode == DRW::DwgLTypeControlObjectType
        && (numEntries > childHandles.size()
            || childHandles.size() - numEntries != 2u))
        return false;
    dwgBufferW& body = beginObject(handle);
    body.putObjType(m_version, typeCode);
    if (m_version > DRW::AC1014 && m_version < DRW::AC1024)
        body.putRawLong32(0);
    body.putHandle(makeOwnHandle(handle));
    body.putBitShort(0);   // extDataSize
    body.putBitLong(0);    // numReactors
    if (m_version > DRW::AC1015)
        body.putBit(0);    // xDictFlag
    if (m_version > DRW::AC1024)
        body.putBit(0);    // Have binary data (AC1027+)
    if (!writeControlEntryCount(body, typeCode, numEntries, m_version))
        return false;
    if (typeCode == DRW::DwgDimStyleControlObjectType)
        body.putRawChar8(0);
    if (m_version > DRW::AC1018)
        body.putBit(0);
    dwgBufferW *hdlBuf = &body;
    const bool useControlHandleScratch =
        (m_version > DRW::AC1014 && m_version < DRW::AC1021) ||
        (m_version == DRW::AC1021 && hasR2007DetachedControlHandles(typeCode)) ||
        m_version > DRW::AC1021;
    if (useControlHandleScratch) {
        m_objectHandles.reset();
        hdlBuf = &m_objectHandles;
    }
    hdlBuf->putHandle(makeSoftPointer(0));
    hdlBuf->putHandle(makeHardOwner(0));
    std::size_t childIndex = 0;
    for (std::uint32_t child : childHandles) {
        const bool ltypePhantom =
            typeCode == DRW::DwgLTypeControlObjectType
            && childIndex >= numEntries;
        hdlBuf->putHandle(ltypePhantom ? makeHardOwner(child)
                                       : makeSoftOwner(child));
        ++childIndex;
    }
    finishObject();
    if (objectWriteFailed())
        return false;
    m_emittedDwgTableControlHandles.insert(handle);
    return true;
    };
    if (!scopedEmptyControl)
        return emit();
    CompoundWriteCheckpoint checkpoint;
    const auto previousGeneration = m_nextDwgObjectFrameGeneration;
    try {
        checkpoint = checkpointCompoundWrite();
    } catch (...) {
        markObjectWriteFailure();
        return false;
    }
    try {
        if (emit())
            return true;
    } catch (...) {
        // Preserve the pre-emission transaction on a late allocation failure.
    }
    try {
        rollbackCompoundWrite(checkpoint);
    } catch (...) {
        m_nextDwgObjectFrameGeneration = previousGeneration;
        markObjectWriteFailure();
        return false;
    }
    m_nextDwgObjectFrameGeneration = previousGeneration;
    markObjectWriteFailure();
    return false;
}

std::uint32_t dwgWriter15::defineBlock(const std::string& name,
                                 const DRW_Coord& basePoint,
                                 int insUnits) {
    if (m_legacyModelChainActive) {
        if (!m_legacyEmptyBlockActive || name != m_legacyEmptyBlock.name
            || basePoint.x != 0.0 || basePoint.y != 0.0 || basePoint.z != 0.0
            || insUnits != 0 || !m_userBlocks.empty()) {
            m_legacyModelChainConflict = true;
            return 0;
        }
        struct PlannedBlockScope {
            bool& flag; bool old;
            explicit PlannedBlockScope(bool& value) : flag(value), old(value) { flag = true; }
            ~PlannedBlockScope() { flag = old; }
        } scope(m_legacyPlannedFrameWriting);
        const auto handle = defineBlockImpl(name, basePoint, insUnits, nullptr);
        if (handle == 0u) m_legacyModelChainConflict = true;
        return handle;
    }
    return defineBlockImpl(name, basePoint, insUnits, nullptr);
}

std::uint32_t dwgWriter15::defineBlock(
    const DRW_Block_Record& record) {
    if (m_legacyModelChainActive) { m_legacyModelChainConflict = true; return 0; }
    if (record.flags < 0 || (record.flags & ~kDwgBlockRecordFlagsMask) != 0
        || record.xrefIndex < -1 || record.xrefIndex > 65534
        || (m_version > DRW::AC1018 && record.xrefIndex != -1)
        || (m_version <= DRW::AC1018 && record.insUnits != 0)
        || (m_version <= DRW::AC1018
            && (!record.canExplode || record.blockScaling != 0))
        || record.previewData.size() > kMaxDwgBlockRecordPreviewBytes
        || record.previewData.size()
               > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())
        || !record.extData.empty() || !record.appData.empty()
        || !record.reactorHandles.empty() || record.xDictHandle != 0
        || record.insertHandles.size() > dwgSafety::MaxOwnedObjectCount) {
        return 0;
    }
    {
        std::set<std::uint32_t> uniqueInsertHandles;
        for (const std::uint32_t handle : record.insertHandles) {
            if (handle == 0u || handle == DRW::NoHandle
                || !uniqueInsertHandles.insert(handle).second)
                return 0;
        }
    }

    // ODA §20.4.52 specifies this common hard-pointer slot as NULL. Preserve
    // the parsed value in the model, but do not emit a non-null value whose
    // meaning is not part of the specified BLOCK_RECORD wire form.
    if (record.xrefBlockHandle.ref != DRW::NoHandle
        || record.xrefBlockHandle.ref64 != DRW::NoHandle) {
        return 0;
    }

    DwgBlockRecordWriteMetadata metadata;
    try {
        metadata.flags = record.flags;
        metadata.xrefUnloaded = record.xrefUnloaded;
        metadata.xrefIndex = record.xrefIndex;
        metadata.xrefPath = record.xrefPath;
        metadata.description = record.description;
        metadata.previewData = record.previewData;
        metadata.canExplode = record.canExplode;
        metadata.blockScaling = record.blockScaling;
    } catch (...) {
        return 0;
    }

    const bool isXref = (metadata.flags & 4) != 0;
    const bool isOverlay = (metadata.flags & 8) != 0;
    if (isOverlay && !isXref)
        return 0;
    // A null layout reference is the ordinary/XREF block profile. Only a
    // paper-layout block carries a nonnull association; XREFs cannot own it.
    if (isXref && record.layoutHandle != 0)
        return 0;
    const bool carriesXrefState = (metadata.flags & (16 | 32 | 64)) != 0
        || metadata.xrefUnloaded || metadata.xrefIndex >= 0
        || !metadata.xrefPath.empty();
    if (carriesXrefState && !isXref) {
        return 0;
    }
    if (isXref && !record.entMap.empty()) {
        return 0;
    }

    return defineBlockImpl(record.name, record.basePoint, record.insUnits,
                           &metadata, record.layoutHandle,
                           &record.insertHandles);
}

bool dwgWriter15::ensureUserBlockIndex() const {
    if (m_userBlockIndexReady)
        return true;

    std::vector<std::pair<std::uint32_t, std::size_t>> index;
    try {
        index.reserve(m_userBlocks.size());
        for (std::size_t position = 0; position < m_userBlocks.size();
             ++position) {
            index.emplace_back(m_userBlocks[position].blockRecordHandle,
                               position);
        }
        std::sort(index.begin(), index.end(),
                  [](const std::pair<std::uint32_t, std::size_t>& left,
                     const std::pair<std::uint32_t, std::size_t>& right) {
                      return left.first < right.first;
                  });
    } catch (...) {
        m_userBlockIndex.clear();
        return false;
    }

    for (std::size_t position = 1; position < index.size(); ++position) {
        if (index[position - 1].first == index[position].first) {
            m_userBlockIndex.clear();
            return false;
        }
    }
    m_userBlockIndex.swap(index);
    m_userBlockIndexReady = true;
    return true;
}

dwgWriter15::PendingUserBlock* dwgWriter15::findUserBlock(
    std::uint32_t blockRecordHandle) {
    const dwgWriter15* constWriter = this;
    return const_cast<PendingUserBlock*>(
        constWriter->findUserBlock(blockRecordHandle));
}

const dwgWriter15::PendingUserBlock* dwgWriter15::findUserBlock(
    std::uint32_t blockRecordHandle) const {
    if (ensureUserBlockIndex()) {
        const auto position = std::lower_bound(
            m_userBlockIndex.cbegin(), m_userBlockIndex.cend(),
            blockRecordHandle,
            [](const std::pair<std::uint32_t, std::size_t>& entry,
               std::uint32_t handle) { return entry.first < handle; });
        if (position == m_userBlockIndex.cend()
            || position->first != blockRecordHandle)
            return nullptr;
        return &m_userBlocks[position->second];
    }

    const auto block = std::find_if(
        m_userBlocks.cbegin(), m_userBlocks.cend(),
        [blockRecordHandle](const PendingUserBlock& candidate) {
            return candidate.blockRecordHandle == blockRecordHandle;
        });
    return block == m_userBlocks.cend() ? nullptr : &*block;
}

void dwgWriter15::invalidateUserBlockIndex() noexcept {
    m_userBlockIndex.clear();
    m_userBlockIndexReady = false;
}

std::uint32_t dwgWriter15::defineBlockImpl(
    const std::string& name, const DRW_Coord& basePoint, int insUnits,
    DwgBlockRecordWriteMetadata* metadata, std::uint32_t layoutHandle,
    const std::vector<std::uint32_t>* expectedInsertHandles) {
    if (m_blockControlEmitted || m_activeUserBlockRecordHandle != 0) {
        return 0;
    }
    const std::string key = toUpperCase(name);
    if (key.empty()) {
        return 0;
    }
    if (insUnits < 0
        || static_cast<std::uint64_t>(insUnits)
               > std::numeric_limits<std::uint16_t>::max()) {
        return 0;
    }
    const auto existing = m_userBlockHandles.find(key);
    if (existing != m_userBlockHandles.end()) {
        PendingUserBlock* pending = findUserBlock(existing->second);
        if (pending == nullptr || pending->name != name
            || pending->layoutHandle != layoutHandle
            || pending->insUnits != insUnits
            || pending->basePoint.x != basePoint.x
            || pending->basePoint.y != basePoint.y
            || pending->basePoint.z != basePoint.z)
            return 0;
        if (expectedInsertHandles != nullptr
            && pending->hasExpectedInsertHandles
            && pending->expectedInsertHandles != *expectedInsertHandles)
            return 0;
        if (metadata == nullptr) {
            if (expectedInsertHandles != nullptr
                && !pending->hasExpectedInsertHandles) {
                std::vector<std::uint32_t> stagedExpected;
                try {
                    stagedExpected = *expectedInsertHandles;
                } catch (...) {
                    return 0;
                }
                pending->expectedInsertHandles = std::move(stagedExpected);
                pending->hasExpectedInsertHandles = true;
            }
            return existing->second;
        }
        if (pending->hasWriteMetadata) {
            const DwgBlockRecordWriteMetadata& prior =
                pending->writeMetadata;
            const bool same = prior.flags == metadata->flags
                && prior.xrefUnloaded == metadata->xrefUnloaded
                && prior.xrefIndex == metadata->xrefIndex
                && prior.xrefPath == metadata->xrefPath
                && prior.description == metadata->description
                && prior.previewData == metadata->previewData
                && prior.canExplode == metadata->canExplode
                && prior.blockScaling == metadata->blockScaling;
            if (!same)
                return 0;
            if (expectedInsertHandles != nullptr
                && !pending->hasExpectedInsertHandles) {
                std::vector<std::uint32_t> stagedExpected;
                try {
                    stagedExpected = *expectedInsertHandles;
                } catch (...) {
                    return 0;
                }
                pending->expectedInsertHandles = std::move(stagedExpected);
                pending->hasExpectedInsertHandles = true;
            }
            return existing->second;
        }
        std::vector<std::uint32_t> stagedExpected;
        try {
            if (expectedInsertHandles != nullptr
                && !pending->hasExpectedInsertHandles)
                stagedExpected = *expectedInsertHandles;
            pending->writeMetadata = std::move(*metadata);
        } catch (...) {
            return 0;
        }
        if (expectedInsertHandles != nullptr
            && !pending->hasExpectedInsertHandles) {
            pending->expectedInsertHandles = std::move(stagedExpected);
            pending->hasExpectedInsertHandles = true;
        }
        pending->hasWriteMetadata = true;
        return existing->second;
    }

    const EntityFrameCheckpoint checkpoint = checkpointEntityFrame();
    CompoundWriteCheckpoint classCheckpoint;
    bool hasClassCheckpoint = false;
    bool frameStarted = false;
    const auto fail = [this, &checkpoint, &classCheckpoint,
                       &hasClassCheckpoint, &frameStarted]() {
        if (hasClassCheckpoint)
            rollbackCompoundWrite(classCheckpoint);
        else
            rollbackEntityFrame(checkpoint, frameStarted);
        return std::uint32_t{0};
    };

    // An opt-in block uses identities reserved with the complete graph;
    // ordinary writers retain the existing allocator route.
    std::uint32_t blockRecH = m_legacyModelChainActive
        ? m_legacyEmptyBlock.blockRecordHandle : m_handles.next();
    std::uint32_t blockH = m_legacyModelChainActive
        ? m_legacyEmptyBlock.blockHandle : m_handles.next();
    std::uint32_t endBlockH = m_legacyModelChainActive
        ? m_legacyEmptyBlock.endBlockHandle : m_handles.next();
    if (m_dwgClassInstanceHandleOwners.find(blockH)
            != m_dwgClassInstanceHandleOwners.end()
        || m_dwgClassInstanceHandleOwners.find(endBlockH)
            != m_dwgClassInstanceHandleOwners.end()) {
        classCheckpoint = checkpointCompoundWrite();
        hasClassCheckpoint = true;
    }

    // Block entity for the start of the block body.
    DRW_Block bk;
    bk.handle = blockH;
    bk.layerH.ref = DRW::DwgLayer0Handle;
    bk.color = 256;  // BYLAYER
    bk.name = name;
    bk.parentHandle = blockRecH;
    bk.setIsEnd(false);
    {
        frameStarted = true;
        dwgBufferW& body = beginObject(blockH);
        if (m_version > DRW::AC1018) {
            m_objectStrings.reset();
            m_objectHandles.reset();
            if (!bk.encodeDwg(m_version, &body, /*bs=*/0,
                              &m_objectStrings, &m_objectHandles)) {
                return fail();
            }
        } else if (m_version > DRW::AC1014) {
            m_objectHandles.reset();
            ScopedDwgLegacyEntityLinks links(
                m_legacyModelChainActive ? &body : nullptr,
                m_legacyModelChainActive ? &m_objectHandles : nullptr, 0, 0);
            if (!bk.encodeDwg(m_version, &body, /*bs=*/0,
                              nullptr, &m_objectHandles)) {
                return fail();
            }
        } else if (!bk.encodeDwg(m_version, &body, /*bs=*/0)) {
            return fail();
        }

        finishObject();
        if (objectWriteFailed())
            return fail();
    }

    if (consumeWriteFailurePointForTest(kBeforeEndBlockFrame))
        return fail();

    // ENDBLK entity.
    DRW_Block endBlk;
    endBlk.handle = endBlockH;
    endBlk.layerH.ref = DRW::DwgLayer0Handle;
    endBlk.color = 256;
    endBlk.parentHandle = blockRecH;
    endBlk.setIsEnd(true);
    {
        dwgBufferW& body = beginObject(endBlockH);
        if (m_version > DRW::AC1018) {
            m_objectStrings.reset();
            m_objectHandles.reset();
            if (!endBlk.encodeDwg(m_version, &body, /*bs=*/0,
                                  &m_objectStrings, &m_objectHandles)) {
                return fail();
            }
        } else if (m_version > DRW::AC1014) {
            m_objectHandles.reset();
            ScopedDwgLegacyEntityLinks links(
                m_legacyModelChainActive ? &body : nullptr,
                m_legacyModelChainActive ? &m_objectHandles : nullptr, 0, 0);
            if (!endBlk.encodeDwg(m_version, &body, /*bs=*/0,
                                  nullptr, &m_objectHandles)) {
                return fail();
            }
        } else if (!endBlk.encodeDwg(m_version, &body, /*bs=*/0)) {
            return fail();
        }

        finishObject();
        if (objectWriteFailed())
            return fail();
    }

    if (consumeWriteFailurePointForTest(kAfterEndBlockFrame))
        return fail();

    m_emittedDwgBlockEntityHandles.insert(blockH);
    m_emittedDwgBlockEntityHandles.insert(endBlockH);

    PendingUserBlock block;
    block.blockRecordHandle = blockRecH;
    block.layoutHandle = layoutHandle;
    block.ownerKind = layoutHandle == 0
        ? BlockOwnerKind::OrdinaryBlock : BlockOwnerKind::PaperLayout;
    block.blockHandle = blockH;
    block.endBlockHandle = endBlockH;
    block.name = name;
    block.basePoint = basePoint;
    block.insUnits = insUnits;
    if (metadata != nullptr) {
        block.hasWriteMetadata = true;
        block.writeMetadata = std::move(*metadata);
    }
    if (expectedInsertHandles != nullptr) {
        try {
            block.expectedInsertHandles = *expectedInsertHandles;
        } catch (...) {
            return fail();
        }
        block.hasExpectedInsertHandles = true;
    }
    m_userBlocks.push_back(block);
    invalidateUserBlockIndex();
    m_userBlockHandles.emplace(key, blockRecH);
    return blockRecH;
}

bool dwgWriter15::beginBlockContent(std::uint32_t blockRecordHandle) {
    if (m_legacyModelChainActive) {
        const PendingUserBlock* block = findUserBlock(blockRecordHandle);
        if (m_legacyModelChainConflict || m_blockControlEmitted
            || m_activeUserBlockRecordHandle != 0 || !m_legacyEmptyBlockActive
            || m_legacyBlockContent.empty() || !m_modelSpaceEntityHandles.empty()
            || blockRecordHandle != m_legacyEmptyBlock.blockRecordHandle
            || block == nullptr || !block->entityHandles.empty()) {
            m_legacyModelChainConflict = true;
            return false;
        }
        m_activeUserBlockRecordHandle = blockRecordHandle;
        return true;
    }
    if (m_blockControlEmitted || m_activeUserBlockRecordHandle != 0)
        return false;
    const PendingUserBlock* block = findUserBlock(blockRecordHandle);
    if (block == nullptr
        || (block->hasWriteMetadata
            && (block->writeMetadata.flags & (4 | 8)) != 0))
        return false;
    m_activeUserBlockRecordHandle = blockRecordHandle;
    return true;
}

bool dwgWriter15::endBlockContent() {
    if (m_legacyModelChainActive) {
        const PendingUserBlock* block = findUserBlock(m_activeUserBlockRecordHandle);
        if (m_legacyModelChainConflict || m_blockControlEmitted
            || m_activeUserBlockRecordHandle != m_legacyEmptyBlock.blockRecordHandle
            || block == nullptr || block->entityHandles.size() != m_legacyBlockContent.size()) {
            m_legacyModelChainConflict = true;
            return false;
        }
        m_activeUserBlockRecordHandle = 0;
        return true;
    }
    if (m_blockControlEmitted || m_activeUserBlockRecordHandle == 0)
        return false;
    m_activeUserBlockRecordHandle = 0;
    return true;
}

bool dwgWriter15::hasClosedAc1018TableBlockGraph(
    const DRW_Table& table) const {
    if (m_version != DRW::AC1018 || m_activeUserBlockRecordHandle != 0
        || table.eType != DRW::TABLE || table.handle == 0u
        || table.handle == DRW::NoHandle
        || table.blockRecH.ref == 0u || table.blockRecH.ref == DRW::NoHandle
        || !validAc1018TableBlockRecordReference(table.blockRecH)
        || table.m_tableStyleHandle == 0u
        || table.m_tableStyleHandle == DRW::NoHandle
        || table.m_content.m_tableStyleHandle != table.m_tableStyleHandle
        || !hasEmittedDwgObjectHandle(table.m_tableStyleHandle)
        || table.name.size() < 3u || table.name[0] != '*'
        || std::toupper(static_cast<unsigned char>(table.name[1])) != 'T')
        return false;

    if (table.xDictHandle != 0u) {
        if (table.xDictHandle == DRW::NoHandle
            || table.xDictHandle == table.handle
            || table.xDictHandle == table.blockRecH.ref
            || table.xDictHandle == table.m_tableStyleHandle
            || !hasEmittedDwgObjectHandle(table.xDictHandle))
            return false;
        if (m_rejectedDwgHandleOverflow
            || std::find(m_rejectedDwgObjectHandles.cbegin(),
                         m_rejectedDwgObjectHandles.cend(), table.xDictHandle)
                != m_rejectedDwgObjectHandles.cend())
            return false;
        const auto matches = std::count_if(
            m_emittedEmptyExtensionDictionaries.cbegin(),
            m_emittedEmptyExtensionDictionaries.cend(),
            [&table](const EmittedEmptyExtensionDictionary& entry) {
                return entry.handle == table.xDictHandle
                    && entry.parentHandle == table.handle;
            });
        const auto sameHandle = std::count_if(
            m_emittedEmptyExtensionDictionaries.cbegin(),
            m_emittedEmptyExtensionDictionaries.cend(),
            [&table](const EmittedEmptyExtensionDictionary& entry) {
                return entry.handle == table.xDictHandle;
            });
        if (matches != 1 || sameHandle != 1)
            return false;
    }

    const auto layer = m_writingCtx.layerMap.find(toUpperCase(table.layer));
    if (table.layer.empty() || layer == m_writingCtx.layerMap.cend()
        || layer->second != table.dwgLayerHandle()
        || (m_dwgTableRecordsComplete
            && m_emittedDwgTableRecordHandles.count(layer->second) != 1u))
        return false;
    if (table.dwgLinetypeHandle() != 0u) {
        const auto linetype = m_writingCtx.ltypeMap.find(
            toUpperCase(table.lineType));
        if (linetype == m_writingCtx.ltypeMap.cend()
            || linetype->second != table.dwgLinetypeHandle()
            || (m_dwgTableRecordsComplete
                && m_emittedDwgTableRecordHandles.count(linetype->second)
                    != 1u))
            return false;
    }

    if (table.plotStyle != DRW::DefaultPlotStyle) {
        if (table.plotStyle <= 0
            || static_cast<std::uint32_t>(table.plotStyle)
                != m_defaultPlotStylePlaceholderHandle
            || !m_defaultPlotStyleGraphEmitted
            || !hasEmittedDwgObjectHandle(
                m_defaultPlotStylePlaceholderHandle))
            return false;
    }
    if (table.hasAcDbColorH != (table.acDbColorHandle != 0u))
        return false;
    if (table.hasAcDbColorH) {
        if (!hasEmittedDwgObjectHandle(table.acDbColorHandle))
            return false;
        std::uint16_t colorClassNumber = 0u;
        if (!getDwgClassInstanceClass(table.acDbColorHandle,
                                     colorClassNumber))
            return false;
        const auto colorClass = std::find_if(
            m_dwgClassDefinitions.cbegin(), m_dwgClassDefinitions.cend(),
            [colorClassNumber](const DwgClassDefinition& definition) {
                return definition.m_classNum == colorClassNumber
                    && definition.m_className == "AcDbColor"
                    && definition.m_recordName == "DBCOLOR";
            });
        if (colorClass == m_dwgClassDefinitions.cend())
            return false;
    }

    std::uint16_t styleClassNumber = 0u;
    if (!getDwgClassInstanceClass(table.m_tableStyleHandle,
                                 styleClassNumber))
        return false;
    const auto styleClass = std::find_if(
        m_dwgClassDefinitions.cbegin(), m_dwgClassDefinitions.cend(),
        [styleClassNumber](const DwgClassDefinition& definition) {
            return definition.m_classNum == styleClassNumber
                && definition.m_className == "AcDbTableStyle"
                && definition.m_recordName == "TABLESTYLE";
        });
    if (styleClass == m_dwgClassDefinitions.cend())
        return false;

    constexpr std::array<std::uint32_t, 3> rowTextStyleFlags{{
        0x00020000u, 0x00040000u, 0x00080000u
    }};
    for (std::size_t index = 0u; index < rowTextStyleFlags.size(); ++index) {
        const bool selected =
            (table.m_tableOverrideFlags & rowTextStyleFlags[index]) != 0u;
        const std::uint32_t target =
            table.m_tableRowTextStyleHandles[index];
        if (!selected) {
            if (target != 0u)
                return false;
            continue;
        }
        if (target == 0u || target == DRW::NoHandle
            || target == table.handle || target == table.m_tableStyleHandle
            || !hasEmittedDwgObjectHandle(target))
            return false;
        const std::size_t styleNameMatches = static_cast<std::size_t>(
            std::count_if(m_writingCtx.styleMap.cbegin(),
                m_writingCtx.styleMap.cend(), [target](const auto& entry) {
                    return entry.second == target;
                }));
        const std::size_t emittedStyleRecords = static_cast<std::size_t>(
            std::count(m_emittedDwgTableRecordHandles.cbegin(),
                       m_emittedDwgTableRecordHandles.cend(), target));
        if (styleNameMatches != 1u
            || (m_dwgTableRecordsComplete && emittedStyleRecords != 1u))
            return false;
        if (!table.m_tableRowTextStyleNames[index].empty()) {
            const auto namedStyle = m_writingCtx.styleMap.find(
                toUpperCase(table.m_tableRowTextStyleNames[index]));
            if (namedStyle == m_writingCtx.styleMap.cend()
                || namedStyle->second != target)
                return false;
        }
    }

    const auto equalsAsciiNoCase = [](const std::string& value,
                                     const std::string& expected) {
        const std::size_t length = expected.size();
        if (value.size() != length)
            return false;
        for (std::size_t index = 0u; index < length; ++index) {
            const auto foldAscii = [](unsigned char character) {
                return character >= 'a' && character <= 'z'
                    ? static_cast<unsigned char>(character - 'a' + 'A')
                    : character;
            };
            if (foldAscii(static_cast<unsigned char>(value[index]))
                != foldAscii(static_cast<unsigned char>(expected[index])))
                return false;
        }
        return true;
    };
    std::uint32_t styleDictionaryHandle = 0u;
    std::size_t styleDictionaryRootCount = 0u;
    for (const DRW::DwgNamedObjectDictionaryEntry& entry :
         m_pendingDwgNamedObjectDictionaryEntries) {
        if (equalsAsciiNoCase(entry.name, "ACAD_TABLESTYLE")
            && entry.childHandle != 0u) {
            styleDictionaryHandle = entry.childHandle;
            ++styleDictionaryRootCount;
        }
    }
    if (styleDictionaryRootCount != 1u
        || styleDictionaryHandle == DRW::NoHandle)
        return false;
    const std::size_t styleDictionaryFrameCount =
        static_cast<std::size_t>(std::count_if(
            m_emittedDwgDictionaries.cbegin(), m_emittedDwgDictionaries.cend(),
            [styleDictionaryHandle](const EmittedDwgDictionary& dictionary) {
                return dictionary.handle == styleDictionaryHandle;
            }));
    if (styleDictionaryFrameCount != 1u)
        return false;
    const auto styleDictionary = std::find_if(
        m_emittedDwgDictionaries.cbegin(), m_emittedDwgDictionaries.cend(),
        [styleDictionaryHandle](const EmittedDwgDictionary& dictionary) {
            return dictionary.handle == styleDictionaryHandle
                && dictionary.parentHandle
                    == DRW::DwgNamedObjectsDictionaryHandle
                && dictionary.hardOwner == 1;
        });
    if (styleDictionary == m_emittedDwgDictionaries.cend())
        return false;
    const std::size_t emittedTableStyleCount =
        static_cast<std::size_t>(std::count_if(
            m_emittedDwgTableStyles.cbegin(), m_emittedDwgTableStyles.cend(),
            [&table](const EmittedDwgTableStyle& emitted) {
                return emitted.handle == table.m_tableStyleHandle;
            }));
    if (emittedTableStyleCount != 1u)
        return false;
    const auto emittedTableStyle = std::find_if(
        m_emittedDwgTableStyles.cbegin(), m_emittedDwgTableStyles.cend(),
        [&table](const EmittedDwgTableStyle& emitted) {
            return emitted.handle == table.m_tableStyleHandle;
        });
    if (emittedTableStyle == m_emittedDwgTableStyles.cend())
        return false;
    if (emittedTableStyle->parentHandle != styleDictionaryHandle)
        return false;
    const std::size_t styleDictionaryMemberCount =
        static_cast<std::size_t>(std::count_if(
            styleDictionary->entries.cbegin(), styleDictionary->entries.cend(),
            [&table](const DRW::DwgNamedObjectDictionaryEntry& entry) {
                return entry.childHandle == table.m_tableStyleHandle;
            }));
    const std::size_t styleDictionaryNameMatchCount =
        static_cast<std::size_t>(std::count_if(
            styleDictionary->entries.cbegin(), styleDictionary->entries.cend(),
            [&table, &emittedTableStyle, &equalsAsciiNoCase](
                const DRW::DwgNamedObjectDictionaryEntry& entry) {
                return entry.childHandle == table.m_tableStyleHandle
                    && equalsAsciiNoCase(entry.name,
                                         emittedTableStyle->name);
            }));
    if (styleDictionaryMemberCount != 1u
        || styleDictionaryNameMatchCount != 1u)
        return false;

    const PendingUserBlock* block = findUserBlock(table.blockRecH.ref);
    if (block == nullptr || block->ownerKind != BlockOwnerKind::OrdinaryBlock
        || toUpperCase(block->name) != toUpperCase(table.name)
        || block->entityHandles.empty() || block->insertHandles.size() != 1u
        || block->insertHandles.front() != table.handle
        || std::count(block->insertHandles.cbegin(),
                      block->insertHandles.cend(), table.handle) != 1
        || m_blockInsertReferenceHandles.count(
               blockInsertReferenceKey(block->blockRecordHandle,
                                       table.handle)) != 1u)
        return false;

    // The table's BLOCK_RECORD must actually own every emitted child and the
    // per-writer ownership index must agree. This is graph closure evidence,
    // not a claim that those children reproduce the table's displayed cells.
    for (const std::uint32_t entityHandle : block->entityHandles) {
        const auto owner = m_blockOwnedEntityHandles.find(
            std::make_pair(block->blockRecordHandle, entityHandle));
        if (entityHandle == 0u || entityHandle == DRW::NoHandle
            || owner == m_blockOwnedEntityHandles.end())
            return false;
    }
    return true;
}

dwgWriter15::EntityFrameCheckpoint
dwgWriter15::checkpointEntityFrame() const {
    EntityFrameCheckpoint checkpoint;
    checkpoint.bufferSize = m_buf.size();
    checkpoint.lastDwgObjectFrameGeneration = m_lastDwgObjectFrameValid
        ? m_lastDwgObjectFrame.generation : 0;
    checkpoint.objectMapSize = m_objectMap.size();
    checkpoint.emittedLayoutAssociationCount =
        m_emittedLayoutAssociations.size();
    checkpoint.emittedLayoutDictionaryCount =
        m_emittedLayoutDictionaries.size();
    checkpoint.emittedEmptyExtensionDictionaryCount =
        m_emittedEmptyExtensionDictionaries.size();
    checkpoint.currentHandle = m_currentHandle;
    checkpoint.frameWriteError = m_frameWriteError;
    checkpoint.entityEedWriteFailure = m_entityEedWriteFailure;
    checkpoint.objectEedWriteFailure = m_objectEedWriteFailure;
    checkpoint.pendingFrameProvenance = m_pendingDwgObjectFrameProvenance;
    checkpoint.currentFrameProvenance = m_currentDwgObjectFrameProvenance;
    return checkpoint;
}

void dwgWriter15::rollbackEntityFrame(
    const EntityFrameCheckpoint& checkpoint, bool frameStarted) {
    // Like full transaction rollback, retain a newly-discovered EED failure
    // while discarding all other frame-local state.
    const bool entityEedFailure = m_entityEedWriteFailure;
    const bool objectEedFailure = m_objectEedWriteFailure;
    if (checkpoint.bufferSize <= m_buf.size())
        m_buf.truncate(checkpoint.bufferSize);
    if (checkpoint.objectMapSize <= m_objectMap.size()) {
        for (std::size_t index = checkpoint.objectMapSize;
             index < m_objectMap.size(); ++index)
            m_emittedDwgObjectHandles.erase(m_objectMap[index].first);
        m_objectMap.resize(checkpoint.objectMapSize);
    }
    if (checkpoint.emittedLayoutAssociationCount
        <= m_emittedLayoutAssociations.size()) {
        m_emittedLayoutAssociations.resize(
            checkpoint.emittedLayoutAssociationCount);
    }
    if (checkpoint.emittedLayoutDictionaryCount
        <= m_emittedLayoutDictionaries.size()) {
        m_emittedLayoutDictionaries.resize(
            checkpoint.emittedLayoutDictionaryCount);
    }
    if (checkpoint.emittedEmptyExtensionDictionaryCount
        <= m_emittedEmptyExtensionDictionaries.size()) {
        m_emittedEmptyExtensionDictionaries.resize(
            checkpoint.emittedEmptyExtensionDictionaryCount);
    }

    if (checkpoint.blockOwnedEntityRecorded) {
        std::vector<std::uint32_t>* handles = nullptr;
        const std::uint32_t ownerHandle = checkpoint.blockOwnerHandle;
        if (ownerHandle == reservedHandle::BLOCK_MODEL_SPACE) {
            handles = &m_modelSpaceEntityHandles;
        } else if (ownerHandle == reservedHandle::BLOCK_PAPER_SPACE) {
            handles = &m_paperSpaceEntityHandles;
        } else {
            PendingUserBlock* owner = findUserBlock(ownerHandle);
            if (owner != nullptr) {
                handles = &owner->entityHandles;
            }
        }
        if (handles != nullptr && !handles->empty()
            && handles->back() == checkpoint.entityHandle
            && m_blockOwnedEntityHandles.erase(
                   std::make_pair(ownerHandle, checkpoint.entityHandle)) != 0) {
            handles->pop_back();
            m_dwgGroupMemberTargetHandles.clear();
            m_dwgGroupMemberTargetIndexReady = false;
        }
    }

    if (checkpoint.blockInsertReferenceRecorded
        && m_version > DRW::AC1014) {
        std::vector<std::uint32_t>* handles = nullptr;
        const std::uint32_t ownerHandle = checkpoint.insertOwnerHandle;
        if (ownerHandle == reservedHandle::BLOCK_MODEL_SPACE) {
            handles = &m_modelSpaceInsertHandles;
        } else if (ownerHandle == reservedHandle::BLOCK_PAPER_SPACE) {
            handles = &m_paperSpaceInsertHandles;
        } else {
            PendingUserBlock* owner = findUserBlock(ownerHandle);
            if (owner != nullptr)
                handles = &owner->insertHandles;
        }
        if (handles != nullptr && !handles->empty()
            && handles->back() == checkpoint.entityHandle
            && m_blockInsertReferenceHandles.erase(blockInsertReferenceKey(
                   ownerHandle, checkpoint.entityHandle)) != 0)
            handles->pop_back();
    }

    m_frameWriteError = checkpoint.frameWriteError;
    m_entityEedWriteFailure = checkpoint.entityEedWriteFailure
        || entityEedFailure;
    m_objectEedWriteFailure = checkpoint.objectEedWriteFailure
        || objectEedFailure;
    m_pendingDwgObjectFrameProvenance = checkpoint.pendingFrameProvenance;
    m_currentDwgObjectFrameProvenance = checkpoint.currentFrameProvenance;
    m_currentHandle = checkpoint.currentHandle;
    m_objectBody.reset();
    m_objectStrings.reset();
    m_objectHandles.reset();
    if (frameStarted) {
        m_lastDwgObjectFrame = {};
        m_lastDwgObjectFrameValid = false;
    }
}

bool dwgWriter15::rollbackEntityFrameBatch(
    const std::vector<EntityFrameCheckpoint>& checkpoints) {
    if (checkpoints.empty())
        return false;

    const std::uint32_t ownerHandle = checkpoints.front().blockOwnerHandle;
    const std::size_t firstObjectMapSize = checkpoints.front().objectMapSize;
    const std::size_t expectedObjectMapSize =
        firstObjectMapSize + checkpoints.size();
    if (expectedObjectMapSize < firstObjectMapSize
        || m_objectMap.size() != expectedObjectMapSize
        || !m_lastEntityWriteCheckpointValid
        || m_lastEntityWriteUsesCompoundCheckpoint
        || m_lastEntityFrameCheckpoint.entityHandle
               != checkpoints.back().entityHandle)
        return false;

    std::vector<std::uint32_t>* ownerEntities = nullptr;
    if (ownerHandle == reservedHandle::BLOCK_MODEL_SPACE) {
        ownerEntities = &m_modelSpaceEntityHandles;
    } else if (ownerHandle == reservedHandle::BLOCK_PAPER_SPACE) {
        ownerEntities = &m_paperSpaceEntityHandles;
    } else {
        PendingUserBlock* owner = findUserBlock(ownerHandle);
        if (owner != nullptr)
            ownerEntities = &owner->entityHandles;
    }
    if (ownerEntities == nullptr || ownerEntities->size() < checkpoints.size())
        return false;

    std::size_t priorBufferSize = 0;
    const EntityFrameCheckpoint* insertReference = nullptr;
    for (std::size_t index = 0; index < checkpoints.size(); ++index) {
        const EntityFrameCheckpoint& checkpoint = checkpoints[index];
        const std::size_t expectedMapIndex = firstObjectMapSize + index;
        if (checkpoint.objectMapSize != expectedMapIndex
            || checkpoint.bufferSize > m_buf.size()
            || (index != 0 && checkpoint.bufferSize <= priorBufferSize)
            || checkpoint.blockOwnerHandle != ownerHandle
            || !checkpoint.blockOwnedEntityRecorded
            || checkpoint.entityHandle == 0
            || m_objectMap[expectedMapIndex].first != checkpoint.entityHandle
            || m_emittedDwgObjectHandles.find(checkpoint.entityHandle)
                   == m_emittedDwgObjectHandles.end()
            || m_blockOwnedEntityHandles.find(
                   std::make_pair(ownerHandle, checkpoint.entityHandle))
                   == m_blockOwnedEntityHandles.end())
            return false;
        priorBufferSize = checkpoint.bufferSize;
        if (checkpoint.blockInsertReferenceRecorded) {
            if (index != 0 || insertReference != nullptr
                || checkpoint.entityHandle
                    != checkpoints.front().entityHandle
                || checkpoint.insertOwnerHandle == 0
                || checkpoint.insertOwnerHandle == DRW::NoHandle)
                return false;
            insertReference = &checkpoint;
        }
    }

    for (std::size_t offset = 0; offset < checkpoints.size(); ++offset) {
        const std::uint32_t expectedHandle =
            checkpoints[checkpoints.size() - 1 - offset].entityHandle;
        if ((*ownerEntities)[ownerEntities->size() - 1 - offset]
            != expectedHandle)
            return false;
    }

    if (insertReference != nullptr) {
        std::vector<std::uint32_t>* insertHandles = nullptr;
        const std::uint32_t insertOwnerHandle =
            insertReference->insertOwnerHandle;
        if (insertOwnerHandle == reservedHandle::BLOCK_MODEL_SPACE) {
            insertHandles = &m_modelSpaceInsertHandles;
        } else if (insertOwnerHandle == reservedHandle::BLOCK_PAPER_SPACE) {
            insertHandles = &m_paperSpaceInsertHandles;
        } else {
            PendingUserBlock* insertOwner = findUserBlock(insertOwnerHandle);
            if (insertOwner != nullptr)
                insertHandles = &insertOwner->insertHandles;
        }
        if (insertHandles == nullptr || insertHandles->empty()
            || insertHandles->back() != insertReference->entityHandle
            || m_blockInsertReferenceHandles.find(blockInsertReferenceKey(
                   insertOwnerHandle, insertReference->entityHandle))
                == m_blockInsertReferenceHandles.end())
            return false;
    }

    for (auto checkpoint = checkpoints.crbegin();
         checkpoint != checkpoints.crend(); ++checkpoint)
        rollbackEntityFrame(*checkpoint, true);
    clearLastEntityWriteCheckpoint();
    return true;
}

void dwgWriter15::clearLastEntityWriteCheckpoint() noexcept {
    m_lastEntityWriteCheckpointValid = false;
    m_lastEntityWriteUsesCompoundCheckpoint = false;
    m_lastEntityCompoundCheckpoint = {};
    m_lastEntityFrameCheckpoint = {};
}

dwgWriter::CompoundWriteCheckpoint dwgWriter15::checkpointCompoundWrite() const {
    CompoundWriteCheckpoint checkpoint = dwgWriter::checkpointCompoundWrite();
    checkpoint.lastDwgObjectFrameGeneration = m_lastDwgObjectFrameValid
        ? m_lastDwgObjectFrame.generation : 0;
    checkpoint.objectMapSize = m_objectMap.size();
    checkpoint.modelSpaceEntityCount = m_modelSpaceEntityHandles.size();
    checkpoint.ac1021PolylineGroupVertexCount = m_ac1021PolylineGroupVertexLedger.size();
    checkpoint.paperSpaceEntityCount = m_paperSpaceEntityHandles.size();
    checkpoint.modelSpaceInsertCount = m_modelSpaceInsertHandles.size();
    checkpoint.paperSpaceInsertCount = m_paperSpaceInsertHandles.size();
    checkpoint.emittedLayoutAssociationCount =
        m_emittedLayoutAssociations.size();
    checkpoint.emittedLayoutDictionaryCount =
        m_emittedLayoutDictionaries.size();
    checkpoint.emittedDwgDictionaryCount = m_emittedDwgDictionaries.size();
    checkpoint.emittedEmptyExtensionDictionaryCount =
        m_emittedEmptyExtensionDictionaries.size();
    checkpoint.emittedDwgTableStyleCount = m_emittedDwgTableStyles.size();
    checkpoint.userBlockEntityCounts.reserve(m_userBlocks.size());
    checkpoint.userBlockInsertCounts.reserve(m_userBlocks.size());
    for (const PendingUserBlock& block : m_userBlocks)
        checkpoint.userBlockEntityCounts.push_back(block.entityHandles.size());
    for (const PendingUserBlock& block : m_userBlocks)
        checkpoint.userBlockInsertCounts.push_back(block.insertHandles.size());
    checkpoint.emittedDwgTableRecordHandles =
        m_emittedDwgTableRecordHandles;
    checkpoint.emittedDwgTableControlHandles =
        m_emittedDwgTableControlHandles;
    checkpoint.dwgTableRecordsComplete = m_dwgTableRecordsComplete;
    checkpoint.emittedDwgNamedObjectDictionaryEntries =
        m_emittedDwgNamedObjectDictionaryEntries;
    checkpoint.dwgNamedObjectDictionaryComplete =
        m_dwgNamedObjectDictionaryComplete;
    checkpoint.plotSettingsDictionaryHandle =
        m_plotSettingsDictionaryHandle;
    checkpoint.defaultPlotStyleDictionaryHandle = m_defaultPlotStyleDictionaryHandle;
    checkpoint.defaultPlotStylePlaceholderHandle = m_defaultPlotStylePlaceholderHandle;
    checkpoint.defaultPlotStyleGraphEmitted = m_defaultPlotStyleGraphEmitted;
    checkpoint.defaultMaterialHandles = m_defaultMaterialHandles;
    checkpoint.defaultMaterialGraphEmitted = m_defaultMaterialGraphEmitted;
    checkpoint.canonicalLayerHandles = m_canonicalLayerHandles;
    checkpoint.canonicalPlotTarget = m_canonicalPlotTarget;
    checkpoint.canonicalPlotDictionary = m_canonicalPlotDictionary;
    checkpoint.canonicalPlaceholderOwner = m_canonicalPlaceholderOwner;
    checkpoint.canonicalPlotDictionarySeen = m_canonicalPlotDictionarySeen;
    checkpoint.canonicalPlaceholderSeen = m_canonicalPlaceholderSeen;
    checkpoint.deferredPlotSettingsDictionary =
        m_deferredPlotSettingsDictionary;
    checkpoint.deferredPlotSettingsDictionaryEmitted =
        m_deferredPlotSettingsDictionaryEmitted;
    checkpoint.deferredPlotSettingsEntryCount =
        m_deferredPlotSettingsEntries.size();
    checkpoint.emittedDwgGroupDictionaryEntries =
        m_emittedDwgGroupDictionaryEntries;
    checkpoint.emittedDwgGroupFrames = m_emittedDwgGroupFrames;
    checkpoint.dwgGroupDictionaryComplete = m_dwgGroupDictionaryComplete;
    checkpoint.emittedDwgBlockRecords = m_emittedDwgBlockRecords;
    checkpoint.emittedDwgBlockEntityHandles = m_emittedDwgBlockEntityHandles;
    checkpoint.dwgBlockStructureComplete = m_dwgBlockStructureComplete;
    checkpoint.frameWriteError = m_frameWriteError;
    checkpoint.entityEedWriteFailure = m_entityEedWriteFailure;
    checkpoint.objectEedWriteFailure = m_objectEedWriteFailure;
    checkpoint.userBlockCount = m_userBlocks.size();
    checkpoint.userBlockHandles.clear();
    checkpoint.userBlockHandles.insert(m_userBlockHandles.cbegin(),
                                       m_userBlockHandles.cend());
    checkpoint.activeUserBlockRecordHandle = m_activeUserBlockRecordHandle;
    checkpoint.blockControlEmitted = m_blockControlEmitted;
    return checkpoint;
}

dwgWriter::CompoundWriteCheckpoint
dwgWriter15::checkpointPublicTransaction() const {
    CompoundWriteCheckpoint checkpoint = checkpointCompoundWrite();
    checkpoint.hasAdmissionState = true;
    checkpoint.writingContext = m_writingCtx;
    checkpoint.pendingLTypes = m_pendingLTypes;
    checkpoint.pendingLayers = m_pendingLayers;
    checkpoint.layer0 = m_layer0;
    checkpoint.haveLayer0 = m_haveLayer0;
    checkpoint.pendingStyles = m_pendingStyles;
    checkpoint.pendingUcs = m_pendingUcs;
    checkpoint.pendingViews = m_pendingViews;
    checkpoint.pendingVports = m_pendingVports;
    checkpoint.pendingViewportEntityHeaders =
        m_pendingViewportEntityHeaders;
    checkpoint.pendingDimstyles = m_pendingDimstyles;
    checkpoint.pendingAppIds = m_pendingAppIds;
    checkpoint.ltypeByBlock = m_ltypeByBlock;
    checkpoint.ltypeByLayer = m_ltypeByLayer;
    checkpoint.ltypeContinuous = m_ltypeContinuous;
    checkpoint.haveLtypeByBlock = m_haveLtypeByBlock;
    checkpoint.haveLtypeByLayer = m_haveLtypeByLayer;
    checkpoint.haveLtypeContinuous = m_haveLtypeContinuous;
    checkpoint.standardStyle = m_standardStyle;
    checkpoint.haveStandardStyle = m_haveStandardStyle;
    checkpoint.activeVport = m_activeVport;
    checkpoint.haveActiveVport = m_haveActiveVport;
    checkpoint.standardDimstyle = m_standardDimstyle;
    checkpoint.haveStandardDimstyle = m_haveStandardDimstyle;
    checkpoint.pendingDwgNamedObjectDictionaryEntries =
        m_pendingDwgNamedObjectDictionaryEntries;
    checkpoint.pendingDwgGroupDictionaryEntries =
        m_pendingDwgGroupDictionaryEntries;
    return checkpoint;
}

void dwgWriter15::rollbackCompoundWrite(
    const CompoundWriteCheckpoint& checkpoint) {
    // Rollback must remove bytes and ownership state, but an EED validation
    // failure is a file-level error and must remain visible to dwgRW::write().
    const bool entityEedFailure = m_entityEedWriteFailure;
    const bool objectEedFailure = m_objectEedWriteFailure;
    invalidatePendingViewportEntityHeaderIndex();
    invalidatePendingDwgDictionaryIndexes();
    invalidateEmittedDwgGroupEntryIndex();
    invalidateEmittedDwgGroupFrameIndex();
    m_dwgGroupMemberTargetHandles.clear();
    m_dwgGroupMemberTargetIndexReady = false;
    if (checkpoint.ac1021PolylineGroupVertexCount <= m_ac1021PolylineGroupVertexLedger.size()) {
        for (std::size_t index = checkpoint.ac1021PolylineGroupVertexCount;
             index < m_ac1021PolylineGroupVertexLedger.size(); ++index)
            m_ac1021PolylineGroupVertexIndex.erase(m_ac1021PolylineGroupVertexLedger[index]);
        m_ac1021PolylineGroupVertexLedger.resize(checkpoint.ac1021PolylineGroupVertexCount);
    }
    dwgWriter::rollbackCompoundWrite(checkpoint);
    if (checkpoint.emittedLayoutAssociationCount
        <= m_emittedLayoutAssociations.size()) {
        m_emittedLayoutAssociations.resize(
            checkpoint.emittedLayoutAssociationCount);
    }
    if (checkpoint.emittedLayoutDictionaryCount
        <= m_emittedLayoutDictionaries.size()) {
        m_emittedLayoutDictionaries.resize(
            checkpoint.emittedLayoutDictionaryCount);
    }
    if (checkpoint.emittedDwgDictionaryCount
        <= m_emittedDwgDictionaries.size()) {
        m_emittedDwgDictionaries.resize(
            checkpoint.emittedDwgDictionaryCount);
    }
    if (checkpoint.emittedEmptyExtensionDictionaryCount
        <= m_emittedEmptyExtensionDictionaries.size()) {
        m_emittedEmptyExtensionDictionaries.resize(
            checkpoint.emittedEmptyExtensionDictionaryCount);
    }
    if (checkpoint.emittedDwgTableStyleCount
        <= m_emittedDwgTableStyles.size()) {
        m_emittedDwgTableStyles.resize(
            checkpoint.emittedDwgTableStyleCount);
    }
    const auto eraseRolledBackEntityHandles =
        [this](std::uint32_t ownerHandle,
               const std::vector<std::uint32_t>& handles,
               std::size_t retainedCount) {
            for (std::size_t index = retainedCount;
                 index < handles.size(); ++index)
                m_blockOwnedEntityHandles.erase(
                    std::make_pair(ownerHandle, handles[index]));
        };
    const auto eraseRolledBackInsertReferences =
        [this](std::uint32_t ownerHandle,
               const std::vector<std::uint32_t>& handles,
               std::size_t retainedCount) {
            for (std::size_t index = retainedCount;
                 index < handles.size(); ++index)
                m_blockInsertReferenceHandles.erase(
                    blockInsertReferenceKey(ownerHandle, handles[index]));
        };
    if (checkpoint.objectMapSize <= m_objectMap.size()) {
        for (std::size_t index = checkpoint.objectMapSize;
             index < m_objectMap.size(); ++index)
            m_emittedDwgObjectHandles.erase(m_objectMap[index].first);
        m_objectMap.resize(checkpoint.objectMapSize);
    }
    if (checkpoint.modelSpaceEntityCount <= m_modelSpaceEntityHandles.size()) {
        eraseRolledBackEntityHandles(
            reservedHandle::BLOCK_MODEL_SPACE, m_modelSpaceEntityHandles,
            checkpoint.modelSpaceEntityCount);
        m_modelSpaceEntityHandles.resize(checkpoint.modelSpaceEntityCount);
    }
    if (checkpoint.paperSpaceEntityCount <= m_paperSpaceEntityHandles.size()) {
        eraseRolledBackEntityHandles(
            reservedHandle::BLOCK_PAPER_SPACE, m_paperSpaceEntityHandles,
            checkpoint.paperSpaceEntityCount);
        m_paperSpaceEntityHandles.resize(checkpoint.paperSpaceEntityCount);
    }
    if (checkpoint.modelSpaceInsertCount <= m_modelSpaceInsertHandles.size()) {
        eraseRolledBackInsertReferences(
            reservedHandle::BLOCK_MODEL_SPACE, m_modelSpaceInsertHandles,
            checkpoint.modelSpaceInsertCount);
        m_modelSpaceInsertHandles.resize(checkpoint.modelSpaceInsertCount);
    }
    if (checkpoint.paperSpaceInsertCount <= m_paperSpaceInsertHandles.size()) {
        eraseRolledBackInsertReferences(
            reservedHandle::BLOCK_PAPER_SPACE, m_paperSpaceInsertHandles,
            checkpoint.paperSpaceInsertCount);
        m_paperSpaceInsertHandles.resize(checkpoint.paperSpaceInsertCount);
    }
    if (checkpoint.userBlockEntityCounts.size() == m_userBlocks.size()) {
        for (std::size_t index = 0; index < m_userBlocks.size(); ++index) {
            const std::size_t count = checkpoint.userBlockEntityCounts[index];
            if (count <= m_userBlocks[index].entityHandles.size()) {
                eraseRolledBackEntityHandles(
                    m_userBlocks[index].blockRecordHandle,
                    m_userBlocks[index].entityHandles, count);
                m_userBlocks[index].entityHandles.resize(count);
            }
        }
    }
    if (checkpoint.userBlockInsertCounts.size() == m_userBlocks.size()) {
        for (std::size_t index = 0; index < m_userBlocks.size(); ++index) {
            const std::size_t count = checkpoint.userBlockInsertCounts[index];
            if (count <= m_userBlocks[index].insertHandles.size()) {
                eraseRolledBackInsertReferences(
                    m_userBlocks[index].blockRecordHandle,
                    m_userBlocks[index].insertHandles, count);
                m_userBlocks[index].insertHandles.resize(count);
            }
        }
    }
    const std::size_t previousUserBlockCount = m_userBlocks.size();
    if (checkpoint.userBlockCount <= m_userBlocks.size()) {
        for (std::size_t index = checkpoint.userBlockCount;
             index < m_userBlocks.size(); ++index) {
            eraseRolledBackEntityHandles(
                m_userBlocks[index].blockRecordHandle,
                m_userBlocks[index].entityHandles, 0);
            eraseRolledBackInsertReferences(
                m_userBlocks[index].blockRecordHandle,
                m_userBlocks[index].insertHandles, 0);
        }
        m_userBlocks.resize(checkpoint.userBlockCount);
    }
    if (m_userBlocks.size() != previousUserBlockCount)
        invalidateUserBlockIndex();
    m_userBlockHandles.clear();
    m_userBlockHandles.insert(checkpoint.userBlockHandles.cbegin(),
                              checkpoint.userBlockHandles.cend());
    m_emittedDwgTableRecordHandles =
        checkpoint.emittedDwgTableRecordHandles;
    m_emittedDwgTableControlHandles =
        checkpoint.emittedDwgTableControlHandles;
    m_dwgTableRecordsComplete = checkpoint.dwgTableRecordsComplete;
    m_emittedDwgNamedObjectDictionaryEntries =
        checkpoint.emittedDwgNamedObjectDictionaryEntries;
    m_dwgNamedObjectDictionaryComplete =
        checkpoint.dwgNamedObjectDictionaryComplete;
    m_plotSettingsDictionaryHandle =
        checkpoint.plotSettingsDictionaryHandle;
    m_defaultPlotStyleDictionaryHandle = checkpoint.defaultPlotStyleDictionaryHandle;
    m_defaultPlotStylePlaceholderHandle = checkpoint.defaultPlotStylePlaceholderHandle;
    m_defaultPlotStyleGraphEmitted = checkpoint.defaultPlotStyleGraphEmitted;
    m_defaultMaterialHandles = checkpoint.defaultMaterialHandles;
    m_defaultMaterialGraphEmitted = checkpoint.defaultMaterialGraphEmitted;
    m_canonicalLayerHandles = checkpoint.canonicalLayerHandles;
    m_canonicalPlotTarget = checkpoint.canonicalPlotTarget;
    m_canonicalPlotDictionary = checkpoint.canonicalPlotDictionary;
    m_canonicalPlaceholderOwner = checkpoint.canonicalPlaceholderOwner;
    m_canonicalPlotDictionarySeen = checkpoint.canonicalPlotDictionarySeen;
    m_canonicalPlaceholderSeen = checkpoint.canonicalPlaceholderSeen;
    m_deferredPlotSettingsDictionary =
        checkpoint.deferredPlotSettingsDictionary;
    m_deferredPlotSettingsDictionaryEmitted =
        checkpoint.deferredPlotSettingsDictionaryEmitted;
    if (checkpoint.deferredPlotSettingsEntryCount
        <= m_deferredPlotSettingsEntries.size()) {
        m_deferredPlotSettingsEntries.resize(
            checkpoint.deferredPlotSettingsEntryCount);
    }
    m_emittedDwgGroupDictionaryEntries =
        checkpoint.emittedDwgGroupDictionaryEntries;
    m_emittedDwgGroupFrames = checkpoint.emittedDwgGroupFrames;
    m_dwgGroupDictionaryComplete = checkpoint.dwgGroupDictionaryComplete;
    m_emittedDwgBlockRecords = checkpoint.emittedDwgBlockRecords;
    m_emittedDwgBlockEntityHandles =
        checkpoint.emittedDwgBlockEntityHandles;
    m_dwgBlockStructureComplete = checkpoint.dwgBlockStructureComplete;
    m_frameWriteError = checkpoint.frameWriteError;
    m_entityEedWriteFailure = checkpoint.entityEedWriteFailure
        || entityEedFailure;
    m_objectEedWriteFailure = checkpoint.objectEedWriteFailure
        || objectEedFailure;
    m_activeUserBlockRecordHandle = checkpoint.activeUserBlockRecordHandle;
    m_blockControlEmitted = checkpoint.blockControlEmitted;
    if (checkpoint.hasAdmissionState) {
        m_writingCtx = checkpoint.writingContext;
        m_pendingLTypes = checkpoint.pendingLTypes;
        m_pendingLayers = checkpoint.pendingLayers;
        m_layer0 = checkpoint.layer0;
        m_haveLayer0 = checkpoint.haveLayer0;
        m_pendingStyles = checkpoint.pendingStyles;
        m_pendingUcs = checkpoint.pendingUcs;
        m_pendingViews = checkpoint.pendingViews;
        m_pendingVports = checkpoint.pendingVports;
        m_pendingViewportEntityHeaders =
            checkpoint.pendingViewportEntityHeaders;
        m_pendingDimstyles = checkpoint.pendingDimstyles;
        m_pendingAppIds = checkpoint.pendingAppIds;
        m_ltypeByBlock = checkpoint.ltypeByBlock;
        m_ltypeByLayer = checkpoint.ltypeByLayer;
        m_ltypeContinuous = checkpoint.ltypeContinuous;
        m_haveLtypeByBlock = checkpoint.haveLtypeByBlock;
        m_haveLtypeByLayer = checkpoint.haveLtypeByLayer;
        m_haveLtypeContinuous = checkpoint.haveLtypeContinuous;
        m_standardStyle = checkpoint.standardStyle;
        m_haveStandardStyle = checkpoint.haveStandardStyle;
        m_activeVport = checkpoint.activeVport;
        m_haveActiveVport = checkpoint.haveActiveVport;
        m_standardDimstyle = checkpoint.standardDimstyle;
        m_haveStandardDimstyle = checkpoint.haveStandardDimstyle;
        m_pendingDwgNamedObjectDictionaryEntries =
            checkpoint.pendingDwgNamedObjectDictionaryEntries;
        m_pendingDwgGroupDictionaryEntries =
            checkpoint.pendingDwgGroupDictionaryEntries;
    }
    m_objectBody.reset();
    m_objectStrings.reset();
    m_objectHandles.reset();
    m_currentHandle = 0;
    m_lastDwgObjectFrame = {};
    m_lastDwgObjectFrameValid = false;
    clearLastEntityWriteCheckpoint();
}

bool dwgWriter15::stageLegacyModelChain(
    const std::vector<DRW_DwgLegacyModelChainEntry>& plan) {
    return stageLegacyModelChain(plan, {});
}

const DRW_DwgLegacyPolylineChain* dwgWriter15::legacyPolylinePlan(
    std::uint32_t parent) const noexcept {
    const auto it = m_legacyPolylineChains.find(parent);
    return it == m_legacyPolylineChains.end() ? nullptr : &it->second;
}

bool dwgWriter15::stageLegacyModelChain(
    const std::vector<DRW_DwgLegacyModelChainEntry>& plan,
    const std::vector<DRW_DwgLegacyPolylineChain>& polylines) {
    return stageLegacyModelChain(plan, polylines, {}, {});
}

const DRW_DwgLegacyInsertChain* dwgWriter15::legacyInsertPlan(
    std::uint32_t parent) const noexcept {
    const auto it = m_legacyInsertChains.find(parent);
    return it == m_legacyInsertChains.end() ? nullptr : &it->second;
}

bool dwgWriter15::stageLegacyModelChain(
    const std::vector<DRW_DwgLegacyModelChainEntry>& plan,
    const std::vector<DRW_DwgLegacyPolylineChain>& polylines,
    const DRW_DwgLegacyEmptyBlock& block,
    const std::vector<DRW_DwgLegacyInsertChain>& inserts) {
    return stageLegacyModelChain(plan, polylines, block, inserts, {});
}

bool dwgWriter15::stageLegacyModelChain(
    const std::vector<DRW_DwgLegacyModelChainEntry>& plan,
    const std::vector<DRW_DwgLegacyPolylineChain>& polylines,
    const DRW_DwgLegacyEmptyBlock& block,
    const std::vector<DRW_DwgLegacyInsertChain>& inserts,
    const std::vector<DRW_DwgLegacyModelChainEntry>& blockContent) {
    if (m_version != DRW::AC1015 || m_legacyModelChainActive
        || m_dwgTableRecordsComplete || !m_buf.data().empty() || plan.empty()
        || plan.size() > dwgSafety::MaxOwnedObjectCount
        || polylines.size() > plan.size() || inserts.size() > plan.size()
        || blockContent.size() > dwgSafety::MaxOwnedObjectCount
        || (!blockContent.empty() && inserts.empty()))
        return false;
    const bool hasBlock = !inserts.empty();
    if ((!hasBlock && (!block.name.empty() || block.blockRecordHandle != 0u
                       || block.blockHandle != 0u || block.endBlockHandle != 0u))
        || (hasBlock && (block.name.empty() || block.name.size() > 255u
             || !std::all_of(block.name.begin(), block.name.end(), [](unsigned char c) {
                 return (c >= 'A' && c <= 'Z')
                     || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '$';
             }))))
        return false;
    // Bound the complete graph before allocating indexes or copying chains.
    std::size_t identityCount = plan.size();
    if (blockContent.size() > dwgSafety::MaxDwgReadObjectCount - identityCount) return false;
    identityCount += blockContent.size();
    if (hasBlock) {
        if (identityCount > dwgSafety::MaxDwgReadObjectCount - 3u) return false;
        identityCount += 3u;
    }
    for (const auto& insert : inserts) {
        if (identityCount >= dwgSafety::MaxDwgReadObjectCount
            || insert.attributeHandles.size()
                > dwgSafety::MaxDwgReadObjectCount - identityCount - 1u)
            return false;
        identityCount += insert.attributeHandles.size() + 1u;
    }
    for (const auto& polyline : polylines) {
        if (identityCount >= dwgSafety::MaxDwgReadObjectCount
            || polyline.vertexHandles.size()
                > dwgSafety::MaxDwgReadObjectCount - identityCount - 1u)
            return false;
        identityCount += polyline.vertexHandles.size() + 1u;
    }
    try {
        std::set<std::uint32_t> seen;
        std::set<std::uint32_t> polylineParents;
        std::set<std::uint32_t> insertParents;
        for (const auto& entry : plan) {
            if (entry.handle < 0x30u
                || entry.handle == std::numeric_limits<std::uint32_t>::max()
                || m_handles.isUnavailable(entry.handle)
                || !seen.insert(entry.handle).second
                || (entry.type != DRW::LINE && entry.type != DRW::CIRCLE
                    && entry.type != DRW::ARC && entry.type != DRW::POLYLINE
                    && entry.type != DRW::INSERT))
                return false;
            if (entry.type == DRW::POLYLINE) polylineParents.insert(entry.handle);
            if (entry.type == DRW::INSERT) insertParents.insert(entry.handle);
        }
        for (const auto& entry : blockContent) {
            if (entry.handle < 0x30u
                || entry.handle == std::numeric_limits<std::uint32_t>::max()
                || m_handles.isUnavailable(entry.handle)
                || !seen.insert(entry.handle).second
                || (entry.type != DRW::LINE && entry.type != DRW::CIRCLE
                    && entry.type != DRW::ARC))
                return false;
        }
        if (polylineParents.size() != polylines.size()
            || insertParents.size() != inserts.size()) return false;
        if (hasBlock) {
            for (const auto handle : {block.blockRecordHandle, block.blockHandle, block.endBlockHandle}) {
                if (handle < 0x30u || handle == std::numeric_limits<std::uint32_t>::max()
                    || m_handles.isUnavailable(handle) || !seen.insert(handle).second)
                    return false;
            }
        }
        std::map<std::uint32_t, DRW_DwgLegacyInsertChain> insertChains;
        std::map<std::uint32_t, std::pair<std::uint32_t, std::size_t>> insertChildren;
        for (const auto& insert : inserts) {
            if (insertParents.count(insert.parentHandle) == 0
                || insert.blockRecordHandle != block.blockRecordHandle
                || insert.attributeHandles.empty()
                || insert.attributeHandles.size() > dwgSafety::MaxOwnedObjectCount
                || !insertChains.emplace(insert.parentHandle, insert).second)
                return false;
            for (std::size_t i = 0; i <= insert.attributeHandles.size(); ++i) {
                const auto handle = i == insert.attributeHandles.size()
                    ? insert.seqendHandle : insert.attributeHandles[i];
                if (handle < 0x30u || handle == std::numeric_limits<std::uint32_t>::max()
                    || m_handles.isUnavailable(handle) || !seen.insert(handle).second
                    || !insertChildren.emplace(handle, std::make_pair(insert.parentHandle, i)).second)
                    return false;
            }
        }
        std::map<std::uint32_t, DRW_DwgLegacyPolylineChain> chains;
        std::map<std::uint32_t, std::pair<std::uint32_t, std::size_t>> children;
        for (const auto& polyline : polylines) {
            if (polylineParents.count(polyline.parentHandle) == 0
                || polyline.vertexHandles.size() < 2u
                || polyline.vertexHandles.size() > dwgSafety::MaxOwnedObjectCount
                || !chains.emplace(polyline.parentHandle, polyline).second)
                return false;
            for (std::size_t i = 0; i <= polyline.vertexHandles.size(); ++i) {
                const auto handle = i == polyline.vertexHandles.size()
                    ? polyline.seqendHandle : polyline.vertexHandles[i];
                if (handle < 0x30u || handle == std::numeric_limits<std::uint32_t>::max()
                    || m_handles.isUnavailable(handle) || !seen.insert(handle).second
                    || !children.emplace(handle, std::make_pair(polyline.parentHandle, i)).second)
                    return false;
            }
        }
        auto savedHandles = m_handles;
        try {
            for (const auto handle : seen) m_handles.reserveForWrite(handle);
            m_legacyModelChain = plan;
            m_legacyBlockContent = blockContent;
            m_legacyEmptyBlock = block;
            m_legacyEmptyBlockActive = hasBlock;
            m_legacyInsertChains = std::move(insertChains);
            m_legacyInsertChildOwners = std::move(insertChildren);
            m_legacyPolylineChains = std::move(chains);
            m_legacyPolylineChildOwners = std::move(children);
            m_legacyModelChainHandles = std::move(seen);
        } catch (...) {
            m_handles = std::move(savedHandles);
            return false;
        }
    } catch (...) { return false; }
    m_legacyModelChainActive = true;
    return true;
}

bool dwgWriter15::emitDeferredBlockControl() {
    if (m_legacyModelChainActive
        && (m_legacyModelChainConflict
            || m_modelSpaceEntityHandles.size() != m_legacyModelChain.size()
            || !m_paperSpaceEntityHandles.empty()
            || m_userBlocks.size() != (m_legacyEmptyBlockActive ? 1u : 0u)
            || (m_legacyEmptyBlockActive
                && (m_userBlocks.front().blockRecordHandle != m_legacyEmptyBlock.blockRecordHandle
                    || m_userBlocks.front().entityHandles.size() != m_legacyBlockContent.size()))))
        return false;
    if (m_legacyModelChainActive) {
        if (m_legacyEmptyBlockActive) {
            const auto& block = m_userBlocks.front();
            if (block.name != m_legacyEmptyBlock.name
                || block.blockHandle != m_legacyEmptyBlock.blockHandle
                || block.endBlockHandle != m_legacyEmptyBlock.endBlockHandle)
                return false;
            for (std::size_t i = 0; i < m_legacyBlockContent.size(); ++i)
                if (block.entityHandles[i] != m_legacyBlockContent[i].handle) return false;
            std::size_t referenceIndex = 0;
            for (const auto& entry : m_legacyModelChain) {
                if (entry.type != DRW::INSERT) continue;
                if (referenceIndex >= block.insertHandles.size()
                    || block.insertHandles[referenceIndex++] != entry.handle)
                    return false;
            }
            if (referenceIndex != block.insertHandles.size()) return false;
        }
        for (const auto& entry : m_legacyPolylineChains)
            if (!hasEmittedDwgObjectHandle(entry.second.seqendHandle)) return false;
        for (const auto& entry : m_legacyInsertChains)
            if (!hasEmittedDwgObjectHandle(entry.second.seqendHandle)) return false;
    }
    if (m_blockControlEmitted)
        return true;
    if (m_activeUserBlockRecordHandle != 0)
        return false;

    const CompoundWriteCheckpoint checkpoint = checkpointCompoundWrite();
    m_dwgBlockStructureComplete = false;
    m_emittedDwgBlockRecords.clear();

    const auto fail = [this, &checkpoint]() {
        rollbackCompoundWrite(checkpoint);
        return false;
    };

    std::uint32_t modelLayoutHandle = 0;
    std::uint32_t paperLayoutHandle = 0;
    if (!validateDwgLayoutBlockAssociations(
            modelLayoutHandle, paperLayoutHandle))
        return fail();

    // Reuse one bounded handle buffer for legacy ordering and membership
    // validation. Modern BLOCK_RECORD lists keep source order and need no
    // ownership copy during emission.
    std::vector<std::uint32_t> entityHandleScratch;
    const auto copyEntityHandlesToScratch =
        [&entityHandleScratch](const std::vector<std::uint32_t>& handles) {
            if (handles.size() > dwgSafety::MaxOwnedObjectCount)
                return false;
            try {
                entityHandleScratch.assign(handles.cbegin(), handles.cend());
            } catch (...) {
                return false;
            }
            return true;
        };

    if (m_version <= DRW::AC1015 && !m_legacyModelChainActive) {
        // Optional custom entities are valid in the legacy object map but may
        // carry preserved high handles; do not let those frames make the
        // mandatory built-in entity chain appear non-contiguous. Keep this
        // allowlist narrow: only typed entities whose class definitions are
        // already present in the writer manifest may use it.
        const auto isLegacyOptionalCustomEntity = [this](std::uint32_t handle) {
            const auto owner = m_dwgClassInstanceHandleOwners.find(handle);
            if (owner == m_dwgClassInstanceHandleOwners.end())
                return false;
            const DwgClassDefinition* definition =
                findDwgClassDefinition(owner->second);
            return definition != nullptr
                && (definition->m_className == "AcDbRText"
                    || definition->m_className == "AcDbArcAlignedText"
                    || definition->m_className == "AcDbHelix"
                    || definition->m_className == "AcDbCamera");
        };
        const auto isContiguousChain = [](const std::vector<std::uint32_t>& handles) {
            for (std::size_t index = 1; index < handles.size(); ++index) {
                if (handles[index - 1] == std::numeric_limits<std::uint32_t>::max()
                    || handles[index] != handles[index - 1] + 1u)
                    return false;
            }
            return true;
        };
        if (!copyEntityHandlesToScratch(m_modelSpaceEntityHandles))
            return fail();
        entityHandleScratch.erase(
            std::remove_if(entityHandleScratch.begin(),
                           entityHandleScratch.end(),
                           isLegacyOptionalCustomEntity),
            entityHandleScratch.end());
        std::sort(entityHandleScratch.begin(), entityHandleScratch.end());
        if (!isContiguousChain(entityHandleScratch))
            return fail();

        if (!copyEntityHandlesToScratch(m_paperSpaceEntityHandles))
            return fail();
        entityHandleScratch.erase(
            std::remove_if(entityHandleScratch.begin(),
                           entityHandleScratch.end(),
                           isLegacyOptionalCustomEntity),
            entityHandleScratch.end());
        std::sort(entityHandleScratch.begin(), entityHandleScratch.end());
        if (!isContiguousChain(entityHandleScratch))
            return fail();
        for (const PendingUserBlock& block : m_userBlocks) {
            if (!copyEntityHandlesToScratch(block.entityHandles))
                return fail();
            entityHandleScratch.erase(
                std::remove_if(entityHandleScratch.begin(),
                               entityHandleScratch.end(),
                               isLegacyOptionalCustomEntity),
                entityHandleScratch.end());
            std::sort(entityHandleScratch.begin(), entityHandleScratch.end());
            if (!isContiguousChain(entityHandleScratch))
                return fail();
        }
    }

    // Every child listed by a BLOCK_RECORD must already have a committed
    // object frame. Reuse a single per-list scratch buffer to detect duplicate
    // entries in O(n log n), then resolve frame membership through the
    // publication index. The final object-map sort belongs to HANDLES, after
    // all structural frames have been appended.
    const auto validateEntityList = [this, &entityHandleScratch,
                                     &copyEntityHandlesToScratch](
            const std::vector<std::uint32_t>& handles) {
        if (!copyEntityHandlesToScratch(handles))
            return false;
        std::sort(entityHandleScratch.begin(), entityHandleScratch.end());
        if (std::adjacent_find(entityHandleScratch.cbegin(),
                               entityHandleScratch.cend())
            != entityHandleScratch.cend())
            return false;
        for (const std::uint32_t handle : entityHandleScratch) {
            if (handle == 0 || !hasEmittedDwgObjectHandle(handle))
                return false;
        }
        return true;
    };
    for (const PendingUserBlock& block : m_userBlocks) {
        if (!validateEntityList(block.entityHandles)
            || !validateEntityList(block.insertHandles))
            return fail();
        if (block.hasExpectedInsertHandles
            && block.insertHandles != block.expectedInsertHandles)
            return fail();
        if (block.hasWriteMetadata
            && (block.writeMetadata.flags & (4 | 8)) != 0) {
            if (!block.entityHandles.empty())
                return fail();
        }
    }
    if (!validateEntityList(m_modelSpaceEntityHandles)
        || !validateEntityList(m_paperSpaceEntityHandles)
        || !validateEntityList(m_modelSpaceInsertHandles)
        || !validateEntityList(m_paperSpaceInsertHandles))
        return fail();

    for (const PendingUserBlock& block : m_userBlocks) {
        const std::vector<std::uint32_t>* entityHandles =
            &block.entityHandles;
        if (m_version <= DRW::AC1015 && !m_legacyModelChainActive) {
            if (!copyEntityHandlesToScratch(block.entityHandles))
                return fail();
            std::sort(entityHandleScratch.begin(),
                      entityHandleScratch.end());
            entityHandles = &entityHandleScratch;
        }
        struct PlannedBlockRecordScope {
            bool& flag; bool old;
            PlannedBlockRecordScope(bool& value, bool active) : flag(value), old(value) { if (active) flag = true; }
            ~PlannedBlockRecordScope() { flag = old; }
        } plannedRecord(m_legacyPlannedFrameWriting, m_legacyEmptyBlockActive
                        && block.blockRecordHandle == m_legacyEmptyBlock.blockRecordHandle);
        if (!emitBlockRecord(block.blockRecordHandle, block.name,
                             block.basePoint, block.blockHandle,
                             block.endBlockHandle, *entityHandles,
                             block.insertHandles,
                             block.insUnits,
                             block.hasWriteMetadata
                                 ? &block.writeMetadata : nullptr,
                             block.layoutHandle))
            return fail();
        if (m_emittedDwgBlockRecords.size() == 1
            && consumeWriteFailurePointForTest(kAfterFirstBlockRecord))
            return fail();
    }

    // Modelspace and paperspace records are emitted only after the entity
    // callback has supplied every member of their owned-handle lists. Their
    // entities use ownerless entmode 2/1; the BLOCK_RECORD list is the
    // authoritative reachability graph.
    const DRW_Coord origin{0.0, 0.0, 0.0};
    if (m_version <= DRW::AC1015 && !m_legacyModelChainActive) {
        if (!copyEntityHandlesToScratch(m_modelSpaceEntityHandles))
            return fail();
        std::sort(entityHandleScratch.begin(), entityHandleScratch.end());
        if (!emitBlockRecord(reservedHandle::BLOCK_MODEL_SPACE, "*Model_Space",
                             origin, reservedHandle::BLOCK_MODEL_ENTITY,
                             reservedHandle::BLOCK_MODEL_END,
                             entityHandleScratch, m_modelSpaceInsertHandles,
                             0, nullptr, modelLayoutHandle))
            return fail();
        if (!copyEntityHandlesToScratch(m_paperSpaceEntityHandles))
            return fail();
        std::sort(entityHandleScratch.begin(), entityHandleScratch.end());
        if (!emitBlockRecord(reservedHandle::BLOCK_PAPER_SPACE, "*Paper_Space",
                             origin, reservedHandle::BLOCK_PAPER_ENTITY,
                             reservedHandle::BLOCK_PAPER_END,
                             entityHandleScratch, m_paperSpaceInsertHandles,
                             0, nullptr, paperLayoutHandle))
            return fail();
    } else if (!emitBlockRecord(
                   reservedHandle::BLOCK_MODEL_SPACE, "*Model_Space", origin,
                   reservedHandle::BLOCK_MODEL_ENTITY,
                   reservedHandle::BLOCK_MODEL_END, m_modelSpaceEntityHandles,
                   m_modelSpaceInsertHandles, 0, nullptr,
                   modelLayoutHandle)
               || !emitBlockRecord(
                   reservedHandle::BLOCK_PAPER_SPACE, "*Paper_Space", origin,
                   reservedHandle::BLOCK_PAPER_ENTITY,
                   reservedHandle::BLOCK_PAPER_END, m_paperSpaceEntityHandles,
                   m_paperSpaceInsertHandles, 0, nullptr,
                   paperLayoutHandle)) {
        return fail();
    }

    // BLOCK_CONTROL: numEntries = user blocks count; +2 phantoms for
    // MODEL_SPACE + PAPER_SPACE are appended to the child handle list.
    std::vector<std::uint32_t> children;
    children.reserve(m_userBlocks.size() + 2);
    for (const PendingUserBlock& block : m_userBlocks)
        children.push_back(block.blockRecordHandle);
    children.push_back(reservedHandle::BLOCK_MODEL_SPACE);
    children.push_back(reservedHandle::BLOCK_PAPER_SPACE);

    dwgBufferW& body = beginObject(reservedHandle::BLOCK_CONTROL);
    body.putObjType(m_version, DRW::DwgBlockControlObjectType);
    if (m_version > DRW::AC1014 && m_version < DRW::AC1024) {
        body.putRawLong32(0);  // objSize stub (R2000/R2004 only)
    }
    body.putHandle(makeOwnHandle(reservedHandle::BLOCK_CONTROL));  // own handle: code 0 (plan 3.6)
    body.putBitShort(0);  // extDataSize
    body.putBitLong(0);   // numReactors
    if (m_version > DRW::AC1015) {
        body.putBit(0);   // xDictFlag=0 (R2004+)
    }
    if (m_version > DRW::AC1024) {
        body.putBit(0);   // Have binary data (AC1027+)
    }
    body.putBitLong(static_cast<std::int32_t>(m_userBlocks.size()));
    if (m_version > DRW::AC1018) {
        body.putBit(0);   // stringBit = 0 (R2007+)
    }
    dwgBufferW *hdlBuf = &body;
    // R2000-R2007 store a handle-start bit offset in the object header;
    // R2010+ uses a separate handle stream size. Stage these handles so each
    // version's finishObject assembles and records its boundary correctly.
    if (m_version > DRW::AC1014) {
        m_objectHandles.reset();
        hdlBuf = &m_objectHandles;
    }
    hdlBuf->putHandle(makeSoftPointer(0)); // NullH (typed soft pointer)
    hdlBuf->putHandle(makeHardOwner(0));  // XDic null
    for (std::size_t index = 0; index < children.size(); ++index) {
        const std::uint32_t handle = children[index];
        const bool isModelOrPaperSpace = index >= m_userBlocks.size();
        hdlBuf->putHandle(isModelOrPaperSpace
                              ? makeHardOwner(handle)
                              : makeSoftOwner(handle));
    }
    finishObject();
    if (objectWriteFailed())
        return fail();
    m_blockControlEmitted = true;
    m_dwgBlockStructureComplete = true;
    return true;
}

bool dwgWriter15::emitBlockEntity(std::uint32_t handle,
                                  const std::string& name, bool isEnd) {
    DRW_Block bk;
    bk.handle = handle;
    // These built-in paper-space boundary entities have no explicit owner;
    // their common entity mode therefore identifies their paper-space owner.
    if (handle == reservedHandle::BLOCK_PAPER_ENTITY
        || handle == reservedHandle::BLOCK_PAPER_END)
        bk.space = DRW::PaperSpace;
    bk.layerH.ref = DRW::DwgLayer0Handle;
    bk.color = 256;  // BYLAYER
    bk.name = name;
    bk.setIsEnd(isEnd);
    dwgBufferW& body = beginObject(handle);
    if (m_version > DRW::AC1018) {
        m_objectStrings.reset();
        m_objectHandles.reset();
        if (!bk.encodeDwg(m_version, &body, 0, &m_objectStrings,
                          &m_objectHandles)) {
            m_frameWriteError = true;
            return false;
        }
    } else if (m_version > DRW::AC1014) {
        m_objectHandles.reset();
        ScopedDwgLegacyEntityLinks links(
            m_legacyModelChainActive ? &body : nullptr,
            m_legacyModelChainActive ? &m_objectHandles : nullptr, 0, 0);
        if (!bk.encodeDwg(m_version, &body, 0, nullptr, &m_objectHandles)) {
            m_frameWriteError = true;
            return false;
        }
    } else {
        if (!bk.encodeDwg(m_version, &body, 0)) {
            m_frameWriteError = true;
            return false;
        }
    }
    finishObject();
    if (objectWriteFailed())
        return false;
    m_emittedDwgBlockEntityHandles.insert(handle);
    return true;
}

bool dwgWriter15::emitBlockRecord(std::uint32_t handle,
                                  const std::string& name,
                                  const DRW_Coord& basePoint,
                                  std::uint32_t blockHandle,
                                  std::uint32_t endBlockHandle,
                                  const std::vector<std::uint32_t>& entityHandles,
                                  const std::vector<std::uint32_t>& insertHandles,
                                  int insUnits,
                                  const DwgBlockRecordWriteMetadata* metadata,
                                  std::uint32_t layoutHandle) {
    if (entityHandles.size() > dwgSafety::MaxOwnedObjectCount
        || insertHandles.size() > dwgSafety::MaxOwnedObjectCount)
        return false;
    const int flags = metadata == nullptr ? 0 : metadata->flags;
    const bool blockIsXref = (flags & 4) != 0;
    const bool xrefOverlaid = (flags & 8) != 0;
    if ((blockIsXref || xrefOverlaid) && !entityHandles.empty())
        return false;
    if (metadata != nullptr && metadata->xrefUnloaded && !blockIsXref)
        return false;
    if (metadata != nullptr
        && metadata->previewData.size() > kMaxDwgBlockRecordPreviewBytes)
        return false;
    dwgBufferW& body = beginObject(handle);

    // Common table-entry preamble.
    body.putObjType(m_version, DRW::DwgBlockRecordObjectType);
    if (m_version > DRW::AC1014 && m_version < DRW::AC1024) {
        body.putRawLong32(0);              // objSize stub (R2000/R2004 only)
    }
    body.putHandle(makeOwnHandle(handle));  // own handle: code 0 (plan 3.6)
    body.putBitShort(0);                   // extDataSize
    body.putBitLong(0);                    // numReactors
    if (m_version > DRW::AC1015) {
        body.putBit(0);                    // xDictFlag=0 (R2004+)
    }
    if (m_version > DRW::AC1024) {
        body.putBit(0);                    // Have binary data (AC1027+)
    }

    // Block_Record body — mirrors DRW_Block_Record::parseDwg. AC1015/AC1018
    // keep strings inline but use objSize for the trailing handle stream.
    dwgBufferW *strBuf = (m_version > DRW::AC1018) ? &m_objectStrings : &body;
    dwgBufferW *hdlBuf = (m_version > DRW::AC1014) ? &m_objectHandles : &body;
    if (m_version > DRW::AC1018) {
        m_objectStrings.reset();
    }
    if (m_version > DRW::AC1014) {
        m_objectHandles.reset();
    }

    strBuf->putVariableText(m_version, name);
    if (m_version > DRW::AC1018) {
        // R2007 derives is_xref_ref from the table-record form and stores
        // only the BS resolved-state value. The dependent bit is derived
        // only when that value is 256; it is not emitted separately.
        body.putBitShort((flags & 16) != 0 ? 256u : 0u);
    } else {
        body.putBit((flags & 64) != 0);    // XREF reference state
        const std::uint16_t xrefIndexPlusOne = metadata != nullptr
            && metadata->xrefIndex >= 0
            ? static_cast<std::uint16_t>(metadata->xrefIndex + 1) : 0u;
        body.putBitShort(xrefIndexPlusOne);
        body.putBit((flags & 16) != 0);    // XREF dependent state
    }
    body.putBit((flags & 1) != 0);         // anonymous (*U)
    body.putBit((flags & 2) != 0);         // has attribute definitions
    body.putBit(blockIsXref);
    body.putBit(xrefOverlaid);
    if (m_version > DRW::AC1014) {
        // ODA §20.4.52 stores zero for a loaded XREF. This is the separate
        // BLOCK group-71 unload state, not DXF group-70 bit 32 (resolved).
        const bool xrefUnloaded = blockIsXref && metadata != nullptr
            && metadata->xrefUnloaded;
        body.putBit(xrefUnloaded);
    }
    if (m_version > DRW::AC1015 && !blockIsXref && !xrefOverlaid) {
        body.putBitLong(static_cast<std::int32_t>(entityHandles.size()));
    }
    body.put3BitDouble(basePoint);         // basePoint 3BD
    strBuf->putVariableText(m_version,
                            metadata == nullptr ? std::string{}
                                                : metadata->xrefPath);
    for (std::size_t index = 0; index < insertHandles.size(); ++index) {
        body.putRawChar8(1);               // one RC marker per INSERT
    }
    body.putRawChar8(0);                   // insertCount terminator (R2000+)
    strBuf->putVariableText(m_version,
                            metadata == nullptr ? std::string{}
                                                : metadata->description);
    const std::size_t previewSize = metadata == nullptr
        ? 0u : metadata->previewData.size();
    body.putBitLong(static_cast<std::int32_t>(previewSize));
    if (previewSize != 0u)
        body.putBytes(metadata->previewData.data(), previewSize);
    if (m_version > DRW::AC1018) {
        body.putBitShort(static_cast<std::uint16_t>(insUnits));
        body.putBit(metadata != nullptr && metadata->canExplode);
        body.putRawChar8(metadata == nullptr ? 0u
                                              : metadata->blockScaling);
    }

    // Handle stream.
    hdlBuf->putHandle(makeSoftPointer(reservedHandle::BLOCK_CONTROL)); // parent
    hdlBuf->putHandle(makeHardOwner(0));                              // XDic null
    hdlBuf->putHandle(makeHardPointer(0)); // ODA specifies NULL here.
    hdlBuf->putHandle(makeHardOwner(blockHandle));                    // BLOCK entity
    if (m_version <= DRW::AC1015 && !blockIsXref && !xrefOverlaid) {
        const std::uint32_t first = entityHandles.empty() ? 0 : entityHandles.front();
        const std::uint32_t last = entityHandles.empty() ? 0 : entityHandles.back();
        hdlBuf->putHandle(makeSoftPointer(first));
        hdlBuf->putHandle(makeSoftPointer(last));
    } else if (!blockIsXref && !xrefOverlaid) {
        for (std::uint32_t entityHandle : entityHandles)
            hdlBuf->putHandle(makeHardOwner(entityHandle));
    }
    hdlBuf->putHandle(makeHardOwner(endBlockHandle)); // ENDBLK entity
    for (std::uint32_t insertHandle : insertHandles)
        hdlBuf->putHandle(makeSoftPointer(insertHandle));
    hdlBuf->putHandle(makeHardPointer(layoutHandle)); // associated LAYOUT

    finishObject();
    if (objectWriteFailed())
        return false;
    DRW::DwgBlockWriteRecord result;
    result.blockRecordHandle = handle;
    result.blockHandle = blockHandle;
    result.endBlockHandle = endBlockHandle;
    result.entityHandles = entityHandles;
    result.insertHandles = insertHandles;
    m_emittedDwgBlockRecords.push_back(std::move(result));
    return true;
}

// --- Shared preamble helper for full table-record emitters -----------------
// Writes OT + objSize stub (R2000/R2004) + own-handle + extDataSize +
// numReactors + xDictFlag.  Sets up strBuf/hdlBuf for AC1024 three-stream;
// caller passes back the strBuf/hdlBuf pointers via the out-params so the
// subsequent encodeDwg call lands its strings/handles in the right buffer.
static bool emitRecordPreamble(dwgBufferW& body, DRW::Version version,
                                std::uint16_t otype, std::uint32_t handle,
                                dwgBufferW& strBuf, dwgBufferW& hdlBuf,
                                dwgBufferW*& sb, dwgBufferW*& hb,
                                std::int32_t numReactors = 0,
                                std::uint8_t xDictFlag = 0,
                                bool hasDsData = false,
                                const std::vector<DRW_Variant*>* extData = nullptr,
                                const std::vector<DRW_Entity::PendingHandleRef>* appIdRefs = nullptr,
                                const std::vector<DRW_Entity::PendingHandleRef>* layerRefs = nullptr,
                                std::uint16_t codePage = 30) {
    body.putObjType(version, otype);
    if (version > DRW::AC1014 && version < DRW::AC1024)
        body.putRawLong32(0);
    // The object's OWN handle at the start of the object data must use code 0
    // (handle-holder), not a code-4 reference: a code-4 here desyncs the whole
    // object on read ("Invalid object handle ... @7.2"). The control objects via
    // emitControlObject already use makeOwnHandle; the table records did not.
    // (write-review / plan 3.6)
    body.putHandle(makeOwnHandle(handle));
    if (extData == nullptr) {
        body.putBitShort(0);
    } else if (appIdRefs == nullptr || layerRefs == nullptr
               || !DRW_Entity::encodeDwgEed(version, *extData, *appIdRefs,
                                             *layerRefs, codePage, body)) {
        return false;
    }
    body.putBitLong(numReactors);
    if (version > DRW::AC1015)
        body.putBit(xDictFlag);   // xDictFlag
    if (version > DRW::AC1024)
        body.putBit(hasDsData ? 1 : 0);   // Have binary data (AC1027+)
    if (version > DRW::AC1018) {
        strBuf.reset();
        sb = &strBuf;
    } else {
        sb = &body;
    }
    if (version > DRW::AC1014) {
        hdlBuf.reset();
        hb = &hdlBuf;
    } else {
        hb = &body;
    }
    return body.isGood();
}

template <typename ExtData>
bool prepareDwgEedRefs(
    DRW::Version version, const ExtData& extData,
    const DRW_WritingContext& context,
    std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    appIdRefs.clear();
    layerRefs.clear();
    if (extData.empty())
        return true;

    bool haveAppId = false;
    try {
        for (std::size_t index = 0; index < extData.size(); ++index) {
            const auto& value = extData[index];
            if (!value)
                return false;
            if (value->code() == 1001) {
                if (value->type() != DRW_Variant::STRING
                    || value->content.s == nullptr || value->content.s->empty()) {
                    return false;
                }
                const auto appId = context.appidMap.find(
                    toUpperCase(*value->content.s));
                if (appId == context.appidMap.end())
                    return false;
                appIdRefs.push_back({index, appId->second});
                haveAppId = true;
            } else if (value->code() == 1003) {
                if (!haveAppId || value->type() != DRW_Variant::STRING
                    || value->content.s == nullptr || value->content.s->empty()) {
                    return false;
                }
                if (value->canReplayDwgRawLayerReference(version)) {
                    layerRefs.push_back(
                        {index, 0, value->rawDwgLayerReference(), true});
                    continue;
                }
                const auto layer = context.layerMap.find(
                    toUpperCase(*value->content.s));
                if (layer == context.layerMap.end())
                    return false;
                layerRefs.push_back({index, layer->second});
            } else if (!haveAppId) {
                return false;
            }
        }
    } catch (...) {
        return false;
    }
    return haveAppId;
}

bool readDwgRepresentableTableAppDataHandle(const DRW_Variant& value,
                                            int expectedCode,
                                            std::uint32_t& handle) {
    if (value.code() != expectedCode || value.type() != DRW_Variant::STRING
        || value.content.s == nullptr || value.content.s->empty())
        return false;
    const std::string& text = *value.content.s;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(),
                                        handle, 16);
    return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
}

bool hasOnlyRepresentedTableAppData(const DRW_TableEntry& entry) {
    if (entry.appData.empty())
        return true;

    std::vector<std::uint32_t> parsedReactors;
    bool sawXDictionary = false;
    for (const auto& group : entry.appData) {
        if (group.size() < 2u)
            return false;
        auto field = group.cbegin();
        if (field->code() != 102 || field->type() != DRW_Variant::STRING
            || field->content.s == nullptr)
            return false;
        const std::string& opener = *field->content.s;
        ++field;

        if (opener == "ACAD_REACTORS") {
            bool closed = false;
            for (; field != group.cend(); ++field) {
                if (field->code() == 102
                    && field->type() == DRW_Variant::STRING
                    && field->content.s != nullptr
                    && *field->content.s == "}") {
                    closed = std::next(field) == group.cend();
                    break;
                }
                std::uint32_t handle = 0u;
                if (!readDwgRepresentableTableAppDataHandle(*field, 330,
                                                            handle))
                    return false;
                if (handle != 0u)
                    parsedReactors.push_back(handle);
            }
            if (!closed)
                return false;
        } else if (opener == "ACAD_XDICTIONARY") {
            if (sawXDictionary)
                return false;
            sawXDictionary = true;
            if (field == group.cend())
                return false;
            std::uint32_t handle = 0u;
            if (!readDwgRepresentableTableAppDataHandle(*field, 360, handle))
                return false;
            ++field;
            if (field == group.cend() || field->code() != 102
                || field->type() != DRW_Variant::STRING
                || field->content.s == nullptr || *field->content.s != "}"
                || std::next(field) != group.cend()
                || handle != entry.xDictHandle)
                return false;
        } else {
            return false;
        }
    }
    return parsedReactors == entry.reactorHandles;
}

template <typename Object>
bool prepareDwgCommonObjectState(Object& object, DRW::Version version) {
    if (object.reactorHandles.size() > dwgSafety::MaxReactorCount
        || object.reactorHandles.size()
            > static_cast<std::size_t>(
                std::numeric_limits<std::int32_t>::max())
        || object.extensionDictionaryFlag() > 1)
        return false;

    const std::uint8_t effectiveXDictFlag = version > DRW::AC1015
        ? object.extensionDictionaryFlag() : 0;
    object.setDwgCommonObjectState(
        static_cast<std::int32_t>(object.reactorHandles.size()),
        effectiveXDictFlag, object.hasDataStorageBinaryData());
    return true;
}

bool dwgWriter15::finishTableRecord(std::uint32_t handle) {
    finishObject();
    if (objectWriteFailed())
        return false;
    m_emittedDwgTableRecordHandles.insert(handle);
    if (m_emittedDwgTableRecordHandles.size() == 1
        && consumeWriteFailurePointForTest(kAfterFirstDeferredTableRecord))
        return false;
    return true;
}

bool dwgWriter15::getEmittedDwgTableRecordHandles(
    std::vector<std::uint32_t>& handles) const {
    handles.clear();
    if (!m_dwgTableRecordsComplete)
        return false;
    handles.assign(m_emittedDwgTableRecordHandles.cbegin(),
                   m_emittedDwgTableRecordHandles.cend());
    return true;
}

bool dwgWriter15::getEmittedDwgTableControlHandles(
    std::vector<std::uint32_t>& handles) const {
    handles.clear();
    if (!m_dwgTableRecordsComplete)
        return false;
    handles.assign(m_emittedDwgTableControlHandles.cbegin(),
                   m_emittedDwgTableControlHandles.cend());
    return true;
}

bool dwgWriter15::ensurePendingDwgDictionaryIndexes() const {
    if (m_pendingDwgDictionaryIndexesReady)
        return true;

    std::unordered_set<std::string> namedObjectNames;
    std::unordered_set<std::uint32_t> namedObjectChildHandles;
    std::unordered_set<std::string> groupNames;
    std::unordered_set<std::uint32_t> groupHandles;
    try {
        namedObjectNames.reserve(
            m_pendingDwgNamedObjectDictionaryEntries.size());
        namedObjectChildHandles.reserve(
            m_pendingDwgNamedObjectDictionaryEntries.size());
        groupNames.reserve(m_pendingDwgGroupDictionaryEntries.size());
        groupHandles.reserve(m_pendingDwgGroupDictionaryEntries.size());
        std::uint32_t plotStyleHandle = 0;
        for (const auto& entry : m_pendingDwgNamedObjectDictionaryEntries) {
            const bool isPlotStyle = isPlotStyleDictionaryKey(entry.name);
            if (entry.name.empty() || entry.childHandle == 0
                || !namedObjectNames.insert(entry.name).second
                || (isPlotStyle && (entry.childHandle == DRW::NoHandle
                                    || plotStyleHandle != 0
                                    || namedObjectChildHandles.count(entry.childHandle) != 0))
                || (!isPlotStyle && plotStyleHandle != 0
                    && entry.childHandle == plotStyleHandle))
                return false;
            namedObjectChildHandles.insert(entry.childHandle);
            if (isPlotStyle) plotStyleHandle = entry.childHandle;
        }
        for (const auto& entry : m_pendingDwgGroupDictionaryEntries) {
            const std::string key = toUpperCase(entry.name);
            if (key.empty() || entry.childHandle == 0
                || entry.childHandle == DRW::NoHandle
                || !groupNames.insert(key).second
                || !groupHandles.insert(entry.childHandle).second)
                return false;
        }
    } catch (const std::exception&) {
        return false;
    }

    m_pendingDwgNamedObjectDictionaryNames.swap(namedObjectNames);
    m_pendingDwgNamedObjectDictionaryChildHandles.swap(
        namedObjectChildHandles);
    m_pendingDwgGroupNames.swap(groupNames);
    m_pendingDwgGroupHandles.swap(groupHandles);
    m_pendingDwgDictionaryIndexesReady = true;
    return true;
}

void dwgWriter15::invalidatePendingDwgDictionaryIndexes() noexcept {
    m_pendingDwgNamedObjectDictionaryNames.clear();
    m_pendingDwgNamedObjectDictionaryChildHandles.clear();
    m_pendingDwgGroupNames.clear();
    m_pendingDwgGroupHandles.clear();
    m_pendingDwgDictionaryIndexesReady = false;
}

bool dwgWriter15::ensurePendingViewportEntityHeaderIndex() const {
    if (m_pendingViewportEntityHeaderIndexReady)
        return true;

    std::unordered_set<std::uint32_t> handles;
    try {
        handles.reserve(m_pendingViewportEntityHeaders.size());
        for (const auto& entry : m_pendingViewportEntityHeaders)
            handles.insert(entry.first);
    } catch (const std::exception&) {
        return false;
    }

    m_pendingViewportEntityHeaderHandles.swap(handles);
    m_pendingViewportEntityHeaderIndexReady = true;
    return true;
}

void dwgWriter15::invalidatePendingViewportEntityHeaderIndex() noexcept {
    m_pendingViewportEntityHeaderHandles.clear();
    m_pendingViewportEntityHeaderIndexReady = false;
}

bool dwgWriter15::ensureEmittedDwgGroupEntryIndex() const {
    if (m_emittedDwgGroupEntryIndexReady)
        return true;

    std::unordered_map<std::uint32_t, std::size_t> indexes;
    try {
        indexes.reserve(m_emittedDwgGroupDictionaryEntries.size());
        for (std::size_t index = 0;
             index < m_emittedDwgGroupDictionaryEntries.size(); ++index) {
            if (!indexes.emplace(
                    m_emittedDwgGroupDictionaryEntries[index].childHandle,
                    index).second)
                return false;
        }
    } catch (const std::exception&) {
        return false;
    }

    m_emittedDwgGroupEntryIndexes.swap(indexes);
    m_emittedDwgGroupEntryIndexReady = true;
    return true;
}

void dwgWriter15::invalidateEmittedDwgGroupEntryIndex() noexcept {
    m_emittedDwgGroupEntryIndexes.clear();
    m_emittedDwgGroupEntryIndexReady = false;
}

bool dwgWriter15::ensureEmittedDwgGroupFrameIndex() const {
    if (m_emittedDwgGroupFrameIndexReady)
        return true;

    std::unordered_set<std::uint32_t> handles;
    try {
        handles.reserve(m_emittedDwgGroupFrames.size());
        for (const auto& entry : m_emittedDwgGroupFrames) {
            if (!handles.insert(entry.childHandle).second)
                return false;
        }
    } catch (const std::exception&) {
        return false;
    }

    m_emittedDwgGroupFrameHandles.swap(handles);
    m_emittedDwgGroupFrameIndexReady = true;
    return true;
}

void dwgWriter15::invalidateEmittedDwgGroupFrameIndex() noexcept {
    m_emittedDwgGroupFrameHandles.clear();
    m_emittedDwgGroupFrameIndexReady = false;
}

bool dwgWriter15::registerDwgNamedObjectDictionaryEntry(
    const std::string& name, std::uint32_t childHandle) {
    if (isLegacyModelChainHandle(childHandle)) {
        m_legacyModelChainConflict = true;
        return false;
    }
    std::string normalizedName;
    try {
        normalizedName = toUpperCase(name);
    } catch (const std::exception&) {
        return false;
    }
    const bool plotSettingsEntry = normalizedName == "ACAD_PLOTSETTINGS";
    const bool plotStyleEntry = isPlotStyleDictionaryKey(name);
    const bool materialEntry = normalizedName == "ACAD_MATERIAL";
    if (defaultDwgMaterialGraphActive()
        && (materialEntry || std::find(m_defaultMaterialHandles.begin(),
              m_defaultMaterialHandles.end(), childHandle) != m_defaultMaterialHandles.end())) {
        m_defaultMaterialConflict = true;
        return false;
    }

    if (m_dwgTableRecordsComplete || name.empty() || childHandle == 0
        || (m_defaultPlotStylePlaceholderHandle != 0
            && childHandle == m_defaultPlotStylePlaceholderHandle)
        || (plotStyleEntry && childHandle == DRW::NoHandle)
        || childHandle == DRW::DwgNamedObjectsDictionaryHandle
        || childHandle == DRW::DwgAcadGroupDictionaryHandle
        || normalizedName == "ACAD_GROUP"
        || (plotSettingsEntry && m_plotSettingsDictionaryHandle != 0)
        || !ensurePendingDwgDictionaryIndexes()
        || m_pendingDwgGroupHandles.find(childHandle)
            != m_pendingDwgGroupHandles.end()
        || m_pendingDwgNamedObjectDictionaryNames.find(name)
            != m_pendingDwgNamedObjectDictionaryNames.end()) {
        return false;
    }
    // Only this reserved root key is case-folded here. A plotstyle dictionary
    // must have one unambiguous NOD key and cannot alias another root child.
    // Preserve every unrelated key's existing admission behavior.
    if (plotStyleEntry
        || m_pendingDwgNamedObjectDictionaryChildHandles.count(childHandle) != 0) {
        for (const auto& entry : m_pendingDwgNamedObjectDictionaryEntries) {
            const bool existingPlotStyle = isPlotStyleDictionaryKey(entry.name);
            if ((plotStyleEntry && existingPlotStyle)
                || ((plotStyleEntry || existingPlotStyle)
                    && entry.childHandle == childHandle))
                return false;
        }
    }
    try {
        m_handles.reserve(childHandle);
        const auto nameResult = m_pendingDwgNamedObjectDictionaryNames.insert(
            name);
        if (!nameResult.second)
            return false;
        const auto handleResult =
            m_pendingDwgNamedObjectDictionaryChildHandles.insert(childHandle);
        try {
            m_pendingDwgNamedObjectDictionaryEntries.push_back(
                {name, childHandle});
            if (plotSettingsEntry)
                m_plotSettingsDictionaryHandle = childHandle;
        } catch (...) {
            m_pendingDwgNamedObjectDictionaryNames.erase(nameResult.first);
            if (handleResult.second)
                m_pendingDwgNamedObjectDictionaryChildHandles.erase(
                    handleResult.first);
            return false;
        }
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

bool dwgWriter15::stageDefaultDwgMaterialGraph() {
    static_assert(std::is_nothrow_move_assignable<HandleAllocator>::value,
                  "failed material admission must restore allocator state");
    if ((m_version != DRW::AC1021 && m_version != DRW::AC1024 && m_version != DRW::AC1027)
        || (m_version == DRW::AC1027 && m_defaultPlotStylePlaceholderHandle == 0)
        || m_dwgClassesFrozen || m_dwgClassManifestPrepared
        || m_sectionOffsets.count(recno::HEADER) || m_sectionOffsets.count(recno::CLASSES)
        || m_dwgTableRecordsComplete || defaultDwgMaterialGraphActive()
        || hasRawDwgSections() || !m_rawClassNumRemap.empty() || !m_rawObjectOverrides.empty())
        return false;
    for (const auto& definition : m_dwgClassDefinitions)
        if (definition.m_className == "AcDbMaterial"
            || definition.m_recordName == "MATERIAL") return false;
    try {
        for (const auto& entry : m_pendingDwgNamedObjectDictionaryEntries)
            if (toUpperCase(entry.name) == "ACAD_MATERIAL") return false;

        CompoundWriteCheckpoint checkpoint = checkpointPublicTransaction();
        HandleAllocator allocator = m_handles;
        const auto fail = [this, &checkpoint, &allocator] {
            m_handles = std::move(allocator);
            try { rollbackCompoundWrite(checkpoint); }
            catch (...) { m_frameWriteError = true; }
            return false;
        };
        try {
            std::array<std::uint32_t, 4> handles{};
            for (auto& handle : handles) {
                handle = m_handles.next();
                if (handle == 0 || handle == DRW::NoHandle) return fail();
            }
            if (!registerDwgNamedObjectDictionaryEntry("ACAD_MATERIAL", handles[0]))
                return fail();
            DwgClassDefinition definition;
            definition.m_classNum = nextFreeCustomClassNum();
            definition.m_proxyFlag = 0x481;
            definition.m_appName = "ObjectDBX Classes";
            definition.m_className = "AcDbMaterial";
            definition.m_recordName = "MATERIAL";
            definition.m_entityFlagRaw = 0x1f3;
            definition.m_dwgVersion = 23;
            definition.m_maintenanceVersion = 14;
            for (std::size_t i = 1; i < handles.size(); ++i)
                if (!registerTypedObjectClass(definition, handles[i])) return fail();
            m_defaultMaterialHandles = handles;
            return true;
        } catch (...) { return fail(); }
    } catch (...) { return false; }
}

bool dwgWriter15::stageDefaultDwgPlotStyleGraph() {
    static_assert(std::is_nothrow_move_assignable<HandleAllocator>::value,
                  "failed graph admission must restore allocator state");
    if (m_version < DRW::AC1015 || m_version > DRW::AC1032
        || m_dwgClassesFrozen || m_dwgClassManifestPrepared
        || m_sectionOffsets.find(recno::HEADER) != m_sectionOffsets.end()
        || m_sectionOffsets.find(recno::CLASSES) != m_sectionOffsets.end()
        || m_dwgTableRecordsComplete || m_defaultPlotStyleDictionaryHandle != 0)
        return false;
    // Unlike ordinary write rollback, failed admission must not retain these
    // private identities: no caller has received the planning callback yet.
    try {
        CompoundWriteCheckpoint checkpoint = checkpointPublicTransaction();
        HandleAllocator allocator = m_handles;
        const auto fail = [this, &checkpoint, &allocator] {
            m_handles = std::move(allocator);
            try {
                rollbackCompoundWrite(checkpoint);
            } catch (...) {
                m_frameWriteError = true;
            }
            return false;
        };
        try {
            const std::uint32_t dictionaryHandle = m_handles.next();
            const std::uint32_t placeholderHandle = m_handles.next();
            if (dictionaryHandle == 0 || dictionaryHandle == DRW::NoHandle
                || placeholderHandle == 0 || placeholderHandle == DRW::NoHandle
                || dictionaryHandle == placeholderHandle
                || !registerDwgNamedObjectDictionaryEntry("ACAD_PLOTSTYLENAME", dictionaryHandle)
                || !registerDictionaryWithDefaultObjectClass(dictionaryHandle))
                return fail();
            m_defaultPlotStyleDictionaryHandle = dictionaryHandle;
            m_defaultPlotStylePlaceholderHandle = placeholderHandle;
            return true;
        } catch (...) {
            return fail();
        }
    } catch (...) {
        // Copying the checkpoint or allocator happens before any mutation.
        return false;
    }
}

bool dwgWriter15::registerPlotSettingsObjectClass(std::uint32_t handle) {
    if (m_deferredPlotSettingsDictionary) {
        if (handle == 0)
            return dwgWriter::registerPlotSettingsObjectClass(0);
        return dwgWriter::registerPlotSettingsObjectClass(handle);
    }

    // A caller-supplied PLOTSETTINGS dictionary is already part of the
    // source graph. Keep its records and key associations on that path.
    if (m_plotSettingsDictionaryHandle != 0) {
        if (handle == 0)
            return false;
        return dwgWriter::registerPlotSettingsObjectClass(handle);
    }

    if (m_dwgTableRecordsComplete || m_dwgClassesFrozen)
        return false;

    const CompoundWriteCheckpoint checkpoint = checkpointPublicTransaction();
    // Reserve an explicit object identity before allocating the dictionary
    // handle, so the generated owner cannot collide with a selected record.
    if (handle != 0
        && !dwgWriter::registerPlotSettingsObjectClass(handle)) {
        rollbackCompoundWrite(checkpoint);
        return false;
    }
    const std::uint32_t dictionaryHandle = m_handles.next();
    if (dictionaryHandle == 0
        || !registerDwgNamedObjectDictionaryEntry(
            "ACAD_PLOTSETTINGS", dictionaryHandle)) {
        rollbackCompoundWrite(checkpoint);
        return false;
    }
    m_deferredPlotSettingsDictionary = true;
    if (handle == 0
        && !dwgWriter::registerPlotSettingsObjectClass(0)) {
        rollbackCompoundWrite(checkpoint);
        return false;
    }
    return true;
}

bool dwgWriter15::registerDwgGroupDictionaryEntry(
    const std::string& name, std::uint32_t groupHandle) {
    if (isLegacyModelChainHandle(groupHandle)) {
        m_legacyModelChainConflict = true;
        return false;
    }
    if (defaultDwgMaterialGraphActive()
        && std::find(m_defaultMaterialHandles.begin(), m_defaultMaterialHandles.end(),
                     groupHandle) != m_defaultMaterialHandles.end()) {
        m_defaultMaterialConflict = true;
        return false;
    }

    if (m_dwgTableRecordsComplete || name.empty() || groupHandle == 0
        || (m_defaultPlotStylePlaceholderHandle != 0
            && groupHandle == m_defaultPlotStylePlaceholderHandle)
        || groupHandle == DRW::NoHandle
        || groupHandle == DRW::DwgNamedObjectsDictionaryHandle
        || groupHandle == DRW::DwgAcadGroupDictionaryHandle
        || m_pendingDwgGroupDictionaryEntries.size()
            >= DRW_Dictionary::kMaxEntries
        || !ensurePendingDwgDictionaryIndexes()) {
        return false;
    }

    std::string key;
    try {
        key = toUpperCase(name);
    } catch (const std::exception&) {
        return false;
    }
    if (key.empty()
        || m_pendingDwgGroupNames.find(key) != m_pendingDwgGroupNames.end()
        || m_pendingDwgGroupHandles.find(groupHandle)
            != m_pendingDwgGroupHandles.end()
        || m_pendingDwgNamedObjectDictionaryChildHandles.find(groupHandle)
            != m_pendingDwgNamedObjectDictionaryChildHandles.end()) {
        return false;
    }

    try {
        m_handles.reserve(groupHandle);
        const auto nameResult = m_pendingDwgGroupNames.insert(key);
        if (!nameResult.second)
            return false;
        const auto handleResult =
            m_pendingDwgGroupHandles.insert(groupHandle);
        try {
            m_pendingDwgGroupDictionaryEntries.push_back({name, groupHandle});
        } catch (...) {
            m_pendingDwgGroupNames.erase(nameResult.first);
            if (handleResult.second)
                m_pendingDwgGroupHandles.erase(handleResult.first);
            return false;
        }
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

bool dwgWriter15::getEmittedDwgNamedObjectDictionaryEntries(
    std::vector<DRW::DwgNamedObjectDictionaryEntry>& entries) const {
    entries.clear();
    if (!m_dwgNamedObjectDictionaryComplete)
        return false;
    entries = m_emittedDwgNamedObjectDictionaryEntries;
    return true;
}

bool dwgWriter15::validateDwgNamedObjectDictionaryEntries() const {
    if (!m_dwgNamedObjectDictionaryComplete
        || !m_dwgGroupDictionaryComplete
        || (m_deferredPlotSettingsDictionary
            && !m_deferredPlotSettingsDictionaryEmitted)
        || m_defaultMaterialConflict
        || (defaultDwgMaterialGraphActive() && !m_defaultMaterialGraphEmitted)
        || (m_defaultPlotStyleDictionaryHandle != 0
            && (!m_defaultPlotStyleGraphEmitted
                || m_defaultPlotStylePlaceholderHandle == 0))
        || m_emittedDwgNamedObjectDictionaryEntries.empty()) {
        return false;
    }
    const auto hasSingleFrame = [this](std::uint32_t handle) {
        return hasEmittedDwgObjectHandle(handle);
    };
    if (!hasSingleFrame(DRW::DwgNamedObjectsDictionaryHandle)
        || !hasSingleFrame(DRW::DwgAcadGroupDictionaryHandle)) {
        return false;
    }
    if (m_defaultPlotStyleDictionaryHandle != 0
        && (!hasSingleFrame(m_defaultPlotStyleDictionaryHandle)
            || !hasSingleFrame(m_defaultPlotStylePlaceholderHandle)
            || std::count_if(m_emittedDwgNamedObjectDictionaryEntries.cbegin(),
                m_emittedDwgNamedObjectDictionaryEntries.cend(), [this](const auto& entry) {
                    return isPlotStyleDictionaryKey(entry.name)
                        && entry.childHandle == m_defaultPlotStyleDictionaryHandle;
                }) != 1))
        return false;

    if (defaultDwgMaterialGraphActive()) {
        for (const auto handle : m_defaultMaterialHandles)
            if (!hasSingleFrame(handle)) return false;
        if (std::count_if(m_emittedDwgNamedObjectDictionaryEntries.begin(),
              m_emittedDwgNamedObjectDictionaryEntries.end(), [this](const auto& entry) {
                  return toUpperCase(entry.name) == "ACAD_MATERIAL"
                      && entry.childHandle == m_defaultMaterialHandles[0];
              }) != 1) return false;
    }

    if (!m_canonicalLayerHandles.empty()) {
        // Other layers may otherwise collide with selected names after CAD
        // case folding or trimming, even when their byte spellings differ.
        if ((m_haveLayer0 && !canonicalLayerNameValid(m_layer0.name))
            || std::any_of(m_pendingLayers.begin(), m_pendingLayers.end(),
                [this](const auto& layer) { return !canonicalLayerNameValid(layer.second.name); }))
            return false;
        if ((m_version != DRW::AC1021 && m_version != DRW::AC1024
             && m_version != DRW::AC1027)
            || m_canonicalLayerHandles.size() > dwgSafety::MaxOwnedObjectCount
            || !m_canonicalPlotDictionarySeen || !m_canonicalPlaceholderSeen
            || m_canonicalPlotTarget == 0u || m_canonicalPlotDictionary == 0u
            || m_canonicalPlaceholderOwner != m_canonicalPlotDictionary
            || !hasSingleFrame(m_canonicalPlotTarget)
            || !hasSingleFrame(m_canonicalPlotDictionary)
            || std::count_if(m_emittedDwgNamedObjectDictionaryEntries.cbegin(),
                m_emittedDwgNamedObjectDictionaryEntries.cend(), [this](const auto& entry) {
                    return isPlotStyleDictionaryKey(entry.name)
                        && entry.childHandle == m_canonicalPlotDictionary;
                }) != 1) return false;
        for (const auto layer : m_canonicalLayerHandles)
            if (m_emittedDwgTableRecordHandles.count(layer) != 1u) return false;
    }

    std::set<std::string> names;
    for (const auto& entry : m_emittedDwgNamedObjectDictionaryEntries) {
        if (entry.name.empty() || entry.childHandle == 0
            || !names.insert(entry.name).second
            || !hasSingleFrame(entry.childHandle)) {
            return false;
        }
    }
    const auto group = m_emittedDwgNamedObjectDictionaryEntries.cbegin();
    if (group->name != "ACAD_GROUP"
        || group->childHandle != DRW::DwgAcadGroupDictionaryHandle
        || m_emittedDwgGroupDictionaryEntries
            != m_pendingDwgGroupDictionaryEntries
        || m_emittedDwgGroupFrames != m_pendingDwgGroupDictionaryEntries) {
        return false;
    }

    if (m_deferredPlotSettingsDictionary) {
        const auto plotSettingsEntry = std::find_if(
            m_emittedDwgNamedObjectDictionaryEntries.cbegin(),
            m_emittedDwgNamedObjectDictionaryEntries.cend(),
            [this](const DRW::DwgNamedObjectDictionaryEntry& entry) {
                return entry.name == "ACAD_PLOTSETTINGS"
                    && entry.childHandle
                        == m_plotSettingsDictionaryHandle;
            });
        if (m_plotSettingsDictionaryHandle == 0
            || plotSettingsEntry
                == m_emittedDwgNamedObjectDictionaryEntries.cend()
            || m_deferredPlotSettingsEntries.empty())
            return false;
    }

    std::set<std::string> groupNames;
    std::set<std::uint32_t> groupHandles;
    for (const auto& entry : m_pendingDwgGroupDictionaryEntries) {
        if (entry.name.empty() || entry.childHandle == 0
            || entry.childHandle == DRW::NoHandle
            || !groupNames.insert(toUpperCase(entry.name)).second
            || !groupHandles.insert(entry.childHandle).second
            || !hasSingleFrame(entry.childHandle)) {
            return false;
        }
    }
    return true;
}

bool dwgWriter15::getEmittedDwgBlockWriteResult(
    DRW::DwgBlockWriteResult& result) const {
    result = {};
    if (!m_dwgBlockStructureComplete)
        return false;
    result.blockControlHandle = DRW::DwgBlockControlHandle;
    result.blockEntityHandles.assign(m_emittedDwgBlockEntityHandles.cbegin(),
                                     m_emittedDwgBlockEntityHandles.cend());
    result.blockRecords = m_emittedDwgBlockRecords;
    return true;
}

// Shared body of the per-type table-record emitters below.  Each of them
// differs only in the DRW type it copies and the DWG object-type code it
// stamps into the preamble, so the sequence - copy, prepare common object
// state, prepare table-entry EED, open the object, write the standard
// preamble, encode the payload, finish the record - lives here once.
template <typename T>
bool dwgWriter15::emitTableRecord(std::uint32_t handle, const T& source,
                                  std::uint16_t objectType) {
    T object = source;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs))
        return false;
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(body, m_version, objectType, handle,
                            m_objectStrings, m_objectHandles, sb, hb,
                            object.reactorCount(), object.extensionDictionaryFlag(),
                            object.hasDataStorageBinaryData(), &object.extData,
                            &appIdRefs, &layerRefs, fileCodePageId()))
        return false;
    if (!object.encodeDwg(m_version, &body, sb, hb))
        return false;

    return finishTableRecord(handle);
}

bool dwgWriter15::emitLtypeRecord(std::uint32_t handle, const DRW_LType& lt) {
    const bool syntheticLegacy = m_version == DRW::AC1015
        && ((handle == reservedHandle::LTYPE_BYBLOCK && !m_haveLtypeByBlock)
            || (handle == reservedHandle::LTYPE_BYLAYER && !m_haveLtypeByLayer)
            || (handle == reservedHandle::LTYPE_CONTINUOUS && !m_haveLtypeContinuous));
    if (!syntheticLegacy)
        return emitTableRecord(handle, lt, DRW::DwgLTypeObjectType);
    DRW_LType object = lt;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs))
        return false;
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb = nullptr, *hb = nullptr;
    if (!emitRecordPreamble(body, m_version, DRW::DwgLTypeObjectType, handle,
                            m_objectStrings, m_objectHandles, sb, hb,
                            object.reactorCount(), object.extensionDictionaryFlag(),
                            object.hasDataStorageBinaryData(), &object.extData,
                            &appIdRefs, &layerRefs, fileCodePageId())
        || !object.encodeDwgForWriter(m_version, &body, sb, hb, true))
        return false;
    return finishTableRecord(handle);
}

bool dwgWriter15::emitLayerRecord(std::uint32_t handle, const DRW_Layer& lay) {
    // Resolve linetype name to a handle before encoding.
    DRW_Layer layerCopy = lay;
    std::string ltUpper = toUpperCase(lay.lineType);
    auto ltIt = m_writingCtx.ltypeMap.find(ltUpper);
    if (ltIt != m_writingCtx.ltypeMap.end()) {
        layerCopy.lTypeH = makeHardPointer(ltIt->second);
    } else {
        layerCopy.lTypeH = makeHardPointer(reservedHandle::LTYPE_CONTINUOUS);
    }
    if (!prepareDwgCommonObjectState(layerCopy, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(layerCopy, appIdRefs, layerRefs))
        return false;

    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(body, m_version, DRW::DwgLayerObjectType, handle,
                            m_objectStrings, m_objectHandles, sb, hb,
                            layerCopy.reactorCount(),
                            layerCopy.extensionDictionaryFlag(),
                            layerCopy.hasDataStorageBinaryData(),
                            &layerCopy.extData, &appIdRefs, &layerRefs,
                            fileCodePageId()))
        return false;
    const bool typedLegacyNulls = (handle == DRW::DwgLayer0Handle && !m_haveLayer0
        && (m_version == DRW::AC1015 || (m_version == DRW::AC1027
            && defaultDwgMaterialGraphActive())))
        || (m_version == DRW::AC1015 && m_defaultPlotStyleDictionaryHandle != 0u
            && layerCopy.xDictHandle == 0u && layerCopy.xrefBlockHandle.ref == 0u);
    if (!layerCopy.encodeDwgForWriter(m_version, &body, sb, hb, typedLegacyNulls))
        return false;

    if (!finishTableRecord(handle)) return false;
    if (std::binary_search(m_canonicalLayerHandles.begin(),
                           m_canonicalLayerHandles.end(), handle)) {
        const std::array<std::pair<std::uint8_t, std::uint64_t>, 5> expected{{
            {DRW::DwgSoftPointer, DRW::DwgLayerControlHandle},
            {DRW::DwgHardPointer, 0u}, {DRW::DwgHardPointer, m_canonicalPlotTarget},
            {DRW::DwgHardPointer, m_defaultMaterialHandles[3]},
            {DRW::DwgHardPointer, layerCopy.lTypeH.ref}}};
        if (m_version == DRW::AC1027) {
            const std::array<std::pair<std::uint8_t, std::uint64_t>, 6> expectedR2013{{
                expected[0], expected[1], expected[2], expected[3], expected[4],
                {DRW::DwgHardPointer, 0u}}};
            if (!canonicalHandleTailMatches(m_lastDwgObjectFrame, m_version, handle,
                                            DRW::DwgLayerObjectType, expectedR2013))
                return false;
        } else if (!canonicalHandleTailMatches(m_lastDwgObjectFrame, m_version, handle,
                                               DRW::DwgLayerObjectType, expected)) {
            return false;
        }
    }
    return true;
}

bool dwgWriter15::emitStyleRecord(std::uint32_t handle, const DRW_Textstyle& ts) {
    const bool syntheticLegacy = m_version == DRW::AC1015
        && handle == DRW::DwgStandardTextStyleHandle && !m_haveStandardStyle;
    if (!syntheticLegacy)
        return emitTableRecord(handle, ts, DRW::DwgStyleObjectType);
    DRW_Textstyle object = ts;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs))
        return false;
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb = nullptr, *hb = nullptr;
    if (!emitRecordPreamble(body, m_version, DRW::DwgStyleObjectType, handle,
                            m_objectStrings, m_objectHandles, sb, hb,
                            object.reactorCount(), object.extensionDictionaryFlag(),
                            object.hasDataStorageBinaryData(), &object.extData,
                            &appIdRefs, &layerRefs, fileCodePageId())
        || !object.encodeDwgForWriter(m_version, &body, sb, hb, true))
        return false;
    return finishTableRecord(handle);
}

bool dwgWriter15::emitUcsRecord(std::uint32_t handle, const DRW_UCS& ucs) {
    return emitTableRecord(handle, ucs, DRW::DwgUcsObjectType);
}

bool dwgWriter15::emitViewRecord(std::uint32_t handle, const DRW_View& view) {
    return emitTableRecord(handle, view, DRW::DwgViewObjectType);
}

bool dwgWriter15::emitVportRecord(std::uint32_t handle, const DRW_Vport& vp) {
    const bool synthetic = handle == DRW::DwgActiveVportHandle
        && !m_haveActiveVport && m_version >= DRW::AC1015;
    if (!synthetic)
        return emitTableRecord(handle, vp, DRW::DwgVPortObjectType);
    DRW_Vport object = vp;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs))
        return false;
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb = nullptr, *hb = nullptr;
    if (!emitRecordPreamble(body, m_version, DRW::DwgVPortObjectType, handle,
                            m_objectStrings, m_objectHandles, sb, hb,
                            object.reactorCount(), object.extensionDictionaryFlag(),
                            object.hasDataStorageBinaryData(), &object.extData,
                            &appIdRefs, &layerRefs, fileCodePageId())
        || !object.encodeDwgForWriter(m_version, &body, sb, hb, true))
        return false;
    return finishTableRecord(handle);
}

bool dwgWriter15::emitViewportEntityHeaderRecord(
    std::uint32_t handle, const DRW_ViewportEntityHeader& source) {
    // No typed DataStorage section is emitted for this table-side phase.
    // Do not serialize a presence bit that cannot be accounted for by the
    // OBJECTS replay ledger.
    if (m_version > DRW::AC1024 && source.hasDataStorageBinaryData())
        return false;
    DRW_ViewportEntityHeader header = source;
    const std::uint8_t effectiveXDictFlag = m_version > DRW::AC1015
        ? header.extensionDictionaryFlag() : 0;
    header.setDwgCommonObjectState(
        static_cast<std::int32_t>(header.reactorHandles.size()),
        effectiveXDictFlag, header.hasDataStorageBinaryData());
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(header, appIdRefs, layerRefs))
        return false;

    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version, DRW_ViewportEntityHeader::kDwgType, handle,
            m_objectStrings, m_objectHandles, sb, hb,
            static_cast<std::int32_t>(header.reactorHandles.size()),
            effectiveXDictFlag, header.hasDataStorageBinaryData(),
            &header.extData, &appIdRefs, &layerRefs, fileCodePageId()))
        return false;
    if (!header.encodeDwg(m_version, &body, sb, hb))
        return false;
    return finishTableRecord(handle);
}

bool dwgWriter15::emitAppIdRecord(std::uint32_t handle, const DRW_AppId& ai) {
    if (handle != DRW::DwgAcadAppIdHandle || m_version < DRW::AC1015)
        return emitTableRecord(handle, ai, DRW::DwgAppIdObjectType);
    DRW_AppId object = ai;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs))
        return false;
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb = nullptr, *hb = nullptr;
    if (!emitRecordPreamble(body, m_version, DRW::DwgAppIdObjectType, handle,
                            m_objectStrings, m_objectHandles, sb, hb,
                            object.reactorCount(), object.extensionDictionaryFlag(),
                            object.hasDataStorageBinaryData(), &object.extData,
                            &appIdRefs, &layerRefs, fileCodePageId())
        || !object.encodeDwgForWriter(m_version, &body, sb, hb, true))
        return false;
    return finishTableRecord(handle);
}

bool dwgWriter15::emitDimstyleRecord(std::uint32_t handle, const DRW_Dimstyle& ds) {
    // DIMSTYLE's external-reference block handle is modeled but not yet
    // resolved into the writer's target block-record graph. Do not publish a
    // stale source handle into a newly assembled DWG.
    if (ds.xrefBlockHandle.ref != 0u
        && ds.xrefBlockHandle.ref != DRW::NoHandle)
        return false;
    const bool synthetic = handle == DRW::DwgStandardDimstyleHandle
        && !m_haveStandardDimstyle && m_version >= DRW::AC1015;
    if (!synthetic)
        return emitTableRecord(handle, ds, DRW::DwgDimStyleObjectType);
    DRW_Dimstyle object = ds;
    if (m_version == DRW::AC1015)
        object.dimtxstyH.ref = DRW::DwgStandardTextStyleHandle;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs))
        return false;
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb = nullptr, *hb = nullptr;
    if (!emitRecordPreamble(body, m_version, DRW::DwgDimStyleObjectType, handle,
                            m_objectStrings, m_objectHandles, sb, hb,
                            object.reactorCount(), object.extensionDictionaryFlag(),
                            object.hasDataStorageBinaryData(), &object.extData,
                            &appIdRefs, &layerRefs, fileCodePageId())
        || !object.encodeDwgForWriter(m_version, &body, sb, hb, true))
        return false;
    return finishTableRecord(handle);
}

bool dwgWriter15::emitAcDbPlaceholderObject(
    std::uint32_t handle, const DRW_AcDbPlaceholder& placeholder,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version, dwgObjType::ACDBPLACEHOLDER, handle,
            m_objectStrings, m_objectHandles,
            sb, hb, placeholder.reactorCount(),
            placeholder.extensionDictionaryFlag(),
            placeholder.hasDataStorageBinaryData(), &placeholder.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    DRW_UNUSED(sb);
    const bool syntheticLegacy = m_version == DRW::AC1015
        && m_writingDefaultPlotStyleGraph
        && handle == m_defaultPlotStylePlaceholderHandle;
    if (!placeholder.encodeDwgForWriter(m_version, &body, hb, syntheticLegacy))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::emitVbaProjectObject(
    std::uint32_t handle, const DRW_VbaProject& project,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb;
    dwgBufferW *hb;
    if (!emitRecordPreamble(
            body, m_version, DRW_VbaProject::kDwgType, handle,
            m_objectStrings, m_objectHandles, sb, hb, project.reactorCount(),
            project.extensionDictionaryFlag(),
            project.hasDataStorageBinaryData(), &project.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!project.encodeDwg(m_version, &body, sb, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeAcDbPlaceholder(
    const DRW_AcDbPlaceholder& placeholder) {
    // ACDBPLACEHOLDER (ODA fixed type 80) is universally available since
    // R2000.  Encoder is version-clean — only the standard string/handle
    // split-buffer routing on `version > AC1018` (no AC1018+-only fields).
    // PR 13d broadened the writer gate from AC1021+ to AC1015+ in step
    // with the filter-side `canWriteFixedTypeObjects` dispatch.
    if (m_version < DRW::AC1015
        || placeholder.reactorHandles.size() > dwgSafety::MaxReactorCount
        || placeholder.xDictFlag > 1)
        return false;

    DRW_AcDbPlaceholder object = placeholder;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    const std::uint8_t effectiveXDictFlag = m_version > DRW::AC1015
        ? object.extensionDictionaryFlag() : 0;
    object.setDwgCommonObjectState(
        static_cast<std::int32_t>(object.reactorHandles.size()),
        effectiveXDictFlag, object.hasDataStorageBinaryData());
    if (!emitAcDbPlaceholderObject(object.handle, object, appIdRefs,
                                   layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (m_canonicalPlotTarget != 0u && object.handle == m_canonicalPlotTarget) {
        const bool reactorProfile = m_version == DRW::AC1021
            || (m_version == DRW::AC1024 && m_writingDefaultPlotStyleGraph);
        const std::array<std::pair<std::uint8_t, std::uint64_t>, 1> expected{{
            {DRW::DwgSoftPointer, object.parentHandle}}};
        const std::array<std::pair<std::uint8_t, std::uint64_t>, 2> expectedR2007{{
            expected[0], {DRW::DwgSoftPointer, object.parentHandle}}};
        const bool rolesMatch = reactorProfile
            ? canonicalHandleTailMatches(m_lastDwgObjectFrame, m_version, object.handle,
                                         dwgObjType::ACDBPLACEHOLDER, expectedR2007)
            : canonicalHandleTailMatches(m_lastDwgObjectFrame, m_version, object.handle,
                                         dwgObjType::ACDBPLACEHOLDER, expected);
        const bool reactorsMatch = reactorProfile
            ? placeholder.reactorCount() == 1 && object.reactorCount() == 1
                && object.reactorHandles.size() == 1u
                && object.reactorHandles.front() == object.parentHandle
            : placeholder.reactorCount() == 0 && object.reactorCount() == 0;
        if (m_canonicalPlaceholderSeen || !reactorsMatch
            || object.extensionDictionaryFlag() != 1u || object.xDictHandle != 0u
            || object.hasDataStorageBinaryData() || !object.appData.empty()
            || !object.extData.empty() || !object.pendingAppIdResolutions.empty()
            || !object.pendingLayerRefResolutions.empty() || !rolesMatch)
            return false;
        m_canonicalPlaceholderOwner = object.parentHandle;
        m_canonicalPlaceholderSeen = true;
    }
    return true;
}

bool dwgWriter15::writeVbaProject(const DRW_VbaProject& project) {
    if (m_version < DRW::AC1015
        || project.m_data.size() > dwgSafety::MaxBufferSize
        || project.m_dataSize != project.m_data.size()
        || project.extensionDictionaryFlag() > 1
        || project.reactorHandles.size() > dwgSafety::MaxReactorCount
        || project.hasDataStorageBinaryData()) {
        return false;
    }

    DRW_VbaProject object = project;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!emitVbaProjectObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::writeDbColor(const DRW_DbColor& color) {
    if (m_version < DRW::AC1018
        || color.reactorHandles.size() > dwgSafety::MaxReactorCount
        || color.extensionDictionaryFlag() > 1)
        return false;

    DRW_DbColor object = color;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb, *hb;
    const std::uint16_t classNum = typedDwgClassNum(
        "AcDbColor", "DBCOLOR", DRW_DbColor::kDwgClassNum);
    if (!emitRecordPreamble(
            body, m_version, classNum, object.handle, m_objectStrings,
            m_objectHandles, sb, hb, object.reactorCount(),
            object.extensionDictionaryFlag(), object.hasDataStorageBinaryData(),
            &object.extData, &appIdRefs, &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeDimensionAssociation(
    const DRW_DimensionAssociation& association) {
    if (m_version < DRW::AC1021
        || association.reactorHandles.size() > dwgSafety::MaxReactorCount
        || association.extensionDictionaryFlag() > 1)
        return false;

    DRW_DimensionAssociation object = association;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerDimensionAssociationObjectClass(object.handle))
        return false;

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbDimAssoc", "DIMASSOC",
                             DRW_DimensionAssociation::kDwgClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeEvaluationGraph(const DRW_EvaluationGraph& graph) {
    if (m_version < DRW::AC1021
        || graph.reactorHandles.size() > dwgSafety::MaxReactorCount
        || graph.extensionDictionaryFlag() > 1)
        return false;

    DRW_EvaluationGraph object = graph;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerEvaluationGraphObjectClass(object.handle))
        return false;

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbEvalGraph", "ACAD_EVALUATION_GRAPH",
                             DRW_EvaluationGraph::kDwgClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::emitBlockRepresentationDataObject(
    std::uint32_t handle, const DRW_BlockRepresentationData& data,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version, dwgObjType::BLOCKREPRESENTATION, handle,
            m_objectStrings, m_objectHandles, sb, hb, data.reactorCount(),
            data.extensionDictionaryFlag(), data.hasDataStorageBinaryData(),
            &data.extData, &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!data.encodeDwg(m_version, &body, sb, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeBlockRepresentationData(
    const DRW_BlockRepresentationData& data) {
    if (m_version < DRW::AC1015
        || data.reactorHandles.size() > dwgSafety::MaxReactorCount
        || data.extensionDictionaryFlag() > 1
        || data.hasDataStorageBinaryData())
        return false;

    DRW_BlockRepresentationData object = data;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);

    if (!emitBlockRepresentationDataObject(object.handle, object, appIdRefs,
                                           layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::emitSunObject(
    std::uint32_t handle, const DRW_Sun& sun,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbSun", "SUN", DRW_Sun::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            sun.reactorCount(), sun.extensionDictionaryFlag(),
            sun.hasDataStorageBinaryData(), &sun.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!sun.encodeDwg(m_version, &body, hb, sb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeSun(const DRW_Sun& sun) {
    if (m_version < DRW::AC1021
        || sun.reactorHandles.size() > dwgSafety::MaxReactorCount
        || sun.xDictFlag > 1 || sun.hasDataStorageBinaryData())
        return false;

    DRW_Sun object = sun;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerSunObjectClass(object.handle))
        return false;

    if (!emitSunObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::emitTvDevicePropertiesObject(
    std::uint32_t handle, const DRW_TvDeviceProperties& properties,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbTvDeviceProperties", "TVDEVICEPROPERTIES",
                             DRW_TvDeviceProperties::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            properties.reactorCount(), properties.extensionDictionaryFlag(),
            properties.hasDataStorageBinaryData(), &properties.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!properties.encodeDwg(m_version, &body, sb, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeTvDeviceProperties(
    const DRW_TvDeviceProperties& properties) {
    if (m_version < DRW::AC1015
        || properties.reactorHandles.size() > dwgSafety::MaxReactorCount
        || properties.xDictFlag > 1)
        return false;

    DRW_TvDeviceProperties object = properties;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerTvDevicePropertiesObjectClass(object.handle))
        return false;

    if (!emitTvDevicePropertiesObject(object.handle, object, appIdRefs,
                                      layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::writeVxControl(const DRW_VxControl& control) {
    if (m_version < DRW::AC1015
        || control.recordHandles.size() > dwgSafety::MaxReactorCount
        || control.reactorHandles.size() > dwgSafety::MaxReactorCount
        || (control.hasDwgRawData()
            && control.dwgRawDataVersion() != DRW::UNKNOWNV
            && control.dwgRawDataVersion() != m_version))
        return false;

    DRW_VxControl object = control;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerVxControlObjectClass(object.handle))
        return false;

    const std::uint8_t effectiveXDictFlag = m_version > DRW::AC1015
        ? object.extensionDictionaryFlag() : 0;
    object.setDwgCommonObjectState(
        static_cast<std::int32_t>(object.reactorHandles.size()),
        effectiveXDictFlag, object.hasDataStorageBinaryData());

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbVxControl", "VXCONTROL",
                             DRW_VxControl::kDwgClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            static_cast<std::int32_t>(object.reactorHandles.size()),
            effectiveXDictFlag, object.hasDataStorageBinaryData(),
            &object.extData, &appIdRefs, &layerRefs, fileCodePageId())) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (!object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    if (objectWriteFailed()) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::writeVxTableRecord(const DRW_VxTableRecord& record) {
    if (m_version < DRW::AC1015
        || record.reactorHandles.size() > dwgSafety::MaxReactorCount
        || (record.hasDwgRawData()
            && record.dwgRawDataVersion() != DRW::UNKNOWNV
            && record.dwgRawDataVersion() != m_version))
        return false;

    DRW_VxTableRecord object = record;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerVxTableRecordObjectClass(object.handle))
        return false;

    const std::uint8_t effectiveXDictFlag = m_version > DRW::AC1015
        ? object.extensionDictionaryFlag() : 0;
    object.setDwgCommonObjectState(
        static_cast<std::int32_t>(object.reactorHandles.size()),
        effectiveXDictFlag, object.hasDataStorageBinaryData());

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbVxTableRecord", "VXTABLERECORD",
                             DRW_VxTableRecord::kDwgClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            static_cast<std::int32_t>(object.reactorHandles.size()),
            effectiveXDictFlag, object.hasDataStorageBinaryData(),
            &object.extData, &appIdRefs, &layerRefs, fileCodePageId())) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (!object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    if (objectWriteFailed()) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::emitMLeaderStyleObject(std::uint32_t handle,
                                         const DRW_MLeaderStyle& style,
                                         const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                                         const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbMLeaderStyle", "MLEADERSTYLE",
                             DRW_MLeaderStyle::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            style.reactorCount(), style.extensionDictionaryFlag(),
            style.hasDataStorageBinaryData(), &style.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!style.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

// Shared body of the typed OBJECT writers below.  Each differs only in the DRW
// payload type, the minimum file version it supports and which register/emit
// pair it drives; the sequence - version gate, copy, prepare common object
// state, prepare table-entry EED (recording an EED failure so a caller can
// tell it apart from a plain refusal), mint or reserve the handle, register
// the class, emit the object - is identical.
//
// Writers whose gate inspects more than the version, or that keep extra state,
// keep their own bodies.
template <typename T>
bool dwgWriter15::writeTypedObject(
    const T& source, DRW::Version minVersion,
    bool (dwgWriter::*registerClass)(std::uint32_t),
    bool (dwgWriter15::*emitObject)(
        std::uint32_t, const T&,
        const std::vector<DRW_Entity::PendingHandleRef>&,
        const std::vector<DRW_Entity::PendingHandleRef>&)) {
    if (m_version < minVersion)
        return false;

    T object = source;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!(this->*registerClass)(object.handle))
        return false;

    if (!(this->*emitObject)(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::writeMLeaderStyle(const DRW_MLeaderStyle& style) {
    return writeTypedObject(style, DRW::AC1015,
                            &dwgWriter::registerMLeaderStyleObjectClass,
                            &dwgWriter15::emitMLeaderStyleObject);
}

bool dwgWriter15::emitTableStyleObject(std::uint32_t handle,
                                       const DRW_TableStyle& style,
                                       const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                                       const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbTableStyle", "TABLESTYLE",
                             DRW_TableStyle::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            style.reactorCount(), style.extensionDictionaryFlag(),
            style.hasDataStorageBinaryData(), &style.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!style.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeTableStyle(const DRW_TableStyle& style) {
    if (m_version < DRW::AC1015
        || (m_version > DRW::AC1021
            && m_version != DRW::AC1024 && m_version != DRW::AC1027
            && m_version != DRW::AC1032))
        return false;

    DRW_TableStyle object = style;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerTableStyleObjectClass(object.handle))
        return false;

    EmittedDwgTableStyle emittedStyle;
    try {
        emittedStyle.handle = object.handle;
        emittedStyle.parentHandle = object.parentHandle;
        emittedStyle.name = object.m_name;
        m_emittedDwgTableStyles.reserve(
            m_emittedDwgTableStyles.size() + 1u);
    } catch (...) {
        return false;
    }

    if (!emitTableStyleObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    m_emittedDwgTableStyles.push_back(std::move(emittedStyle));
    return true;
}

bool dwgWriter15::emitCellStyleMapObject(
    std::uint32_t handle, const DRW_CellStyleMap& styleMap,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbCellStyleMap", "CELLSTYLEMAP", 0),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            styleMap.reactorCount(), styleMap.extensionDictionaryFlag(),
            styleMap.hasDataStorageBinaryData(), &styleMap.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!styleMap.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeCellStyleMap(
    const DRW_CellStyleMap& styleMap) {
    // ODA v5.4.1 defines this object starting in R2007. Later versions stay
    // gated until their class/graph compatibility is independently traced.
    if (m_version != DRW::AC1021)
        return false;
    return writeTypedObject(
        styleMap, DRW::AC1021,
        &dwgWriter::registerCellStyleMapObjectClass,
        &dwgWriter15::emitCellStyleMapObject);
}

bool dwgWriter15::emitTableContentObject(
    std::uint32_t handle, const DRW_TableContentObject& content,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbTableContent", "TABLECONTENT", 0),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            content.reactorCount(), content.extensionDictionaryFlag(),
            content.hasDataStorageBinaryData(), &content.extData,
            &appIdRefs, &layerRefs, fileCodePageId()))
        return false;
    if (!content.encodeDwg(m_version, &body, sb, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeTableContent(const DRW_TableContentObject& content) {
    // ODA §20.4.97: this standalone frame belongs to R21. R24+ puts the
    // content inside TABLE and needs the enclosing entity/graph writer.
    if (m_version != DRW::AC1021)
        return false;
    return writeTypedObject(
        content, DRW::AC1021,
        &dwgWriter::registerTableContentObjectClass,
        &dwgWriter15::emitTableContentObject);
}

bool dwgWriter15::emitMaterialObject(
    std::uint32_t handle, const DRW_Material& material,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbMaterial", "MATERIAL",
                             DRW_Material::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            material.reactorCount(), material.extensionDictionaryFlag(),
            material.hasDataStorageBinaryData(), &material.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!material.encodeDwg(m_version, &body, sb, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeMaterial(const DRW_Material& material) {
    if (defaultDwgMaterialGraphActive()) { m_defaultMaterialConflict = true; return false; }
    DRW_Material object = material;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerMaterialObjectClass(object.handle))
        return false;

    if (!emitMaterialObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::emitLightListObject(
    std::uint32_t handle, const DRW_LightList& lightList,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbLightList", "LIGHTLIST", 0), handle,
            m_objectStrings, m_objectHandles, sb, hb,
            lightList.reactorCount(), lightList.extensionDictionaryFlag(),
            lightList.hasDataStorageBinaryData(), &lightList.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!lightList.encodeDwg(m_version, &body, sb, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeLightList(const DRW_LightList& lightList) {
    if (m_version < DRW::AC1015
        || lightList.m_lightCount > DRW_LightList::kMaxLightCount
        || lightList.m_lights.size()
            != static_cast<std::size_t>(lightList.m_lightCount)
        || lightList.m_classVersion > static_cast<std::uint32_t>(
            std::numeric_limits<std::int32_t>::max())) {
        return false;
    }

    DRW_LightList object = lightList;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerLightListObjectClass(object.handle))
        return false;

    if (!emitLightListObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

// DICTIONARY (ODA fixed type 42) — no class registration required.  Uses the
// standard preamble + DRW_Dictionary::encodeDwg sandwich, mirroring the
// SUN / PLACEHOLDER native-writer pattern.  Per-entry string handles live
// in sb so the AC1018+ split-buffer convention applies.
bool dwgWriter15::emitDictionaryObject(
    std::uint32_t handle, const DRW_Dictionary& dictionary,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs,
    bool typedLegacyNulls) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version, dwgObjType::DICTIONARY, handle,
            m_objectStrings, m_objectHandles,
            sb, hb, dictionary.reactorCount(),
            dictionary.extensionDictionaryFlag(),
            dictionary.hasDataStorageBinaryData(), &dictionary.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!dictionary.encodeDwgImpl(m_version, &body, sb, hb, false,
                                  typedLegacyNulls))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::conflictsDefaultDwgPlotStyleDictionary(
    const DRW_Dictionary& dictionary, std::uint32_t defaultEntry,
    bool withDefault) const noexcept {
    if (m_defaultPlotStyleDictionaryHandle == 0)
        return false;
    if (withDefault && m_writingDefaultPlotStyleGraph
        && dictionary.handle == m_defaultPlotStyleDictionaryHandle)
        return false;
    if (dictionary.handle == m_defaultPlotStyleDictionaryHandle
        || dictionary.handle == m_defaultPlotStylePlaceholderHandle
        || defaultEntry == m_defaultPlotStyleDictionaryHandle
        || defaultEntry == m_defaultPlotStylePlaceholderHandle)
        return true;
    for (const auto& entry : dictionary.m_entries)
        if (entry.m_handle == m_defaultPlotStyleDictionaryHandle
            || entry.m_handle == m_defaultPlotStylePlaceholderHandle)
            return true;
    return false;
}

bool dwgWriter15::conflictsDefaultDwgMaterialDictionary(
    const DRW_Dictionary& dictionary, std::uint32_t defaultEntry) const noexcept {
    if (!defaultDwgMaterialGraphActive()) return false;
    if (m_writingDefaultMaterialGraph && dictionary.handle == m_defaultMaterialHandles[0])
        return false;
    const auto protectedHandle = [this](std::uint32_t h) {
        return h != 0 && std::find(m_defaultMaterialHandles.begin(),
            m_defaultMaterialHandles.end(), h) != m_defaultMaterialHandles.end();
    };
    if (protectedHandle(dictionary.handle) || protectedHandle(dictionary.parentHandle)
        || protectedHandle(defaultEntry)) return true;
    for (const auto& entry : dictionary.m_entries)
        if (protectedHandle(entry.m_handle)) return true;
    return false;
}

bool dwgWriter15::conflictsLegacyModelChainDictionary(
    const DRW_Dictionary& dictionary) const noexcept {
    if (!m_legacyModelChainActive) return false;
    if (isLegacyModelChainHandle(dictionary.handle)
        || isLegacyModelChainHandle(static_cast<std::uint32_t>(dictionary.parentHandle)))
        return true;
    for (const auto& entry : dictionary.m_entries)
        if (isLegacyModelChainHandle(entry.m_handle)) return true;
    return false;
}

bool dwgWriter15::writeDictionary(const DRW_Dictionary& dictionary) {
    if (conflictsLegacyModelChainDictionary(dictionary)) { m_legacyModelChainConflict = true; return false; }
    if (conflictsDefaultDwgMaterialDictionary(dictionary)) { m_defaultMaterialConflict = true; return false; }
    if (conflictsDefaultDwgPlotStyleDictionary(dictionary))
        return false;
    if (dictionary.handle == DRW::DwgNamedObjectsDictionaryHandle
        || dictionary.handle == DRW::DwgAcadGroupDictionaryHandle) {
        return false;
    }
    DRW_Dictionary object = dictionary;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }

    const auto layoutDictionaryRegistration = std::find_if(
        m_pendingDwgNamedObjectDictionaryEntries.cbegin(),
        m_pendingDwgNamedObjectDictionaryEntries.cend(),
        [](const DRW::DwgNamedObjectDictionaryEntry& entry) {
            return entry.name == "ACAD_LAYOUT";
        });
    const bool capturesLayoutDictionary =
        layoutDictionaryRegistration
            != m_pendingDwgNamedObjectDictionaryEntries.cend()
        && layoutDictionaryRegistration->childHandle == object.handle;
    const bool capturesRegisteredDictionary = std::any_of(
        m_pendingDwgNamedObjectDictionaryEntries.cbegin(),
        m_pendingDwgNamedObjectDictionaryEntries.cend(),
        [&object](const DRW::DwgNamedObjectDictionaryEntry& entry) {
            return entry.childHandle == object.handle;
        });
    const bool capturesEmptyExtension = m_version == DRW::AC1018
        && typeid(dictionary) == typeid(DRW_Dictionary)
        && !capturesRegisteredDictionary
        && object.handle != 0u && object.handle != DRW::NoHandle
        && object.parentHandle != 0u && object.parentHandle != DRW::NoHandle
        && object.handle != object.parentHandle && object.m_entries.empty()
        && object.hasCompleteDwgEntries() && object.name.empty()
        && object.flags == 0 && object.extData.empty() && object.appData.empty()
        && object.reactorHandles.empty() && object.reactorCount() == 0
        && object.xDictHandle == 0u && !object.hasDataStorageBinaryData()
        && !object.hasDwgCommonLinkTail()
        && object.pendingAppIdResolutions.empty()
        && object.pendingLayerRefResolutions.empty();
    if (capturesEmptyExtension) {
        try {
            m_emittedEmptyExtensionDictionaries.reserve(
                m_emittedEmptyExtensionDictionaries.size() + 1u);
        } catch (...) {
            return false;
        }
    }
    EmittedDwgDictionary emittedDwgDictionary;
    EmittedLayoutDictionary emittedLayoutDictionary;
    if (capturesRegisteredDictionary) {
        try {
            emittedDwgDictionary.handle = object.handle;
            emittedDwgDictionary.parentHandle = object.parentHandle;
            emittedDwgDictionary.cloning = object.cloning;
            emittedDwgDictionary.hardOwner = object.hardOwner;
            emittedDwgDictionary.entries.reserve(object.m_entries.size());
            for (const DRW_Dictionary::Entry& entry : object.m_entries)
                emittedDwgDictionary.entries.push_back(
                    {entry.m_name, entry.m_handle});
            m_emittedDwgDictionaries.reserve(
                m_emittedDwgDictionaries.size() + 1u);
        } catch (...) {
            return false;
        }
    }
    if (capturesLayoutDictionary) {
        if (!m_emittedLayoutDictionaries.empty())
            return false;
        emittedLayoutDictionary.handle = object.handle;
        emittedLayoutDictionary.parentHandle = object.parentHandle;
        try {
            emittedLayoutDictionary.entries.reserve(object.m_entries.size());
            for (const DRW_Dictionary::Entry& entry : object.m_entries)
                emittedLayoutDictionary.entries.push_back(
                    {entry.m_name, entry.m_handle});
            m_emittedLayoutDictionaries.reserve(
                m_emittedLayoutDictionaries.size() + 1u);
        } catch (...) {
            return false;
        }
    }
    if (!emitDictionaryObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (capturesRegisteredDictionary)
        m_emittedDwgDictionaries.push_back(std::move(emittedDwgDictionary));
    if (capturesEmptyExtension)
        m_emittedEmptyExtensionDictionaries.push_back(
            {object.handle, object.parentHandle});
    if (capturesLayoutDictionary)
        m_emittedLayoutDictionaries.push_back(
            std::move(emittedLayoutDictionary));
    return true;
}

bool dwgWriter15::hasEmittedDwgDictionary(std::uint32_t handle) const {
    return handle != 0u && handle != DRW::NoHandle
        && std::any_of(m_emittedDwgDictionaries.cbegin(),
                       m_emittedDwgDictionaries.cend(),
                       [handle](const EmittedDwgDictionary& dictionary) {
                           return dictionary.handle == handle;
                       });
}

void dwgWriter15::recordRejectedDwgHandle(std::uint32_t handle) noexcept {
    if (handle == 0u || handle == DRW::NoHandle)
        return;
    try {
        if (m_rejectedDwgObjectHandles.size() >= 100000u) {
            m_rejectedDwgHandleOverflow = true;
            return;
        }
        m_rejectedDwgObjectHandles.push_back(handle);
    } catch (...) {
        m_rejectedDwgHandleOverflow = true;
    }
}

void dwgWriter15::restoreRejectedDwgHandles(std::size_t count,
                                             bool overflow) noexcept {
    if (count <= m_rejectedDwgObjectHandles.size())
        m_rejectedDwgObjectHandles.resize(count);
    m_rejectedDwgHandleOverflow = overflow;
}

// XRECORD (ODA fixed type 0x4f = 79) — no class registration required.
// XRECORD's encoder takes (buf, /*strBuf=*/nullptr, hb): strings are emitted
// inline as byte-counted data within the data section, so no separate
// string buffer is needed.
bool dwgWriter15::emitXRecordObject(std::uint32_t handle,
                                    const DRW_XRecord& xrecord,
                                    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                                    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version, dwgObjType::XRECORD, handle,
            m_objectStrings, m_objectHandles,
            sb, hb, xrecord.reactorCount(), xrecord.extensionDictionaryFlag(),
            xrecord.hasDataStorageBinaryData(), &xrecord.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    DRW_UNUSED(sb);
    if (!xrecord.encodeDwg(m_version, &body, nullptr, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeXRecord(const DRW_XRecord& xrecord) {
    // Check before allocation, preamble emission, or deferred handle work.
    // Unknown application bytes cannot be converted or patched as typed data.
    if (!xrecord.canReplayDwgRawData(m_version))
        return false;
    DRW_XRecord object = xrecord;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!emitXRecordObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::emitPlotSettingsObject(
    std::uint32_t handle, const DRW_PlotSettings& plotSettings,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    const std::uint16_t classNum = typedDwgClassNum(
        "AcDbPlotSettings", "PLOTSETTINGS", 0);
    if (classNum < 500)
        return false;

    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version, classNum, handle, m_objectStrings,
            m_objectHandles, sb, hb, plotSettings.reactorCount(),
            plotSettings.extensionDictionaryFlag(),
            plotSettings.hasDataStorageBinaryData(), &plotSettings.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!plotSettings.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writePlotSettings(
    const DRW_PlotSettings& plotSettings) {
    if (m_version < DRW::AC1015
        || plotSettings.reactorHandles.size() > dwgSafety::MaxReactorCount
        || plotSettings.extensionDictionaryFlag() > 1
        || plotSettings.hasDataStorageBinaryData())
        return false;

    DRW_PlotSettings object = plotSettings;
    const bool appendToDeferredDictionary =
        m_deferredPlotSettingsDictionary;
    if (appendToDeferredDictionary) {
        if (m_deferredPlotSettingsDictionaryEmitted
            || m_plotSettingsDictionaryHandle == 0
            || (object.parentHandle != 0
                && object.parentHandle != m_plotSettingsDictionaryHandle
                && object.parentHandle
                    != DRW::DwgNamedObjectsDictionaryHandle)
            || m_deferredPlotSettingsEntries.size()
                >= DRW_Dictionary::kMaxEntries)
            return false;
        object.parentHandle = m_plotSettingsDictionaryHandle;
    } else if (plotSettings.handle == 0
               && m_plotSettingsDictionaryHandle != 0) {
        // A handle-less object cannot be inserted into a caller-managed
        // dictionary: its dictionary entry needs the generated object handle.
        return false;
    }
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);

    DRW_Dictionary::Entry deferredEntry;
    if (appendToDeferredDictionary) {
        std::string key;
        try {
            if (object.pageSetupName.empty()) {
                object.pageSetupName = "LIBDXFRW_PLOTSETTINGS_"
                    + std::to_string(object.handle);
            }
            key = toUpperCase(object.pageSetupName);
            for (const DRW_Dictionary::Entry& entry :
                 m_deferredPlotSettingsEntries) {
                if (toUpperCase(entry.m_name) == key)
                    return false;
            }
            m_deferredPlotSettingsEntries.reserve(
                m_deferredPlotSettingsEntries.size() + 1u);
            deferredEntry.m_name = object.pageSetupName;
        } catch (const std::exception&) {
            return false;
        }
        deferredEntry.m_handle = object.handle;
    }
    if (!emitPlotSettingsObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    const std::uint16_t classNumber = typedDwgClassNum(
        "AcDbPlotSettings", "PLOTSETTINGS", 0);
    bool classInstanceRecorded = false;
    if (plotSettings.handle != 0) {
        markDwgClassInstanceEmitted(object.handle);
        std::uint16_t emittedClassNumber = 0;
        classInstanceRecorded =
            getDwgClassInstanceClass(object.handle, emittedClassNumber)
            && emittedClassNumber == classNumber;
    } else {
        classInstanceRecorded = noteDwgClassInstanceEmitted(classNumber);
    }
    if (!classInstanceRecorded)
        return false;
    if (appendToDeferredDictionary)
        m_deferredPlotSettingsEntries.push_back(std::move(deferredEntry));
    return true;
}

bool dwgWriter15::rejectDefaultDwgMaterialFrameConflicts() {
    if (!defaultDwgMaterialGraphActive() || m_writingDefaultMaterialGraph) return false;
    const auto protectedHandle = [this](std::uint64_t h) {
        return h != 0 && std::find(m_defaultMaterialHandles.begin(),
            m_defaultMaterialHandles.end(), h) != m_defaultMaterialHandles.end();
    };
    bool conflict = protectedHandle(m_currentHandle);
    std::size_t queuedLayerMaterialReferences = 0u;
    const auto check = [this, &protectedHandle, &conflict,
                        &queuedLayerMaterialReferences](const dwgBufferW& buffer) {
        for (const auto& token : buffer.handleOccurrences()) {
            std::uint64_t target = token.reference;
            switch (token.code) {
            case 6: target = static_cast<std::uint64_t>(m_currentHandle) + 1u; break;
            case 8:
                if (m_currentHandle == 0) { conflict = true; continue; }
                target = m_currentHandle - 1u; break;
            case 10:
                if (target > std::numeric_limits<std::uint64_t>::max() - m_currentHandle) {
                    conflict = true; continue;
                }
                target += m_currentHandle; break;
            case 12:
                if (target > m_currentHandle) { conflict = true; continue; }
                target = m_currentHandle - target; break;
            case 14: target = m_currentHandle; break;
            case 9: case 11: case 13: case 15:
                conflict = true; continue; // unknown relative role cannot be bounded
            default: break;
            }
            if (!protectedHandle(target)) continue;
            const bool nodEdge = m_currentHandle == DRW::DwgNamedObjectsDictionaryHandle
                && token.reference == m_defaultMaterialHandles[0]
                && token.code == DRW::DwgSoftOwnership;
            const bool layerEdge = m_currentHandle == DRW::DwgLayer0Handle
                && token.reference == m_defaultMaterialHandles[3]
                && token.code == DRW::DwgHardPointer;
            // Selected layers are checked against their exact emitted tail
            // later. The older AC1024 route also permits its queued layers.
            const bool selectedCanonicalLayer = std::binary_search(
                m_canonicalLayerHandles.begin(), m_canonicalLayerHandles.end(),
                m_currentHandle);
            const bool queuedLayerEdge = &buffer == &m_objectHandles
                && token.reference == m_defaultMaterialHandles[3]
                && token.code == DRW::DwgHardPointer
                && (selectedCanonicalLayer
                    || (m_version == DRW::AC1024
                        && std::any_of(m_pendingLayers.begin(), m_pendingLayers.end(),
                            [this](const auto& layer) {
                                return layer.first == m_currentHandle
                                    && layer.second.materialHandle.ref == m_defaultMaterialHandles[3];
                            })));
            if (queuedLayerEdge && ++queuedLayerMaterialReferences != 1u)
                conflict = true;
            if (!nodEdge && !layerEdge && !queuedLayerEdge) conflict = true;
        }
    };
    check(m_objectBody); check(m_objectStrings); check(m_objectHandles);
    if (conflict) m_defaultMaterialConflict = true;
    return conflict;
}

bool dwgWriter15::emitDefaultDwgMaterial(std::size_t index) {
    if (index < 1 || index > 3 || !defaultDwgMaterialGraphActive()) return false;
    const std::array<drwDwgDefaultMaterial::Kind, 3> kinds{{
        drwDwgDefaultMaterial::Kind::ByBlock, drwDwgDefaultMaterial::Kind::ByLayer,
        drwDwgDefaultMaterial::Kind::Global}};
    dwgBufferW fields, strings;
    if (!drwDwgDefaultMaterial::build(m_version, kinds[index - 1], fields, strings))
        return false;
    dwgBufferW& body = beginObject(m_defaultMaterialHandles[index]);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(body, m_version,
            typedDwgClassNum("AcDbMaterial", "MATERIAL", 0),
            m_defaultMaterialHandles[index], m_objectStrings, m_objectHandles,
            sb, hb, 1, 1, false)) return false;
    const auto append = [](dwgBufferW& dst, const dwgBufferW& src) {
        for (std::uint64_t i = 0; i < src.bitCount(); ++i)
            dst.putBit((src.data()[static_cast<std::size_t>(i / 8u)]
                >> (7u - i % 8u)) & 1u);
    };
    append(body, fields);
    append(*sb, strings);
    hb->putHandle(makeSoftPointer(m_defaultMaterialHandles[0]));
    hb->putHandle(makeSoftPointer(m_defaultMaterialHandles[0]));
    if (!body.isGood() || !sb->isGood() || !hb->isGood()) return false;
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::completeDeferredDwgObjects() {
    if (defaultDwgMaterialGraphActive() && !m_defaultMaterialGraphEmitted) {
        if (!m_dwgTableRecordsComplete || !m_dwgNamedObjectDictionaryComplete)
            return false;
        try {
            const CompoundWriteCheckpoint checkpoint = checkpointCompoundWrite();
            const auto fail = [this, &checkpoint] {
                try { rollbackCompoundWrite(checkpoint); }
                catch (...) { m_frameWriteError = true; }
                return false;
            };
            try {
                DRW_Dictionary dictionary;
                dictionary.handle = m_defaultMaterialHandles[0];
                dictionary.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
                dictionary.cloning = 1; dictionary.hardOwner = 0;
                dictionary.setDwgCommonObjectState(0, 1, false);
                dictionary.m_entries = {{"ByBlock", m_defaultMaterialHandles[1]},
                    {"ByLayer", m_defaultMaterialHandles[2]},
                    {"Global", m_defaultMaterialHandles[3]}};
                struct ScopedWrite {
                    bool& active;
                    explicit ScopedWrite(bool& value) : active(value) { active = true; }
                    ~ScopedWrite() { active = false; }
                };
                {
                    ScopedWrite scope(m_writingDefaultMaterialGraph);
                    if (!writeDictionary(dictionary)) return fail();
                    if (consumeWriteFailurePointForTest(kAfterDefaultMaterialDictionary))
                        throw std::bad_alloc();
                    for (std::size_t i = 1; i < m_defaultMaterialHandles.size(); ++i)
                        if (!emitDefaultDwgMaterial(i)) return fail();
                }
                m_defaultMaterialGraphEmitted = true;
            } catch (...) { return fail(); }
        } catch (...) { return false; }
    }

    if (m_defaultPlotStyleDictionaryHandle != 0 && !m_defaultPlotStyleGraphEmitted) {
        if (!m_dwgTableRecordsComplete || !m_dwgNamedObjectDictionaryComplete
            || m_defaultPlotStylePlaceholderHandle == 0)
            return false;
        try {
            const CompoundWriteCheckpoint checkpoint = checkpointCompoundWrite();
            const auto fail = [this, &checkpoint] {
                try {
                    rollbackCompoundWrite(checkpoint);
                } catch (...) {
                    // A rollback allocation failure cannot safely publish.
                    m_frameWriteError = true;
                }
                return false;
            };
            try {
                DRW_DictionaryWithDefault dictionary;
                dictionary.handle = m_defaultPlotStyleDictionaryHandle;
                dictionary.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
                dictionary.cloning = 1;
                dictionary.hardOwner = 0;
                const bool hasPersistentReactorProfile = m_version >= DRW::AC1015
                    && m_version <= DRW::AC1024;
                if (hasPersistentReactorProfile) {
                    dictionary.reactorHandles.push_back(
                        DRW::DwgNamedObjectsDictionaryHandle);
                }
                dictionary.setDwgCommonObjectState(
                    hasPersistentReactorProfile ? 1 : 0,
                    m_version >= DRW::AC1018 ? 1 : 0, false);
                dictionary.m_entries.push_back({"Normal", m_defaultPlotStylePlaceholderHandle});
                dictionary.m_defaultEntryHandle = m_defaultPlotStylePlaceholderHandle;
                DRW_AcDbPlaceholder placeholder;
                placeholder.handle = m_defaultPlotStylePlaceholderHandle;
                placeholder.parentHandle = m_defaultPlotStyleDictionaryHandle;
                if (hasPersistentReactorProfile)
                    placeholder.reactorHandles.push_back(dictionary.handle);
                placeholder.setDwgCommonObjectState(
                    hasPersistentReactorProfile ? 1 : 0,
                    m_version >= DRW::AC1018 ? 1 : 0, false);
                {
                    struct ScopedDefaultGraphWrite {
                        bool& active;
                        explicit ScopedDefaultGraphWrite(bool& flag) : active(flag) { active = true; }
                        ~ScopedDefaultGraphWrite() { active = false; }
                    } scope(m_writingDefaultPlotStyleGraph);
                    if (!writeDictionaryWithDefault(dictionary)) return fail();
                    if (consumeWriteFailurePointForTest(kAfterDefaultPlotStyleDictionary))
                        throw std::bad_alloc();
                    if (!writeAcDbPlaceholder(placeholder)) return fail();
                }
                m_defaultPlotStyleGraphEmitted = true;
            } catch (...) {
                return fail();
            }
        } catch (...) {
            // A checkpoint-copy failure occurs before graph frames are written.
            return false;
        }
    }
    if (!m_deferredPlotSettingsDictionary)
        return true;
    if (m_deferredPlotSettingsDictionaryEmitted)
        return true;
    if (!m_dwgTableRecordsComplete || !m_dwgNamedObjectDictionaryComplete
        || m_plotSettingsDictionaryHandle == 0
        || m_deferredPlotSettingsEntries.empty()
        || m_deferredPlotSettingsEntries.size()
            > DRW_Dictionary::kMaxEntries)
        return false;

    DRW_Dictionary dictionary;
    dictionary.handle = m_plotSettingsDictionaryHandle;
    dictionary.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
    dictionary.cloning = 1;
    dictionary.hardOwner = 1;
    try {
        dictionary.m_entries = m_deferredPlotSettingsEntries;
    } catch (const std::exception&) {
        return false;
    }

    const CompoundWriteCheckpoint checkpoint = checkpointCompoundWrite();
    if (!writeDictionary(dictionary)) {
        rollbackCompoundWrite(checkpoint);
        return false;
    }
    m_deferredPlotSettingsDictionaryEmitted = true;
    return true;
}

// LAYOUT (ODA fixed type 82, §20.4.84) — no class registration required.
// Encoder needs both string and handle buffers (AC1018+ split, inline
// pre-AC1018).
bool dwgWriter15::emitLayoutObject(std::uint32_t handle,
                                   const DRW_Layout& layout,
                                   const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                                   const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version, dwgObjType::LAYOUT, handle,
            m_objectStrings, m_objectHandles,
            sb, hb, layout.reactorCount(), layout.extensionDictionaryFlag(),
            layout.hasDataStorageBinaryData(), &layout.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!layout.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeLayout(const DRW_Layout& layout) {
    if (layout.viewportCount < 0
        || layout.viewportCount > DRW_Layout::kMaxViewportCount
        || layout.viewportHandles.size()
            != static_cast<std::size_t>(layout.viewportCount)
        || m_emittedLayoutAssociations.size()
            >= dwgSafety::MaxDwgReadObjectCount)
        return false;
    DRW_Layout object = layout;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    const std::uint64_t associatedBlockRecord =
        object.paperSpaceBlockRecordHandle.ref64 != 0
            ? object.paperSpaceBlockRecordHandle.ref64
            : object.paperSpaceBlockRecordHandle.ref;
    if (associatedBlockRecord > std::numeric_limits<std::uint32_t>::max()
        || associatedBlockRecord == DRW::NoHandle
        || (object.paperSpaceBlockRecordHandle.ref != 0
            && object.paperSpaceBlockRecordHandle.ref
                   != associatedBlockRecord))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    EmittedLayoutAssociation association;
    association.layoutHandle = object.handle;
    association.paperSpaceBlockRecordHandle =
        static_cast<std::uint32_t>(associatedBlockRecord);
    association.parentDictionaryHandle = object.parentHandle;
    try {
        association.name = object.name;
        m_emittedLayoutAssociations.reserve(
            m_emittedLayoutAssociations.size() + 1u);
    } catch (...) {
        return false;
    }
    if (!emitLayoutObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    m_emittedLayoutAssociations.push_back(std::move(association));
    return true;
}

bool dwgWriter15::validateDwgLayoutBlockAssociations(
    std::uint32_t& modelLayoutHandle,
    std::uint32_t& paperLayoutHandle) const {
    modelLayoutHandle = 0;
    paperLayoutHandle = 0;
    if (m_emittedLayoutAssociations.size()
        > dwgSafety::MaxDwgReadObjectCount)
        return false;

    std::vector<EmittedLayoutAssociation> layouts;
    try {
        layouts = m_emittedLayoutAssociations;
        std::sort(layouts.begin(), layouts.end(),
            [](const EmittedLayoutAssociation& left,
               const EmittedLayoutAssociation& right) {
                if (left.layoutHandle != right.layoutHandle)
                    return left.layoutHandle < right.layoutHandle;
                return left.paperSpaceBlockRecordHandle
                    < right.paperSpaceBlockRecordHandle;
            });
    } catch (...) {
        return false;
    }
    for (std::size_t index = 1; index < layouts.size(); ++index) {
        if (layouts[index - 1].layoutHandle == layouts[index].layoutHandle)
            return false;
    }

    std::uint32_t layoutDictionaryHandle = 0;
    for (const DRW::DwgNamedObjectDictionaryEntry& entry
         : m_pendingDwgNamedObjectDictionaryEntries) {
        if (entry.name != "ACAD_LAYOUT")
            continue;
        if (layoutDictionaryHandle != 0 || entry.childHandle == 0
            || entry.childHandle == DRW::NoHandle)
            return false;
        layoutDictionaryHandle = entry.childHandle;
    }

    if (layoutDictionaryHandle == 0) {
        if (!m_emittedLayoutDictionaries.empty())
            return false;
    } else {
        if (m_emittedLayoutDictionaries.size() != 1u)
            return false;
        const EmittedLayoutDictionary& dictionary =
            m_emittedLayoutDictionaries.front();
        if (dictionary.handle != layoutDictionaryHandle
            || dictionary.parentHandle != DRW::DwgNamedObjectsDictionaryHandle
            || dictionary.entries.size() != layouts.size())
            return false;

        std::set<std::string> names;
        std::set<std::uint32_t> childHandles;
        for (const DRW::DwgNamedObjectDictionaryEntry& entry
             : dictionary.entries) {
            if (entry.name.empty() || entry.childHandle == 0
                || entry.childHandle == DRW::NoHandle
                || !names.insert(entry.name).second
                || !childHandles.insert(entry.childHandle).second)
                return false;
            const auto layout = std::lower_bound(
                layouts.cbegin(), layouts.cend(), entry.childHandle,
                [](const EmittedLayoutAssociation& candidate,
                   std::uint32_t handle) {
                    return candidate.layoutHandle < handle;
                });
            if (layout == layouts.cend()
                || layout->layoutHandle != entry.childHandle
                || layout->name != entry.name
                || layout->parentDictionaryHandle != layoutDictionaryHandle)
                return false;
        }
    }

    for (std::size_t index = 0; index < layouts.size(); ++index) {
        const EmittedLayoutAssociation& layout = layouts[index];
        if (layout.layoutHandle == 0 || layout.layoutHandle == DRW::NoHandle)
            return false;
        const std::uint32_t blockRecordHandle =
            layout.paperSpaceBlockRecordHandle;
        if (blockRecordHandle == 0)
            continue;
        if (layoutDictionaryHandle == 0
            || layout.parentDictionaryHandle != layoutDictionaryHandle)
            return false;

        if (blockRecordHandle == reservedHandle::BLOCK_MODEL_SPACE) {
            if (modelLayoutHandle != 0)
                return false;
            modelLayoutHandle = layout.layoutHandle;
            continue;
        }
        if (blockRecordHandle == reservedHandle::BLOCK_PAPER_SPACE) {
            if (paperLayoutHandle != 0)
                return false;
            paperLayoutHandle = layout.layoutHandle;
            continue;
        }

        const PendingUserBlock* owner = findUserBlock(blockRecordHandle);
        if (owner == nullptr || owner->ownerKind != BlockOwnerKind::PaperLayout
            || owner->layoutHandle != layout.layoutHandle)
            return false;
    }

    for (const PendingUserBlock& owner : m_userBlocks) {
        if (owner.ownerKind != BlockOwnerKind::PaperLayout
            || owner.layoutHandle == 0)
            continue;
        const auto layout = std::lower_bound(
            layouts.cbegin(), layouts.cend(), owner.layoutHandle,
            [](const EmittedLayoutAssociation& candidate,
               std::uint32_t handle) {
                return candidate.layoutHandle < handle;
            });
        if (layout == layouts.cend() || layout->layoutHandle != owner.layoutHandle
            || layout->paperSpaceBlockRecordHandle
                != owner.blockRecordHandle
            || layoutDictionaryHandle == 0
            || layout->parentDictionaryHandle != layoutDictionaryHandle)
            return false;
    }
    return true;
}

// GROUP (ODA fixed type 72) — no class registration required.  Standard
// preamble + DRW_Group::encodeDwg sandwich.  description is the only
// string field (carried in sb for AC1018+ split); entityHandles live in
// the handle stream.  Mirrors the DICTIONARY / LAYOUT shape from PR 8b/8c.
bool dwgWriter15::emitGroupObject(std::uint32_t handle,
                                  const DRW_Group& group,
                                  const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                                  const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version, dwgObjType::GROUP, handle,
            m_objectStrings, m_objectHandles,
            sb, hb, group.reactorCount(), group.extensionDictionaryFlag(),
            group.hasDataStorageBinaryData(), &group.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!group.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::ensureDwgGroupMemberTargetIndex() {
    if (m_dwgGroupMemberTargetIndexReady)
        return true;

    if (m_blockOwnedEntityHandles.size()
        > dwgSafety::MaxDwgReadObjectCount)
        return false;

    std::vector<std::uint32_t> handles;
    try {
        handles.reserve(m_blockOwnedEntityHandles.size());
        for (const auto& ownerAndHandle : m_blockOwnedEntityHandles)
            handles.push_back(ownerAndHandle.second);
        std::sort(handles.begin(), handles.end());
    } catch (...) {
        return false;
    }
    handles.erase(std::unique(handles.begin(), handles.end()), handles.end());
    m_dwgGroupMemberTargetHandles.swap(handles);
    m_dwgGroupMemberTargetIndexReady = true;
    return true;
}

bool dwgWriter15::writeGroup(const DRW_Group& group) {
    if (!m_dwgGroupDictionaryComplete || group.handle == 0
        || group.handle == DRW::NoHandle
        || group.parentHandle != DRW::DwgAcadGroupDictionaryHandle
        || group.name.empty()) {
        return false;
    }
    if (!ensureEmittedDwgGroupEntryIndex()
        || !ensureEmittedDwgGroupFrameIndex()) {
        return false;
    }
    const auto registrationIndex = m_emittedDwgGroupEntryIndexes.find(
        group.handle);
    if (registrationIndex == m_emittedDwgGroupEntryIndexes.end()
        || registrationIndex->second
            >= m_emittedDwgGroupDictionaryEntries.size()
        || m_emittedDwgGroupDictionaryEntries[registrationIndex->second].name
            != group.name
        || m_emittedDwgGroupFrameHandles.find(group.handle)
            != m_emittedDwgGroupFrameHandles.end()) {
        return false;
    }
    const DRW::DwgNamedObjectDictionaryEntry& registration =
        m_emittedDwgGroupDictionaryEntries[registrationIndex->second];
    if (!ensureDwgGroupMemberTargetIndex())
        return false;
    std::set<std::uint32_t> memberHandles;
    bool usesQualifiedPolylineVertex = false;
    for (const std::uint32_t memberHandle : group.m_entityHandles) {
        const bool blockOwned = std::binary_search(m_dwgGroupMemberTargetHandles.cbegin(),
                                                   m_dwgGroupMemberTargetHandles.cend(),
                                                   memberHandle);
        const bool qualifiedVertex = !blockOwned && m_version == DRW::AC1021
            && m_ac1021PolylineGroupVertexIndex.count({memberHandle, group.handle}) == 1u;
        if (memberHandle == 0 || memberHandle == DRW::NoHandle
            || !memberHandles.insert(memberHandle).second
            || (!blockOwned && !qualifiedVertex)) {
            return false;
        }
        usesQualifiedPolylineVertex = usesQualifiedPolylineVertex || qualifiedVertex;
    }
    if (usesQualifiedPolylineVertex
        && (group.m_entityHandles.size() != 1u || !group.m_description.empty()
            || group.m_isUnnamed || !group.m_selectable || group.flags != 0
            || group.reactorHandles != std::vector<std::uint32_t>{group.parentHandle}
            || group.extensionDictionaryFlag() != 1u || group.xDictHandle != 0u
            || group.hasDataStorageBinaryData() || !group.appData.empty()
            || !group.extData.empty()))
        return false;

    DRW_Group object = group;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    const auto frameResult = m_emittedDwgGroupFrameHandles.insert(group.handle);
    if (!frameResult.second)
        return false;
    try {
        m_emittedDwgGroupFrames.push_back(registration);
    } catch (const std::exception&) {
        m_emittedDwgGroupFrameHandles.erase(frameResult.first);
        return false;
    }
    struct ScopedVertexGroup {
        bool& slot;
        bool previous;
        ~ScopedVertexGroup() { slot = previous; }
    } groupScope{m_writingAc1021PolylineVertexGroup, m_writingAc1021PolylineVertexGroup};
    m_writingAc1021PolylineVertexGroup = usesQualifiedPolylineVertex;
    if (!emitGroupObject(object.handle, object, appIdRefs, layerRefs)) {
        m_emittedDwgGroupFrames.pop_back();
        m_emittedDwgGroupFrameHandles.erase(frameResult.first);
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

// MLINESTYLE (fixed object type 73) needs no CLASS registration. Record the
// emitted handle by style name so later MLINE entities can point at it.
bool dwgWriter15::emitMLineStyleObject(std::uint32_t handle,
                                       const DRW_MLineStyle& style,
                                       const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                                       const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version, dwgObjType::MLINESTYLE, handle,
            m_objectStrings, m_objectHandles,
            sb, hb, style.reactorCount(), style.extensionDictionaryFlag(),
            style.hasDataStorageBinaryData(), &style.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!style.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeMLineStyle(const DRW_MLineStyle& style) {
    DRW_MLineStyle object = style;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    const std::string upper = toUpperCase(object.name);
    auto existing = m_writingCtx.mlineStyleMap.find(upper);
    if (!upper.empty() && existing != m_writingCtx.mlineStyleMap.end()) {
        if (object.handle == 0 || object.handle == existing->second) {
            if (!object.extData.empty()) {
                m_objectEedWriteFailure = true;
                return false;
            }
            return true;
        }
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    for (DRW_MLineElement& element : object.elements) {
        if (element.linetypeHandle != 0 || element.linetype.empty())
            continue;
        auto lineTypeIt = m_writingCtx.ltypeMap.find(toUpperCase(element.linetype));
        if (lineTypeIt != m_writingCtx.ltypeMap.end())
            element.linetypeHandle = lineTypeIt->second;
    }
    if (!emitMLineStyleObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (!upper.empty())
        m_writingCtx.mlineStyleMap[upper] = object.handle;
    return true;
}

// RASTERVARIABLES (AcDbRasterVariables, custom class 505) — class
// registration required so the reader's CLASSES section dispatch can
// resolve the recordName back to DRW_RasterVariables::parseDwg. Standard
// preamble + DRW_RasterVariables::encodeDwg sandwich. Encoder ignores
// strBuf (no string fields), so pass nullptr to mirror parse semantics.
bool dwgWriter15::emitRasterVariablesObject(
    std::uint32_t handle, const DRW_RasterVariables& rasterVariables,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbRasterVariables", "RASTERVARIABLES",
                             DRW_RasterVariables::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            rasterVariables.reactorCount(),
            rasterVariables.extensionDictionaryFlag(),
            rasterVariables.hasDataStorageBinaryData(), &rasterVariables.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    DRW_UNUSED(sb);
    if (!rasterVariables.encodeDwg(m_version, &body, nullptr, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeRasterVariables(
    const DRW_RasterVariables& rasterVariables) {
    // PR 13f — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (only the standard hb = version > AC1018 split-buffer
    // routing).  Parser mirrors the same shape.  The matching class
    // registration in writeDwgClasses now gates on
    // canRegisterCustomClassObjects (≥AC1015).
    if (m_version < DRW::AC1015
        || rasterVariables.m_classVersion < 0
        || rasterVariables.m_classVersion > 10
        || (rasterVariables.m_imageFrame != 0
            && rasterVariables.m_imageFrame != 1)
        || (rasterVariables.m_imageQuality != 0
            && rasterVariables.m_imageQuality != 1)
        || rasterVariables.m_units < 0 || rasterVariables.m_units > 8)
        return false;

    DRW_RasterVariables object = rasterVariables;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerRasterVariablesObjectClass(object.handle))
        return false;

    if (!emitRasterVariablesObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

// WIPEOUTVARIABLES (AcDbWipeoutVariables, custom class 529). Body has one
// display-frame flag, then the common handle prefix in the object handle
// stream.
bool dwgWriter15::emitWipeoutVariablesObject(
    std::uint32_t handle, const DRW_WipeoutVariables& wipeoutVariables,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbWipeoutVariables", "WIPEOUTVARIABLES",
                             DRW_WipeoutVariables::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            wipeoutVariables.reactorCount(),
            wipeoutVariables.extensionDictionaryFlag(),
            wipeoutVariables.hasDataStorageBinaryData(),
            &wipeoutVariables.extData, &appIdRefs, &layerRefs,
            fileCodePageId())) {
        return false;
    }
    DRW_UNUSED(sb);
    if (!wipeoutVariables.encodeDwg(m_version, &body, nullptr, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeWipeoutVariables(
    const DRW_WipeoutVariables& wipeoutVariables) {
    if (m_version < DRW::AC1015 || wipeoutVariables.m_displayFrame > 1u)
        return false;

    DRW_WipeoutVariables object = wipeoutVariables;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerWipeoutVariablesObjectClass(object.handle))
        return false;

    if (!emitWipeoutVariablesObject(object.handle, object, appIdRefs,
                                    layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

// IMAGEDEF (fixed type 102 / dwgObjType::IMAGEDEF) — no CLASSES entry.
// Inverts DRW_ImageDef::parseDwg; handle is assigned when the caller left it 0.
bool dwgWriter15::writeImageDef(DRW_ImageDef& imageDef) {
    if (imageDef.reactorHandles.size() > dwgSafety::MaxReactorCount
        || imageDef.extensionDictionaryFlag() > 1
        || (m_version > DRW::AC1024
            && imageDef.hasDataStorageBinaryData()))
        return false;
    if (!prepareDwgCommonObjectState(imageDef, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(imageDef, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !imageDef.extData.empty();
        return false;
    }
    if (imageDef.handle == 0) {
        imageDef.handle = m_handles.next();
    } else {
        m_handles.reserve(imageDef.handle);
    }
    dwgBufferW& body = beginObject(imageDef.handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version, /*typeCode=*/102, imageDef.handle,
            m_objectStrings, m_objectHandles, sb, hb,
            imageDef.reactorCount(), imageDef.extensionDictionaryFlag(),
            imageDef.hasDataStorageBinaryData(), &imageDef.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        m_objectEedWriteFailure = !imageDef.extData.empty();
        return false;
    }
    if (!imageDef.encodeDwg(m_version, &body, sb, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

// IMAGEDEF_REACTOR (custom class 532). Body is class_version + common handles.
bool dwgWriter15::writeImageDefinitionReactor(
    DRW_ImageDefinitionReactor& reactor) {
    if (reactor.reactorHandles.size() > dwgSafety::MaxReactorCount
        || reactor.extensionDictionaryFlag() > 1
        || (m_version > DRW::AC1024
            && reactor.hasDataStorageBinaryData()))
        return false;
    if (!prepareDwgCommonObjectState(reactor, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(reactor, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !reactor.extData.empty();
        return false;
    }
    if (reactor.handle == 0) {
        reactor.handle = m_handles.next();
    } else {
        m_handles.reserve(reactor.handle);
    }
    if (!registerImageDefReactorObjectClass(reactor.handle))
        return false;
    dwgBufferW& body = beginObject(reactor.handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbRasterImageDefReactor", "IMAGEDEF_REACTOR",
                             DRW_ImageDefinitionReactor::kDwgClassNum),
            reactor.handle, m_objectStrings, m_objectHandles, sb, hb,
            reactor.reactorCount(), reactor.extensionDictionaryFlag(),
            reactor.hasDataStorageBinaryData(), &reactor.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        m_objectEedWriteFailure = !reactor.extData.empty();
        return false;
    }
    DRW_UNUSED(sb);
    if (!reactor.encodeDwg(m_version, &body, nullptr, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writePointCloudDef(DRW_PointCloudDef& definition) {
    if (m_version < DRW::AC1015
        || definition.reactorHandles.size() > dwgSafety::MaxReactorCount
        || definition.extensionDictionaryFlag() > 1)
        return false;
    if (!prepareDwgCommonObjectState(definition, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(definition, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !definition.extData.empty();
        return false;
    }
    if (definition.handle == 0)
        definition.handle = m_handles.next();
    else
        m_handles.reserve(definition.handle);
    if (!registerPointCloudDefObjectClass(definition.m_kind,
                                          definition.handle))
        return false;

    std::uint16_t classNum = DRW_PointCloudDef::kDwgClassNumDefinition;
    const char *className = "AcDbPointCloudDef";
    const char *recordName = "POINTCLOUDDEFINITION";
    switch (definition.m_kind) {
    case DRW_PointCloudDef::Definition:
        classNum = DRW_PointCloudDef::kDwgClassNumDefinition;
        break;
    case DRW_PointCloudDef::DefinitionEx:
        classNum = DRW_PointCloudDef::kDwgClassNumDefinitionEx;
        className = "AcDbPointCloudDefEx";
        recordName = "POINTCLOUDDEFINITIONEX";
        break;
    case DRW_PointCloudDef::Reactor:
        classNum = DRW_PointCloudDef::kDwgClassNumReactor;
        className = "AcDbPointCloudDefReactor";
        recordName = "POINTCLOUDDEFREACTOR";
        break;
    case DRW_PointCloudDef::ReactorEx:
        classNum = DRW_PointCloudDef::kDwgClassNumReactorEx;
        className = "AcDbPointCloudDefReactorEx";
        recordName = "POINTCLOUDDEFREACTOREX";
        break;
    }

    dwgBufferW& body = beginObject(definition.handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version, typedDwgClassNum(className, recordName, classNum),
            definition.handle, m_objectStrings, m_objectHandles, sb, hb,
            definition.reactorCount(), definition.extensionDictionaryFlag(),
            definition.hasDataStorageBinaryData(), &definition.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        m_objectEedWriteFailure = !definition.extData.empty();
        return false;
    }
    if (!definition.encodeDwg(m_version, &body, sb, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeNavisworksModelDef(DRW_NavisworksModelDef& definition) {
    if (m_version < DRW::AC1015
        || definition.reactorHandles.size() > dwgSafety::MaxReactorCount
        || definition.extensionDictionaryFlag() > 1)
        return false;
    if (!prepareDwgCommonObjectState(definition, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(definition, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !definition.extData.empty();
        return false;
    }
    if (definition.handle == 0)
        definition.handle = m_handles.next();
    else
        m_handles.reserve(definition.handle);
    if (!registerNavisworksModelDefObjectClass(definition.handle))
        return false;

    dwgBufferW& body = beginObject(definition.handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbNavisworksModelDef", "NAVISWORKSMODELDEF",
                             DRW_NavisworksModelDef::kDwgClassNum),
            definition.handle, m_objectStrings, m_objectHandles, sb, hb,
            definition.reactorCount(), definition.extensionDictionaryFlag(),
            definition.hasDataStorageBinaryData(), &definition.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        m_objectEedWriteFailure = !definition.extData.empty();
        return false;
    }
    if (!definition.encodeDwg(m_version, &body, sb, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writePointCloudColorMap(DRW_PointCloudColorMap& colorMap) {
    if (m_version < DRW::AC1015
        || colorMap.reactorHandles.size() > dwgSafety::MaxReactorCount
        || colorMap.extensionDictionaryFlag() > 1)
        return false;
    if (!prepareDwgCommonObjectState(colorMap, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(colorMap, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !colorMap.extData.empty();
        return false;
    }
    if (colorMap.handle == 0)
        colorMap.handle = m_handles.next();
    else
        m_handles.reserve(colorMap.handle);
    if (!registerPointCloudColorMapObjectClass(colorMap.handle))
        return false;

    dwgBufferW& body = beginObject(colorMap.handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbPointCloudColorMap", "POINTCLOUDCOLORMAP",
                             DRW_PointCloudColorMap::kDwgClassNum),
            colorMap.handle, m_objectStrings, m_objectHandles, sb, hb,
            colorMap.reactorCount(), colorMap.extensionDictionaryFlag(),
            colorMap.hasDataStorageBinaryData(), &colorMap.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        m_objectEedWriteFailure = !colorMap.extData.empty();
        return false;
    }
    if (!colorMap.encodeDwg(m_version, &body, sb, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

// GEODATA (AcDbGeoData, custom class 506) - class registration required.
// Standard preamble + DRW_GeoData::encodeDwg sandwich. Encoder writes its own
// common-handle prefix into hb (the preamble does not) plus several
// variable-text fields into sb.
bool dwgWriter15::emitGeoDataObject(
    std::uint32_t handle, const DRW_GeoData& geoData,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbGeoData", "GEODATA",
                             DRW_GeoData::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            geoData.reactorCount(), geoData.extensionDictionaryFlag(),
            geoData.hasDataStorageBinaryData(), &geoData.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!geoData.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeGeoData(const DRW_GeoData& geoData) {
    // PR 13f — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (only the standard sb/hb = version > AC1018 split-
    // buffer routing).  Parser mirrors the same shape.
    if (m_version < DRW::AC1015
        || geoData.reactorHandles.size() > dwgSafety::MaxReactorCount
        || geoData.extensionDictionaryFlag() > 1)
        return false;
    if (geoData.m_version >= 2
        && (geoData.m_coordinatesType < 0 || geoData.m_coordinatesType > 3))
        return false;
    if ((geoData.m_version == 1
            && geoData.m_scaleEstimationMethod != 1)
        || ((geoData.m_version == 2 || geoData.m_version == 3)
            && (geoData.m_scaleEstimationMethod < 1
                || geoData.m_scaleEstimationMethod > 4)))
        return false;
    if ((geoData.m_version == 2 || geoData.m_version == 3)
        && (geoData.m_horizontalUnits < 0 || geoData.m_horizontalUnits > 24
            || geoData.m_verticalUnits < 0 || geoData.m_verticalUnits > 24))
        return false;

    DRW_GeoData object = geoData;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerGeoDataObjectClass(object.handle))
        return false;

    if (!emitGeoDataObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

// SPATIAL_FILTER (AcDbSpatialFilter, custom class 507) — class registration
// required.  Standard preamble + DRW_SpatialFilter::encodeDwg sandwich.
// Encoder writes its own common-handle prefix into hb; body fields are
// pure numeric (no strings), so strBuf is unused.
bool dwgWriter15::emitSpatialFilterObject(std::uint32_t handle,
                                          const DRW_SpatialFilter& filter,
                                          const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                                          const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbSpatialFilter", "SPATIAL_FILTER",
                             DRW_SpatialFilter::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            filter.reactorCount(), filter.extensionDictionaryFlag(),
            filter.hasDataStorageBinaryData(), &filter.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    DRW_UNUSED(sb);
    if (!filter.encodeDwg(m_version, &body, nullptr, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeSpatialFilter(const DRW_SpatialFilter& filter) {
    // PR 13f — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (only the standard hb = version > AC1018 split-buffer
    // routing).  Parser mirrors the same shape.
    if (m_version < DRW::AC1015
        || filter.reactorHandles.size() > dwgSafety::MaxReactorCount
        || filter.extensionDictionaryFlag() > 1)
        return false;

    DRW_SpatialFilter object = filter;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerSpatialFilterObjectClass(object.handle))
        return false;

    if (!emitSpatialFilterObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

// SCALE (AcDbScale, custom class 508) — class registration required.  The
// SCALE encoder writes body fields only; the wrapper emits the common handle
// prefix (parentHandle + reactors + xdic) in the handle stream.  PR 8d.2a.
bool dwgWriter15::emitScaleObject(
    std::uint32_t handle, const DRW_Scale& scale,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbScale", "SCALE", DRW_Scale::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            scale.reactorCount(), scale.extensionDictionaryFlag(),
            scale.hasDataStorageBinaryData(), &scale.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!scale.encodeDwg(m_version, &body, sb))
        return false;

    hb->putHandle(makeSoftPointer(static_cast<std::uint32_t>(scale.parentHandle)));
    for (std::int32_t i = 0; i < scale.reactorCount(); ++i) {
        const std::uint32_t reactor =
            static_cast<std::size_t>(i) < scale.reactorHandles.size()
                ? scale.reactorHandles[static_cast<std::size_t>(i)] : 0u;
        hb->putHandle(makeSoftPointer(reactor));
    }
    if (scale.extensionDictionaryFlag() != 1)
        hb->putHandle(makeHardOwner(scale.xDictHandle));
    finishObject();
    return !objectWriteFailed();
}

    // PR 13g — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (body-only emit; common handle prefix written by
    // emitScaleObject itself).
bool dwgWriter15::writeScale(const DRW_Scale& scale) {
    return writeTypedObject(scale, DRW::AC1015,
                            &dwgWriter::registerScaleObjectClass,
                            &dwgWriter15::emitScaleObject);
}

// IDBUFFER (AcDbIdBuffer, custom class 509) — class registration required.
// Standard preamble + DRW_IDBuffer::encodeDwg sandwich.  Encoder owns its
// common-handle prefix + object-id handles in hb.  PR 8d.2a.
bool dwgWriter15::emitIDBufferObject(
    std::uint32_t handle, const DRW_IDBuffer& idBuffer,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbIdBuffer", "IDBUFFER",
                             DRW_IDBuffer::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            idBuffer.reactorCount(), idBuffer.extensionDictionaryFlag(),
            idBuffer.hasDataStorageBinaryData(), &idBuffer.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    DRW_UNUSED(sb);
    if (!idBuffer.encodeDwg(m_version, &body, nullptr, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

    // PR 13g — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (only the standard hb = version > AC1018 split-buffer
    // routing).
bool dwgWriter15::writeIDBuffer(const DRW_IDBuffer& idBuffer) {
    return writeTypedObject(idBuffer, DRW::AC1015,
                            &dwgWriter::registerIDBufferObjectClass,
                            &dwgWriter15::emitIDBufferObject);
}

// LAYER_INDEX (AcDbLayerIndex, custom class 510) — class registration
// required.  Standard preamble + DRW_LayerIndex::encodeDwg sandwich.  Encoder
// writes entry names through sb (TV); per-entry handles + common prefix go
// to hb.  PR 8d.2a.
bool dwgWriter15::emitLayerIndexObject(
    std::uint32_t handle, const DRW_LayerIndex& layerIndex,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbLayerIndex", "LAYER_INDEX",
                             DRW_LayerIndex::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            layerIndex.reactorCount(), layerIndex.extensionDictionaryFlag(),
            layerIndex.hasDataStorageBinaryData(), &layerIndex.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!layerIndex.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

    // PR 13g — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (only the standard sb/hb = version > AC1018 split-
    // buffer routing).
bool dwgWriter15::writeLayerIndex(const DRW_LayerIndex& layerIndex) {
    return writeTypedObject(layerIndex, DRW::AC1015,
                            &dwgWriter::registerLayerIndexObjectClass,
                            &dwgWriter15::emitLayerIndexObject);
}

// SPATIAL_INDEX (AcDbSpatialIndex, custom class 511) — class registration
// required.  Standard preamble + DRW_SpatialIndex::encodeDwg sandwich.
// Encoder writes timestamps to the body and the common-handle prefix to hb
// (the latter only at R2007+; gated to AC1021+ anyway).  PR 8d.2a.
bool dwgWriter15::emitSpatialIndexObject(
    std::uint32_t handle, const DRW_SpatialIndex& spatialIndex,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbSpatialIndex", "SPATIAL_INDEX",
                             DRW_SpatialIndex::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            spatialIndex.reactorCount(), spatialIndex.extensionDictionaryFlag(),
            spatialIndex.hasDataStorageBinaryData(), &spatialIndex.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    DRW_UNUSED(sb);
    if (!spatialIndex.encodeDwg(m_version, &body, nullptr, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

    // PR 13g — broaden gate from AC1021+ to AC1015+.  Encoder gates the
    // common handle prefix on `version > AC1018` (parser does the same),
    // so at AC1015/AC1018 the wrapper emits an opaque body and no handle
    // tail — matching the parser's expectation.
bool dwgWriter15::writeSpatialIndex(const DRW_SpatialIndex& spatialIndex) {
    return writeTypedObject(spatialIndex, DRW::AC1015,
                            &dwgWriter::registerSpatialIndexObjectClass,
                            &dwgWriter15::emitSpatialIndexObject);
}

// DICTIONARYVAR (AcDbDictionaryVar, custom class 512) — class registration
// required.  Standard preamble + DRW_DictionaryVar::encodeDwg sandwich.
// Encoder owns its common-handle prefix; the m_value string goes through
// sb at AC1018+.  PR 8d.2a.
bool dwgWriter15::emitDictionaryVarObject(
    std::uint32_t handle, const DRW_DictionaryVar& dictionaryVar,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbDictionaryVar", "DICTIONARYVAR",
                             DRW_DictionaryVar::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            dictionaryVar.reactorCount(),
            dictionaryVar.extensionDictionaryFlag(),
            dictionaryVar.hasDataStorageBinaryData(), &dictionaryVar.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!dictionaryVar.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

    // PR 13g — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (only the standard sb/hb = version > AC1018 split-
    // buffer routing).
bool dwgWriter15::writeDictionaryVar(const DRW_DictionaryVar& dictionaryVar) {
    return writeTypedObject(dictionaryVar, DRW::AC1015,
                            &dwgWriter::registerDictionaryVarObjectClass,
                            &dwgWriter15::emitDictionaryVarObject);
}

// DICTIONARYWDFLT (AcDbDictionaryWithDefault, custom class 513) — class
// registration required.  The encoder delegates to
// DRW_Dictionary::encodeDwg for the body + per-entry handle list, then
// appends the single default-entry handle at the tail (mirroring its
// parser).  Standard preamble + encoder sandwich.  PR 8d.2b.
bool dwgWriter15::emitDictionaryWithDefaultObject(
    std::uint32_t handle, const DRW_DictionaryWithDefault& dictionary,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbDictionaryWithDefault",
                             "ACDBDICTIONARYWDFLT",
                             DRW_DictionaryWithDefault::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            dictionary.reactorCount(), dictionary.extensionDictionaryFlag(),
            dictionary.hasDataStorageBinaryData(), &dictionary.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    const bool typedLegacyNulls = m_version == DRW::AC1015
        && m_defaultPlotStyleDictionaryHandle != 0u
        && handle == m_defaultPlotStyleDictionaryHandle;
    if (!dictionary.encodeDwgForWriter(m_version, &body, sb, hb, typedLegacyNulls))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeDictionaryWithDefault(const DRW_DictionaryWithDefault& dictionary) {
    if (conflictsLegacyModelChainDictionary(dictionary)) { m_legacyModelChainConflict = true; return false; }
    if (conflictsDefaultDwgMaterialDictionary(dictionary, dictionary.m_defaultEntryHandle)) { m_defaultMaterialConflict = true; return false; }
    if (conflictsDefaultDwgPlotStyleDictionary(
            dictionary, dictionary.m_defaultEntryHandle, true))
        return false;
    // PR 13h — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (delegates to DRW_Dictionary::encodeDwg, then
    // appends a single default-entry hard pointer at the tail).
    if (m_version < DRW::AC1015)
        return false;

    DRW_DictionaryWithDefault object = dictionary;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerDictionaryWithDefaultObjectClass(object.handle))
        return false;

    if (!emitDictionaryWithDefaultObject(object.handle, object, appIdRefs,
                                         layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (m_canonicalPlotTarget != 0u && object.m_defaultEntryHandle == m_canonicalPlotTarget) {
        const bool reactorProfile = m_version == DRW::AC1021
            || (m_version == DRW::AC1024 && m_writingDefaultPlotStyleGraph);
        const std::array<std::pair<std::uint8_t, std::uint64_t>, 3> expected{{
            {DRW::DwgSoftPointer, DRW::DwgNamedObjectsDictionaryHandle},
            {DRW::DwgSoftOwnership, m_canonicalPlotTarget},
            {DRW::DwgHardPointer, m_canonicalPlotTarget}}};
        const std::array<std::pair<std::uint8_t, std::uint64_t>, 4> expectedR2007{{
            expected[0], expected[0], expected[1], expected[2]}};
        const auto objectType = typedDwgClassNum(
            "AcDbDictionaryWithDefault", "ACDBDICTIONARYWDFLT",
            DRW_DictionaryWithDefault::kDwgClassNum);
        const bool rolesMatch = reactorProfile
            ? canonicalHandleTailMatches(m_lastDwgObjectFrame, m_version, object.handle,
                                         objectType, expectedR2007)
            : canonicalHandleTailMatches(m_lastDwgObjectFrame, m_version, object.handle,
                                         objectType, expected);
        const bool reactorsMatch = reactorProfile
            ? dictionary.reactorCount() == 1 && object.reactorCount() == 1
                && object.reactorHandles.size() == 1u
                && object.reactorHandles.front() == DRW::DwgNamedObjectsDictionaryHandle
            : dictionary.reactorCount() == 0 && object.reactorCount() == 0;
        if (m_canonicalPlotDictionarySeen
            || object.parentHandle != DRW::DwgNamedObjectsDictionaryHandle
            || object.m_entries.size() != 1u || object.m_entries.front().m_name != "Normal"
            || object.m_entries.front().m_handle != m_canonicalPlotTarget
            || object.cloning != 1 || object.hardOwner != 0 || !reactorsMatch
            || object.extensionDictionaryFlag() != 1u
            || object.xDictHandle != 0u || object.hasDataStorageBinaryData()
            || !object.appData.empty() || !object.extData.empty()
            || !object.pendingAppIdResolutions.empty() || !object.pendingLayerRefResolutions.empty()
            || !rolesMatch)
            return false;
        m_canonicalPlotDictionary = object.handle;
        m_canonicalPlotDictionarySeen = true;
    }
    return true;
}

// SORTENTSTABLE (AcDbSortentsTable, custom class 514) — class registration
// required.  Standard preamble + DRW_SortEntsTable::encodeDwg sandwich.
// Encoder inverts the usual "all handles in handle stream" convention: per-
// entry sort handles go inline in the body section BEFORE the common
// prefix; block-owner + entity handles follow in the handle stream.
// PR 8d.2b.
bool dwgWriter15::emitSortEntsTableObject(
    std::uint32_t handle, const DRW_SortEntsTable& sortEntsTable,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbSortentsTable", "SORTENTSTABLE",
                             DRW_SortEntsTable::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            sortEntsTable.reactorCount(),
            sortEntsTable.extensionDictionaryFlag(),
            sortEntsTable.hasDataStorageBinaryData(), &sortEntsTable.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    DRW_UNUSED(sb);
    if (!sortEntsTable.encodeDwg(m_version, &body, nullptr, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

    // PR 13h — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (inline sort handles in body, then standard hb =
    // version > AC1018 split-buffer routing for the common prefix +
    // block-owner + entity handles).
bool dwgWriter15::writeSortEntsTable(const DRW_SortEntsTable& sortEntsTable) {
    return writeTypedObject(sortEntsTable, DRW::AC1015,
                            &dwgWriter::registerSortEntsTableObjectClass,
                            &dwgWriter15::emitSortEntsTableObject);
}

// FIELDLIST (AcDbFieldList, custom class 515) — class registration
// required.  Standard preamble + DRW_FieldList::encodeDwg sandwich.
// Encoder owns common-handle prefix + per-field handle list in hb.  No
// strings.  PR 8d.2b.
bool dwgWriter15::emitFieldListObject(std::uint32_t handle,
                                      const DRW_FieldList& fieldList,
                                      const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                                      const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbFieldList", "FIELDLIST",
                             DRW_FieldList::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            fieldList.reactorCount(), fieldList.extensionDictionaryFlag(),
            fieldList.hasDataStorageBinaryData(), &fieldList.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    DRW_UNUSED(sb);
    if (!fieldList.encodeDwg(m_version, &body, nullptr, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

    // PR 13h — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (no strings; only the standard hb = version > AC1018
    // split-buffer routing).
bool dwgWriter15::writeFieldList(const DRW_FieldList& fieldList) {
    return writeTypedObject(fieldList, DRW::AC1015,
                            &dwgWriter::registerFieldListObjectClass,
                            &dwgWriter15::emitFieldListObject);
}

// FIELD (AcDbField, custom class 516) — class registration required.
// Standard preamble + DRW_Field::encodeDwg sandwich.  Encoder writes
// evaluator/code/format/messages through sb (TV), CadValue + child values
// through buf+sb, and common-prefix + child handles + object handles
// inline through hb.  PR 8d.2b.
bool dwgWriter15::emitFieldObject(
    std::uint32_t handle, const DRW_Field& field,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbField", "FIELD", DRW_Field::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            field.reactorCount(), field.extensionDictionaryFlag(),
            field.hasDataStorageBinaryData(), &field.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!field.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

    // PR 13h — broaden gate from AC1021+ to AC1015+.  Encoder has a
    // `version < AC1021` branch for m_formatString, mirrored in the
    // parser, so the pre-R2007 path is exercised; nested CadValue /
    // child values go through writeCadValue/readCadValue (also
    // version-clean).
bool dwgWriter15::writeField(const DRW_Field& field) {
    return writeTypedObject(field, DRW::AC1015,
                            &dwgWriter::registerFieldObjectClass,
                            &dwgWriter15::emitFieldObject);
}

bool dwgWriter15::emitUnderlayDefinitionObject(
    std::uint32_t handle, const DRW_UnderlayDefinition& definition,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    const std::uint16_t sourceClassNum = underlayDefinitionClassNum(definition);
    if (sourceClassNum == 0)
        return false;
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum(
                definition.kind == DRW_UnderlayDefinition::DGN
                    ? "AcDbDgnDefinition"
                    : definition.kind == DRW_UnderlayDefinition::DWF
                        ? "AcDbDwfDefinition"
                        : "AcDbPdfDefinition",
                definition.kind == DRW_UnderlayDefinition::DGN
                    ? "DGNDEFINITION"
                    : definition.kind == DRW_UnderlayDefinition::DWF
                        ? "DWFDEFINITION"
                        : "PDFDEFINITION",
                sourceClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            definition.reactorCount(), definition.extensionDictionaryFlag(),
            definition.hasDataStorageBinaryData(), &definition.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!definition.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeUnderlayDefinition(
    const DRW_UnderlayDefinition& definition) {
    if (m_version < DRW::AC1015
        || definition.reactorHandles.size() > dwgSafety::MaxReactorCount
        || definition.xDictFlag > 1
        || underlayDefinitionClassNum(definition) == 0)
        return false;

    DRW_UnderlayDefinition object = definition;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerUnderlayDefinitionObjectClass(object.kind, object.handle))
        return false;

    if (!emitUnderlayDefinitionObject(object.handle, object, appIdRefs,
                                      layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::writeSection(const DRW_Section& section) {
    if (m_version < DRW::AC1021 || section.hasDataStorageBinaryData())
        return false;

    DRW_Section object = section;
    if ((object.m_kind != DRW_Section::Manager
         && object.m_kind != DRW_Section::Settings)
        || !prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerSectionObjectClass(object.m_kind, object.handle))
        return false;
    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum(
                object.m_kind == DRW_Section::Manager
                    ? "AcDbSectionManager" : "AcDbSectionSettings",
                object.m_kind == DRW_Section::Manager
                    ? "SECTION_MANAGER" : "SECTION_SETTINGS",
                object.m_kind == DRW_Section::Manager
                    ? DRW_Section::kDwgClassNumManager
                    : DRW_Section::kDwgClassNumSettings),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeBackground(const DRW_Background& background) {
    const int kind = static_cast<int>(background.m_kind);
    if (m_version < DRW::AC1015
        || kind < static_cast<int>(DRW_Background::Solid)
        || kind > static_cast<int>(DRW_Background::Skylight)
        || background.reactorHandles.size() > dwgSafety::MaxReactorCount
        || background.extensionDictionaryFlag() > 1
        || background.hasDataStorageBinaryData())
        return false;

    DRW_Background object = background;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerBackgroundObjectClass(object.m_kind, object.handle))
        return false;

    const char* className = "AcDbSolidBackground";
    const char* recordName = "SOLIDBACKGROUND";
    std::uint16_t fallbackClassNum = DRW_Background::kDwgClassNumSolid;
    switch (object.m_kind) {
    case DRW_Background::Gradient:
        className = "AcDbGradientBackground";
        recordName = "GRADIENTBACKGROUND";
        fallbackClassNum = DRW_Background::kDwgClassNumGradient;
        break;
    case DRW_Background::GroundPlane:
        className = "AcDbGroundPlaneBackground";
        recordName = "GROUNDPLANEBACKGROUND";
        fallbackClassNum = DRW_Background::kDwgClassNumGroundPlane;
        break;
    case DRW_Background::Image:
        className = "AcDbImageBackground";
        recordName = "IMAGEBACKGROUND";
        fallbackClassNum = DRW_Background::kDwgClassNumImage;
        break;
    case DRW_Background::Ibl:
        className = "AcDbIBLBackground";
        recordName = "IBLBACKGROUND";
        fallbackClassNum = DRW_Background::kDwgClassNumIbl;
        break;
    case DRW_Background::Skylight:
        className = "AcDbSkyBackground";
        recordName = "SKYLIGHTBACKGROUND";
        fallbackClassNum = DRW_Background::kDwgClassNumSkylight;
        break;
    case DRW_Background::Solid:
        break;
    }

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum(className, recordName, fallbackClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeSunStudy(const DRW_SunStudy& study) {
    if (m_version < DRW::AC1015 || study.hasDataStorageBinaryData()
        || !study.hasCompleteDxfArrays())
        return false;

    DRW_SunStudy object = study;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerSunStudyObjectClass(object.handle))
        return false;

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbSunStudy", "SUNSTUDY",
                             DRW_SunStudy::kDwgClassNum),
            object.handle,
            m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeMotionPath(const DRW_MotionPath& path) {
    if (m_version < DRW::AC1015
        || path.reactorHandles.size() > dwgSafety::MaxReactorCount
        || path.extensionDictionaryFlag() > 1
        || path.hasDataStorageBinaryData())
        return false;

    DRW_MotionPath object = path;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerMotionPathObjectClass(object.handle))
        return false;

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbMotionPath", "MOTIONPATH",
                             DRW_MotionPath::kDwgClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeCurvePath(const DRW_CurvePath& path) {
    if (m_version < DRW::AC1015
        || path.reactorHandles.size() > dwgSafety::MaxReactorCount
        || path.extensionDictionaryFlag() > 1
        || path.hasDataStorageBinaryData())
        return false;

    DRW_CurvePath object = path;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerCurvePathObjectClass(object.handle))
        return false;

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbCurvePath", "CURVEPATH",
                             DRW_CurvePath::kDwgClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writePointPath(const DRW_PointPath& path) {
    if (m_version < DRW::AC1015
        || path.reactorHandles.size() > dwgSafety::MaxReactorCount
        || path.extensionDictionaryFlag() > 1
        || path.hasDataStorageBinaryData())
        return false;

    DRW_PointPath object = path;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerPointPathObjectClass(object.handle))
        return false;

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbPointPath", "POINTPATH",
                             DRW_PointPath::kDwgClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeObjectPtr(const DRW_ObjectPtr& objectPtr) {
    if (m_version < DRW::AC1015 || objectPtr.hasDataStorageBinaryData())
        return false;

    DRW_ObjectPtr object = objectPtr;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerObjectPtrObjectClass(object.handle))
        return false;

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbObjectPtr", "OBJECT_PTR",
                             DRW_ObjectPtr::kDwgClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writePartialViewingIndex(
    const DRW_PartialViewingIndex& index) {
    if (m_version < DRW::AC1015 || index.hasDataStorageBinaryData())
        return false;

    DRW_PartialViewingIndex object = index;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerPartialViewingIndexObjectClass(object.handle))
        return false;

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbPartialViewingIndex", "PARTIAL_VIEWING_INDEX",
                             DRW_PartialViewingIndex::kDwgClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeRenderSettings(const DRW_RenderSettings& settings) {
    if (m_version < DRW::AC1015 || settings.hasDataStorageBinaryData())
        return false;

    DRW_RenderSettings object = settings;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerRenderSettingsObjectClass(object.m_kind, object.handle))
        return false;

    const char *className = nullptr;
    const char *recordName = nullptr;
    std::uint16_t fallbackClassNum = 0;
    switch (object.m_kind) {
    case DRW_RenderSettings::Settings:
        className = "AcDbRenderSettings";
        recordName = "RENDERSETTINGS";
        fallbackClassNum = DRW_RenderSettings::kDwgClassNumSettings;
        break;
    case DRW_RenderSettings::MentalRay:
        className = "AcDbMentalRayRenderSettings";
        recordName = "MENTALRAYRENDERSETTINGS";
        fallbackClassNum = DRW_RenderSettings::kDwgClassNumMentalRay;
        break;
    case DRW_RenderSettings::RapidRT:
        className = "AcDbRapidRTRenderSettings";
        recordName = "RAPIDRTRENDERSETTINGS";
        fallbackClassNum = DRW_RenderSettings::kDwgClassNumRapidRT;
        break;
    case DRW_RenderSettings::Entry:
        className = "AcDbRenderEntry";
        recordName = "RENDERENTRY";
        fallbackClassNum = DRW_RenderSettings::kDwgClassNumEntry;
        break;
    case DRW_RenderSettings::Environment:
        className = "AcDbRenderEnvironment";
        recordName = "RENDERENVIRONMENT";
        fallbackClassNum = DRW_RenderSettings::kDwgClassNumEnvironment;
        break;
    case DRW_RenderSettings::Global:
        className = "AcDbRenderGlobal";
        recordName = "RENDERGLOBAL";
        fallbackClassNum = DRW_RenderSettings::kDwgClassNumGlobal;
        break;
    default:
        return false;
    }

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum(className, recordName, fallbackClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeVisualStyle(const DRW_VisualStyle& style) {
    if (m_version < DRW::AC1015 || style.hasDataStorageBinaryData())
        return false;

    DRW_VisualStyle object = style;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerVisualStyleObjectClass(object.handle))
        return false;

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbVisualStyle", "VISUALSTYLE",
                             DRW_VisualStyle::kDwgClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

// --- add*() methods: register user table records for deferred emission -----

std::uint32_t dwgWriter15::addLType(const DRW_LType& lt) {
    if (m_dwgTableRecordsComplete)
        return 0;
    std::string upper = toUpperCase(lt.name);
    if (upper.empty())
        return 0;
    const auto existing = m_writingCtx.ltypeMap.find(upper);
    if (existing != m_writingCtx.ltypeMap.end()) {
        if (existing->second == reservedHandle::LTYPE_BYBLOCK) {
            m_ltypeByBlock = lt;
            m_haveLtypeByBlock = true;
        } else if (existing->second == reservedHandle::LTYPE_BYLAYER) {
            m_ltypeByLayer = lt;
            m_haveLtypeByLayer = true;
        } else if (existing->second == reservedHandle::LTYPE_CONTINUOUS) {
            m_ltypeContinuous = lt;
            m_haveLtypeContinuous = true;
        }
        return existing->second;
    }
    std::uint32_t h = m_handles.next();
    m_writingCtx.ltypeMap[upper] = h;
    m_pendingLTypes.emplace_back(h, lt);
    return h;
}

bool dwgWriter15::freezeCanonicalLayerNameMode() noexcept {
    std::uint32_t flags = 0u;
    if (m_header == nullptr || !m_header->encodeDwgHeaderFlags(flags)) return false;
    m_canonicalLayerExtendedNames = (flags & 0x0800u) != 0u;
    m_canonicalLayerNameModeFrozen = true;
    return true;
}

bool dwgWriter15::canonicalLayerNameValid(const std::string& name) const noexcept {
    if (!m_canonicalLayerNameModeFrozen) return false;
    const std::size_t maximum = m_canonicalLayerExtendedNames ? 255u : 31u;
    if (name.empty() || name.size() > maximum || name.front() == ' '
        || name.back() == ' ') return false;
    for (const unsigned char c : name) {
        const bool alphanumeric = (c >= 'A' && c <= 'Z')
            || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
        if (c >= 'a' && c <= 'z'
            && std::toupper(c) != static_cast<int>(c - 'a' + 'A')) return false;
        if (alphanumeric || c == '_' || c == '-' || c == '$') continue;
        if (!m_canonicalLayerExtendedNames || c < 0x20u || c > 0x7eu
            || std::strchr("<>/\\\":;?*|=',`", c) != nullptr) return false;
    }
    return true;
}

bool dwgWriter15::canonicalLayerNameAvailable(const std::string& name) const {
    const std::string upper = toUpperCase(name);
    if (upper == "0") return !m_haveLayer0;
    return m_writingCtx.layerMap.find(upper) == m_writingCtx.layerMap.end();
}

bool dwgWriter15::bindCanonicalLayer(std::uint32_t layer, std::uint32_t plot) {
    if ((m_version != DRW::AC1021 && m_version != DRW::AC1024
         && m_version != DRW::AC1027) || m_dwgTableRecordsComplete
        || layer == 0u || layer == DRW::NoHandle || plot == 0u || plot == DRW::NoHandle
        || (m_canonicalPlotTarget != 0u && m_canonicalPlotTarget != plot)
        || m_canonicalLayerHandles.size() >= dwgSafety::MaxOwnedObjectCount)
        return false;
    const auto pos = std::lower_bound(m_canonicalLayerHandles.begin(),
                                      m_canonicalLayerHandles.end(), layer);
    if (pos != m_canonicalLayerHandles.end() && *pos == layer) return false;
    try {
        m_canonicalLayerHandles.insert(pos, layer);
        m_canonicalPlotTarget = plot;
        return true;
    } catch (...) { return false; }
}

std::uint32_t dwgWriter15::addLayer(const DRW_Layer& lay) {
    if (m_dwgTableRecordsComplete)
        return 0;
    std::string upper = toUpperCase(lay.name);
    if (upper == "0") {
        // "0" is the reserved layer at fixed handle 0x12; keep its real data
        // (plot flag, color, linetype, ...) so it is emitted faithfully rather
        // than as a default stub. Do not add it to m_pendingLayers (avoids a
        // duplicate record at a second handle).
        m_layer0 = lay;
        m_haveLayer0 = true;
        return DRW::DwgLayer0Handle;
    }
    if (upper.empty())
        return 0;
    if (const auto existing = m_writingCtx.layerMap.find(upper);
        existing != m_writingCtx.layerMap.end())
        return existing->second;
    std::uint32_t h = m_handles.next();
    m_writingCtx.layerMap[upper] = h;
    m_pendingLayers.emplace_back(h, lay);
    return h;
}

std::uint32_t dwgWriter15::addTextstyle(const DRW_Textstyle& ts) {
    if (m_dwgTableRecordsComplete)
        return 0;
    std::string upper = toUpperCase(ts.name);
    if (upper.empty())
        return 0;
    const auto existing = m_writingCtx.styleMap.find(upper);
    if (existing != m_writingCtx.styleMap.end()) {
        if (existing->second == DRW::DwgStandardTextStyleHandle) {
            m_standardStyle = ts;
            m_haveStandardStyle = true;
        }
        return existing->second;
    }
    std::uint32_t h = m_handles.next();
    m_writingCtx.styleMap[upper] = h;
    m_pendingStyles.emplace_back(h, ts);
    return h;
}

bool dwgWriter15::addUcs(DRW_UCS& ucs) {
    if (m_dwgTableRecordsComplete)
        return false;
    std::string upper = toUpperCase(ucs.name);
    if (upper.empty())
        return false;
    const auto existing = m_writingCtx.ucsMap.find(upper);
    if (existing != m_writingCtx.ucsMap.end()) {
        ucs.handle = existing->second;
        return true;
    }
    std::uint32_t h = m_handles.next();
    ucs.handle = h;
    m_writingCtx.ucsMap[upper] = h;
    m_pendingUcs.emplace_back(h, ucs);
    return true;
}

bool dwgWriter15::addView(DRW_View& view) {
    if (m_dwgTableRecordsComplete)
        return false;
    std::string upper = toUpperCase(view.name);
    if (upper.empty())
        return false;
    const auto existing = m_writingCtx.viewMap.find(upper);
    if (existing != m_writingCtx.viewMap.end()) {
        view.handle = existing->second;
        return true;
    }
    // Duplicate names are discarded above; check selected records before
    // changing the caller's handle or staging the base-type copy.
    if (!drwHasEncodableDwgViewTwist(view)) {
        markObjectWriteFailure();
        return false;
    }
    std::uint32_t h = m_handles.next();
    view.handle = h;
    m_writingCtx.viewMap[upper] = h;
    m_pendingViews.emplace_back(h, view);
    return true;
}

bool dwgWriter15::addVport(DRW_Vport& vp) {
    if (m_dwgTableRecordsComplete)
        return false;
    std::string upper = toUpperCase(vp.name);
    if (upper.empty())
        return false;

    const auto existing = m_writingCtx.vportMap.find(upper);
    // Non-active duplicates are ignored by the existing admission contract.
    // Validate only records that will actually be emitted, before changing the
    // caller's handle/owner; addVport stores a base copy even for subclasses.
    if ((existing == m_writingCtx.vportMap.end()
         || existing->second == DRW_Vport::kDwgActiveHandle)
        && !drwHasEncodableDwgVportAngles(vp)) {
        markObjectWriteFailure();
        return false;
    }
    // Every VPORT table record is owned by the fixed VPORT control object;
    // source-file ownership is not part of the output namespace.
    vp.parentHandle = DRW_Vport::kDwgControlHandle;
    if (existing != m_writingCtx.vportMap.end()) {
        vp.handle = existing->second;
        if (existing->second == DRW_Vport::kDwgActiveHandle) {
            m_activeVport = vp;
            m_haveActiveVport = true;
        }
        return true;
    }
    std::uint32_t h = m_handles.next();
    vp.handle = h;
    m_writingCtx.vportMap[upper] = h;
    m_pendingVports.emplace_back(h, vp);
    return true;
}

bool dwgWriter15::addViewportEntityHeader(
    DRW_ViewportEntityHeader& header) {
    if (m_version < DRW::AC1015 || m_dwgTableRecordsComplete
        || (m_version > DRW::AC1024
            && header.hasDataStorageBinaryData()))
        return false;

    if (header.reactorHandles.size() > dwgSafety::MaxReactorCount
        || header.extensionDictionaryFlag() > 1) {
        return false;
    }
    if (header.handle == 0)
        header.handle = m_handles.next();
    else
        m_handles.reserve(header.handle);

    if (!ensurePendingViewportEntityHeaderIndex())
        return false;
    if (m_pendingViewportEntityHeaderHandles.find(header.handle)
        != m_pendingViewportEntityHeaderHandles.end()) {
        return true;
    }

    const auto inserted =
        m_pendingViewportEntityHeaderHandles.insert(header.handle);
    try {
        m_pendingViewportEntityHeaders.emplace_back(header.handle,
                                                    header);
    } catch (...) {
        m_pendingViewportEntityHeaderHandles.erase(inserted.first);
        throw;
    }
    return true;
}

std::uint32_t dwgWriter15::addDimstyle(const DRW_Dimstyle& ds) {
    if (m_dwgTableRecordsComplete)
        return 0;
    std::string upper = toUpperCase(ds.name);
    if (upper.empty())
        return 0;
    if (const auto existing = m_writingCtx.dimstyleMap.find(upper);
        existing != m_writingCtx.dimstyleMap.end()) {
        if (existing->second == DRW::DwgStandardDimstyleHandle) {
            m_standardDimstyle = ds;
            m_haveStandardDimstyle = true;
        }
        return existing->second;
    }
    std::uint32_t h = m_handles.next();
    m_writingCtx.dimstyleMap[upper] = h;
    m_pendingDimstyles.emplace_back(h, ds);
    return h;
}

std::uint32_t dwgWriter15::addAppId(const DRW_AppId& ai) {
    if (m_dwgTableRecordsComplete)
        return 0;
    std::string upper = toUpperCase(ai.name);
    if (upper.empty())
        return 0;
    if (const auto existing = m_writingCtx.appidMap.find(upper);
        existing != m_writingCtx.appidMap.end())
        return existing->second;
    std::uint32_t h = m_handles.next();
    m_writingCtx.appidMap[upper] = h;
    m_pendingAppIds.emplace_back(h, ai);
    return h;
}

std::uint32_t dwgWriter15::findAppIdHandle(const std::string& name) const {
    if (name.empty())
        return 0;
    const auto appId = m_writingCtx.appidMap.find(toUpperCase(name));
    return appId == m_writingCtx.appidMap.end() ? 0u : appId->second;
}

bool dwgWriter15::collectPendingTableEedAppIds() {
    const auto collect = [this](const DRW_TableEntry& entry) {
        for (const DRW_Variant* value : entry.extData) {
            if (value == nullptr)
                return false;
            if (value->code() != 1001)
                continue;
            if (value->type() != DRW_Variant::STRING
                || value->content.s == nullptr || value->content.s->empty()) {
                return false;
            }
            DRW_AppId appId;
            appId.name = *value->content.s;
            if (addAppId(appId) == 0)
                return false;
        }
        return true;
    };
    const auto collectAll = [&collect](const auto& entries) {
        for (const auto& entry : entries) {
            if (!collect(entry.second))
                return false;
        }
        return true;
    };

    return (!m_haveLayer0 || collect(m_layer0))
        && (!m_haveLtypeByBlock || collect(m_ltypeByBlock))
        && (!m_haveLtypeByLayer || collect(m_ltypeByLayer))
        && (!m_haveLtypeContinuous || collect(m_ltypeContinuous))
        && (!m_haveStandardStyle || collect(m_standardStyle))
        && (!m_haveActiveVport || collect(m_activeVport))
        && (!m_haveStandardDimstyle || collect(m_standardDimstyle))
        && collectAll(m_pendingLTypes)
        && collectAll(m_pendingLayers)
        && collectAll(m_pendingStyles)
        && collectAll(m_pendingUcs)
        && collectAll(m_pendingViews)
        && collectAll(m_pendingVports)
        && collectAll(m_pendingViewportEntityHeaders)
        && collectAll(m_pendingDimstyles)
        && [&]() {
            const std::size_t appIdCount = m_pendingAppIds.size();
            for (std::size_t index = 0; index < appIdCount; ++index) {
                const DRW_AppId appId = m_pendingAppIds[index].second;
                if (!collect(appId))
                    return false;
            }
            return true;
        }();
}

bool dwgWriter15::prepareEntityEed(DRW_Entity& entity) {
    entity.dwgEedCodePage = fileCodePageId();
    return prepareDwgEedRefs(m_version, entity.extData, m_writingCtx,
                              entity.dwgEedAppIdWriteRefs,
                              entity.dwgEedLayerWriteRefs);
}

bool dwgWriter15::prepareTableEntryEed(
    const DRW_TableEntry& entry,
    std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    std::vector<DRW_Entity::PendingHandleRef>& layerRefs) const {
    appIdRefs.clear();
    layerRefs.clear();
    if (!entry.pendingAppIdResolutions.empty()
        || !entry.pendingLayerRefResolutions.empty()
        || !hasOnlyRepresentedTableAppData(entry))
        return false;
    return prepareDwgEedRefs(m_version, entry.extData, m_writingCtx, appIdRefs,
                              layerRefs);
}

void dwgWriter15::prepareBlockOwnedEntity(DRW_Entity& entity) {
    if (auto* insert = dynamic_cast<DRW_Insert*>(&entity)) {
        if (insert->blockRecH.ref == 0 && !insert->name.empty()) {
            const auto block = m_userBlockHandles.find(toUpperCase(insert->name));
            if (block != m_userBlockHandles.end())
                insert->blockRecH.ref = block->second;
        }
    }
    if (entity.parentHandle == DRW::NoHandle) {
        if (m_activeUserBlockRecordHandle != 0)
            entity.parentHandle = m_activeUserBlockRecordHandle;
    } else if (m_activeUserBlockRecordHandle == 0
               && (entity.parentHandle == reservedHandle::BLOCK_MODEL_SPACE
                   || entity.parentHandle == reservedHandle::BLOCK_PAPER_SPACE)) {
        // Modelspace/paperspace entities are indexed by their BLOCK_RECORD,
        // but their common entity header uses ownerless entmode 2/1.
        entity.parentHandle = DRW::NoHandle;
    }
}

bool dwgWriter15::canRecordBlockOwnedEntity(const DRW_Entity& entity) const {
    if (entity.parentHandle == DRW::NoHandle)
        return true;

    const bool isCompoundChild = isCompoundEntityChild(entity);
    if (isCompoundChild && m_activeUserBlockRecordHandle != 0)
        return true;

    const PendingUserBlock* owner = findUserBlock(entity.parentHandle);
    if (owner == nullptr) {
        return isCompoundChild;
    }
    return m_activeUserBlockRecordHandle == 0
        || m_activeUserBlockRecordHandle == entity.parentHandle;
}

bool dwgWriter15::recordBlockOwnedEntity(const DRW_Entity& entity,
                                        std::uint32_t *ownerHandleOut) {
    if (m_blockOwnedEntityHandles.size()
        >= dwgSafety::MaxDwgReadObjectCount)
        return false;
    std::vector<std::uint32_t>* handles = nullptr;
    std::uint32_t ownerHandle = DRW::NoHandle;
    const bool isCompoundChild = isCompoundEntityChild(entity);
    if (isCompoundChild && m_activeUserBlockRecordHandle != 0) {
        PendingUserBlock* owner = findUserBlock(
            m_activeUserBlockRecordHandle);
        if (owner == nullptr)
            return false;
        handles = &owner->entityHandles;
        ownerHandle = owner->blockRecordHandle;
    } else if (entity.parentHandle == DRW::NoHandle
               || (isCompoundChild
                   && findUserBlock(entity.parentHandle) == nullptr)) {
        handles = entity.space == DRW::PaperSpace
            ? &m_paperSpaceEntityHandles : &m_modelSpaceEntityHandles;
        ownerHandle = entity.space == DRW::PaperSpace
            ? reservedHandle::BLOCK_PAPER_SPACE
            : reservedHandle::BLOCK_MODEL_SPACE;
    } else {
        PendingUserBlock* owner = findUserBlock(entity.parentHandle);
        if (owner == nullptr
            || (m_activeUserBlockRecordHandle != 0
                && m_activeUserBlockRecordHandle != entity.parentHandle))
            return false;
        handles = &owner->entityHandles;
        ownerHandle = owner->blockRecordHandle;
    }

    const auto inserted = m_blockOwnedEntityHandles.insert(
        std::make_pair(ownerHandle, entity.handle));
    if (!inserted.second)
        return false;
    try {
        handles->push_back(entity.handle);
    } catch (...) {
        m_blockOwnedEntityHandles.erase(inserted.first);
        throw;
    }
    if (ownerHandleOut != nullptr)
        *ownerHandleOut = ownerHandle;
    m_dwgGroupMemberTargetHandles.clear();
    m_dwgGroupMemberTargetIndexReady = false;
    return true;
}

bool dwgWriter15::recordBlockInsertReference(const DRW_Entity& entity) {
    if (m_version <= DRW::AC1014)
        return true;

    const auto* insert = dynamic_cast<const DRW_Insert*>(&entity);
    if (insert == nullptr)
        return true;
    if (insert->blockRecH.ref == DRW::NoHandle)
        return false;

    std::vector<std::uint32_t>* handles = nullptr;
    std::uint32_t ownerHandle = insert->blockRecH.ref;
    if (insert->blockRecH.ref == reservedHandle::BLOCK_MODEL_SPACE) {
        handles = &m_modelSpaceInsertHandles;
    } else if (insert->blockRecH.ref == reservedHandle::BLOCK_PAPER_SPACE) {
        handles = &m_paperSpaceInsertHandles;
    } else {
        PendingUserBlock* owner = findUserBlock(insert->blockRecH.ref);
        if (owner == nullptr)
            return false;
        handles = &owner->insertHandles;
    }

    if (handles->size() >= dwgSafety::MaxOwnedObjectCount
        || m_blockInsertReferenceHandles.size()
               >= dwgSafety::MaxDwgReadObjectCount)
        return false;
    const auto inserted = m_blockInsertReferenceHandles.insert(
        blockInsertReferenceKey(ownerHandle, entity.handle));
    if (!inserted.second)
        return false;
    try {
        handles->push_back(entity.handle);
    } catch (...) {
        m_blockInsertReferenceHandles.erase(inserted.first);
        throw;
    }
    return true;
}

bool dwgWriter15::canRecordRawBlockOwnedEntity(
    const DRW_UnsupportedObject& object) const {
    if (!isReplayableCustomRawEntity(object)
        && !isReplayableFixedModelerBodyRawEntity(object))
        return false;
    if (m_blockOwnedEntityHandles.size()
        >= dwgSafety::MaxDwgReadObjectCount)
        return false;
    if (object.m_blockOwnerHandle != DRW::NoHandle
        && object.m_parentHandle == DRW::NoHandle
        && object.m_blockOwnerHandle != reservedHandle::BLOCK_MODEL_SPACE
        && object.m_blockOwnerHandle != reservedHandle::BLOCK_PAPER_SPACE)
        return false;
    const std::uint32_t blockOwnerHandle = rawBlockOwnerHandle(object);
    if (blockOwnerHandle == reservedHandle::BLOCK_MODEL_SPACE
        || blockOwnerHandle == reservedHandle::BLOCK_PAPER_SPACE) {
        return m_blockOwnedEntityHandles.find(
            std::make_pair(blockOwnerHandle, object.m_handle))
            == m_blockOwnedEntityHandles.end();
    }
    if (blockOwnerHandle == DRW::NoHandle)
        return m_blockOwnedEntityHandles.find(std::make_pair(
            reservedHandle::BLOCK_MODEL_SPACE, object.m_handle))
            == m_blockOwnedEntityHandles.end();
    if (findUserBlock(blockOwnerHandle) == nullptr)
        return false;
    return m_blockOwnedEntityHandles.find(
        std::make_pair(blockOwnerHandle, object.m_handle))
        == m_blockOwnedEntityHandles.end();
}

bool dwgWriter15::recordRawBlockOwnedEntity(
    const DRW_UnsupportedObject& object) {
    if (!canRecordRawBlockOwnedEntity(object))
        return false;
    const std::uint32_t blockOwnerHandle = rawBlockOwnerHandle(object);
    if (blockOwnerHandle == reservedHandle::BLOCK_MODEL_SPACE
        || blockOwnerHandle == reservedHandle::BLOCK_PAPER_SPACE
        || blockOwnerHandle == DRW::NoHandle) {
        const std::uint32_t ownerHandle =
            blockOwnerHandle == reservedHandle::BLOCK_PAPER_SPACE
                ? reservedHandle::BLOCK_PAPER_SPACE
                : reservedHandle::BLOCK_MODEL_SPACE;
        auto& handles = blockOwnerHandle == reservedHandle::BLOCK_PAPER_SPACE
            ? m_paperSpaceEntityHandles : m_modelSpaceEntityHandles;
        const auto inserted = m_blockOwnedEntityHandles.insert(
            std::make_pair(ownerHandle, object.m_handle));
        if (!inserted.second)
            return false;
        try {
            handles.push_back(object.m_handle);
        } catch (...) {
            m_blockOwnedEntityHandles.erase(inserted.first);
            throw;
        }
        return true;
    }
    PendingUserBlock* owner = findUserBlock(blockOwnerHandle);
    if (owner == nullptr)
        return false;
    const auto inserted = m_blockOwnedEntityHandles.insert(
        std::make_pair(blockOwnerHandle, object.m_handle));
    if (!inserted.second)
        return false;
    try {
        owner->entityHandles.push_back(object.m_handle);
    } catch (...) {
        m_blockOwnedEntityHandles.erase(inserted.first);
        throw;
    }
    return true;
}

bool dwgWriter15::encodeEntity(DRW_Entity *ent) {

    const bool modernPolylineChild = m_version == DRW::AC1018 && ent != nullptr
        && m_ac1021PolylineChildPermit == ent
        && (dynamic_cast<DRW_Vertex*>(ent) != nullptr
            || dynamic_cast<DRW_SeqEnd*>(ent) != nullptr);
    if (modernPolylineChild)
        m_ac1021PolylineChildPermit = nullptr;
    if (m_legacyModelChainActive && ent != nullptr && m_blockControlEmitted) {
        m_legacyModelChainConflict = true;
        return false;
    }
    if (ent == nullptr || objectWriteFailed() || m_blockControlEmitted)
        return false;
    std::uint32_t legacyPrevious = 0, legacyNext = 0;
    const auto polylineChild = m_legacyPolylineChildOwners.find(ent->handle);
    const auto insertChild = m_legacyInsertChildOwners.find(ent->handle);
    const bool isPolylineChild = m_legacyModelChainActive
        && polylineChild != m_legacyPolylineChildOwners.end();
    const bool isInsertChild = m_legacyModelChainActive
        && insertChild != m_legacyInsertChildOwners.end();
    const bool plannedChild = isPolylineChild || isInsertChild;
    if (m_legacyModelChainActive) {
        if (m_activeUserBlockRecordHandle != 0) {
            const PendingUserBlock* block = findUserBlock(m_activeUserBlockRecordHandle);
            const std::size_t index = block != nullptr ? block->entityHandles.size() : 0u;
            const bool supported = (ent->eType == DRW::LINE && typeid(*ent) == typeid(DRW_Line))
                || (ent->eType == DRW::CIRCLE && typeid(*ent) == typeid(DRW_Circle))
                || (ent->eType == DRW::ARC && typeid(*ent) == typeid(DRW_Arc));
            if (m_legacyModelChainConflict || plannedChild || block == nullptr
                || m_activeUserBlockRecordHandle != m_legacyEmptyBlock.blockRecordHandle
                || index >= m_legacyBlockContent.size() || !supported
                || ent->handle != m_legacyBlockContent[index].handle
                || ent->eType != m_legacyBlockContent[index].type
                || (ent->parentHandle != DRW::NoHandle
                    && ent->parentHandle != m_activeUserBlockRecordHandle)
                || ent->space != DRW::ModelSpace || hasEmittedDwgObjectHandle(ent->handle)) {
                m_legacyModelChainConflict = true;
                return false;
            }
            if (index != 0u) legacyPrevious = m_legacyBlockContent[index - 1u].handle;
            if (index + 1u < m_legacyBlockContent.size()) legacyNext = m_legacyBlockContent[index + 1u].handle;
        } else if (plannedChild) {
            const auto& child = isPolylineChild ? polylineChild->second : insertChild->second;
            const auto parent = child.first;
            const auto* polyline = isPolylineChild ? legacyPolylinePlan(parent) : nullptr;
            const auto* insert = isInsertChild ? legacyInsertPlan(parent) : nullptr;
            const auto* handles = polyline ? &polyline->vertexHandles
                : insert ? &insert->attributeHandles : nullptr;
            const std::size_t index = child.second;
            const bool isEnd = handles != nullptr && index == handles->size();
            const bool supported = isEnd
                ? ent->eType == DRW::SEQEND && typeid(*ent) == typeid(DRW_SeqEnd)
                : isPolylineChild
                    ? ent->eType == DRW::VERTEX && typeid(*ent) == typeid(DRW_Vertex)
                    : ent->eType == DRW::ATTRIB && typeid(*ent) == typeid(DRW_Attrib);
            if (m_legacyModelChainConflict || (isPolylineChild ? !m_legacyPolylineWriting : !m_legacyInsertWriting)
                || handles == nullptr || index > handles->size() || !supported
                || ent->parentHandle != parent || ent->space != DRW::ModelSpace
                || m_modelSpaceEntityHandles.empty() || m_modelSpaceEntityHandles.back() != parent
                || !hasEmittedDwgObjectHandle(parent) || hasEmittedDwgObjectHandle(ent->handle)
                || (index != 0u && !hasEmittedDwgObjectHandle((*handles)[index - 1u]))) {
                m_legacyModelChainConflict = true;
                return false;
            }
            if (!isEnd) {
                if (index != 0u) legacyPrevious = (*handles)[index - 1u];
                if (index + 1u < handles->size()) legacyNext = (*handles)[index + 1u];
            }
        } else {
            const std::size_t index = m_modelSpaceEntityHandles.size();
            const PendingUserBlock* contentBlock = m_legacyBlockContent.empty()
                ? nullptr : findUserBlock(m_legacyEmptyBlock.blockRecordHandle);
            const bool supportedConcreteType =
                (ent->eType == DRW::LINE && typeid(*ent) == typeid(DRW_Line))
                || (ent->eType == DRW::CIRCLE && typeid(*ent) == typeid(DRW_Circle))
                || (ent->eType == DRW::ARC && typeid(*ent) == typeid(DRW_Arc))
                || (ent->eType == DRW::POLYLINE && typeid(*ent) == typeid(DRW_Polyline)
                    && m_legacyPolylineWriting && legacyPolylinePlan(ent->handle) != nullptr)
                || (ent->eType == DRW::INSERT && typeid(*ent) == typeid(DRW_Insert)
                    && m_legacyInsertWriting && legacyInsertPlan(ent->handle) != nullptr);
            const auto priorHandle = index == 0u || index > m_legacyModelChain.size()
                ? 0u : m_legacyModelChain[index - 1u].handle;
            const auto* priorPolyline = legacyPolylinePlan(priorHandle);
            const auto* priorInsert = legacyInsertPlan(priorHandle);
            if (m_legacyModelChainConflict
                || (!m_legacyBlockContent.empty()
                    && (contentBlock == nullptr || contentBlock->entityHandles.size() != m_legacyBlockContent.size()))
                || index >= m_legacyModelChain.size()
                || ent->handle != m_legacyModelChain[index].handle
                || ent->eType != m_legacyModelChain[index].type || !supportedConcreteType
                || (priorPolyline != nullptr && !hasEmittedDwgObjectHandle(priorPolyline->seqendHandle))
                || (priorInsert != nullptr && !hasEmittedDwgObjectHandle(priorInsert->seqendHandle))
                || ent->parentHandle != DRW::NoHandle || ent->space != DRW::ModelSpace) {
                m_legacyModelChainConflict = true;
                return false;
            }
            if (index != 0u) legacyPrevious = m_legacyModelChain[index - 1u].handle;
            if (index + 1u < m_legacyModelChain.size()) legacyNext = m_legacyModelChain[index + 1u].handle;
        }
    }

    // The case-folding allocation must happen before writer or caller state
    // changes so an allocation failure permits an unchanged retry.
    DRW_Text* resolvedText = nullptr;
    std::uint32_t resolvedTextStyle = 0u;
    if (ent != nullptr && ent->eType == DRW::TEXT
        && dynamic_cast<DRW_RText*>(ent) == nullptr
        && dynamic_cast<DRW_ArcAlignedText*>(ent) == nullptr
        && dynamic_cast<DRW_Attrib*>(ent) == nullptr
        && dynamic_cast<DRW_MText*>(ent) == nullptr) {
        auto* text = dynamic_cast<DRW_Text*>(ent);
        if (text != nullptr && text->styleH.ref == 0u && text->styleH.ref64 == 0u
            && !text->style.empty()) {
            const auto styleIt = m_writingCtx.styleMap.find(toUpperCase(text->style));
            if (styleIt != m_writingCtx.styleMap.end()) {
                resolvedText = text;
                resolvedTextStyle = styleIt->second;
            }
        }
    }

    m_lastEntityWriteCheckpointValid = false;
    m_lastEntityWriteUsesCompoundCheckpoint = false;
    m_lastEntityCompoundCheckpoint = {};
    EntityFrameCheckpoint checkpoint = checkpointEntityFrame();
    CompoundWriteCheckpoint classCheckpoint;
    bool hasClassCheckpoint = false;
    bool frameStarted = false;
    const auto fail = [this, &checkpoint, &classCheckpoint,
                       &hasClassCheckpoint, &frameStarted]() {
        if (hasClassCheckpoint)
            rollbackCompoundWrite(classCheckpoint);
        else
            rollbackEntityFrame(checkpoint, frameStarted);
        return false;
    };

    if (!prepareEntityEed(*ent)) {
        m_entityEedWriteFailure = !ent->extData.empty();
        return fail();
    }
    // Honor caller-set handles (round-trip preservation); allocate a
    // fresh one only when ent->handle is zero.  Either way, reserve
    // the handle in the allocator so a later next() can't return it
    // for a different entity.
    std::uint32_t handle = ent->handle;
    if (handle == 0) {
        handle = m_handles.next();
        ent->handle = handle;
    } else {
        m_handles.reserve(handle);
    }
    // Entity encoders normally write fixed object types.  If a caller has
    // explicitly associated this handle with a custom class instance, keep
    // the comprehensive rollback path for finishObject()'s class ledger.
    if (m_dwgClassInstanceHandleOwners.find(handle)
        != m_dwgClassInstanceHandleOwners.end()) {
        classCheckpoint = checkpointCompoundWrite();
        hasClassCheckpoint = true;
    }
    // Resolve layer name → handle.  Caller may have pre-set layerH.ref for
    // round-trip; only resolve from name when the handle is unset.
    if (ent->layerH.ref == 0) {
        auto layerUp = toUpperCase(ent->layer);
        auto it = m_writingCtx.layerMap.find(layerUp);
        ent->layerH.ref = (it != m_writingCtx.layerMap.end())
            ? it->second
            : DRW::DwgLayer0Handle;  // fallback: layer "0"
    }
    // BYLAYER/BYBLOCK/CONTINUOUS are common-header flags, not named LTYPE
    // references. Resolving them to table rows would turn the implicit state
    // into an explicit handle (and can change the encoded linetype semantics).
    const auto lineTypeName = toUpperCase(ent->lineType);
    const bool specialLineType = lineTypeName.empty()
        || lineTypeName == "BYLAYER" || lineTypeName == "BYBLOCK"
        || lineTypeName == "CONTINUOUS";
    if (ent->lTypeH.ref == 0 && !specialLineType) {
        auto ltUp = toUpperCase(ent->lineType);
        auto it = m_writingCtx.ltypeMap.find(ltUp);
        if (it != m_writingCtx.ltypeMap.end())
            ent->lTypeH.ref = it->second;
    }
    // Drive ltFlags from resolved name/handle so encodeDwgCommon emits the
    // correct BB and encodeDwgEntHandle emits lTypeH when needed.
    if (ent->ltFlags == 0) {
        const auto& ltUp = lineTypeName;
        if (ent->lTypeH.ref != 0
            && ltUp != "BYLAYER" && ltUp != "BYBLOCK" && ltUp != "CONTINUOUS"
            && !ltUp.empty())
            ent->ltFlags = 3;
        else if (ltUp == "BYBLOCK")
            ent->ltFlags = 1;
        else if (ltUp == "CONTINUOUS")
            ent->ltFlags = 2;
    }
    if (ent->eType == DRW::MLINE) {
        auto *mline = dynamic_cast<DRW_MLine *>(ent);
        if (mline != nullptr && mline->styleHandle == 0 && !mline->styleName.empty()) {
            auto styleIt = m_writingCtx.mlineStyleMap.find(toUpperCase(mline->styleName));
            if (styleIt != m_writingCtx.mlineStyleMap.end())
                mline->styleHandle = styleIt->second;
        }
    }
    // Resolve a TEXT style name only when the caller did not supply a
    // handle. The handle is drawing-local, so restore caller state on every
    // return, including exceptions and failed staged writes.
    struct ScopedTextStyleHandle {
        DRW_Text* text{nullptr};
        dwgHandle original{};
        ~ScopedTextStyleHandle() { if (text != nullptr) text->styleH = original; }
    } textStyleScope;
    if (resolvedText != nullptr) {
        textStyleScope.text = resolvedText;
        textStyleScope.original = resolvedText->styleH;
        resolvedText->styleH.ref = resolvedTextStyle;
    }
    prepareBlockOwnedEntity(*ent);
    // R2000+ INSERT records must identify an existing BLOCK_RECORD. The
    // legacy name-only representation is retained for R14 and earlier, but
    // silently emitting a zero target in modern DWG would orphan the INSERT.
    if (m_version > DRW::AC1014) {
        const auto* insert = dynamic_cast<const DRW_Insert*>(ent);
        if (insert != nullptr && insert->blockRecH.ref == DRW::NoHandle)
            return fail();
    }
    if (!canRecordBlockOwnedEntity(*ent))
        return fail();

    struct PlannedFrameScope {
        bool& flag;
        bool previous;
        PlannedFrameScope(bool& value, bool active) : flag(value), previous(value) { if (active) flag = true; }
        ~PlannedFrameScope() { flag = previous; }
    } plannedFrame(m_legacyPlannedFrameWriting, m_legacyModelChainActive);
    frameStarted = true;
    dwgBufferW& body = beginObject(handle);
    bool ok = false;
    if (m_version > DRW::AC1018) {
        m_objectStrings.reset();
        m_objectHandles.reset();
        ok = ent->encodeDwg(m_version, &body, /*bs=*/0,
                             &m_objectStrings, &m_objectHandles);
    } else if (m_version > DRW::AC1014) {
        m_objectHandles.reset();
        ScopedDwgLegacyEntityLinks links(
            m_legacyModelChainActive ? &body : nullptr,
            m_legacyModelChainActive ? &m_objectHandles : nullptr,
            legacyPrevious, legacyNext);
        ok = ent->encodeDwg(m_version, &body, /*bs=*/0,
                             nullptr, &m_objectHandles);
    } else {
        ok = ent->encodeDwg(m_version, &body, /*bs=*/0);
    }
    if (!ok) {
        if (!ent->extData.empty())
            m_entityEedWriteFailure = true;
        return fail();
    }
    finishObject();

    if (objectWriteFailed())
        return fail();

    if (!plannedChild && !modernPolylineChild && !recordBlockOwnedEntity(*ent, &checkpoint.blockOwnerHandle))
        return fail();
    checkpoint.entityHandle = ent->handle;
    checkpoint.blockOwnedEntityRecorded = !plannedChild && !modernPolylineChild;
    if (!recordBlockInsertReference(*ent))
        return fail();
    if (const auto* insert = dynamic_cast<const DRW_Insert*>(ent)) {
        checkpoint.insertOwnerHandle = insert->blockRecH.ref;
        checkpoint.blockInsertReferenceRecorded = m_version > DRW::AC1014;
    }

    m_lastEntityFrameCheckpoint = checkpoint;
    if (hasClassCheckpoint) {
        m_lastEntityCompoundCheckpoint = std::move(classCheckpoint);
        m_lastEntityWriteUsesCompoundCheckpoint = true;
    }
    m_lastEntityWriteCheckpointValid = true;
    return true;
}

bool dwgWriter15::rollbackLastEntityWrite() {
    if (!m_lastEntityWriteCheckpointValid)
        return false;
    if (m_lastEntityWriteUsesCompoundCheckpoint)
        rollbackCompoundWrite(m_lastEntityCompoundCheckpoint);
    else
        rollbackEntityFrame(m_lastEntityFrameCheckpoint, true);
    clearLastEntityWriteCheckpoint();
    return true;
}

bool dwgWriter15::writeDwgObjects() {
    if (m_dwgTableRecordsComplete)
        return true;

    struct DwgObjectsPhaseGuard final {
        dwgWriter15* writer;
        CompoundWriteCheckpoint checkpoint;
        bool committed {false};

        ~DwgObjectsPhaseGuard() {
            if (!committed)
                writer->rollbackCompoundWrite(checkpoint);
        }
    } phaseGuard{this, checkpointCompoundWrite()};

    m_emittedDwgTableRecordHandles.clear();
    m_emittedDwgTableControlHandles.clear();
    m_dwgTableRecordsComplete = false;
    m_emittedDwgNamedObjectDictionaryEntries.clear();
    m_dwgNamedObjectDictionaryComplete = false;
    m_emittedDwgGroupDictionaryEntries.clear();
    m_emittedDwgGroupFrames.clear();
    invalidateEmittedDwgGroupEntryIndex();
    invalidateEmittedDwgGroupFrameIndex();
    m_dwgGroupDictionaryComplete = false;
    m_emittedDwgBlockRecords.clear();
    m_emittedDwgBlockEntityHandles.clear();
    m_dwgBlockStructureComplete = false;
    m_dwgGroupMemberTargetHandles.clear();
    m_dwgGroupMemberTargetIndexReady = false;
    try {
        m_emittedDwgGroupFrames.reserve(
            m_pendingDwgGroupDictionaryEntries.size());
    } catch (const std::exception&) {
        return false;
    }

    // Ensure standard reserved entries are present.  The constructor
    // pre-seeds these; insert is a no-op if a key already exists, which
    // protects user entries added via add*() before this call.
    m_writingCtx.ltypeMap.insert({"CONTINUOUS", reservedHandle::LTYPE_CONTINUOUS});
    m_writingCtx.ltypeMap.insert({"BYLAYER",    reservedHandle::LTYPE_BYLAYER});
    m_writingCtx.ltypeMap.insert({"BYBLOCK",    reservedHandle::LTYPE_BYBLOCK});
    m_writingCtx.layerMap.insert({"0",        DRW::DwgLayer0Handle});
    m_writingCtx.styleMap.insert({"STANDARD", DRW::DwgStandardTextStyleHandle});
    // Named VIEW has no required standard record.
    m_writingCtx.appidMap.insert({"ACAD",     DRW::DwgAcadAppIdHandle});
    m_writingCtx.dimstyleMap.insert({"STANDARD", DRW::DwgStandardDimstyleHandle});
    m_writingCtx.vportMap.insert({"*ACTIVE", DRW::DwgActiveVportHandle});

    // --- LTYPE section --- (emitted before LAYER so ltypeMap is populated
    // when emitLayerRecord resolves lineType→handle)
    {
        std::vector<std::uint32_t> ltypeChildren = {
            reservedHandle::LTYPE_CONTINUOUS
        };
        for (auto& p : m_pendingLTypes) ltypeChildren.push_back(p.first);
        // ODA §20.4.57 excludes BYLAYER and BYBLOCK from Numentries and
        // places their hard-owner handles last, in BYLAYER then BYBLOCK order.
        ltypeChildren.push_back(reservedHandle::LTYPE_BYLAYER);
        ltypeChildren.push_back(reservedHandle::LTYPE_BYBLOCK);
        // numEntries excludes BYBLOCK/BYLAYER phantoms (reader adds +2)
        if (!emitControlObject(
                DRW::DwgLTypeControlObjectType, reservedHandle::LTYPE_CONTROL,
                1 + static_cast<std::uint32_t>(m_pendingLTypes.size()),
                ltypeChildren))
            return false;
        DRW_LType byBlock = m_haveLtypeByBlock ? m_ltypeByBlock : DRW_LType{};
        DRW_LType byLayer = m_haveLtypeByLayer ? m_ltypeByLayer : DRW_LType{};
        DRW_LType continuous = m_haveLtypeContinuous ? m_ltypeContinuous
                                                      : DRW_LType{};
        const auto setSyntheticLtypeCommon = [this](DRW_LType& value, bool supplied) {
            if (!supplied && m_version >= DRW::AC1018) {
                value.setDwgCommonObjectState(0, 1, false);
                value.xrefBlockHandle.code = DRW::DwgHardPointer;
            }
        };
        setSyntheticLtypeCommon(byBlock, m_haveLtypeByBlock);
        setSyntheticLtypeCommon(byLayer, m_haveLtypeByLayer);
        setSyntheticLtypeCommon(continuous, m_haveLtypeContinuous);
        byBlock.name = "BYBLOCK";
        byLayer.name = "BYLAYER";
        continuous.name = "CONTINUOUS";
        if (!emitLtypeRecord(reservedHandle::LTYPE_BYBLOCK, byBlock)
            || !emitLtypeRecord(reservedHandle::LTYPE_BYLAYER, byLayer)
            || !emitLtypeRecord(reservedHandle::LTYPE_CONTINUOUS, continuous)) {
            return false;
        }
        for (auto& p : m_pendingLTypes)
            if (!emitLtypeRecord(p.first, p.second))
                return false;
    }

    // --- LAYER section ---
    {
        std::vector<std::uint32_t> layerChildren = {DRW::DwgLayer0Handle};
        for (auto& p : m_pendingLayers) layerChildren.push_back(p.first);
        if (!emitControlObject(
                DRW::DwgLayerControlObjectType, reservedHandle::LAYER_CONTROL,
                1 + static_cast<std::uint32_t>(m_pendingLayers.size()),
                layerChildren))
            return false;
        if (m_haveLayer0) {
            if (m_version == DRW::AC1027 && defaultDwgMaterialGraphActive()
                && m_layer0.materialHandle.ref != m_defaultMaterialHandles[3])
                return false;
            if (!emitLayerRecord(DRW::DwgLayer0Handle, m_layer0))
                return false;
        } else {
            DRW_Layer layer0;
            if (m_version >= DRW::AC1018) {
                // A synthesized layer 0 has neither extension nor xref.
                // Keep a supplied layer's common state untouched.
                layer0.setDwgCommonObjectState(0, 1, false);
                layer0.xrefBlockHandle.code = DRW::DwgHardPointer;
            }
            layer0.name = "0";
            if (defaultDwgMaterialGraphActive())
                layer0.materialHandle.ref = m_defaultMaterialHandles[3];
            if (m_defaultPlotStyleDictionaryHandle != 0)
                layer0.plotStyleHandle.ref = m_defaultPlotStylePlaceholderHandle;
            if (!emitLayerRecord(DRW::DwgLayer0Handle, layer0))
                return false;
        }
        for (auto& p : m_pendingLayers)
            if (!emitLayerRecord(p.first, p.second))
                return false;
    }

    // --- STYLE section ---
    {
        std::vector<std::uint32_t> styleChildren = {
            DRW::DwgStandardTextStyleHandle};
        for (auto& p : m_pendingStyles) styleChildren.push_back(p.first);
        if (!emitControlObject(
                DRW::DwgStyleControlObjectType, reservedHandle::STYLE_CONTROL,
                1 + static_cast<std::uint32_t>(m_pendingStyles.size()),
                styleChildren))
            return false;
        DRW_Textstyle standardStyle = m_haveStandardStyle
            ? m_standardStyle : DRW_Textstyle{};
        standardStyle.name = "STANDARD";
        if (!m_haveStandardStyle && m_version >= DRW::AC1018) {
            // The built-in STYLE has no extension or xref. Preserve explicit
            // source/caller metadata when a STANDARD style was supplied.
            standardStyle.setDwgCommonObjectState(0, 1, false);
            standardStyle.xrefBlockHandle.code = DRW::DwgHardPointer;
        }
        if (!emitStyleRecord(DRW::DwgStandardTextStyleHandle, standardStyle))
            return false;
        for (auto& p : m_pendingStyles)
            if (!emitStyleRecord(p.first, p.second))
                return false;
    }

    // --- VIEW section --- (named views; no required standard record)
    {
        std::vector<std::uint32_t> viewChildren;
        for (auto& p : m_pendingViews) viewChildren.push_back(p.first);
        if (!emitControlObject(
                DRW::DwgViewControlObjectType, reservedHandle::VIEW_CONTROL,
                static_cast<std::uint32_t>(m_pendingViews.size()),
                viewChildren))
            return false;
        for (auto& p : m_pendingViews)
            if (!emitViewRecord(p.first, p.second))
                return false;
    }

    // --- UCS section --- (named UCS records; no required standard record)
    {
        std::vector<std::uint32_t> ucsChildren;
        for (auto& p : m_pendingUcs) ucsChildren.push_back(p.first);
        if (!emitControlObject(
                DRW::DwgUcsControlObjectType, reservedHandle::UCS_CONTROL,
                static_cast<std::uint32_t>(m_pendingUcs.size()),
                ucsChildren))
            return false;
        for (auto& p : m_pendingUcs)
            if (!emitUcsRecord(p.first, p.second))
                return false;
    }

    // --- VPORT section ---
    {
        std::vector<std::uint32_t> vportChildren = {
            DRW_Vport::kDwgActiveHandle};
        for (auto& p : m_pendingVports) vportChildren.push_back(p.first);
        if (!emitControlObject(
                DRW::DwgVPortControlObjectType, reservedHandle::VPORT_CONTROL,
                1 + static_cast<std::uint32_t>(m_pendingVports.size()),
                vportChildren))
            return false;
        DRW_Vport activeVport = m_haveActiveVport
            ? m_activeVport : DRW_Vport{};
        activeVport.name = "*ACTIVE";
        if (!m_haveActiveVport && m_version >= DRW::AC1018)
            activeVport.setDwgCommonObjectState(0, 1, false);
        if (!emitVportRecord(DRW_Vport::kDwgActiveHandle, activeVport))
            return false;
        for (auto& p : m_pendingVports)
            if (!emitVportRecord(p.first, p.second))
                return false;
    }

    // --- APPID section ---
    {
        std::vector<std::uint32_t> appidChildren = {DRW::DwgAcadAppIdHandle};
        for (auto& p : m_pendingAppIds) appidChildren.push_back(p.first);
        if (!emitControlObject(
                DRW::DwgAppIdControlObjectType, reservedHandle::APPID_CONTROL,
                1 + static_cast<std::uint32_t>(m_pendingAppIds.size()),
                appidChildren))
            return false;
        DRW_AppId acadAppId;
        if (m_version >= DRW::AC1018)
            acadAppId.setDwgCommonObjectState(0, 1, false);
        acadAppId.name = "ACAD";
        if (!emitAppIdRecord(DRW::DwgAcadAppIdHandle, acadAppId))
            return false;
        for (auto& p : m_pendingAppIds)
            if (!emitAppIdRecord(p.first, p.second))
                return false;
    }

    // --- DIMSTYLE section ---
    {
        std::vector<std::uint32_t> dimChildren = {
            DRW::DwgStandardDimstyleHandle};
        for (auto& p : m_pendingDimstyles) dimChildren.push_back(p.first);
        if (!emitControlObject(
                DRW::DwgDimStyleControlObjectType, reservedHandle::DIMSTYLE_CONTROL,
                1 + static_cast<std::uint32_t>(m_pendingDimstyles.size()),
                dimChildren))
            return false;
        DRW_Dimstyle standardDimstyle = m_haveStandardDimstyle
            ? m_standardDimstyle : DRW_Dimstyle{};
        standardDimstyle.name = "STANDARD";
        if (!m_haveStandardDimstyle && m_version >= DRW::AC1018)
            standardDimstyle.setDwgCommonObjectState(0, 1, false);
        standardDimstyle.syncStructToVars();
        if (!emitDimstyleRecord(DRW::DwgStandardDimstyleHandle,
                                standardDimstyle))
            return false;
        for (auto& p : m_pendingDimstyles)
            if (!emitDimstyleRecord(p.first, p.second))
                return false;
    }

    // VPORT_ENTITY_HEADER_CONTROL is a R2000-era table control.  R2004 and
    // later files omit it from both HEADER and OBJECTS, but fixed type-71
    // records remain valid table entries and are emitted below.
    if (m_version < DRW::AC1018
        && !hasRawObjectOverride(
            DRW_ViewportEntityHeader::kDwgControlType,
            DRW_ViewportEntityHeader::kDwgControlHandle)) {
        std::vector<std::uint32_t> viewportHeaderChildren;
        viewportHeaderChildren.reserve(m_pendingViewportEntityHeaders.size());
        for (const auto& entry : m_pendingViewportEntityHeaders)
            viewportHeaderChildren.push_back(entry.first);
        if (!emitControlObject(
                DRW_ViewportEntityHeader::kDwgControlType,
                reservedHandle::VPORT_ENTITY_HEADER_CONTROL,
                static_cast<std::uint32_t>(viewportHeaderChildren.size()),
                viewportHeaderChildren))
            return false;
    }
    for (const auto& entry : m_pendingViewportEntityHeaders)
        if (!emitViewportEntityHeaderRecord(entry.first, entry.second))
            return false;

    // Block entities + Block_Records for *Model_Space and *Paper_Space.
    if (!emitBlockEntity(reservedHandle::BLOCK_MODEL_ENTITY, "*Model_Space",
                         /*isEnd=*/false)
        || !emitBlockEntity(reservedHandle::BLOCK_MODEL_END, std::string{},
                            /*isEnd=*/true)
        || !emitBlockEntity(reservedHandle::BLOCK_PAPER_ENTITY, "*Paper_Space",
                            /*isEnd=*/false)
        || !emitBlockEntity(reservedHandle::BLOCK_PAPER_END, std::string{},
                            /*isEnd=*/true))
        return false;

    // The corresponding BLOCK_RECORD objects are deferred until
    // emitDeferredBlockControl(), after writeEntities() has populated their
    // owned-handle lists.

    DRW_Dictionary namedObjects;
    // These are newly constructed objects with no extension dictionary.
    // R2004+ omits the handle when its missing flag is set (ODA §20.4.44).
    if (m_version >= DRW::AC1018)
        namedObjects.setDwgCommonObjectState(0, 1, false);
    namedObjects.handle = DRW::DwgNamedObjectsDictionaryHandle;
    namedObjects.cloning = 1;
    namedObjects.hardOwner = 0;
    namedObjects.m_entries.push_back(
        {"ACAD_GROUP", DRW::DwgAcadGroupDictionaryHandle});
    for (const auto& entry : m_pendingDwgNamedObjectDictionaryEntries)
        namedObjects.m_entries.push_back({entry.name, entry.childHandle});

    DRW_Dictionary acadGroup;
    if (m_version >= DRW::AC1018)
        acadGroup.setDwgCommonObjectState(0, 1, false);
    acadGroup.handle = DRW::DwgAcadGroupDictionaryHandle;
    acadGroup.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
    acadGroup.cloning = 1;
    acadGroup.hardOwner = 0;
    acadGroup.m_entries.reserve(m_pendingDwgGroupDictionaryEntries.size());
    for (const auto& entry : m_pendingDwgGroupDictionaryEntries)
        acadGroup.m_entries.push_back({entry.name, entry.childHandle});
    const std::vector<DRW_Entity::PendingHandleRef> noEedRefs;
    if (!emitDictionaryObject(namedObjects.handle, namedObjects, noEedRefs,
                              noEedRefs, true)
        || !emitDictionaryObject(acadGroup.handle, acadGroup, noEedRefs,
                                 noEedRefs, true)) {
        return false;
    }
    m_emittedDwgNamedObjectDictionaryEntries.push_back(
        {"ACAD_GROUP", DRW::DwgAcadGroupDictionaryHandle});
    m_emittedDwgNamedObjectDictionaryEntries.insert(
        m_emittedDwgNamedObjectDictionaryEntries.end(),
        m_pendingDwgNamedObjectDictionaryEntries.cbegin(),
        m_pendingDwgNamedObjectDictionaryEntries.cend());
    m_dwgNamedObjectDictionaryComplete = true;
    m_emittedDwgGroupDictionaryEntries =
        m_pendingDwgGroupDictionaryEntries;
    invalidateEmittedDwgGroupEntryIndex();
    m_dwgGroupDictionaryComplete = true;

    m_dwgTableRecordsComplete = true;
    phaseGuard.committed = true;
    return true;
}

namespace {

/// Rebuild raw object body with a replacement OT when raw class nums remapped.
bool patchRawObjectType(const std::vector<std::uint8_t>& raw,
                        DRW::Version version,
                        std::uint16_t newType, std::vector<std::uint8_t>& out,
                        std::int64_t* bitDelta = nullptr) {
    if (raw.empty())
        return false;
    dwgBuffer reader(const_cast<std::uint8_t*>(raw.data()), raw.size());
    (void)reader.getObjType(version);
    if (!reader.isGood())
        return false;
    const std::uint64_t oldTypeBits =
        reader.getPosition() * 8u + reader.getBitPos();
    dwgBufferW typeWriter;
    typeWriter.putObjType(version, newType);
    if (!typeWriter.isGood())
        return false;
    if (bitDelta != nullptr) {
        const std::uint64_t newTypeBits = typeWriter.bitCount();
        if (newTypeBits > std::numeric_limits<std::int64_t>::max()
            || oldTypeBits > std::numeric_limits<std::int64_t>::max())
            return false;
        *bitDelta = static_cast<std::int64_t>(newTypeBits)
            - static_cast<std::int64_t>(oldTypeBits);
    }
    dwgBufferW rebuilt;
    rebuilt.putObjType(version, newType);
    // Copy remaining bits after the original OT.
    while (reader.numRemainingBytes() > 0 || reader.getBitPos() != 0) {
            // Prefer whole remaining bytes when aligned.
        if (reader.getBitPos() == 0 && reader.numRemainingBytes() > 0) {
            const size_t n = reader.numRemainingBytes();
            std::vector<std::uint8_t> tail;
            if (!DRW::resize(tail, static_cast<int>(n)))
                return false;
            if (!reader.getBytes(tail.data(), static_cast<int>(n)))
                return false;
            rebuilt.putBytes(tail.data(), n);
            break;
        }
        const std::uint8_t bit = reader.getBit();
        if (!reader.isGood())
            return false;
        rebuilt.putBit(bit);
    }
    rebuilt.alignToByte();
    out = rebuilt.data();
    return !out.empty();
}

bool readRawObjectHandle(const std::vector<std::uint8_t>& raw,
                         DRW::Version version, dwgHandle& handle) {
    if (raw.empty())
        return false;
    dwgBuffer reader(const_cast<std::uint8_t*>(raw.data()), raw.size());
    (void)reader.getObjType(version);
    if (version > DRW::AC1014 && version < DRW::AC1024)
        (void)reader.getRawLong32();
    if (!reader.isGood())
        return false;
    handle = reader.getHandle();
    return reader.isGood();
}

/// Replace the common object handle in a raw body while retaining every other
/// bit, including the padding after the meaningful R2007+ body bit count.
bool patchRawObjectHandle(const std::vector<std::uint8_t>& raw,
                          DRW::Version version, std::uint32_t newHandle,
                          std::vector<std::uint8_t>& out,
                          std::int64_t* bitDelta = nullptr) {
    if (raw.empty() || raw.size() > std::numeric_limits<std::uint64_t>::max() / 8u)
        return false;
    dwgBuffer reader(const_cast<std::uint8_t*>(raw.data()), raw.size());
    (void)reader.getObjType(version);
    if (version > DRW::AC1014 && version < DRW::AC1024)
        (void)reader.getRawLong32();
    if (!reader.isGood())
        return false;

    const std::uint64_t handleStart =
        reader.getPosition() * 8u + reader.getBitPos();
    const dwgHandle oldHandle = reader.getHandle();
    if (!reader.isGood())
        return false;
    const std::uint64_t handleEnd =
        reader.getPosition() * 8u + reader.getBitPos();
    const std::uint64_t totalBits = static_cast<std::uint64_t>(raw.size()) * 8u;
    if (handleEnd > totalBits)
        return false;

    dwgBufferW handleWriter;
    dwgHandle replacement = oldHandle;
    replacement.ref = newHandle;
    replacement.ref64 = newHandle;
    replacement.size = 0;
    handleWriter.putHandle(replacement);
    if (!handleWriter.isGood())
        return false;
    if (bitDelta != nullptr) {
        const std::uint64_t oldHandleBits = handleEnd - handleStart;
        const std::uint64_t newHandleBits = handleWriter.bitCount();
        if (newHandleBits > std::numeric_limits<std::int64_t>::max()
            || oldHandleBits > std::numeric_limits<std::int64_t>::max())
            return false;
        *bitDelta = static_cast<std::int64_t>(newHandleBits)
            - static_cast<std::int64_t>(oldHandleBits);
    }

    dwgBuffer prefixReader(const_cast<std::uint8_t*>(raw.data()), raw.size());
    dwgBufferW rebuilt;
    for (std::uint64_t i = 0; i < handleStart; ++i) {
        const std::uint8_t bit = prefixReader.getBit();
        if (!prefixReader.isGood())
            return false;
        rebuilt.putBit(bit);
    }

    rebuilt.putHandle(replacement);

    for (std::uint64_t i = handleEnd; i < totalBits; ++i) {
        const std::uint8_t bit = reader.getBit();
        if (!reader.isGood())
            return false;
        rebuilt.putBit(bit);
    }
    rebuilt.alignToByte();
    out = rebuilt.data();
    return !out.empty();
}

struct RawEntityOwnerProbe final : DRW_Entity {
    void applyExtrusion() override {}

    bool parseDwg(DRW::Version version, dwgBuffer *buffer,
                  std::uint32_t bodyBitSize = 0) override {
        return DRW_Entity::parseDwg(version, buffer, nullptr, bodyBitSize)
            && buffer != nullptr && buffer->isGood();
    }

    bool hasOwnerHandle() const noexcept { return ownerHandle; }
    std::uint32_t objectDataBitSize() const noexcept { return objSize; }
};

std::uint64_t rawBitPosition(const dwgBuffer& buffer) {
    return buffer.getPosition() * 8u + buffer.getBitPos();
}

bool copyRawBits(const std::vector<std::uint8_t>& raw,
                 std::uint64_t begin, std::uint64_t end,
                 dwgBufferW& output) {
    if (begin > end
        || raw.size() > std::numeric_limits<std::uint64_t>::max() / 8u
        || end > static_cast<std::uint64_t>(raw.size()) * 8u)
        return false;

    dwgBuffer reader(const_cast<std::uint8_t*>(raw.data()), raw.size());
    if (!reader.setPosition(begin >> 3))
        return false;
    reader.setBitPos(static_cast<std::uint8_t>(begin & 7u));
    if (!reader.isGood())
        return false;
    for (std::uint64_t bit = begin; bit < end; ++bit) {
        output.putBit(reader.getBit());
        if (!reader.isGood())
            return false;
    }
    return output.isGood();
}

/// Keep the embedded AC1015/AC1018/AC1021 object-data boundary synchronized
/// after a prefix rewrite changes the encoded bit width of OT or own-H.
bool adjustRawObjectDataBitSize(const std::vector<std::uint8_t>& raw,
                                DRW::Version version,
                                std::int64_t bitDelta,
                                std::vector<std::uint8_t>& out) {
    if (bitDelta == 0) {
        out = raw;
        return !out.empty();
    }
    if (version <= DRW::AC1014 || version >= DRW::AC1024 || raw.empty()
        || raw.size() > std::numeric_limits<std::uint64_t>::max() / 8u)
        return false;

    dwgBuffer reader(const_cast<std::uint8_t*>(raw.data()), raw.size());
    (void)reader.getObjType(version);
    if (!reader.isGood())
        return false;
    const std::uint64_t sizeFieldBit = rawBitPosition(reader);
    if ((sizeFieldBit & 7u) != 2u)
        return false;
    const std::uint32_t currentBits = reader.getRawLong32();
    if (!reader.isGood())
        return false;
    const std::int64_t adjustedBits =
        static_cast<std::int64_t>(currentBits) + bitDelta;
    const std::uint64_t totalBits =
        static_cast<std::uint64_t>(raw.size()) * 8u;
    if (adjustedBits <= 0
        || static_cast<std::uint64_t>(adjustedBits) > totalBits
        || adjustedBits > std::numeric_limits<std::uint32_t>::max())
        return false;

    dwgBufferW rebuilt;
    if (!copyRawBits(raw, 0, totalBits, rebuilt))
        return false;
    rebuilt.patchRawLong32AtBit(
        static_cast<std::size_t>(sizeFieldBit),
        static_cast<std::uint32_t>(adjustedBits));
    rebuilt.alignToByte();
    if (!rebuilt.isGood() || rebuilt.data().empty())
        return false;
    out = rebuilt.data();
    return true;
}

/// Patch only the validated common owner handle of an AC1021+ entity.
/// R2000/R2004 do not expose a trustworthy opaque body/handle boundary, so
/// their owner remaps remain rejected by the caller.
bool patchRawEntityOwner(const std::vector<std::uint8_t>& raw,
                         DRW::Version version,
                         std::uint32_t sourceOwner,
                         std::uint32_t targetOwner,
                         std::uint32_t bodyBitSize,
                         std::vector<std::uint8_t>& out,
                         std::uint32_t& outputBodyBitSize) {
    if (version < DRW::AC1021 || sourceOwner == DRW::NoHandle
        || targetOwner == DRW::NoHandle || sourceOwner == targetOwner
        || raw.empty()
        || raw.size() > std::numeric_limits<std::uint64_t>::max() / 8u)
        return false;

    const std::uint64_t totalBits =
        static_cast<std::uint64_t>(raw.size()) * 8u;
    RawEntityOwnerProbe probe;
    dwgBuffer common(const_cast<std::uint8_t*>(raw.data()), raw.size());
    if (!probe.parseDwg(version, &common, bodyBitSize)
        || !common.isGood() || !probe.hasOwnerHandle())
        return false;

    const std::uint64_t ownerStart = probe.objectDataBitSize();
    if (ownerStart == 0 || ownerStart >= totalBits)
        return false;

    auto seekBits = [ownerStart](dwgBuffer& reader) {
        if (!reader.setPosition(ownerStart >> 3))
            return false;
        reader.setBitPos(static_cast<std::uint8_t>(ownerStart & 7u));
        return reader.isGood();
    };

    dwgBuffer resolvedReader(const_cast<std::uint8_t*>(raw.data()), raw.size());
    if (!seekBits(resolvedReader))
        return false;
    const dwgHandle resolvedOwner =
        resolvedReader.getOffsetHandle(probe.handle);
    if (!resolvedReader.isGood() || resolvedOwner.ref64 != sourceOwner)
        return false;
    const std::uint64_t ownerEnd = rawBitPosition(resolvedReader);
    if (ownerEnd <= ownerStart || ownerEnd > totalBits)
        return false;

    dwgBufferW rebuilt;
    if (!copyRawBits(raw, 0, ownerStart, rebuilt))
        return false;
    dwgHandle replacement;
    replacement.code = 4; // absolute soft-pointer form, as emitted by entities
    replacement.ref = targetOwner;
    replacement.ref64 = targetOwner;
    rebuilt.putHandle(replacement);
    if (!rebuilt.isGood() || !copyRawBits(raw, ownerEnd, totalBits, rebuilt))
        return false;
    rebuilt.alignToByte();
    if (!rebuilt.isGood() || rebuilt.data().empty())
        return false;

    out = rebuilt.data();
    if (version > DRW::AC1021) {
        const std::uint64_t dataBits = ownerStart;
        const std::uint64_t newTotalBits =
            static_cast<std::uint64_t>(out.size()) * 8u;
        if (newTotalBits < dataBits
            || newTotalBits - dataBits
                   > std::numeric_limits<std::uint32_t>::max())
            return false;
        outputBodyBitSize = static_cast<std::uint32_t>(newTotalBits - dataBits);
    } else {
        outputBodyBitSize = 0;
    }
    return true;
}

} // namespace

bool dwgWriter15::replayRawObject(
    const DRW_UnsupportedObject& object,
    const DRW_ModelerGeometry* typedModeler) {
    if (m_legacyModelChainActive) { m_legacyModelChainConflict = true; return false; }
    if (defaultDwgMaterialGraphActive()) { m_defaultMaterialConflict = true; return false; }
    m_lastDwgObjectFrame = {};
    m_lastDwgObjectFrameValid = false;
    m_currentDwgObjectFrameProvenance =
        m_pendingDwgObjectFrameProvenance;
    m_pendingDwgObjectFrameProvenance = {};
    const bool replayableFixedModelerBody =
        isReplayableFixedModelerBodyRawEntity(object);
    const bool storageBackedModeler = object.m_hasDataStorage
        || (typedModeler != nullptr
            && (typedModeler->hasDataStorageBinaryData()
                || typedModeler->hasDataStorageRecord));
    if ((blockControlEmitted() && object.m_isEntity)
        || object.m_version != m_version
        || (typedModeler != nullptr && !replayableFixedModelerBody)
        || (object.m_isEntity && !isReplayableFixedModelerRawEntity(object)
            && !isReplayableFixedEntityShellRawEntity(object)
            && !isReplayableSurfaceRawEntity(object)
            && !isReplayableCustomRawEntity(object))
        || object.m_handle == 0 || object.m_rawBytes.empty()) {
        return false;
    }
    if (m_version > DRW::AC1021) {
        if (object.m_rawBytes.size()
            > std::numeric_limits<std::uint64_t>::max() / 8u
            || static_cast<std::uint64_t>(object.m_bodyBitSize)
                > static_cast<std::uint64_t>(object.m_rawBytes.size()) * 8u) {
            return false;
        }
    }
    if (object.m_objectSize != 0
        && static_cast<std::uint64_t>(object.m_objectSize)
            != static_cast<std::uint64_t>(object.m_rawBytes.size())) {
        return false;
    }
    if (hasEmittedDwgObjectHandle(object.m_handle)) {
        return false;
    }

    const bool replayableCustomEntity = isReplayableCustomRawEntity(object);
    const bool replayableBlockOwnedEntity = replayableCustomEntity
        || replayableFixedModelerBody;

    // Modeler frames are opaque to the writer. Require an explicit typed
    // peer, reparse the original frame as the baseline, and refuse the raw
    // replay if any observable typed state has changed. A storage-backed
    // frame is admitted only when its exact bounded SAB record is staged for
    // section-level handle/payload closure during AC1027+ finalization.
    if (replayableFixedModelerBody) {
        DRW_DwgCommonLinkEvidence typedLinkEvidence =
            DRW_DwgCommonLinkEvidence::Unknown;
        if (typedModeler != nullptr
            && typedModeler->hasDwgCommonLinkTail()) {
            const bool hasTypedLinks =
                typedModeler->parentHandle != DRW::NoHandle
                || !typedModeler->reactorHandles.empty()
                || typedModeler->dwgReactorCount() != 0
                || typedModeler->xDictHandle != DRW::NoHandle;
            typedLinkEvidence = hasTypedLinks
                ? DRW_DwgCommonLinkEvidence::Validated
                : DRW_DwgCommonLinkEvidence::ValidatedAbsent;
        }
        const std::uint32_t typedBlockOwner =
            typedModeler == nullptr
                ? static_cast<std::uint32_t>(DRW::NoHandle)
            : typedModeler->parentHandle != DRW::NoHandle
                ? typedModeler->parentHandle
                : typedModeler->space == DRW::PaperSpace
                    ? DRW::DwgPaperSpaceBlockRecordHandle
                    : DRW::DwgModelSpaceBlockRecordHandle;
        const bool isSupportedAcdsModeler = typedModeler != nullptr
            && (typedModeler->eType == DRW::REGION
                || typedModeler->eType == DRW::E3DSOLID);
        static constexpr char sabMarker[] = "ACIS BinaryFile";
        const std::size_t sabMarkerLength = sizeof(sabMarker) - 1u;
        char expectedStorageHandleKey[32] = {};
        if (typedModeler != nullptr) {
            std::snprintf(expectedStorageHandleKey,
                          sizeof(expectedStorageHandleKey), "%llX",
                          static_cast<unsigned long long>(
                              typedModeler->dataStorageHandle));
        }
        const bool hasExactStorageMarker = typedModeler != nullptr
            && drw_hasSabMinimumHeader(typedModeler->dataStorageData)
            && typedModeler->hasDataStoragePayloadMarker
            && typedModeler->dataStoragePayloadMarkerSection == "datastore"
            && typedModeler->dataStoragePayloadMarkerLength == sabMarkerLength
            // ODA 24.2.2.3 prefixes the SAB stream with this marker.
            && typedModeler->dataStoragePayloadMarkerOffset == 0u
            && typedModeler->dataStoragePayloadMarkerOffset
                   <= typedModeler->dataStorageData.size()
            && sabMarkerLength
                   <= typedModeler->dataStorageData.size()
                       - typedModeler->dataStoragePayloadMarkerOffset
            && std::equal(sabMarker, sabMarker + sabMarkerLength,
                typedModeler->dataStorageData.begin()
                    + typedModeler->dataStoragePayloadMarkerOffset);
        const bool storageBindingValid = storageBackedModeler
            && m_version >= DRW::AC1027
            && object.m_hasDataStorage
            && typedModeler != nullptr
            && typedModeler->hasDataStorageBinaryData()
            && typedModeler->hasDataStorageRecord
            && isSupportedAcdsModeler
            && typedModeler->dataStorageHandle != DRW::NoHandle
            // The retained DWG frame carries the presence bit, not an
            // alternate AcDs record identity. On reread the record is
            // looked up by this entity's handle.
            && typedModeler->dataStorageHandle == object.m_handle
            && typedModeler->dataStorageHandleKey
                   == expectedStorageHandleKey
            && !typedModeler->dataStorageData.empty()
            && typedModeler->dataStorageData.size()
                   <= DRW_DataStorageConst::PAYLOAD_BLOB_SECTION_CAP
            && hasExactStorageMarker
            && typedModeler->m_dwgAcisPayload.empty()
            && typedModeler->m_dwgSourceVersion == m_version
            && typedModeler->m_modelerVersion == 2
            && !typedModeler->m_isEmpty
            && typedModeler->m_hasModelerData;
        if (typedModeler == nullptr || object.m_isCustomClass
            || object.m_hasDataStorage
                   != typedModeler->hasDataStorageBinaryData()
            || (storageBackedModeler && !storageBindingValid)
            || (!storageBackedModeler
                && (typedModeler->hasDataStorageRecord
                    || !typedModeler->dataStorageData.empty()
                    || typedModeler->hasDataStoragePayloadMarker))
            || object.m_commonLinkEvidence ==
                   DRW_DwgCommonLinkEvidence::Unknown
            || object.m_commonLinkEvidence != typedLinkEvidence
            || typedModeler->m_historyHandle != 0
            || object.m_handle != typedModeler->handle
            || rawBlockOwnerHandle(object) != typedBlockOwner
            || object.m_parentHandle != typedModeler->parentHandle
            || object.m_reactorHandles != typedModeler->reactorHandles
            || object.m_xDictHandle != typedModeler->xDictHandle
            || object.m_numReactors != typedModeler->dwgReactorCount()
            || object.m_xDictFlag != typedModeler->dwgXDictionaryFlag()
            || !drwDwgCommonLinkEvidenceValidated(
                object.m_commonLinkEvidence)
            || object.m_rawBytes != typedModeler->m_rawBytes
            || object.m_bodyBitSize != typedModeler->m_bodyBitSize
            || object.m_objectSize != typedModeler->m_objectSize
            || (typedModeler->m_dwgSourceVersion != DRW::UNKNOWNV
                && typedModeler->m_dwgSourceVersion != m_version)) {
            return false;
        }
        const std::uint16_t expectedObjectType =
            typedModeler->eType == DRW::REGION ? 37u
            : typedModeler->eType == DRW::E3DSOLID ? 38u
            : typedModeler->eType == DRW::BODY ? 39u
            : 0u;
        if (expectedObjectType == 0u
            || object.m_objectType != expectedObjectType)
            return false;

        // object owns this immutable frame for the duration of the parse.
        dwgBuffer sourceFrame(object.m_rawBytes.data(),
                              object.m_rawBytes.size());
        DRW_ModelerGeometry parsedPeer(typedModeler->eType);
        if (!parsedPeer.parseDwg(m_version, &sourceFrame,
                                 object.m_bodyBitSize)
            || !sourceFrame.isGood())
            return false;
        // The retained raw frame was equality-checked before parsing. This
        // peer validates decoded state; copying the input frame into it would
        // only repeat that comparison after an extra full-frame allocation.
        parsedPeer.m_bodyBitSize = object.m_bodyBitSize;
        parsedPeer.m_objectSize = object.m_objectSize;
        if (storageBackedModeler) {
            // These fields are supplied by the exactly-linked AcDs record,
            // not encoded in the entity frame. Validate their admitted
            // profile above, then compare the remaining typed state against
            // the raw-frame parse as usual.
            parsedPeer.hasDataStorageRecord =
                typedModeler->hasDataStorageRecord;
            parsedPeer.dataStorageHandle = typedModeler->dataStorageHandle;
            parsedPeer.dataStorageHandleKey =
                typedModeler->dataStorageHandleKey;
            // AcDs bytes are not encoded in the entity frame. Avoid making a
            // temporary payload copy; finalization compares the pending owned
            // payload snapshot with the linked section record.
            parsedPeer.dataStorageSegmentIndex =
                typedModeler->dataStorageSegmentIndex;
            parsedPeer.dataStorageSchemaIndex =
                typedModeler->dataStorageSchemaIndex;
            parsedPeer.hasDataStoragePayloadMarker =
                typedModeler->hasDataStoragePayloadMarker;
            parsedPeer.dataStoragePayloadMarkerOffset =
                typedModeler->dataStoragePayloadMarkerOffset;
            parsedPeer.dataStoragePayloadMarkerLength =
                typedModeler->dataStoragePayloadMarkerLength;
            parsedPeer.dataStoragePayloadMarkerSection =
                typedModeler->dataStoragePayloadMarkerSection;
            parsedPeer.m_modelerVersion = typedModeler->m_modelerVersion;
            parsedPeer.m_dwgSourceVersion = typedModeler->m_dwgSourceVersion;
        }

        const auto sameCoord = [](const DRW_Coord& lhs,
                                  const DRW_Coord& rhs) {
            return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
        };
        const auto sameVariant = [this, &sameCoord](
            const DRW_Variant& lhs, const DRW_Variant& rhs) {
            if (lhs.type() != rhs.type() || lhs.code() != rhs.code()
                || lhs.isLayerRef() != rhs.isLayerRef()
                || lhs.canReplayDwgRawLayerReference(m_version)
                    != rhs.canReplayDwgRawLayerReference(m_version))
                return false;
            if (lhs.canReplayDwgRawLayerReference(m_version)
                && lhs.rawDwgLayerReference()
                    != rhs.rawDwgLayerReference())
                return false;
            switch (lhs.type()) {
            case DRW_Variant::STRING:
                return std::strcmp(lhs.c_str(), rhs.c_str()) == 0;
            case DRW_Variant::INTEGER:
                return lhs.i_val() == rhs.i_val();
            case DRW_Variant::INTEGER64:
                return lhs.i64_val() == rhs.i64_val();
            case DRW_Variant::DOUBLE:
                return lhs.d_val() == rhs.d_val();
            case DRW_Variant::COORD: {
                const DRW_Coord* lhsCoord = lhs.coord();
                const DRW_Coord* rhsCoord = rhs.coord();
                return lhsCoord != nullptr && rhsCoord != nullptr
                    && sameCoord(*lhsCoord, *rhsCoord);
            }
            case DRW_Variant::BINARY: {
                const auto* lhsBytes = lhs.binary();
                const auto* rhsBytes = rhs.binary();
                return lhsBytes != nullptr && rhsBytes != nullptr
                    && *lhsBytes == *rhsBytes;
            }
            case DRW_Variant::INVALID:
                return true;
            }
            return false;
        };
        const auto sameExtData = [&sameVariant](
            const std::vector<std::shared_ptr<DRW_Variant>>& lhs,
            const std::vector<std::shared_ptr<DRW_Variant>>& rhs) {
            if (lhs.size() != rhs.size())
                return false;
            for (std::size_t i = 0; i < lhs.size(); ++i) {
                if (static_cast<bool>(lhs[i]) != static_cast<bool>(rhs[i])
                    || (lhs[i] && !sameVariant(*lhs[i], *rhs[i])))
                    return false;
            }
            return true;
        };
        const auto sameAppData = [&sameVariant](
            const std::list<std::list<DRW_Variant>>& lhs,
            const std::list<std::list<DRW_Variant>>& rhs) {
            if (lhs.size() != rhs.size())
                return false;
            auto lhsGroup = lhs.cbegin();
            auto rhsGroup = rhs.cbegin();
            for (; lhsGroup != lhs.cend(); ++lhsGroup, ++rhsGroup) {
                if (lhsGroup->size() != rhsGroup->size())
                    return false;
                auto lhsValue = lhsGroup->cbegin();
                auto rhsValue = rhsGroup->cbegin();
                for (; lhsValue != lhsGroup->cend(); ++lhsValue, ++rhsValue) {
                    if (!sameVariant(*lhsValue, *rhsValue))
                        return false;
                }
            }
            return true;
        };
        const auto samePendingRefs = [](
            const std::vector<DRW_Entity::PendingHandleRef>& lhs,
            const std::vector<DRW_Entity::PendingHandleRef>& rhs) {
            if (lhs.size() != rhs.size())
                return false;
            for (std::size_t i = 0; i < lhs.size(); ++i) {
                if (lhs[i].indexInExtData != rhs[i].indexInExtData
                    || lhs[i].handleRef != rhs[i].handleRef
                    || lhs[i].rawHandleRef != rhs[i].rawHandleRef
                    || lhs[i].useRawHandleRef != rhs[i].useRawHandleRef)
                    return false;
            }
            return true;
        };
        const auto sameDwHandle = [](const dwgHandle& lhs,
                                     const dwgHandle& rhs) {
            return lhs.code == rhs.code && lhs.size == rhs.size
                && lhs.ref == rhs.ref && lhs.ref64 == rhs.ref64;
        };
        const auto sameCommonState = [&](const DRW_ModelerGeometry& lhs,
                                         const DRW_ModelerGeometry& rhs) {
            return lhs.eType == rhs.eType && lhs.handle == rhs.handle
                && lhs.parentHandle == rhs.parentHandle
                && lhs.space == rhs.space && lhs.layer == rhs.layer
                && lhs.lineType == rhs.lineType && lhs.material == rhs.material
                && lhs.color == rhs.color && lhs.lWeight == rhs.lWeight
                && lhs.ltypeScale == rhs.ltypeScale
                && lhs.visible == rhs.visible
                && lhs.numProxyGraph == rhs.numProxyGraph
                && lhs.proxyGraphics == rhs.proxyGraphics
                && lhs.color24 == rhs.color24 && lhs.colorName == rhs.colorName
                && lhs.transparency == rhs.transparency
                && lhs.plotStyle == rhs.plotStyle && lhs.shadow == rhs.shadow
                && lhs.shadowHandle == rhs.shadowHandle
                && lhs.fullVisualStyleHandle == rhs.fullVisualStyleHandle
                && lhs.faceVisualStyleHandle == rhs.faceVisualStyleHandle
                && lhs.edgeVisualStyleHandle == rhs.edgeVisualStyleHandle
                && lhs.reactorHandles == rhs.reactorHandles
                && lhs.xDictHandle == rhs.xDictHandle
                && lhs.hasDataStorageRecord == rhs.hasDataStorageRecord
                && lhs.dataStorageHandle == rhs.dataStorageHandle
                && lhs.dataStorageHandleKey == rhs.dataStorageHandleKey
                && lhs.dataStorageSegmentIndex == rhs.dataStorageSegmentIndex
                && lhs.dataStorageSchemaIndex == rhs.dataStorageSchemaIndex
                && lhs.hasDataStoragePayloadMarker
                    == rhs.hasDataStoragePayloadMarker
                && lhs.dataStoragePayloadMarkerOffset
                    == rhs.dataStoragePayloadMarkerOffset
                && lhs.dataStoragePayloadMarkerLength
                    == rhs.dataStoragePayloadMarkerLength
                && lhs.dataStoragePayloadMarkerSection
                    == rhs.dataStoragePayloadMarkerSection
                && lhs.hasDataStorageBinaryData()
                    == rhs.hasDataStorageBinaryData()
                && lhs.haveExtrusion == rhs.haveExtrusion
                && sameExtData(lhs.extData, rhs.extData)
                && sameAppData(lhs.appData, rhs.appData)
                && samePendingRefs(lhs.pendingAppIdResolutions,
                                   rhs.pendingAppIdResolutions)
                && samePendingRefs(lhs.pendingLayerRefResolutions,
                                   rhs.pendingLayerRefResolutions)
                && lhs.haveNextLinks == rhs.haveNextLinks
                && lhs.hasDsData == rhs.hasDsData
                && lhs.plotFlags == rhs.plotFlags && lhs.ltFlags == rhs.ltFlags
                && lhs.materialFlag == rhs.materialFlag
                && lhs.shadowFlag == rhs.shadowFlag
                && lhs.hasFullVisualStyle == rhs.hasFullVisualStyle
                && lhs.hasFaceVisualStyle == rhs.hasFaceVisualStyle
                && lhs.hasEdgeVisualStyle == rhs.hasEdgeVisualStyle
                && lhs.hasAcDbColorH == rhs.hasAcDbColorH
                && lhs.acDbColorHandle == rhs.acDbColorHandle
                && sameDwHandle(lhs.lTypeH, rhs.lTypeH)
                && sameDwHandle(lhs.layerH, rhs.layerH)
                && lhs.nextEntLink == rhs.nextEntLink
                && lhs.prevEntLink == rhs.prevEntLink
                && lhs.ownerHandle == rhs.ownerHandle
                && lhs.xDictFlag == rhs.xDictFlag
                && lhs.numReactors == rhs.numReactors
                && lhs.commonLinkTailValidated
                    == rhs.commonLinkTailValidated
                && lhs.objSize == rhs.objSize
                && lhs.dwgDataEndBit == rhs.dwgDataEndBit
                && lhs.oType == rhs.oType
                && samePendingRefs(lhs.dwgEedAppIdWriteRefs,
                                   rhs.dwgEedAppIdWriteRefs)
                && samePendingRefs(lhs.dwgEedLayerWriteRefs,
                                   rhs.dwgEedLayerWriteRefs)
                && lhs.dwgEedCodePage == rhs.dwgEedCodePage
                && sameCoord(lhs.extAxisX, rhs.extAxisX)
                && sameCoord(lhs.extAxisY, rhs.extAxisY);
        };
        const auto samePayloadChunks = [](
            const std::vector<DRW_ModelerPayloadChunk>& lhs,
            const std::vector<DRW_ModelerPayloadChunk>& rhs) {
            if (lhs.size() != rhs.size())
                return false;
            for (std::size_t i = 0; i < lhs.size(); ++i) {
                if (lhs[i].m_groupCode != rhs[i].m_groupCode
                    || lhs[i].m_offset != rhs[i].m_offset
                    || lhs[i].m_length != rhs[i].m_length)
                    return false;
            }
            return true;
        };
        const auto samePayloadRanges = [](
            const std::vector<DRW_ModelerPayloadRange>& lhs,
            const std::vector<DRW_ModelerPayloadRange>& rhs) {
            if (lhs.size() != rhs.size())
                return false;
            for (std::size_t i = 0; i < lhs.size(); ++i) {
                if (lhs[i].m_kind != rhs[i].m_kind
                    || lhs[i].m_section != rhs[i].m_section
                    || lhs[i].m_offset != rhs[i].m_offset
                    || lhs[i].m_bitOffset != rhs[i].m_bitOffset
                    || lhs[i].m_length != rhs[i].m_length
                    || lhs[i].m_declaredByteSize
                        != rhs[i].m_declaredByteSize
                    || lhs[i].m_consistency != rhs[i].m_consistency
                    || lhs[i].m_confidence != rhs[i].m_confidence
                    || lhs[i].m_markerText != rhs[i].m_markerText)
                    return false;
            }
            return true;
        };
        if (!sameCommonState(*typedModeler, parsedPeer)
            || typedModeler->m_modelerVersion
                != parsedPeer.m_modelerVersion
            || typedModeler->m_hasDxfModelerFlag
                != parsedPeer.m_hasDxfModelerFlag
            || typedModeler->m_dxfModelerFlag
                != parsedPeer.m_dxfModelerFlag
            || typedModeler->m_hasDxfModelerUid
                != parsedPeer.m_hasDxfModelerUid
            || typedModeler->m_dxfModelerUid
                != parsedPeer.m_dxfModelerUid
            || typedModeler->m_dwgSourceVersion
                != parsedPeer.m_dwgSourceVersion
            || typedModeler->m_bodyBitSize != parsedPeer.m_bodyBitSize
            || typedModeler->m_objectSize != parsedPeer.m_objectSize
            || typedModeler->m_isEmpty != parsedPeer.m_isEmpty
            || typedModeler->m_hasModelerData
                != parsedPeer.m_hasModelerData
            || typedModeler->m_modelerDataUnknownBit
                != parsedPeer.m_modelerDataUnknownBit
            || typedModeler->m_hasWireframe != parsedPeer.m_hasWireframe
            || typedModeler->m_historyHandle != parsedPeer.m_historyHandle
            || typedModeler->m_dwgAcisPayload
                != parsedPeer.m_dwgAcisPayload
            || !samePayloadChunks(typedModeler->m_dxfPayloadChunks,
                                  parsedPeer.m_dxfPayloadChunks)
            || !samePayloadRanges(typedModeler->m_payloadRanges,
                                  parsedPeer.m_payloadRanges))
            return false;
    } else if (typedModeler != nullptr) {
        return false;
    }
    if (replayableBlockOwnedEntity
        && object.m_blockOwnerHandle != DRW::NoHandle
        && object.m_parentHandle == DRW::NoHandle
        && object.m_blockOwnerHandle != reservedHandle::BLOCK_MODEL_SPACE
        && object.m_blockOwnerHandle != reservedHandle::BLOCK_PAPER_SPACE)
        return false;
    if (replayableBlockOwnedEntity && !canRecordRawBlockOwnedEntity(object))
        return false;

    // Fixed modeler frames contain opaque body/handle data, including a
    // history-object reference that this raw carrier cannot safely remap.
    // Require the encoded entity handle to remain identical before reserving
    // any target handle or mutating writer state. A typed modeler rewrite is
    // required for handle remapping.
    if (replayableFixedModelerBody) {
        dwgHandle sourceEntityHandle;
        if (!readRawObjectHandle(object.m_rawBytes, m_version,
                                 sourceEntityHandle)
            || sourceEntityHandle.ref64 != object.m_handle)
            return false;
    }

    m_handles.reserve(object.m_handle);
    const std::vector<DwgClassDefinition> classDefinitionsBefore =
        m_dwgClassDefinitions;
    const auto rawClassInstanceHandlesBefore = m_rawClassInstanceHandles;
    const auto classInstanceHandleOwnersBefore =
        m_dwgClassInstanceHandleOwners;
    const auto classInstanceEmittedHandlesBefore =
        m_dwgClassInstanceEmittedHandles;
    const auto classInstanceEmissionCountsBefore =
        m_dwgClassInstanceEmissionCounts;
    const auto rawClassNumRemapBefore = m_rawClassNumRemap;
    const auto rawObjectOverridesBefore = m_rawObjectOverrides;
    const bool classConflictBefore = m_hasDwgClassConflict;
    const std::size_t outputStart = m_buf.size();
    const std::size_t objectMapStart = m_objectMap.size();
    const std::size_t pendingStorageStart =
        m_pendingModelerDataStorageReplay.size();
    const std::size_t pendingStorageBytesStart =
        m_pendingModelerDataStoragePayloadBytes;
    bool committed = false;
    const auto rollback = [&]() {
        if (committed)
            return;
        m_dwgClassDefinitions = classDefinitionsBefore;
        m_rawClassInstanceHandles = rawClassInstanceHandlesBefore;
        m_dwgClassInstanceHandleOwners = classInstanceHandleOwnersBefore;
        m_dwgClassInstanceEmittedHandles =
            classInstanceEmittedHandlesBefore;
        m_dwgClassInstanceEmissionCounts = classInstanceEmissionCountsBefore;
        m_rawClassNumRemap = rawClassNumRemapBefore;
        m_rawObjectOverrides = rawObjectOverridesBefore;
        m_hasDwgClassConflict = classConflictBefore;
        m_buf.truncate(outputStart);
        if (objectMapStart <= m_objectMap.size()) {
            for (std::size_t index = objectMapStart;
                 index < m_objectMap.size(); ++index)
                m_emittedDwgObjectHandles.erase(m_objectMap[index].first);
            m_objectMap.resize(objectMapStart);
        }
        if (pendingStorageStart <= m_pendingModelerDataStorageReplay.size())
            m_pendingModelerDataStorageReplay.resize(pendingStorageStart);
        m_pendingModelerDataStoragePayloadBytes = pendingStorageBytesStart;
    };
    const auto fail = [&]() {
        m_currentDwgObjectFrameProvenance = {};
        rollback();
        m_lastDwgObjectFrame = {};
        m_lastDwgObjectFrameValid = false;
        return false;
    };

    PendingModelerDataStorageReplay pendingStorage;
    if (storageBackedModeler) {
        const std::size_t payloadSize = typedModeler->dataStorageData.size();
        if (payloadSize > DRW_DataStorageConst::PAYLOAD_BLOB_SECTION_CAP
            || m_pendingModelerDataStoragePayloadBytes
                   > DRW_DataStorageConst::PAYLOAD_BLOB_SECTION_CAP
                       - payloadSize)
            return fail();
        try {
            pendingStorage.modelerHandle = object.m_handle;
            pendingStorage.recordHandle = typedModeler->dataStorageHandle;
            pendingStorage.recordHandleKey =
                typedModeler->dataStorageHandleKey;
            pendingStorage.ownerHandle = rawBlockOwnerHandle(object);
            pendingStorage.segmentIndex =
                typedModeler->dataStorageSegmentIndex;
            pendingStorage.schemaIndex =
                typedModeler->dataStorageSchemaIndex;
            pendingStorage.payload = typedModeler->dataStorageData;
            pendingStorage.hasPayloadMarker =
                typedModeler->hasDataStoragePayloadMarker;
            pendingStorage.payloadMarkerOffset =
                typedModeler->dataStoragePayloadMarkerOffset;
            pendingStorage.payloadMarkerLength =
                typedModeler->dataStoragePayloadMarkerLength;
            pendingStorage.payloadMarkerSection =
                typedModeler->dataStoragePayloadMarkerSection;
            m_pendingModelerDataStorageReplay.reserve(
                m_pendingModelerDataStorageReplay.size() + 1u);
        } catch (...) {
            return fail();
        }
    }
    const std::uint32_t sourceOwner = object.m_parentHandle;
    const std::uint32_t targetOwner = rawBlockOwnerHandle(object);
    // A fixed modeler body is replayed as an opaque same-version frame. Its
    // owner relationship is part of that frame's provenance; moving it would
    // require a typed modeler rewrite rather than an unqualified raw replay.
    if (replayableFixedModelerBody && sourceOwner != DRW::NoHandle
        && targetOwner != sourceOwner)
        return fail();
    if (!registerRawObjectClass(object))
        return fail();

    const std::uint16_t sourceType =
        static_cast<std::uint16_t>(object.m_objectType);
    const std::uint16_t writerType = remappedRawClassNum(object);
    if (m_currentDwgObjectFrameProvenance.classNumber != 0
        && m_currentDwgObjectFrameProvenance.classNumber != writerType)
        return fail();
    const std::vector<std::uint8_t>* bodyBytes = &object.m_rawBytes;
    std::vector<std::uint8_t> patched;
    std::vector<std::uint8_t> patchedHandle;
    std::vector<std::uint8_t> patchedDataSize;
    std::vector<std::uint8_t> patchedOwner;
    std::uint32_t bodyBitSize = object.m_bodyBitSize;
    std::int64_t objectDataBitDelta = 0;
    if (object.m_isCustomClass && sourceType >= 500 && writerType != sourceType) {
        std::int64_t bitDelta = 0;
        if (!patchRawObjectType(object.m_rawBytes, m_version, writerType,
                                patched, &bitDelta))
            return fail();
        bodyBytes = &patched;
        objectDataBitDelta += bitDelta;
    }
    dwgHandle encodedHandle;
    if (!readRawObjectHandle(*bodyBytes, m_version, encodedHandle))
        return fail();
    if (encodedHandle.ref64 != object.m_handle) {
        std::int64_t bitDelta = 0;
        if (!patchRawObjectHandle(*bodyBytes, m_version, object.m_handle,
                                  patchedHandle, &bitDelta))
            return fail();
        bodyBytes = &patchedHandle;
        objectDataBitDelta += bitDelta;
    }
    if (m_version < DRW::AC1024 && objectDataBitDelta != 0) {
        if (!adjustRawObjectDataBitSize(*bodyBytes, m_version,
                                        objectDataBitDelta, patchedDataSize))
            return fail();
        bodyBytes = &patchedDataSize;
    }
    if (replayableCustomEntity && sourceOwner != DRW::NoHandle
        && targetOwner != sourceOwner) {
        if (!patchRawEntityOwner(*bodyBytes, m_version, sourceOwner,
                                 targetOwner, bodyBitSize, patchedOwner,
                                 bodyBitSize))
            return fail();
        bodyBytes = &patchedOwner;
    }
    if (bodyBytes->empty())
        return fail();
    dwgBuffer typeReader(
        const_cast<std::uint8_t*>(bodyBytes->data()), bodyBytes->size());
    const std::uint16_t actualType = typeReader.getObjType(m_version);
    if (!typeReader.isGood() || actualType != writerType)
        return fail();
    if (bodyBytes->size() > DwgObjectFrame::MaxBodySize
        || bodyBytes->size() > static_cast<std::size_t>(
            std::numeric_limits<std::int32_t>::max())) {
        return fail();
    }
    if (m_buf.size() > std::numeric_limits<std::uint32_t>::max()) {
        return fail();
    }

    const std::uint32_t frameStart = static_cast<std::uint32_t>(m_buf.size());
    m_buf.putModularShort(static_cast<std::int32_t>(bodyBytes->size()));
    if (!m_buf.isGood()) {
        return fail();
    }
    if (m_version > DRW::AC1021)
        m_buf.putUModularChar(bodyBitSize);
    if (!m_buf.isGood()) {
        return fail();
    }
    const size_t bodyStart = m_buf.size();
    m_buf.putBytes(bodyBytes->data(), bodyBytes->size());

    const std::uint16_t crc = m_buf.crc16(0xC0C1, static_cast<size_t>(frameStart),
                                    bodyStart + bodyBytes->size());
    m_buf.putRawShort16(crc);
    if (!m_buf.isGood()
        || m_buf.size() > std::numeric_limits<std::uint32_t>::max()) {
        return fail();
    }
    if (!publishDwgObjectMapEntry(object.m_handle, frameStart))
        return fail();
    if (storageBackedModeler) {
        m_pendingModelerDataStoragePayloadBytes +=
            pendingStorage.payload.size();
        m_pendingModelerDataStorageReplay.push_back(
            std::move(pendingStorage));
    }
    if (replayableBlockOwnedEntity
        && !recordRawBlockOwnedEntity(object)) {
        return fail();
    }
    markDwgClassInstanceEmitted(object.m_handle);
    if (m_nextDwgObjectFrameGeneration == 0
        || m_nextDwgObjectFrameGeneration
               == std::numeric_limits<std::uint64_t>::max()) {
        return fail();
    }
    m_lastDwgObjectFrame.valid = true;
    m_lastDwgObjectFrame.generation = m_nextDwgObjectFrameGeneration++;
    m_lastDwgObjectFrame.objectHandle = object.m_handle;
    m_lastDwgObjectFrame.version = m_version;
    m_lastDwgObjectFrame.dataBitSize = m_version > DRW::AC1021
        ? bodyBitSize : static_cast<std::uint64_t>(bodyBytes->size()) * 8u;
    m_lastDwgObjectFrame.handleBitSize = 0;
    m_lastDwgObjectFrame.classNumber = actualType;
    m_lastDwgObjectFrame.writerOperation =
        m_currentDwgObjectFrameProvenance.writerOperation;
    m_lastDwgObjectFrame.admissionToken =
        m_currentDwgObjectFrameProvenance.admissionToken;
    committed = true;
    m_lastDwgObjectFrameValid = true;
    m_currentDwgObjectFrameProvenance = {};
    return true;
}

bool dwgWriter15::validatePendingModelerDataStorageReplay(
    const DRW_DataStorageSection* section,
    const std::uint8_t* sectionBytes,
    std::size_t sectionByteLength) const {
    if (section == nullptr)
        return m_pendingModelerDataStorageReplay.empty();
    if (section->m_name != "AcDb:AcDsPrototype_1b"
        || section->m_version != m_version || section->parseFailed
        || !section->structurallyValid || !section->replayAllowed)
        return false;
    if (sectionBytes == nullptr
        || sectionByteLength != section->sectionByteLength)
        return false;

    bool allPayloadsRetainable =
        sectionByteLength <= DRW_DataStorageConst::PAYLOAD_BLOB_SECTION_CAP;
    std::size_t retainedPayloadBytes = 0;
    for (const DRW_DataStorageRecord& record : section->records) {
        const std::uint64_t payloadLength = record.isBlobReference
            ? record.blobTotalByteLength : record.dataByteLength;
        if (!allPayloadsRetainable
            || payloadLength
                   > DRW_DataStorageConst::PAYLOAD_BLOB_SECTION_CAP
                       - retainedPayloadBytes) {
            allPayloadsRetainable = false;
            continue;
        }
        retainedPayloadBytes += static_cast<std::size_t>(payloadLength);
    }
    if (!m_pendingModelerDataStorageReplay.empty()
        && (m_version < DRW::AC1027 || !allPayloadsRetainable))
        return false;

    std::set<std::uint32_t> modelerHandles;
    std::set<std::uint64_t> recordHandles;
    std::set<std::string> recordHandleKeys;
    std::set<const DRW_DataStorageRecord*> boundRecords;
    static constexpr char sabMarker[] = "ACIS BinaryFile";
    constexpr std::size_t sabMarkerLength = sizeof(sabMarker) - 1u;

    for (const PendingModelerDataStorageReplay& pending :
         m_pendingModelerDataStorageReplay) {
        if (pending.modelerHandle == 0 || pending.recordHandle == 0
            || pending.recordHandle != pending.modelerHandle
            || pending.recordHandleKey.empty() || pending.payload.empty()
            || pending.payload.size()
                   > DRW_DataStorageConst::PAYLOAD_BLOB_SECTION_CAP
            || !drw_hasSabMinimumHeader(pending.payload)
            || !pending.hasPayloadMarker
            || pending.payloadMarkerSection != "datastore"
            || pending.payloadMarkerLength != sabMarkerLength
            || pending.payloadMarkerOffset != 0u
            || pending.payloadMarkerOffset > pending.payload.size()
            || sabMarkerLength
                   > pending.payload.size() - pending.payloadMarkerOffset
            || !std::equal(sabMarker, sabMarker + sabMarkerLength,
                pending.payload.begin() + pending.payloadMarkerOffset)
            || !modelerHandles.insert(pending.modelerHandle).second
            || !recordHandles.insert(pending.recordHandle).second
            || !recordHandleKeys.insert(pending.recordHandleKey).second)
            return false;

        const DRW_DataStorageRecord* byHandle =
            section->findRecordByHandle(pending.recordHandle);
        const DRW_DataStorageRecord* byKey =
            section->findRecordByHandleKey(pending.recordHandleKey);
        DataStoragePayloadInspection payloadInspection;
        if (byHandle == nullptr || byKey == nullptr || byHandle != byKey
            || byHandle->handle != pending.recordHandle
            || byHandle->handleKey != pending.recordHandleKey
            || !byHandle->isHandleSafe
            || byHandle->segmentIndex != pending.segmentIndex
            || byHandle->schemaIndex != pending.schemaIndex
            || !inspectDataStorageRecordPayload(
                sectionBytes, sectionByteLength, *section, *byHandle,
                &pending.payload, payloadInspection)
            || payloadInspection.hasAcisMarker != pending.hasPayloadMarker
            || (payloadInspection.hasAcisMarker
                && payloadInspection.acisMarkerOffset
                       != pending.payloadMarkerOffset)
            || (payloadInspection.hasAcisMarker
                ? (pending.payloadMarkerLength != sabMarkerLength
                    || pending.payloadMarkerSection != "datastore")
                : (pending.payloadMarkerLength != 0u
                    || !pending.payloadMarkerSection.empty()))
            || (byHandle->isBlobReference
                ? byHandle->blobTotalByteLength != pending.payload.size()
                : byHandle->dataByteLength != pending.payload.size()))
            return false;

        const std::size_t recordIndex = static_cast<std::size_t>(
            byHandle - section->records.data());
        const auto handleIndexes =
            section->recordIndexesByHandle.find(pending.recordHandle);
        const auto keyIndexes =
            section->recordIndexesByHandleKey.find(pending.recordHandleKey);
        if (recordIndex >= section->records.size()
            || handleIndexes == section->recordIndexesByHandle.end()
            || keyIndexes == section->recordIndexesByHandleKey.end()
            || handleIndexes->second.size() != 1u
            || keyIndexes->second.size() != 1u
            || handleIndexes->second.front() != recordIndex
            || keyIndexes->second.front() != recordIndex
            || !boundRecords.insert(byHandle).second)
            return false;

        // Frame publication rejects duplicate handles, so presence in the
        // synchronized emitted-handle ledger means exactly one object frame.
        if (!hasEmittedDwgObjectHandle(pending.modelerHandle))
            return false;

        std::uint32_t membershipOwner = pending.ownerHandle;
        if (pending.ownerHandle == DRW::NoHandle
            || pending.ownerHandle == reservedHandle::BLOCK_MODEL_SPACE) {
            membershipOwner = reservedHandle::BLOCK_MODEL_SPACE;
        } else if (pending.ownerHandle
                   == reservedHandle::BLOCK_PAPER_SPACE) {
            membershipOwner = reservedHandle::BLOCK_PAPER_SPACE;
        }
        if (m_blockOwnedEntityHandles.count(
                std::make_pair(membershipOwner, pending.modelerHandle)) == 0u)
            return false;
    }

    try {
        std::set<std::pair<std::uint32_t, std::uint64_t>> requiredSearch;
        for (const PendingModelerDataStorageReplay& pending :
             m_pendingModelerDataStorageReplay)
            requiredSearch.emplace(pending.schemaIndex, pending.recordHandle);
        if (!validateModelerDataStorageSearch(
                *section, sectionBytes, sectionByteLength, requiredSearch))
            return false;
    } catch (...) {
        return false;
    }

    // A marker-bearing payload that the old retaining parse would have copied
    // must have exactly one typed modeler peer; otherwise the writer would
    // silently reproduce an orphan solid or attach it by encounter order.
    std::size_t eligiblePayloadBytes = 0;
    const bool sectionFitsRetentionCap = sectionByteLength
        <= DRW_DataStorageConst::PAYLOAD_BLOB_SECTION_CAP;
    for (const DRW_DataStorageRecord& record : section->records) {
        const std::uint64_t payloadLength = record.isBlobReference
            ? record.blobTotalByteLength : record.dataByteLength;
        const bool payloadWouldHaveBeenRetained = sectionFitsRetentionCap
            && payloadLength
                   <= DRW_DataStorageConst::PAYLOAD_BLOB_SECTION_CAP
                       - eligiblePayloadBytes;
        if (!payloadWouldHaveBeenRetained)
            continue;
        eligiblePayloadBytes += static_cast<std::size_t>(payloadLength);
        DataStoragePayloadInspection payloadInspection;
        if (!inspectDataStorageRecordPayload(
                sectionBytes, sectionByteLength, *section, record, nullptr,
                payloadInspection))
            return false;
        if (payloadInspection.hasAcisMarker
            && boundRecords.count(&record) == 0u)
            return false;
    }
    return true;
}

bool dwgWriter15::hasEmittedDwgObjectHandle(std::uint32_t handle) const {
    return m_emittedDwgObjectHandles.find(handle)
        != m_emittedDwgObjectHandles.end();
}

bool dwgWriter15::publishDwgObjectMapEntry(
    std::uint32_t handle, std::uint32_t frameOffset) {
    const auto inserted = m_emittedDwgObjectHandles.insert(handle);
    if (!inserted.second)
        return false;
    try {
        m_objectMap.emplace_back(handle, frameOffset);
    } catch (...) {
        m_emittedDwgObjectHandles.erase(inserted.first);
        throw;
    }
    return true;
}

dwgBufferW& dwgWriter15::beginObject(std::uint32_t handle) {
    m_currentHandle = handle;
    m_currentDwgObjectFrameProvenance =
        m_pendingDwgObjectFrameProvenance;
    m_pendingDwgObjectFrameProvenance = {};
    m_lastDwgObjectFrame = {};
    m_lastDwgObjectFrameValid = false;
    m_objectBody.reset();
    m_objectBody.setEedLayerByteOrder(m_eedLayerByteOrder);
    m_objectStrings.reset();
    m_objectHandles.reset();
    return m_objectBody;
}

bool dwgWriter15::captureLastDwgObjectHandleOccurrences(
    std::uint64_t mergedStringBaseBit, std::uint64_t mergedHandleBaseBit,
    bool stringsMergedIntoData, bool handlesMergedIntoData) {
    DRW::DwgObjectFrameReceipt frame;
    frame.objectHandle = m_currentHandle;
    frame.version = m_version;
    frame.dataBitSize = m_objectBody.bitCount();
    frame.handleBitSize = handlesMergedIntoData
        ? 0 : m_objectHandles.bitCount();

    if (m_objectBody.data().empty()) {
        m_lastDwgObjectFrame = {};
        m_lastDwgObjectFrameValid = false;
        m_currentDwgObjectFrameProvenance = {};
        return false;
    }
    dwgBuffer typeReader(
        const_cast<std::uint8_t*>(m_objectBody.data().data()),
        m_objectBody.data().size());
    frame.classNumber = typeReader.getObjType(m_version);
    if (!typeReader.isGood()
        || (m_currentDwgObjectFrameProvenance.classNumber != 0
            && frame.classNumber
                   != m_currentDwgObjectFrameProvenance.classNumber)) {
        m_lastDwgObjectFrame = {};
        m_lastDwgObjectFrameValid = false;
        m_currentDwgObjectFrameProvenance = {};
        return false;
    }
    frame.writerOperation =
        m_currentDwgObjectFrameProvenance.writerOperation;
    frame.admissionToken =
        m_currentDwgObjectFrameProvenance.admissionToken;

    std::uint32_t dataOrdinal = 0;
    std::uint32_t handleOrdinal = 0;
    bool valid = true;
    const auto append = [&frame, &valid](
            const dwgBufferW& buffer, DRW::DwgObjectHandleStream stream,
            DRW::DwgObjectSerializedSegment segment, std::uint64_t baseBit,
            std::uint32_t& segmentOrdinal) {
        for (const DRW::DwgHandleWriteOccurrence& token
             : buffer.handleOccurrences()) {
            if (token.endBit < token.startBit
                || baseBit > std::numeric_limits<std::uint64_t>::max()
                                  - token.endBit
                || segmentOrdinal == std::numeric_limits<std::uint32_t>::max()) {
                valid = false;
                return;
            }
            DRW::DwgHandleWriteOccurrence rebased = token;
            rebased.startBit += baseBit;
            rebased.endBit += baseBit;
            frame.occurrences.push_back(
                {frame.objectHandle, stream, segment, segmentOrdinal++, rebased});
        }
    };

    append(m_objectBody, DRW::DwgObjectHandleStream::Data,
           DRW::DwgObjectSerializedSegment::Data, 0, dataOrdinal);
    if (stringsMergedIntoData) {
        append(m_objectStrings, DRW::DwgObjectHandleStream::Strings,
               DRW::DwgObjectSerializedSegment::Data, mergedStringBaseBit,
               dataOrdinal);
    } else if (!m_objectStrings.handleOccurrences().empty()) {
        valid = false;
    }
    append(m_objectHandles, DRW::DwgObjectHandleStream::Handles,
           handlesMergedIntoData ? DRW::DwgObjectSerializedSegment::Data
                                  : DRW::DwgObjectSerializedSegment::Handles,
           handlesMergedIntoData ? mergedHandleBaseBit : 0,
           handlesMergedIntoData ? dataOrdinal : handleOrdinal);

    if (!valid || frame.objectHandle == 0 || frame.version == DRW::UNKNOWNV
        || m_nextDwgObjectFrameGeneration == 0) {
        m_lastDwgObjectFrame = {};
        m_lastDwgObjectFrameValid = false;
        return false;
    }
    frame.generation = m_nextDwgObjectFrameGeneration++;
    frame.valid = true;
    m_lastDwgObjectFrame = std::move(frame);
    m_lastDwgObjectFrameValid = true;
    m_currentDwgObjectFrameProvenance = {};
    return true;
}

bool dwgWriter15::getLastDwgObjectHandleOccurrences(
    std::vector<DRW::DwgObjectHandleOccurrence>& occurrences) const {
    occurrences.clear();
    if (!m_lastDwgObjectFrameValid)
        return false;
    occurrences = m_lastDwgObjectFrame.occurrences;
    return true;
}

bool dwgWriter15::getLastDwgObjectFrame(
    DRW::DwgObjectFrameReceipt& frame) const {
    frame = {};
    if (!m_lastDwgObjectFrameValid)
        return false;
    frame = m_lastDwgObjectFrame;
    return true;
}

void dwgWriter15::configureTextCodec() {
    m_textCodec.setVersion(m_version, false);
    const char* codePage = dwgCodePageName(fileCodePageId());
    m_textCodec.setByteCodePage(codePage == nullptr ? "ANSI_1252" : codePage);
    m_buf.setDecoder(&m_textCodec);
    m_objectBody.setDecoder(&m_textCodec);
    m_objectStrings.setDecoder(&m_textCodec);
    m_objectHandles.setDecoder(&m_textCodec);
}

std::uint16_t dwgWriter15::fileCodePageId() const {
    if (m_header != nullptr) {
        const auto it = m_header->vars.find("$DWGCODEPAGE");
        if (it != m_header->vars.end() && it->second != nullptr
            && it->second->type() == DRW_Variant::STRING
            && it->second->content.s != nullptr
            && !it->second->content.s->empty()) {
            return dwgCodePageId(it->second->content.s->c_str());
        }
    }
    return dwgCodePageId(nullptr);
}

void dwgWriter15::finishObject() {
    if (m_legacyModelChainActive && isLegacyModelChainHandle(m_currentHandle)
        && !m_legacyPlannedFrameWriting) {
        m_legacyModelChainConflict = true;
        m_frameWriteError = true;
        return;
    }
    // For AC1015 non-entity objects the first handle-stream token is the
    // common owner (§20.1); some object codecs use hard ownership there.
    // A planned model-space entity cannot also own an unrelated object in
    // this bounded profile. Inspect only this owner role:
    // later GROUP member and other pointer roles remain valid.
    if (m_legacyModelChainActive && !isLegacyModelChainHandle(m_currentHandle)
        && !m_objectHandles.handleOccurrences().empty()) {
        const auto& owner = m_objectHandles.handleOccurrences().front();
        if ((owner.code == DRW::DwgSoftPointer
             || owner.code == DRW::DwgHardOwnership)
            && owner.reference <= std::numeric_limits<std::uint32_t>::max()
            && isLegacyModelChainHandle(static_cast<std::uint32_t>(owner.reference))) {
            m_legacyModelChainConflict = true;
            m_frameWriteError = true;
            return;
        }
    }
    // Reject a duplicate before writing another frame. The final HANDLES pass
    // retains its independent uniqueness validation as a defense-in-depth
    // check on the serialized object map.
    if (hasEmittedDwgObjectHandle(m_currentHandle)) {
        recordRejectedDwgHandle(m_currentHandle);
        m_frameWriteError = true;
        return;
    }

    const bool splitLegacyHandles =
        m_version > DRW::AC1014 && m_version < DRW::AC1021;
    const bool stagedLegacyHandles =
        splitLegacyHandles && !m_objectHandles.data().empty();

    if (!m_buf.isGood() || !m_objectBody.isGood() || !m_objectStrings.isGood()
        || !m_objectHandles.isGood()) {
        m_frameWriteError = true;
        return;
    }

    // The legacy object-size field is a signed modular short and the
    // patched RL is a 32-bit bit count. Reject an unrepresentable frame
    // before any narrowing conversion or frame publication.
    if (m_objectBody.size() > std::numeric_limits<std::uint32_t>::max() / 8u
        || m_objectHandles.size() > std::numeric_limits<std::uint32_t>::max()
        || m_buf.size() > std::numeric_limits<std::uint32_t>::max()) {
        m_frameWriteError = true;
        return;
    }
    const std::uint64_t dataBitCount64 = m_objectBody.bitCount();
    if (dataBitCount64 > std::numeric_limits<std::uint32_t>::max()) {
        m_frameWriteError = true;
        return;
    }
    const std::uint32_t dataBitCount =
        static_cast<std::uint32_t>(dataBitCount64);

    std::uint64_t mergedHandleBaseBit = 0;
    if (stagedLegacyHandles) {
        // AC1015/AC1018 use one on-disk stream, but objSize still marks the
        // start of its trailing handle stream. Keep the handle bytes in a
        // scratch buffer until that boundary has been patched.
        mergedHandleBaseBit = m_objectBody.bitCount();
        if (!m_objectBody.data().empty()) {
            const std::uint8_t bsCode = (m_objectBody.data()[0] >> 6) & 0x03;
            const size_t rlBitOffset =
                (bsCode == 0x01) ? 10 : (bsCode == 0x00) ? 18 : 2;
            m_objectBody.patchRawLong32AtBit(rlBitOffset, dataBitCount);
        }
        if (m_objectHandles.size()
            > std::numeric_limits<std::uint32_t>::max() - m_objectBody.size()) {
            m_frameWriteError = true;
            m_objectHandles.reset();
            return;
        }
        m_objectBody.putBytes(m_objectHandles.data().data(),
                              m_objectHandles.data().size());
    }

    // Back-patch the RL objSize ("size of object in bits, not including CRC")
    // at the correct stream-bit offset.  The offset equals the BS width for
    // the oType, which we detect from the first 2 bits of the body:
    //   code "01" → RC payload → 10-bit BS → RL at bit 10
    //   code "00" → RS payload → 18-bit BS → RL at bit 18
    //   code "10" or "11" → no payload → 2-bit BS → RL at bit 2
    std::uint32_t bitCount = 0;
    if (!stagedLegacyHandles) {
        if (m_objectBody.size() > std::numeric_limits<std::uint32_t>::max() / 8u) {
            m_frameWriteError = true;
            return;
        }
        bitCount = static_cast<std::uint32_t>(m_objectBody.size()) * 8;
        if (m_objectBody.bitPos() != 0)
            bitCount -= static_cast<std::uint32_t>(8 - m_objectBody.bitPos());
    }
    if (!stagedLegacyHandles && !m_objectBody.data().empty()) {
        std::uint8_t bsCode = (m_objectBody.data()[0] >> 6) & 0x03;
        size_t rlBitOffset = (bsCode == 0x01) ? 10 : (bsCode == 0x00) ? 18 : 2;
        m_objectBody.patchRawLong32AtBit(rlBitOffset, bitCount);
    }

    // Byte-align the body — the trailing CRC must start aligned and
    // the per-object frame size must be in whole bytes.
    m_objectBody.alignToByte();
    if (m_objectBody.size() > std::numeric_limits<std::uint32_t>::max()
        || m_objectBody.size()
               > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())
        || m_buf.size() > std::numeric_limits<std::uint32_t>::max()) {
        m_frameWriteError = true;
        return;
    }
    std::uint32_t bodyBytes = static_cast<std::uint32_t>(m_objectBody.size());

    std::uint32_t frameStart = static_cast<std::uint32_t>(m_buf.size());

    // MS objectSize = byte count of body (no CRC).  Per the master plan
    // interpretation (b): CRC follows immediately after the slurped body
    // and is not included in `size`.
    m_buf.putModularShort(static_cast<std::int32_t>(bodyBytes));
    if (!m_buf.isGood()) {
        m_frameWriteError = true;
        return;
    }
    size_t bodyStartOffset = m_buf.size();
    m_buf.putBytes(m_objectBody.data().data(), bodyBytes);

    // CRC covers MS prefix + body — spec §20.2: "The CRC includes the size
    // bytes."  frameStart was recorded before putModularShort.
    std::uint16_t crc = m_buf.crc16(0xC0C1, static_cast<size_t>(frameStart),
                              bodyStartOffset + bodyBytes);
    m_buf.putRawShort16(crc);

    if (m_buf.size() > std::numeric_limits<std::uint32_t>::max()) {
        m_frameWriteError = true;
        return;
    }

    if (!captureLastDwgObjectHandleOccurrences(
            0, mergedHandleBaseBit, false, stagedLegacyHandles)) {
        m_buf.truncate(frameStart);
        m_frameWriteError = true;
        m_currentHandle = 0;
        m_objectStrings.reset();
        m_objectHandles.reset();
        return;
    }
    if (!publishDwgObjectMapEntry(m_currentHandle, frameStart)) {
        m_buf.truncate(frameStart);
        m_frameWriteError = true;
        m_currentHandle = 0;
        m_objectStrings.reset();
        m_objectHandles.reset();
        m_lastDwgObjectFrame = {};
        m_lastDwgObjectFrameValid = false;
        m_currentDwgObjectFrameProvenance = {};
        return;
    }
    markDwgClassInstanceEmitted(m_currentHandle);
    m_currentHandle = 0;
    m_objectStrings.reset();
    m_objectHandles.reset();
}

bool dwgWriter15::writeDwgHandles() {
    if (m_legacyModelChainActive
        && (m_legacyModelChainConflict || !m_blockControlEmitted
            || m_modelSpaceEntityHandles.size() != m_legacyModelChain.size()))
        return false;
    if (!m_buf.isGood())
        return false;

    std::uint32_t maxHandle = 0;
    if (!validateEmittedHandleMap(maxHandle))
        return false;

    std::uint32_t base = 0;
    if (!objectBaseOffset(base))
        return false;

    size_t sectionStart = m_buf.size();
    dwgBufferW handles;

    if (m_objectMap.empty()) {
        // Empty object map: single terminator page of `RS_BE(2) + CRC16_BE`.
        // size_BE = 2 means "this page contains 2 bytes including the size word".
        std::uint16_t pageSize = 2;
        handles.putBERawShort16(pageSize);
        std::uint16_t crc = handles.crc16(0xC0C1, 0, handles.size());
        handles.putBERawShort16(crc);
    } else {
        // Sort by handle for monotonic UMC deltas.  Per the master plan
        // (LibreDWG insertion-sort at encode.c:2394-2406): in practice the
        // map is already monotonic because objects are emitted in handle
        // order, but the sort defends against future out-of-order emit.
        std::sort(m_objectMap.begin(), m_objectMap.end());
        for (size_t i = 1; i < m_objectMap.size(); ++i) {
            if (m_objectMap[i - 1].first == m_objectMap[i].first) {
                // Two objects were emitted with the same handle, so the
                // object map is ambiguous and the file would be unreadable.
                DRW_DBG("dwgWriter15::writeDwgHandles: duplicate handle ");
                DRW_DBGH(m_objectMap[i].first);
                DRW_DBG("\n");
                return false;
            }
        }

        // HANDLES stores section-relative offsets. Validate the subtraction
        // before emitting a page so an invalid writer state cannot wrap an
        // unsigned delta and leave a partial HANDLES section behind.
        if (std::any_of(m_objectMap.cbegin(), m_objectMap.cend(),
                        [base](const auto& entry) {
                            return entry.second < base;
                        })) {
            return false;
        }

        // Page-emit walk. Each page is bounded at <=2030 bytes of (size
        // field + entries) to leave 2 bytes for the trailing CRC under the
        // 2032-byte ODA spec limit.
        constexpr size_t kMaxPagePayloadBytes = 2030;

        size_t entryStart = 0;
        while (entryStart < m_objectMap.size()) {
            size_t pageStart = handles.size();
            // Reserve the size field; we know its width is 2 bytes (RS BE)
            // so we know the payload budget is kMaxPagePayloadBytes - 2.
            handles.putBERawShort16(0);  // patched after entries

            std::uint32_t prevHandle = 0;
            std::uint32_t prevOffset = 0;
            size_t entryEnd = entryStart;
            while (entryEnd < m_objectMap.size()) {
                std::uint32_t h = m_objectMap[entryEnd].first;
                std::uint32_t off = m_objectMap[entryEnd].second - base;

                // UMC + MC are each at most five bytes. If adding that
                // upper bound would exceed the page, close it now.
                if ((handles.size() - pageStart) + 10 > kMaxPagePayloadBytes)
                    break;

                handles.putUModularChar(h - prevHandle);
                const std::int64_t locationDelta =
                    static_cast<std::int64_t>(off)
                    - static_cast<std::int64_t>(prevOffset);
                handles.putModularChar(locationDelta);
                if (!handles.isGood())
                    return false;
                prevHandle = h;
                prevOffset = off;
                ++entryEnd;
            }

            // A valid entry must fit in an otherwise empty page. This guard
            // prevents an empty page loop if the encoding bound changes.
            if (entryEnd == entryStart)
                return false;

            // Patch the page size (BE) at pageStart. patchRawShort16 emits
            // LE, so write the BE bytes directly here.
            std::uint16_t pageSize = static_cast<std::uint16_t>(
                handles.size() - pageStart);
            handles.data()[pageStart] =
                static_cast<std::uint8_t>((pageSize >> 8) & 0xFF);
            handles.data()[pageStart + 1] =
                static_cast<std::uint8_t>(pageSize & 0xFF);

            // CRC16 BE over (size + entries).
            std::uint16_t crc = handles.crc16(
                0xC0C1, pageStart, handles.size());
            handles.putBERawShort16(crc);

            entryStart = entryEnd;
        }

        // Terminator page (2-byte size word + CRC of just the size).
        size_t termStart = handles.size();
        std::uint16_t termSize = 2;
        handles.putBERawShort16(termSize);
        std::uint16_t termCrc = handles.crc16(
            0xC0C1, termStart, handles.size());
        handles.putBERawShort16(termCrc);
    }

    std::uint64_t finalSize = 0;
    if (!handles.isGood() || handles.bitPos() != 0
        || handles.size() > std::numeric_limits<std::uint32_t>::max()
        || sectionStart > std::numeric_limits<std::uint32_t>::max()
        || !dwgSafety::add(static_cast<std::uint64_t>(sectionStart),
                           static_cast<std::uint64_t>(handles.size()),
                           finalSize)
        || finalSize > std::numeric_limits<std::uint32_t>::max())
        return false;
    m_buf.putBytes(handles.data().data(), handles.size());
    if (!m_buf.isGood())
        return false;
    m_sectionOffsets[recno::HANDLES] =
        static_cast<std::uint32_t>(sectionStart);
    m_sectionSizes[recno::HANDLES] =
        static_cast<std::uint32_t>(handles.size());
    return true;
}

bool dwgWriter15::validateEmittedHandleMap(std::uint32_t& maxHandle) const {
    maxHandle = 0;
    if (m_emittedDwgObjectHandles.size() != m_objectMap.size())
        return false;
    for (const auto& entry : m_objectMap) {
        if (entry.first == 0 || entry.second >= m_buf.size()
            || !hasEmittedDwgObjectHandle(entry.first))
            return false;
        maxHandle = std::max(maxHandle, entry.first);
    }
    return true;
}

bool dwgWriter15::validateDwgClassInstanceFrames() const {
    if (!m_rawClassInstanceHandles.empty()
        || m_dwgClassInstanceHandleOwners.size()
               != m_dwgClassInstanceEmittedHandles.size())
        return false;

    for (std::uint32_t handle : m_dwgClassInstanceEmittedHandles) {
        const auto owner = m_dwgClassInstanceHandleOwners.find(handle);
        if (owner == m_dwgClassInstanceHandleOwners.end())
            return false;
        if (!hasEmittedDwgObjectHandle(handle))
            return false;
    }
    return true;
}

bool dwgWriter15::writeSecondHeader() {
    if (m_version == DRW::AC1015) {
        std::uint16_t measurement = 0;
        if (!dwgTemplate::headerMeasurement(m_header, measurement))
            return false;
        const auto templateData = dwgTemplate::emptyDescription(measurement);
        m_sectionOffsets[recno::TEMPLATE] =
            static_cast<std::uint32_t>(m_buf.size());
        m_buf.putBytes(templateData.data(), templateData.size());
        m_sectionSizes[recno::TEMPLATE] =
            static_cast<std::uint32_t>(templateData.size());
    }

    const std::vector<std::uint8_t> auxHeader = buildR2000AuxHeaderContent(m_header);
    if (auxHeader.empty())
        return false;
    m_sectionOffsets[recno::AUXHEADER] = static_cast<std::uint32_t>(m_buf.size());
    m_buf.putBytes(auxHeader.data(), auxHeader.size());
    m_sectionSizes[recno::AUXHEADER] = static_cast<std::uint32_t>(auxHeader.size());
    return true;
}

bool dwgWriter15::finalize() {
    if (m_legacyModelChainActive
        && (m_legacyModelChainConflict || !m_blockControlEmitted
            || m_modelSpaceEntityHandles.size() != m_legacyModelChain.size()))
        return false;
    if (objectWriteFailed())
        return false;

    // Patch each section record in the file-header locator table.
    // Each record is 9 bytes: RC recno @ +0, RL address @ +1, RL size @ +5.
    for (std::uint8_t recno = 0; recno < m_numSections; ++recno) {
        auto offIt  = m_sectionOffsets.find(recno);
        auto sizeIt = m_sectionSizes.find(recno);
        std::uint32_t address = (offIt  != m_sectionOffsets.end()) ? offIt->second  : 0;
        std::uint32_t size    = (sizeIt != m_sectionSizes.end())   ? sizeIt->second : 0;
        size_t base = m_recordsOffset + static_cast<size_t>(recno) * 9;
        m_buf.patchRawLong32(base + 1, address);
        m_buf.patchRawLong32(base + 5, size);
    }

    // Recompute file-header CRC over bytes [0 .. m_recordsOffset + 9N)
    // with seed=0, then XOR'd by the per-count constant, then stored at
    // (m_recordsOffset + 9N).
    size_t crcOffset = m_recordsOffset + static_cast<size_t>(m_numSections) * 9;
    std::uint16_t crc = m_buf.crc16(0, 0, crcOffset);
    crc = static_cast<std::uint16_t>(crc ^ seedXorForCount(m_numSections));
    m_buf.patchRawShort16(crcOffset, crc);

    // Flush the accumulator to disk in one write().
    if (m_stream == nullptr || !m_stream->good()) return false;
    const std::vector<std::uint8_t>& bytes = m_buf.data();
    m_stream->write(reinterpret_cast<const char*>(bytes.data()),
                    static_cast<std::streamsize>(bytes.size()));
    return m_stream->good();
}
