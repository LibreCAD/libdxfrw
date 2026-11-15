// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1027_MATERIAL_WIRE_H
#define DX_AC1027_MATERIAL_WIRE_H
#include "dx_raster_dwg_wire.h"
#include "drw_objects.h"
namespace dx_material_wire_detail {
inline bool matches(const DRW_UnsupportedObject& raw,
                    const DRW_Material& typed, DRW::Version version) noexcept {
    using namespace dx_raster_wire::detail;
    if (raw.m_version != version || raw.m_isEntity || !raw.m_isCustomClass
        || raw.m_objectType < 500 || raw.m_recordName != "MATERIAL"
        || raw.m_className != "AcDbMaterial" || raw.m_handle == 0u
        || raw.m_handle != typed.handle || raw.m_parentHandle != typed.parentHandle
        || raw.m_numReactors != 1 || raw.m_reactorHandles.size() != 1u
        || raw.m_reactorHandles != typed.reactorHandles || raw.m_xDictFlag != 1u
        || raw.m_hasDataStorage || raw.m_rawBytes.empty()
        || raw.m_rawBytes.size() > 1024u || raw.m_bodyBitSize == 0u
        || !typed.extData.empty() || !typed.appData.empty()
        || typed.xDictHandle != 0u || typed.hasDataStorageBinaryData()
        || typed.extensionDictionaryFlag() != 1u)
        return false;
    const auto total = static_cast<std::uint32_t>(raw.m_rawBytes.size() * 8u);
    if (raw.m_bodyBitSize >= total) return false;
    const auto end = total - raw.m_bodyBitSize;
    Bits body{raw.m_rawBytes.data(), 0u, end};
    const auto tag = body.take(2u);
    const auto type = static_cast<std::uint32_t>(tag == 0u ? body.take(8u)
        : tag == 1u ? body.take(8u) + 0x1f0u : body.little(2u));
    if (!body.good || type != static_cast<std::uint32_t>(raw.m_objectType)) return false;
    const auto self = handle(body, 0u, 0u, true);
    if (!body.good || self.value != raw.m_handle || body.shortValue() != 0u
        || body.longValue() != 1u || body.take(1u) != 1u
        || (version > DRW::AC1024 && body.take(1u) != 0u)
        || !body.good) return false;
    Bits handles{raw.m_rawBytes.data(), end, total};
    const auto owner = handle(handles, 4u, raw.m_handle);
    const auto reactor = handle(handles, 4u, raw.m_handle);
    if (!handles.good || owner.role != 4u || reactor.role != 4u
        || owner.value != raw.m_parentHandle || reactor.value != raw.m_parentHandle)
        return false;
    const auto tail = handles.end - handles.pos;
    return tail < 8u && handles.take(tail) == 0u && handles.good;
}
// Exact source-witnessed ACAD_MATERIAL dictionary, including all three
// soft-owner item tokens and the independently bounded UTF-16 name stream.
inline bool dictionary(const DRW_UnsupportedObject& raw, const DRW_Dictionary& typed,
                       std::uint32_t owner, DRW::Version version,
                       const char** failureReason = nullptr) noexcept {
    using namespace dx_raster_wire::detail;
    const auto fail = [failureReason](const char* reason) noexcept {
        if (failureReason != nullptr)
            *failureReason = reason;
        return false;
    };
    if (failureReason != nullptr)
        *failureReason = nullptr;
    if (raw.m_version != version || raw.m_objectType != 42
        || raw.m_isEntity || raw.m_isCustomClass
        || raw.m_handle != typed.handle || raw.m_parentHandle != owner)
        return fail("source identity");
    if (version == DRW::AC1024
        ? raw.m_numReactors != 0 || !raw.m_reactorHandles.empty()
        : raw.m_numReactors != 1
            || raw.m_reactorHandles != std::vector<std::uint32_t>{owner})
        return fail("source reactor metadata");
    if (raw.m_xDictFlag != 1u)
        return fail("source XDic missing flag");
    if (raw.m_hasDataStorage)
        return fail("source data-storage flag");
    if (raw.m_rawBytes.empty() || raw.m_rawBytes.size() > 1024u
        || raw.m_bodyBitSize == 0u)
        return fail("source body storage");
    if (typed.m_entries.size() != 3u)
        return fail("dictionary entry count");
    bool hasByLayer = false;
    bool hasByBlock = false;
    bool hasGlobal = false;
    for (const auto& entry : typed.m_entries) {
        bool* found = entry.m_name == "ByLayer" ? &hasByLayer
            : entry.m_name == "ByBlock" ? &hasByBlock
                : entry.m_name == "Global" ? &hasGlobal : nullptr;
        if (found == nullptr || *found)
            return fail("dictionary member set");
        *found = true;
    }
    if (!hasByLayer || !hasByBlock || !hasGlobal)
        return fail("dictionary member set");
    const auto total = static_cast<std::uint32_t>(raw.m_rawBytes.size() * 8u);
    if (raw.m_bodyBitSize >= total) return fail("body bit span");
    const auto end = total - raw.m_bodyBitSize;
    Bits body{raw.m_rawBytes.data(), 0u, end};
    const auto tag = body.take(2u);
    const auto type = static_cast<std::uint32_t>(tag == 0u ? body.take(8u)
        : tag == 1u ? body.take(8u) + 0x1f0u : body.little(2u));
    const auto self = handle(body, 0u, 0u, true);
    if (!body.good || type != 42u)
        return fail("body object type");
    if (self.value != raw.m_handle)
        return fail("body object handle");
    if (body.shortValue() != 0u)
        return fail("body size prefix");
    // ODA §20.4.44 places Numreactors after EED and before the R2004+
    // XDic Missing Flag. The witnessed AC1024 dictionary has no reactors;
    // the selected AC1027 dictionary has one reactor to its NOD owner.
    if (body.longValue() != (version == DRW::AC1024 ? 0u : 1u))
        return fail("body reactor count");
    if (body.take(1u) != 1u)
        return fail("body XDic missing flag");
    if (version > DRW::AC1024 && body.take(1u) != 0u)
        return fail("body data-storage flag");
    if (body.longValue() != 3u)
        return fail("body dictionary item count");
    if (body.shortValue() != 1u)
        return fail("body cloning value");
    if (body.take(8u) != 0u || !body.good || end < 17u)
        return fail("body hard-owner or boundary");
    Bits footer{raw.m_rawBytes.data(), end - 17u, end};
    const auto size = static_cast<std::uint32_t>(footer.little(2u));
    if (footer.take(1u) != 1u || !footer.good || (size != 350u && size != 403u)
        || size > end - 17u || end - 17u - size != body.pos)
        return fail("string footer span");
    Bits text{raw.m_rawBytes.data(), body.pos, end - 17u};
    for (const auto& entry : typed.m_entries) {
        const auto length = text.shortValue();
        if (length != entry.m_name.size() + (size == 403u ? 1u : 0u))
            return fail("dictionary string length/order");
        for (const char c : entry.m_name)
            if (text.little(2u) != static_cast<unsigned char>(c))
                return fail("dictionary string value/order");
        if (size == 403u && text.little(2u) != 0u)
            return fail("dictionary string terminator");
    }
    const auto textTail = text.end - text.pos;
    if ((size == 350u ? textTail != 0u : textTail != 5u)
        || text.take(textTail) != 0u || !text.good)
        return fail("dictionary string tail");
    Bits handles{raw.m_rawBytes.data(), end, total};
    const auto ownerToken = handle(handles, 4u, raw.m_handle);
    if (!handles.good || ownerToken.role != 4u || ownerToken.value != owner)
        return fail("owner handle token");
    if (version > DRW::AC1024) {
        const auto reactorToken = handle(handles, 4u, raw.m_handle);
        if (!handles.good || reactorToken.role != 4u
            || reactorToken.value != owner)
            return fail("reactor handle token");
    }
    for (const auto& entry : typed.m_entries) {
        const auto token = handle(handles, 2u, raw.m_handle);
        if (!handles.good || token.role != 2u || token.value != entry.m_handle)
            return fail("dictionary item handle token/order");
    }
    const auto tail = handles.end - handles.pos;
    return tail < 8u && handles.take(tail) == 0u && handles.good
        ? true : fail("handle tail");
}
} // namespace dx_material_wire_detail

namespace dx_ac1027_material_wire {
inline bool matches(const DRW_UnsupportedObject& raw,
                    const DRW_Material& typed) noexcept {
    return dx_material_wire_detail::matches(raw, typed, DRW::AC1027);
}
inline bool dictionary(const DRW_UnsupportedObject& raw, const DRW_Dictionary& typed,
                       std::uint32_t owner, const char** failureReason = nullptr) noexcept {
    return dx_material_wire_detail::dictionary(raw, typed, owner, DRW::AC1027, failureReason);
}
} // namespace dx_ac1027_material_wire

namespace dx_ac1024_material_wire {
inline bool matches(const DRW_UnsupportedObject& raw,
                    const DRW_Material& typed) noexcept {
    return dx_material_wire_detail::matches(raw, typed, DRW::AC1024);
}
inline bool dictionary(const DRW_UnsupportedObject& raw, const DRW_Dictionary& typed,
                       std::uint32_t owner, const char** failureReason = nullptr) noexcept {
    return dx_material_wire_detail::dictionary(raw, typed, owner, DRW::AC1024, failureReason);
}
} // namespace dx_ac1024_material_wire
#endif
