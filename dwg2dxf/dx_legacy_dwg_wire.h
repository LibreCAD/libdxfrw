// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_LEGACY_DWG_WIRE_H
#define DX_LEGACY_DWG_WIRE_H

#include "drw_objects.h"
#include "dx_raster_dwg_wire.h"
#include "intern/drw_textcodec.h"
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

// Shared bounded R13/R14 object framing and dictionary observer. Callers
// must additionally bind import identity, the complete graph and output.
// ODA v5.4.1 sections 20.1 and 20.4.44; only R14 has the extra dictionary RC.
namespace dx_legacy_dwg_wire {
using dx_raster_wire::detail::Bits;
using dx_raster_wire::detail::handle;
inline bool supported(DRW::Version version) noexcept {
    return version == DRW::AC1012 || version == DRW::AC1014;
}

inline bool text(Bits& bits, std::string_view expected) noexcept {
    const auto length = bits.shortValue();
    if (length != expected.size() && length != expected.size() + 1u) return false;
    for (unsigned char c : expected)
        if (bits.take(8u) != c) return false;
    if (length != expected.size() && bits.take(8u) != 0u) return false;
    return bits.good;
}

// DWG object alignment bits have no specified value. Still require that the
// complete remaining suffix is just the final, bounded byte alignment.
inline bool padding(Bits& bits) noexcept {
    return bits.good && bits.pos <= bits.end && bits.end - bits.pos < 8u
        && bits.skip(bits.end - bits.pos);
}

inline bool reference(Bits& bits, unsigned role, std::uint32_t base,
                      std::uint32_t expected, bool own = false) noexcept {
    const auto h = handle(bits, role, base, own);
    return bits.good && h.value == expected && (expected != 0u || h.role == role);
}

inline bool metadata(const DRW_UnsupportedObject& raw, std::uint32_t id,
                     int type, std::size_t size) noexcept {
    return supported(raw.m_version) && raw.m_handle == id
        && raw.m_objectType == type && !raw.m_isEntity
        && raw.m_objectSize == size && raw.m_rawBytes.size() == size
        && raw.m_bodyBitSize == 0u && raw.m_commonHandleDataValidated
        && drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence)
        && !raw.m_hasDataStorage && raw.m_xDictHandle == 0u;
}

// Source framing only: this does not qualify decoded text or output. Callers
// must bind APPIDs to unchanged records, independently justify the codepage
// interpretation, and check import text issues, semantics, graph and output.
// String bytes are ASCII or the observed codepage-45 UTF-8 profile; integer
// support includes finite 1040-1042 reals and signed 1070/1071 integers.
template<class AppIdSource, class CodepageSource>
bool observeDictionaryEed(const DRW_UnsupportedObject& raw,
                       const DRW_Dictionary& model, bool nod,
                       AppIdSource appIdSource, CodepageSource codepageSource,
                       bool mlineOwner = false) noexcept {
    static_assert(noexcept(appIdSource(std::uint32_t{}, std::declval<const std::string&>())),
                  "APPID verifier must not throw");
    static_assert(noexcept(codepageSource(std::uint16_t{}, std::uint16_t{})),
                  "EED codepage verifier must not throw");
    if (nod && mlineOwner) return false;
    const auto id = model.handle;
    const auto size = raw.m_rawBytes.size();
    const auto count = model.m_entries.size();
    if (!metadata(raw, id, 42u, size) || raw.m_isCustomClass
        || raw.m_hasClassDefinition || model.tType != DRW::DICTIONARY
        || id == 0u || size > UINT32_MAX / 8u || count > 64u
        || raw.m_parentHandle != model.parentHandle
        || raw.m_numReactors != model.reactorCount()
        || raw.m_reactorHandles != model.reactorHandles
        || !model.name.empty() || model.flags != 0u || model.hardOwner != 0
        || model.cloning != 0 || !model.hasCompleteDwgEntries()
        || !model.hasDwgCommonLinkTail() || model.countCap
        || model.xDictHandle != 0u || model.hasDataStorageBinaryData()
        || !model.appData.empty() || model.extData.size() > 256u
        || (!model.extData.empty() && size > 4096u)
        || !model.pendingAppIdResolutions.empty()
        || !model.pendingLayerRefResolutions.empty()
        || model.reactorCount() < 0
        || model.reactorHandles.size() != static_cast<std::size_t>(model.reactorCount())
        || (nod && (model.parentHandle != 0u || !model.reactorHandles.empty()))) return false;
    for (const auto& entry : model.m_entries)
        if (entry.m_name.empty() || entry.m_handle == 0u
            || entry.m_dxfHandleCode != DRW_Dictionary::Entry::DxfHandleCode::Unspecified
            || !std::all_of(entry.m_name.begin(), entry.m_name.end(),
                            [](unsigned char c) { return c > 0u && c < 128u; })) return false;
    Bits bits{raw.m_rawBytes.data(), 0u, static_cast<std::uint32_t>(size * 8u)};
    if (bits.shortValue() != 42u || !reference(bits, 0u, 0u, id, true)) return false;
    dx_raster_wire::Proof eedProof;
    const auto visit = [&](const dx_raster_wire::EedToken& token) noexcept {
        if (token.index >= model.extData.size()) return false;
        const auto* value = model.extData[token.index];
        if (!value || value->code() != static_cast<int>(token.code)) return false;
        if (token.code == 1001u)
            return token.role == 5u && token.value != 0u
                && token.value <= (std::numeric_limits<std::uint32_t>::max)()
                && value->type() == DRW_Variant::STRING && value->content.s
                && appIdSource(static_cast<std::uint32_t>(token.value), *value->content.s);
        if (token.code == 1000u) {
            if (value->type() != DRW_Variant::STRING || !value->content.s
                || token.value != value->content.s->size()
                || !codepageSource(token.codePageBE, token.codePageLE)) return false;
            Bits textBits{raw.m_rawBytes.data(), token.valueSpan.begin, token.valueSpan.end};
            if (textBits.take(8u) != value->content.s->size() || !textBits.skip(16u)) return false;
            bool nonAscii = false;
            for (unsigned char c : *value->content.s) {
                if (c == 0u || textBits.take(8u) != c) return false;
                nonAscii = nonAscii || c > 0x7fu;
            }
            if (nonAscii && (token.codePageBE != 45u || token.codePageLE != 0x2d00u
                || !DRW_TextCodec::isUtf8ScalarText(*value->content.s))) return false;
            return textBits.good && textBits.pos == textBits.end;
        }
        if (token.code >= 1040u && token.code <= 1042u
            && value->type() == DRW_Variant::DOUBLE && std::isfinite(value->d_val())) {
            static_assert(sizeof(double) == sizeof(std::uint64_t), "DWG doubles must be 64-bit");
            std::uint64_t expected = 0u;
            const double number = value->d_val();
            std::memcpy(&expected, &number, sizeof expected);
            Bits realBits{raw.m_rawBytes.data(), token.valueSpan.begin, token.valueSpan.end};
            const auto actual = realBits.little(8u);
            return realBits.good && realBits.pos == realBits.end && actual == expected;
        }
        if (token.code == 1070u && value->type() == DRW_Variant::INTEGER) {
            const auto number = value->i_val();
            return number >= -32768 && number <= 32767
                && token.value == static_cast<std::uint16_t>(number);
        }
        if (token.code == 1071u && value->type() == DRW_Variant::INTEGER)
            return token.value == static_cast<std::uint32_t>(value->i_val());
        return false;
    };
    const unsigned revision = raw.m_version == DRW::AC1012 ? 13u : 14u;
    if (dx_raster_wire::detail::eed(bits, revision, eedProof, visit)
            != dx_raster_wire::Status::Proven
        || eedProof.eedVariants != model.extData.size()) return false;
    const auto bodyEnd = bits.little(4u);
    if (bits.longValue() != model.reactorHandles.size() || bits.longValue() != count
        || (raw.m_version == DRW::AC1014 && bits.take(8u) != 0u)) return false;
    for (const auto& entry : model.m_entries)
        if (!text(bits, entry.m_name)) return false;
    if (bits.pos != bodyEnd || !reference(bits, 4u, id, model.parentHandle)) return false;
    for (const auto reactor : model.reactorHandles)
        if (!reference(bits, 4u, id, reactor)) return false;
    if (!reference(bits, 3u, id, 0u)) return false;
    for (const auto& entry : model.m_entries)
        if (!reference(bits, 2u, id, entry.m_handle)) return false;
    return padding(bits);
}

inline bool dictionary(const DRW_UnsupportedObject& raw,
                       const DRW_Dictionary& model, bool nod,
                       bool mlineOwner = false) noexcept {
    if (!model.extData.empty()) return false;
    return observeDictionaryEed(raw, model, nod,
        [](std::uint32_t, const std::string&) noexcept { return false; },
        [](std::uint16_t, std::uint16_t) noexcept { return false; }, mlineOwner);
}

} // namespace dx_legacy_dwg_wire
#endif
