// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1024_TABLESTYLE_WIRE_H
#define DX_AC1024_TABLESTYLE_WIRE_H

#include "dx_layerstates_dwg_wire.h"
#include <cstring>

// Exact aerial AC1024 source witness only; this does not establish import or
// class provenance, dependency publication, staged output, or TABLE rendering.
// Callers must recheck immutable same-import raw and typed baselines: numeric
// equality here is not a guarantee of identical floating-point encodings.
// ODA DWG v5.4.1 §§2.11-2.13, 20.1, 20.4.44, 20.4.101.2-.4. The observed
// repeated-cell index BL precedes each cell even though §20.4.101.2 omits it.
namespace dx_ac1024_tablestyle_wire {
namespace detail {
using dx_raster_wire::detail::Bits;
inline bool number(Bits& b, double expected) noexcept {
    const auto tag = b.take(2u);
    double value = 0.0;
    if (tag == 0u) {
        auto bits = b.little(8u);
        static_assert(sizeof(bits) == sizeof(value), "64-bit double required");
        std::memcpy(&value, &bits, sizeof(value));
    } else if (tag == 1u) {
        value = 1.0;
    } else if (tag != 2u) {
        return false;
    }
    return b.good && value == expected;
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
inline bool color(Bits& b, const DRW_TableStyleCmcColor& color, std::uint8_t method) noexcept {
    if (color.m_method != method || color.m_flags != 0u || color.m_reservedIndex != 0u
        || !color.m_name.empty() || !color.m_bookName.empty()
        || color.m_rgb != (method == 0xc8u ? 0 : -1)) {
        return false;
    }
    return b.shortValue() == 0u && b.longValue() == (static_cast<std::uint32_t>(method) << 24u)
           && b.take(8u) == 0u && b.good;
}
inline bool link(Bits& h, std::uint8_t role, std::uint8_t length, std::uint32_t value) noexcept {
    const auto v = dx_raster_wire::detail::handle(h, role, 0u);
    return h.good && v.role == role && v.length == length && v.value == value;
}
inline bool cell(Bits& b, Bits& textBits, Bits& h, const DRW_TableStyleCellStyle& c, bool table,
                 unsigned index) noexcept {
    const auto& f = c.m_contentFormat;
    const int alignment = table ? 1 : (index == 3u ? 2 : 5);
    const double height = index == 1u ? 0.25 : 0.18;
    const std::uint32_t style = table ? 0u : 0x11u;
    if (c.m_type != 5 || !c.m_hasData || c.m_propertyOverrideFlags != 0u
        || c.m_mergeFlags != (index == 1u ? 0x8000u : 0u) || c.m_backgroundColor != 256
        || c.m_contentLayout != 1u || f.m_propertyOverrideFlags != 0u || f.m_propertyFlags != 0u
        || f.m_valueDataType != 512 || f.m_valueUnitType != 0 || !f.m_valueFormatString.empty()
        || f.m_rotation != 0.0 || f.m_blockScale != 1.0 || f.m_cellAlignment != alignment
        || f.m_contentColor != 0 || f.m_textStyleHandle != style || f.m_textHeight != height
        || c.m_marginOverrideFlags != 1 || c.m_verticalMargin != 0.06 || c.m_horizontalMargin != 0.06
        || c.m_bottomMargin != 0.06 || c.m_rightMargin != 0.06
        || c.m_marginHorizontalSpacing != (table ? 0.18 : 0.06)
        || c.m_marginVerticalSpacing != (table ? 0.18 : 0.06) || c.m_borders.size() != (table ? 0u : 6u)) {
        return false;
    }
    if (b.longValue() != 5u || b.shortValue() != 1u || b.longValue() != 0u
        || b.longValue() != (index == 1u ? 0x8000u : 0u) || !color(b, c.m_backgroundColorCmc, 0xc8u)
        || b.longValue() != 1u || b.longValue() != 0u || b.longValue() != 0u || b.longValue() != 512u
        || b.longValue() != 0u || !text(textBits, f.m_valueFormatString) || !number(b, 0.0)
        || !number(b, 1.0) || b.longValue() != static_cast<unsigned>(alignment)
        || !color(b, f.m_contentColorCmc, 0xc1u) || !link(h, 5u, table ? 0u : 1u, style)
        || !number(b, height) || b.shortValue() != 1u || !number(b, 0.06) || !number(b, 0.06)
        || !number(b, 0.06) || !number(b, 0.06) || !number(b, table ? 0.18 : 0.06)
        || !number(b, table ? 0.18 : 0.06) || b.longValue() != (table ? 0u : 6u) || !b.good
        || !textBits.good) {
        return false;
    }
    for (unsigned i = 0; i < c.m_borders.size(); ++i) {
        const auto& x = c.m_borders[i];
        if (x.m_edgeFlags != static_cast<int>(1u << i) || x.m_propertyOverrideFlags != 0
            || x.m_borderType != 1 || x.m_color != 0 || x.m_lineWeight != -2 || x.m_lineTypeHandle != 0u
            || x.m_visible != 0 || x.m_doubleLineSpacing != 0.045 || b.longValue() != (1u << i)
            || b.longValue() != 0u || b.longValue() != 1u || !color(b, x.m_colorCmc, 0xc1u)
            || b.longValue() != 0xfffffffeu || !link(h, 5u, 0u, 0u) || b.longValue() != 0u
            || !number(b, 0.045)) {
            return false;
        }
    }
    return b.good && h.good;
}
} // namespace detail
inline bool observeStyle(const DRW_UnsupportedObject& raw, const DRW_TableStyle& s) noexcept {
    if (s.tType != DRW::TABLESTYLE || s.handle != 0x87u || s.parentHandle != 0x86u || !s.name.empty()
        || s.flags != 0u || s.reactorCount() != 1 || s.reactorHandles.size() != 1u
        || s.reactorHandles[0] != 0x86u || s.extensionDictionaryFlag() != 0u || s.xDictHandle != 0x238u
        || s.hasDataStorageBinaryData() || s.hasDwgCommonLinkTail() || !s.appData.empty()
        || !s.extData.empty() || !s.pendingAppIdResolutions.empty() || !s.pendingLayerRefResolutions.empty()
        || s.m_name != "Standard" || s.m_flowDirection != 0 || s.m_flags != 0
        || s.m_horizontalCellMargin != 0.0 || s.m_verticalCellMargin != 0.0 || s.m_titleSuppressed
        || s.m_headerSuppressed || !s.m_r24PrefixPresent || s.m_r24ClassVersion != 0u
        || s.m_r24UnknownValue1 != 8 || s.m_r24UnknownValue2 != 101 || s.m_unknownHandle != 0u
        || !s.m_rowStyles.empty() || s.m_cellStyles.size() != 3u || s.m_tableCellStyle.m_id != 4
        || s.m_tableCellStyle.m_styleClass != 2 || s.m_tableCellStyle.m_name != "Table"
        || raw.m_version != DRW::AC1024 || raw.m_objectType != 504u || raw.m_handle != s.handle
        || raw.m_isEntity || !raw.m_isCustomClass || !raw.m_hasClassDefinition
        || raw.m_recordName != "TABLESTYLE" || raw.m_className != "AcDbTableStyle"
        || raw.m_classAppName != "ObjectDBX Classes" || raw.m_parentHandle != 0x86u
        || raw.m_numReactors != 1 || raw.m_reactorHandles != s.reactorHandles || raw.m_xDictFlag != 0u
        || raw.m_xDictHandle != 0x238u || raw.m_hasDataStorage || !raw.m_commonHandleDataValidated
        || !drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence) || raw.m_objectSize != 837u
        || raw.m_rawBytes.size() != 837u || raw.m_bodyBitSize != 258u) {
        return false;
    }
    detail::Bits b{raw.m_rawBytes.data(), 0u, 5867u};
    if (b.take(2u) != 1u || b.take(8u) + 0x1f0u != 504u) {
        return false;
    }
    const auto self = dx_raster_wire::detail::handle(b, 0u, 0u, true);
    if (self.role != 0u || self.length != 1u || self.value != s.handle || b.shortValue() != 0u
        || b.longValue() != 1u || b.take(1u) != 0u || b.take(8u) != 0u) {
        return false;
    }
    detail::Bits strings{raw.m_rawBytes.data(), 5867u, 6421u};
    if (!detail::text(strings, s.m_name) || b.longValue() != 8u || b.longValue() != 101u) {
        return false;
    }
    detail::Bits h{raw.m_rawBytes.data(), 6438u, 6696u};
    const auto owner = dx_raster_wire::detail::handle(h, 4u, s.handle);
    if (!h.good || owner.role != 8u || owner.length != 0u || owner.value != 0x86u
        || !detail::link(h, 4u, 1u, 0x86u) || !detail::link(h, 3u, 2u, 0x238u)
        || !detail::link(h, 3u, 0u, 0u) || !detail::cell(b, strings, h, s.m_tableCellStyle, true, 4u)
        || b.longValue() != 4u || b.longValue() != 2u || !detail::text(strings, s.m_tableCellStyle.m_name)
        || b.longValue() != 3u) {
        return false;
    }
    constexpr const char* names[]{"_TITLE", "_HEADER", "_DATA"};
    constexpr int classes[]{1, 1, 2};
    for (unsigned i = 0; i < 3u; ++i) {
        const auto& c = s.m_cellStyles[i];
        const auto n = i + 1u;
        if (c.m_id != static_cast<int>(n) || c.m_styleClass != classes[i] || c.m_name != names[i]
            || b.longValue() != n || !detail::cell(b, strings, h, c, false, n) || b.longValue() != n
            || b.longValue() != static_cast<unsigned>(classes[i]) || !detail::text(strings, c.m_name)) {
            return false;
        }
    }
    detail::Bits footer{raw.m_rawBytes.data(), 6421u, 6438u};
    return b.good && b.pos == b.end && strings.good && strings.pos == strings.end
           && footer.little(2u) == 554u && footer.take(1u) == 1u && footer.good && h.take(2u) == 3u
           && h.good && h.pos == h.end;
}
inline bool observeDictionary(const DRW_UnsupportedObject& raw, const DRW_Dictionary& s) noexcept {
    using dx_layerstates_dwg_wire::LinkKind;
    const bool root = s.handle == 0x86u;
    if (!root && s.handle != 0x238u) {
        return false;
    }
    const auto owner = root ? 0xcu : 0x87u;
    if (s.tType != DRW::DICTIONARY || s.parentHandle != owner || !s.name.empty() || s.flags != 0u
        || s.reactorCount() != (root ? 1 : 0) || s.reactorHandles.size() != (root ? 1u : 0u)
        || (root && s.reactorHandles[0] != owner) || s.extensionDictionaryFlag() != 1u
        || s.xDictHandle != 0u || s.hasDataStorageBinaryData() || !s.appData.empty() || !s.extData.empty()
        || !s.pendingAppIdResolutions.empty() || !s.pendingLayerRefResolutions.empty() || s.countCap
        || !s.hasCompleteDwgEntries() || !s.hasDwgCommonLinkTail() || s.cloning != 1
        || s.hardOwner != (root ? 0 : 1) || s.m_entries.size() != (root ? 1u : 0u)
        || (root
            && (s.m_entries[0].m_name != "Standard" || s.m_entries[0].m_handle != 0x87u
                || s.m_entries[0].m_dxfHandleCode != DRW_Dictionary::Entry::DxfHandleCode::Unspecified))
        || raw.m_objectSize != (root ? 34u : 10u) || raw.m_rawBytes.size() != raw.m_objectSize
        || raw.m_bodyBitSize != (root ? 50u : 20u)
        || (raw.m_rawBytes.back() & (root ? 3u : 15u)) != (root ? 3u : 15u)) {
        return false;
    }
    return dx_layerstates_dwg_wire::observeDictionary(
        raw, s, [](std::uint32_t, const std::string&) noexcept { return false; },
        [root](LinkKind kind, std::size_t index, std::uint8_t role, std::uint8_t length) noexcept {
            if (index != 0u) {
                return false;
            }
            if (kind == LinkKind::Owner || kind == LinkKind::Reactor) {
                return (kind == LinkKind::Owner || root) && role == 4u && length == 1u;
            }
            return root && kind == LinkKind::Item && role == 2u && length == 1u;
        });
}
} // namespace dx_ac1024_tablestyle_wire
#endif
