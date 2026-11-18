// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_LAYERSTATES_DWG_WIRE_H
#define DX_LAYERSTATES_DWG_WIRE_H

// Source-only AC1024 DICTIONARY wire observer. This is not an output
// admission predicate: callers must independently bind the frame to the
// current import, select a qualified graph, prove dependencies and inspect
// staged DXF output. It admits only absent xdict/storage, ASCII UTF-16
// dictionary names, and EED composed of APPID and ASCII strings.
// ODA DWG v5.4.1 sections 2.12, 2.13, 20.1, 20.4.44 and 28.
#include "dx_raster_dwg_wire.h"
#include "drw_objects.h"
#include <cstdint>
#include <limits>
#include <string_view>

namespace dx_layerstates_dwg_wire {

enum class LinkKind { Owner, Reactor, Item };

// appIdSource(handle, decodedName) MUST verify an unambiguous APPID from the
// same completed import and its unchanged, version-bound source name. This
// observer calls it for every EED application token; an empty or unproven
// callback must return false. It does not validate emitted APPID records.
// linkSource(kind, index, role, length) MUST verify the independently
// witnessed role and encoded length for every owner, reactor and item. It
// must not infer roles solely from the decoded targets or parser model.
template <class AppIdSource, class LinkSource>
bool observeDictionary(const DRW_UnsupportedObject &raw,
                       const DRW_Dictionary &source,
                       AppIdSource appIdSource,
                       LinkSource linkSource) noexcept {
    using namespace dx_raster_wire;
    static_assert(noexcept(appIdSource(std::uint32_t{}, std::string_view{})),
                  "APPID source verifier must not throw");
    static_assert(noexcept(linkSource(LinkKind::Owner, std::size_t{},
                                      std::uint8_t{}, std::uint8_t{})),
                  "link source verifier must not throw");
    if (raw.m_version != DRW::AC1024 || raw.m_objectType != 42
        || raw.m_handle != source.handle || source.handle == 0u
        || source.handle == DRW::NoHandle || raw.m_isEntity
        || raw.m_isCustomClass || raw.m_hasClassDefinition
        || !raw.m_recordName.empty() || !raw.m_className.empty()
        || !raw.m_classAppName.empty()
        || !drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence)
        || !raw.m_commonHandleDataValidated || raw.m_hasDataStorage || raw.m_xDictFlag != 1u
        || raw.m_xDictHandle != 0u || source.xDictHandle != 0u
        || raw.m_parentHandle != source.parentHandle
        || raw.m_numReactors < 0
        || static_cast<std::size_t>(raw.m_numReactors)
            != source.reactorHandles.size()
        || raw.m_reactorHandles != source.reactorHandles
        || !source.appData.empty() || source.hasDataStorageBinaryData()
        || !source.hasDwgCommonLinkTail() || !source.hasCompleteDwgEntries()
        || source.countCap
        || !source.pendingAppIdResolutions.empty()
        || !source.pendingLayerRefResolutions.empty()
        || source.hardOwner < 0 || source.hardOwner > 1
        || source.cloning < 0 || source.cloning > 65535
        || source.m_entries.size() > 256u || source.reactorHandles.size() > 256u
        || raw.m_rawBytes.empty() || raw.m_rawBytes.size() > 4096u
        || raw.m_rawBytes.size() != raw.m_objectSize || raw.m_bodyBitSize == 0u)
        return false;
    const auto total = static_cast<std::uint32_t>(raw.m_rawBytes.size() * 8u);
    if (raw.m_bodyBitSize >= total)
        return false;
    const auto bodyEnd = total - raw.m_bodyBitSize;
    detail::Bits body{raw.m_rawBytes.data(), 0u, bodyEnd};
    const auto tag = body.take(2u);
    const auto type = tag == 0u ? body.take(8u)
        : tag == 1u ? body.take(8u) + 0x1f0u : body.little(2u);
    const auto self = detail::handle(body, 0u, 0u, true);
    if (!body.good || type != 42u || self.value != source.handle)
        return false;
    Proof proof;
    const auto eed = [&](const EedToken &token) noexcept {
        if (token.index >= source.extData.size()) return false;
        const auto *value = source.extData[token.index];
        if (!value || value->code() != static_cast<int>(token.code)
            || value->type() != DRW_Variant::STRING || !value->content.s)
            return false;
        if (token.code == 1001u) {
            return token.role == 5u && token.value != 0u
                && token.value <= (std::numeric_limits<std::uint32_t>::max)()
                && appIdSource(static_cast<std::uint32_t>(token.value),
                               std::string_view(*value->content.s));
        }
        if (token.code != 1000u) return false;
        detail::Bits text{raw.m_rawBytes.data(), token.valueSpan.begin,
                          token.valueSpan.end};
        if (text.little(2u) != value->content.s->size()) return false;
        for (unsigned char c : *value->content.s)
            if (c > 0x7fu || text.little(2u) != c) return false;
        return text.good && text.pos == text.end;
    };
    if (detail::eed(body, 2010u, proof, eed) != Status::Proven
        || proof.eedVariants != source.extData.size()
        || body.longValue() != source.reactorHandles.size()
        || body.take(1u) != 1u || body.longValue() != source.m_entries.size()
        || body.shortValue() != static_cast<std::uint32_t>(source.cloning)
        || body.take(8u) != static_cast<std::uint32_t>(source.hardOwner)
        || !body.good || bodyEnd < 1u)
        return false;
    detail::Bits present{raw.m_rawBytes.data(), bodyEnd - 1u, bodyEnd};
    const bool hasStrings = present.take(1u) != 0u;
    if (!present.good) return false;
    if (source.m_entries.empty()) {
        if (hasStrings || body.pos != bodyEnd - 1u) return false;
    } else {
        if (!hasStrings || bodyEnd < 17u) return false;
        detail::Bits footer{raw.m_rawBytes.data(), bodyEnd - 17u, bodyEnd - 1u};
        const auto textBits = static_cast<std::uint32_t>(footer.little(2u));
        if (!footer.good || (textBits & 0x8000u) != 0u
            || textBits > bodyEnd - 17u
            || body.pos != bodyEnd - 17u - textBits)
            return false;
        detail::Bits text{raw.m_rawBytes.data(), body.pos, bodyEnd - 17u};
        for (const auto &entry : source.m_entries) {
            if (entry.m_name.empty() || text.shortValue() != entry.m_name.size())
                return false;
            for (unsigned char c : entry.m_name)
                if (c > 0x7fu || text.little(2u) != c) return false;
        }
        if (!text.good || text.pos != text.end) return false;
    }
    detail::Bits wire{raw.m_rawBytes.data(), bodyEnd, total};
    const auto owner = detail::handle(wire, 4u, source.handle);
    if (!wire.good || owner.role == 0u || owner.value != source.parentHandle
        || !linkSource(LinkKind::Owner, 0u, owner.role, owner.length))
        return false;
    std::size_t reactorIndex = 0u;
    for (const auto reactor : source.reactorHandles) {
        if (reactor == 0u || reactor == DRW::NoHandle) return false;
        const auto token = detail::handle(wire, 4u, source.handle);
        if (!wire.good || token.value != reactor
            || !linkSource(LinkKind::Reactor, reactorIndex++, token.role,
                           token.length)) return false;
    }
    std::size_t itemIndex = 0u;
    for (const auto &entry : source.m_entries) {
        if (entry.m_handle == 0u || entry.m_handle == DRW::NoHandle)
            return false;
        const auto token = detail::handle(wire, 2u, source.handle);
        if (!wire.good || token.role != 2u || token.value != entry.m_handle
            || !linkSource(LinkKind::Item, itemIndex++, token.role,
                           token.length)) return false;
    }
    const auto padding = wire.end - wire.pos;
    return padding < 8u && wire.skip(padding) && wire.good;
}
} // namespace dx_layerstates_dwg_wire
#endif
