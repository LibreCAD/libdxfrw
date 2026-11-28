// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1024_VISUALSTYLE_WIRE_H
#define DX_AC1024_VISUALSTYLE_WIRE_H

#include "dx_layerstates_dwg_wire.h"
#include <cstring>

// Source-only observer for the 27 aerial AC1024 VISUALSTYLE records and
// their dictionary 0x99. Callers must bind unchanged raw and typed models
// to the same completed import, prove graph dependencies and staged output.
// ODA DWG v5.4.1 specifies the common encoding and DICTIONARY layout, but
// not the complete VISUALSTYLE body. The latter and the exact roles here are
// constrained to the observed aerial frames and independently screened peer.
namespace dx_ac1024_visualstyle_wire {
namespace detail {
using dx_raster_wire::detail::Bits;

inline std::size_t witnessedSize(std::uint32_t handle) noexcept {
    switch (handle) {
    case 0x9a:
        return 118;
    case 0x9b:
        return 138;
    case 0x9c:
        return 125;
    case 0x9d:
        return 144;
    case 0x9e:
        return 120;
    case 0x9f:
        return 132;
    case 0xa0:
        return 132;
    case 0xa1:
        return 137;
    case 0xa2:
        return 140;
    case 0xa3:
        return 129;
    case 0xa4:
        return 126;
    case 0xa5:
        return 136;
    case 0xa6:
        return 126;
    case 0xa8:
        return 134;
    case 0xa9:
        return 134;
    case 0xaa:
        return 134;
    case 0x19a:
        return 114;
    case 0x281:
        return 128;
    case 0x282:
        return 132;
    case 0x283:
        return 149;
    case 0x284:
        return 135;
    case 0x285:
        return 123;
    case 0x286:
        return 148;
    case 0x287:
        return 125;
    case 0x288:
        return 103;
    case 0x289:
        return 107;
    case 0x28a:
        return 109;
    default:
        return 0;
    }
}
inline bool number(Bits& b, double expected) noexcept {
    const auto tag = b.take(2u);
    double value = 0.0;
    if (tag == 0u) {
        const auto bits = b.little(8u);
        static_assert(sizeof(bits) == sizeof(value), "64-bit double required");
        std::memcpy(&value, &bits, sizeof(value));
    } else if (tag == 1u) {
        value = 1.0;
    } else if (tag != 2u) {
        return false;
    }
    return b.good && std::memcmp(&value, &expected, sizeof(value)) == 0;
}
inline bool text(Bits& b, const std::string& value) noexcept {
    if (value.size() > 256u || b.shortValue() != value.size()) {
        return false;
    }
    for (unsigned char c : value) {
        if (c > 127u || b.little(2u) != c) {
            return false;
        }
    }
    return b.good;
}
inline bool color(Bits& b, const DRW_VisualStyleColor& c, int scalar) noexcept {
    const bool rgb = c.rawRgb == 0xc2ffffffu || c.rawRgb == 0xc2787878u || c.rawRgb == 0xc2808080u;
    const bool indexed = c.rawRgb == 0xc3000007u;
    const bool none = c.rawRgb == 0xc8000000u;
    const int expectedScalar = indexed ? 7 : 256;
    return (rgb || indexed || none) && c.present && c.modern && c.rawIndex == 0u && c.cmcIndex == 0u
           && c.flags == 0u && c.name.empty() && c.bookName.empty() && c.normalizedIndex == scalar
           && scalar == expectedScalar && b.shortValue() == c.rawIndex && b.longValue() == c.rawRgb
           && b.take(8u) == c.flags && b.good;
}
} // namespace detail

inline bool observeStyle(const DRW_UnsupportedObject& raw, const DRW_VisualStyle& style) noexcept {
    const std::size_t size = detail::witnessedSize(style.handle);
    const auto& m = style.m_body;
    const auto& e = m.m_encoding;
    if (!size || style.tType != DRW::VISUALSTYLE || style.parentHandle != 0x99u || !style.name.empty()
        || style.flags != 0u || style.reactorCount() != 1 || style.reactorHandles.size() != 1u
        || style.reactorHandles[0] != 0x99u || style.extensionDictionaryFlag() != 1u
        || style.xDictHandle != 0u || style.hasDataStorageBinaryData() || style.hasDwgCommonLinkTail()
        || !style.appData.empty() || !style.extData.empty() || !style.pendingAppIdResolutions.empty()
        || !style.pendingLayerRefResolutions.empty() || !style.m_bodyDecoded || m.hasR2013bExpansion
        || e.companionCount != 28u || e.colorCount != 5u || !e.hasSourceIsolines
        || e.sourceIsolines != m.edgeIsolines || e.sourceIsolines < 0
        || e.sourceIsolines > DRW_VisualStyleBody::kMaxEdgeIsolines || raw.m_version != DRW::AC1024
        || raw.m_objectType != 506u || raw.m_handle != style.handle || raw.m_isEntity
        || !raw.m_isCustomClass || !raw.m_hasClassDefinition || raw.m_recordName != "VISUALSTYLE"
        || raw.m_className != "AcDbVisualStyle" || raw.m_classAppName != "ObjectDBX Classes"
        || raw.m_parentHandle != 0x99u || raw.m_numReactors != 1
        || raw.m_reactorHandles != style.reactorHandles || raw.m_xDictFlag != 1u || raw.m_xDictHandle != 0u
        || raw.m_hasDataStorage || !raw.m_commonHandleDataValidated
        || !drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence) || raw.m_objectSize != size
        || raw.m_rawBytes.size() != size || raw.m_bodyBitSize != (style.handle == 0x9au ? 24u : 32u)) {
        return false;
    }
    for (std::size_t i = 5u; i < e.colors.size(); ++i) {
        if (e.colors[i].present) {
            return false;
        }
    }
    const auto total = static_cast<std::uint32_t>(size * 8u);
    const auto handleStart = total - raw.m_bodyBitSize;
    if (handleStart < 17u) {
        return false;
    }
    detail::Bits footer{raw.m_rawBytes.data(), handleStart - 17u, handleStart};
    const auto stringBits = static_cast<std::uint32_t>(footer.little(2u));
    if (!footer.good || footer.take(1u) != 1u || (stringBits & 0x8000u) || stringBits > handleStart - 17u) {
        return false;
    }
    const auto stringStart = handleStart - 17u - stringBits;
    detail::Bits b{raw.m_rawBytes.data(), 0u, stringStart};
    if (b.take(2u) != 1u || b.take(8u) + 0x1f0u != 506u) {
        return false;
    }
    const auto self = dx_raster_wire::detail::handle(b, 0u, 0u, true);
    if (!b.good || self.role != 0u || self.value != style.handle
        || self.length != (style.handle < 0x100u ? 1u : 2u) || b.shortValue() != 0u || b.longValue() != 1u
        || b.take(1u) != 1u || b.longValue() != static_cast<std::uint32_t>(style.type)
        || b.shortValue() != static_cast<std::uint32_t>(m.extLightingModel)
        || b.take(1u) != static_cast<unsigned>(m.internalOnly)) {
        return false;
    }
    std::size_t slot = 0u, colorIndex = 0u;
    const auto companion = [&]() noexcept {
        return slot < e.companionCount && b.shortValue() == e.companions[slot++] && b.good;
    };
    const auto integer = [&](int value) noexcept {
        return b.longValue() == static_cast<std::uint32_t>(value) && companion();
    };
    const auto decimal = [&](double value) noexcept { return detail::number(b, value) && companion(); };
    const auto color = [&](int value) noexcept {
        return colorIndex < e.colorCount && detail::color(b, e.colors[colorIndex++], value) && companion();
    };
    if (!integer(m.faceLightingModel) || !integer(m.faceLightingQuality) || !integer(m.faceColorMode)
        || b.shortValue() != static_cast<std::uint32_t>(m.faceModifier) || !companion()
        || !decimal(m.faceOpacity) || !decimal(m.faceSpecular) || !color(m.faceMonoColor)
        || !integer(m.edgeModel) || !integer(m.edgeStyle) || !color(m.edgeIntersectionColor)
        || !color(m.edgeObscuredColor) || !integer(m.edgeObscuredLtype) || !integer(m.edgeIntersectionLtype)
        || !decimal(m.edgeCreaseAngle) || !integer(m.edgeModifier) || !color(m.edgeColor)
        || !decimal(m.edgeOpacity) || !integer(m.edgeWidth) || !integer(m.edgeOverhang)
        || !integer(m.edgeJitter) || !color(m.edgeSilhouetteColor) || !integer(m.edgeSilhouetteWidth)
        || !integer(m.edgeHaloGap) || !integer(e.sourceIsolines)
        || b.take(1u) != static_cast<unsigned>(m.edgeDoHidePrecision) || !companion()
        || !integer(m.displaySettings) || !decimal(m.displayBrightness) || !integer(m.displayShadowType)
        || slot != 28u || colorIndex != 5u || !b.good || b.pos != b.end) {
        return false;
    }
    detail::Bits strings{raw.m_rawBytes.data(), stringStart, handleStart - 17u};
    if (!detail::text(strings, style.desc) || strings.pos != strings.end) {
        return false;
    }
    detail::Bits h{raw.m_rawBytes.data(), handleStart, total};
    const auto owner = dx_raster_wire::detail::handle(h, 4u, style.handle);
    const auto reactor = dx_raster_wire::detail::handle(h, 4u, style.handle);
    const std::uint8_t ownerRole = style.handle == 0x9au ? 8u : style.handle < 0x100u ? 12u : 4u;
    return h.good && owner.role == ownerRole && owner.length == (ownerRole == 8u ? 0u : 1u)
           && owner.value == 0x99u && reactor.role == 4u && reactor.length == 1u && reactor.value == 0x99u
           && h.pos == h.end;
}

inline bool observeDictionary(const DRW_UnsupportedObject& raw, const DRW_Dictionary& source) noexcept {
    using dx_layerstates_dwg_wire::LinkKind;
    static constexpr std::uint32_t handles[]
        = {0x19a, 0x9f,  0xa1,  0xa0, 0x9e,  0xa5, 0xaa,  0xa2,  0xa4,  0x28a, 0xa9, 0x9a,  0x9b, 0x9c,
           0x9d,  0x282, 0x288, 0xa8, 0x289, 0xa3, 0x287, 0x286, 0x283, 0x284, 0xa6, 0x281, 0x285};
    static constexpr const char* names[] = {"*A1",
                                            "2dWireframe",
                                            "3D Hidden",
                                            "3dWireframe",
                                            "Basic",
                                            "Brighten",
                                            "ColorChange",
                                            "Conceptual",
                                            "Dim",
                                            "EdgeColorOff",
                                            "Facepattern",
                                            "Flat",
                                            "FlatWithEdges",
                                            "Gouraud",
                                            "GouraudWithEdges",
                                            "Hidden",
                                            "JitterOff",
                                            "Linepattern",
                                            "OverhangOff",
                                            "Realistic",
                                            "Shaded",
                                            "Shaded with edges",
                                            "Shades of Gray",
                                            "Sketchy",
                                            "Thicken",
                                            "Wireframe",
                                            "X-Ray"};
    if (source.handle != 0x99u || source.parentHandle != 0xcu || !source.name.empty() || source.flags != 0u
        || source.reactorCount() != 1 || source.reactorHandles.size() != 1u
        || source.reactorHandles[0] != 0xcu || source.extensionDictionaryFlag() != 1u
        || source.xDictHandle != 0u || source.hasDataStorageBinaryData() || !source.appData.empty()
        || !source.extData.empty() || !source.pendingAppIdResolutions.empty()
        || !source.pendingLayerRefResolutions.empty() || source.countCap || !source.hasCompleteDwgEntries()
        || !source.hasDwgCommonLinkTail() || source.hardOwner != 0 || source.cloning != 1
        || source.m_entries.size() != 27u || raw.m_objectSize != 604u || raw.m_rawBytes.size() != 604u
        || raw.m_bodyBitSize != 558u || (raw.m_rawBytes.back() & 0x3fu) != 0x3fu) {
        return false;
    }
    for (std::size_t i = 0u; i < 27u; ++i) {
        if (source.m_entries[i].m_name != names[i] || source.m_entries[i].m_handle != handles[i]
            || source.m_entries[i].m_dxfHandleCode != DRW_Dictionary::Entry::DxfHandleCode::Unspecified) {
            return false;
        }
    }
    return dx_layerstates_dwg_wire::observeDictionary(
        raw, source, [](std::uint32_t, const std::string&) noexcept { return false; },
        [](LinkKind kind, std::size_t index, std::uint8_t role, std::uint8_t length) noexcept {
            if (kind == LinkKind::Owner || kind == LinkKind::Reactor) {
                return index == 0u && role == 4u && length == 1u;
            }
            return kind == LinkKind::Item && index < 27u && role == 2u
                   && length == (handles[index] < 0x100u ? 1u : 2u);
        });
}
} // namespace dx_ac1024_visualstyle_wire
#endif
