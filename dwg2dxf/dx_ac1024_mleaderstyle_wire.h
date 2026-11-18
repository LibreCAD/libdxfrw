// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1024_MLEADERSTYLE_WIRE_H
#define DX_AC1024_MLEADERSTYLE_WIRE_H

#include "dx_layerstates_dwg_wire.h"
#include <cstring>

// Exact observed aerial AC1024 MLEADERSTYLE graph wire. This source-only
// observer does not prove same-import provenance, dependency publication,
// staged output, or native MLEADER rendering semantics. The adapter must
// recheck both raw frame and typed model against immutable import baselines.
// ODA DWG v5.4.1 sections 2.11-2.13, 20.1, 20.4.44 and 20.4.87.
namespace dx_ac1024_mleaderstyle_wire {
namespace detail {
using dx_raster_wire::detail::Bits;

inline bool number(Bits& bits, double expected) noexcept {
    const auto tag = bits.take(2u);
    double value = 0.0;
    if (tag == 0u) {
        const auto encoded = bits.little(8u);
        static_assert(sizeof(encoded) == sizeof(value), "64-bit double required");
        std::memcpy(&value, &encoded, sizeof(value));
    } else if (tag == 1u) {
        value = 1.0;
    } else if (tag != 2u) {
        return false;
    }
    return bits.good && value == expected;
}

inline bool color(Bits& b) noexcept {
    return b.shortValue() == 0u && b.longValue() == 0xc1000000u && b.take(8u) == 0u && b.good;
}
inline bool text(Bits& b, const std::string& value) noexcept {
    if (b.shortValue() != value.size()) {
        return false;
    }
    for (unsigned char c : value) {
        if (c > 127u || b.little(2u) != c) {
            return false;
        }
    }
    return b.good;
}
inline bool typedHandle(const dwgHandle& h, std::uint8_t size, std::uint32_t ref) noexcept {
    return h.code == 5u && h.rawCode == 5u && h.size == size && h.ref == ref && h.ref64 == ref;
}
} // namespace detail

inline bool observeStyle(const DRW_UnsupportedObject& raw, const DRW_MLeaderStyle& s) noexcept {
    constexpr std::uint32_t scalarEnd = 739u;
    constexpr std::uint32_t textEnd = 879u;
    constexpr std::uint32_t handleStart = 896u;
    const bool standard = s.handle == 0x148u;
    if (!standard && s.handle != 0x157u) {
        return false;
    }
    const std::size_t size = standard ? 122u : 123u;
    if (s.tType != DRW::MLEADERSTYLE || s.parentHandle != 0x147u || !s.name.empty() || s.flags != 0u
        || s.reactorCount() != 1 || s.reactorHandles.size() != 1u || s.reactorHandles[0] != 0x147u
        || s.extensionDictionaryFlag() != 1u || s.xDictHandle != 0u || s.hasDataStorageBinaryData()
        || s.hasDwgCommonLinkTail() || !s.appData.empty() || !s.extData.empty()
        || !s.pendingAppIdResolutions.empty() || !s.pendingLayerRefResolutions.empty() || s.styleVersion != 2u
        || s.contentType != 2u || s.drawMLeaderOrder != 1u || s.drawLeaderOrder != 0u
        || s.maxLeaderPoints != 2 || s.firstSegmentAngle != 0.0 || s.secondSegmentAngle != 0.0
        || s.leaderType != 1u || s.leaderColor != 0 || s.leaderColorRgb != -1 || !s.leaderColorName.empty()
        || !s.leaderColorBookName.empty() || s.leaderColorDwgMethod != 0xc1u || s.leaderColorDwgFlags != 0u
        || s.leaderColorDwgIndex != 0u || !s.leaderColorDwgMetadataPresent || s.leaderLineWeight != -2
        || !s.landingEnabled || s.landingGap != 0.09 || !s.autoIncludeLanding || s.landingDistance != 0.36
        || s.description != "Standard" || s.arrowHeadSize != 0.18 || !s.textDefault.empty()
        || s.leftAttachment != 1u || s.rightAttachment != 1u || s.textAngleType != 1u
        || s.textAlignmentType != 0u || s.textColor != 0 || s.textColorRgb != -1 || !s.textColorName.empty()
        || !s.textColorBookName.empty() || s.textColorDwgMethod != 0xc1u || s.textColorDwgFlags != 0u
        || s.textColorDwgIndex != 0u || !s.textColorDwgMetadataPresent || s.textHeight != 0.18
        || s.textFrameEnabled || s.alwaysAlignTextLeft || s.alignSpace != 0.18 || s.blockColor != 0
        || s.blockColorRgb != -1 || !s.blockColorName.empty() || !s.blockColorBookName.empty()
        || s.blockColorDwgMethod != 0xc1u || s.blockColorDwgFlags != 0u || s.blockColorDwgIndex != 0u
        || !s.blockColorDwgMetadataPresent || s.blockScale.x != 1.0 || s.blockScale.y != 1.0
        || s.blockScale.z != 1.0 || !s.blockScaleEnabled || s.blockRotation != 0.0 || !s.blockRotationEnabled
        || s.blockConnectionType != 0u || s.scaleFactor != 1.0 || s.propertyChanged != !standard
        || !s.isAnnotative || s.breakSize != 0.125 || s.attachmentDirection != 0u || s.topAttachment != 9u
        || s.bottomAttachment != 9u || s.textExtended
        || !detail::typedHandle(s.leaderLineTypeHandle, 1u, 0x14u)
        || !detail::typedHandle(s.arrowHeadBlockHandle, 0u, 0u)
        || !detail::typedHandle(s.textStyleHandle, 1u, 0x11u) || !detail::typedHandle(s.blockHandle, 0u, 0u)
        || raw.m_version != DRW::AC1024 || raw.m_objectType != 513u || raw.m_handle != s.handle
        || raw.m_isEntity || !raw.m_isCustomClass || !raw.m_hasClassDefinition
        || raw.m_recordName != "MLEADERSTYLE" || raw.m_className != "AcDbMLeaderStyle"
        || raw.m_classAppName != "ACDB_MLEADERSTYLE_CLASS" || raw.m_parentHandle != 0x147u
        || raw.m_numReactors != 1 || raw.m_reactorHandles != s.reactorHandles || raw.m_xDictFlag != 1u
        || raw.m_xDictHandle != 0u || raw.m_hasDataStorage || !raw.m_commonHandleDataValidated
        || !drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence) || raw.m_objectSize != size
        || raw.m_rawBytes.size() != size || raw.m_bodyBitSize != size * 8u - handleStart) {
        return false;
    }
    detail::Bits b{raw.m_rawBytes.data(), 0u, scalarEnd};
    if (b.take(2u) != 1u || b.take(8u) + 0x1f0u != 513u) {
        return false;
    }
    const auto self = dx_raster_wire::detail::handle(b, 0u, 0u, true);
    if (self.role != 0u || self.length != 2u || self.value != s.handle || b.shortValue() != 0u
        || b.longValue() != 1u || b.take(1u) != 1u || b.shortValue() != 2u || b.shortValue() != 2u
        || b.shortValue() != 1u || b.shortValue() != 0u || b.longValue() != 2u || !detail::number(b, 0.0)
        || !detail::number(b, 0.0) || b.shortValue() != 1u || !detail::color(b)
        || b.longValue() != 0xfffffffeu || b.take(1u) != 1u || !detail::number(b, 0.09) || b.take(1u) != 1u
        || !detail::number(b, 0.36) || !detail::number(b, 0.18) || b.shortValue() != 1u
        || b.shortValue() != 1u || b.shortValue() != 1u || b.shortValue() != 0u || !detail::color(b)
        || !detail::number(b, 0.18) || b.take(1u) != 0u || b.take(1u) != 0u || !detail::number(b, 0.18)
        || !detail::color(b) || !detail::number(b, 1.0) || !detail::number(b, 1.0) || !detail::number(b, 1.0)
        || b.take(1u) != 1u || !detail::number(b, 0.0) || b.take(1u) != 1u || b.shortValue() != 0u
        || !detail::number(b, 1.0) || b.take(1u) != (standard ? 0u : 1u) || b.take(1u) != 1u
        || !detail::number(b, 0.125) || b.shortValue() != 0u || b.shortValue() != 9u || b.shortValue() != 9u
        || !b.good || b.pos != b.end) {
        return false;
    }
    detail::Bits text{raw.m_rawBytes.data(), scalarEnd, textEnd};
    if (!detail::text(text, s.description) || !detail::text(text, s.textDefault) || !text.good
        || text.pos != text.end) {
        return false;
    }
    detail::Bits footer{raw.m_rawBytes.data(), textEnd, handleStart};
    if (footer.little(2u) != textEnd - scalarEnd || footer.take(1u) != 1u || !footer.good) {
        return false;
    }
    detail::Bits h{raw.m_rawBytes.data(), handleStart, static_cast<std::uint32_t>(size * 8u)};
    const auto owner = dx_raster_wire::detail::handle(h, 4u, s.handle);
    const auto reactor = dx_raster_wire::detail::handle(h, 4u, s.handle);
    const auto line = dx_raster_wire::detail::handle(h, 5u, s.handle);
    const auto arrow = dx_raster_wire::detail::handle(h, 5u, s.handle);
    const auto style = dx_raster_wire::detail::handle(h, 5u, s.handle);
    const auto block = dx_raster_wire::detail::handle(h, 5u, s.handle);
    return h.good && owner.role == (standard ? 8u : 12u) && owner.length == (standard ? 0u : 1u)
           && owner.value == 0x147u && reactor.role == 4u && reactor.length == 2u && reactor.value == 0x147u
           && line.role == 5u && line.length == 1u && line.value == 0x14u && arrow.role == 5u
           && arrow.length == 0u && arrow.value == 0u && style.role == 5u && style.length == 1u
           && style.value == 0x11u && block.role == 5u && block.length == 0u && block.value == 0u
           && h.pos == h.end;
}
inline bool observeDictionary(const DRW_UnsupportedObject& raw, const DRW_Dictionary& s) noexcept {
    using dx_layerstates_dwg_wire::LinkKind;
    if (s.tType != DRW::DICTIONARY || s.handle != 0x147u || s.parentHandle != 0xcu || !s.name.empty()
        || s.flags != 0u || s.reactorCount() != 1 || s.reactorHandles.size() != 1u
        || s.reactorHandles[0] != 0xcu || s.extensionDictionaryFlag() != 1u || s.xDictHandle != 0u
        || s.hasDataStorageBinaryData() || !s.appData.empty() || !s.extData.empty()
        || !s.pendingAppIdResolutions.empty() || !s.pendingLayerRefResolutions.empty() || s.countCap
        || !s.hasCompleteDwgEntries() || !s.hasDwgCommonLinkTail() || s.cloning != 1 || s.hardOwner != 0
        || s.m_entries.size() != 2u || s.m_entries[0].m_name != "Annotative"
        || s.m_entries[0].m_handle != 0x157u || s.m_entries[1].m_name != "Standard"
        || s.m_entries[1].m_handle != 0x148u
        || s.m_entries[0].m_dxfHandleCode != DRW_Dictionary::Entry::DxfHandleCode::Unspecified
        || s.m_entries[1].m_dxfHandleCode != DRW_Dictionary::Entry::DxfHandleCode::Unspecified
        || raw.m_objectSize != 60u || raw.m_rawBytes.size() != 60u || raw.m_bodyBitSize != 80u) {
        return false;
    }
    return dx_layerstates_dwg_wire::observeDictionary(
        raw, s, [](std::uint32_t, const std::string&) noexcept { return false; },
        [](LinkKind kind, std::size_t index, std::uint8_t role, std::uint8_t length) noexcept {
            if (kind == LinkKind::Owner || kind == LinkKind::Reactor) {
                return index == 0u && role == 4u && length == 1u;
            }
            return kind == LinkKind::Item && index < 2u && role == 2u && length == 2u;
        });
}
} // namespace dx_ac1024_mleaderstyle_wire
#endif
