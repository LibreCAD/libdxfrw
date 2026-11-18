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
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <utility>

namespace dx_layerstates_dwg_wire {

enum class LinkKind { Owner, Reactor, Item, XDictionary };

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
    static_assert(noexcept(appIdSource(std::uint32_t{}, std::declval<const std::string &>())),
                  "APPID source verifier must not throw");
    static_assert(noexcept(linkSource(LinkKind::Owner, std::size_t{},
                                      std::uint8_t{}, std::uint8_t{})),
                  "link source verifier must not throw");
    if (raw.m_version != DRW::AC1024 || raw.m_objectType != 42
        || raw.m_handle != source.handle || source.handle == 0u
        || source.handle == DRW::NoHandle || source.tType != DRW::DICTIONARY
        || raw.m_isEntity
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
                               *value->content.s);
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

// Bounded AC1024 XRECORD observer for the selected LayerStates payload
// vocabulary. This only verifies the ordered wire carriers; it does not
// authorize the producer-specific group 370 compatibility route, compare
// m_values, or prove source identity, dependencies or output transport.
template <class LinkSource>
bool observeXrecord(const DRW_UnsupportedObject &raw,
                    const DRW_XRecord &source, LinkSource linkSource) noexcept {
    using namespace dx_raster_wire;
    static_assert(noexcept(linkSource(LinkKind::Owner, std::size_t{},
                                      std::uint8_t{}, std::uint8_t{})),
                  "link source verifier must not throw");
    if (raw.m_version != DRW::AC1024 || raw.m_objectType != 79
        || raw.m_handle != source.handle || source.handle == 0u
        || source.handle == DRW::NoHandle || source.tType != DRW::XRECORD
        || raw.m_isEntity
        || raw.m_isCustomClass || raw.m_hasClassDefinition
        || !raw.m_recordName.empty() || !raw.m_className.empty()
        || !raw.m_classAppName.empty() || raw.m_hasDataStorage
        || !drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence)
        || !raw.m_commonHandleDataValidated || !source.hasDwgCommonLinkTail()
        || raw.m_parentHandle != source.parentHandle
        || raw.m_xDictHandle != source.xDictHandle || raw.m_xDictFlag > 1u
        || (raw.m_xDictFlag == 1u) != (source.xDictHandle == 0u)
        || raw.m_numReactors < 0
        || static_cast<std::size_t>(raw.m_numReactors) != source.reactorHandles.size()
        || raw.m_reactorHandles != source.reactorHandles
        || source.reactorHandles.size() > 256u || !source.appData.empty()
        || !source.extData.empty() || source.hasDataStorageBinaryData()
        || !source.pendingAppIdResolutions.empty()
        || !source.pendingLayerRefResolutions.empty()
        || source.m_rawDataValid || !source.m_rawData.empty()
        || source.hasDwgRawDataSourceRecord()
        || source.m_cloning < 0 || source.m_cloning > 65535
        || source.m_dataEntries.size() > 256u || raw.m_rawBytes.empty()
        || raw.m_rawBytes.size() > 4096u || raw.m_objectSize != raw.m_rawBytes.size()
        || raw.m_bodyBitSize == 0u)
        return false;
    const auto total = static_cast<std::uint32_t>(raw.m_rawBytes.size() * 8u);
    if (raw.m_bodyBitSize >= total) return false;
    const auto bodyEnd = total - raw.m_bodyBitSize;
    detail::Bits body{raw.m_rawBytes.data(), 0u, bodyEnd};
    const auto tag = body.take(2u);
    const auto type = tag == 0u ? body.take(8u)
        : tag == 1u ? body.take(8u) + 0x1f0u : body.little(2u);
    const auto self = detail::handle(body, 0u, 0u, true);
    if (!body.good || type != 79u || self.value != source.handle
        || body.shortValue() != 0u
        || body.longValue() != source.reactorHandles.size()
        || body.take(1u) != raw.m_xDictFlag)
        return false;
    const auto bytes = body.longValue();
    if (!body.good || bytes > 4096u || body.pos > bodyEnd
        || bytes > (bodyEnd - body.pos) / 8u)
        return false;
    const auto dataEnd = body.pos + bytes * 8u;
    detail::Bits payload{raw.m_rawBytes.data(), body.pos, dataEnd};
    std::size_t handles = 0u;
    for (const auto &entry : source.m_dataEntries) {
        const auto code = payload.little(2u);
        if (!payload.good || code != static_cast<unsigned>(entry.code()))
            return false;
        if (code == 1u || code == 301u || code == 302u) {
            if (entry.type() != DRW_Variant::STRING || !entry.content.s
                || payload.little(2u) != entry.content.s->size()) return false;
            for (unsigned char c : *entry.content.s)
                if (c > 0x7fu || payload.little(2u) != c) return false;
        } else if (code == 330u || code == 331u) {
            const auto value = payload.little(8u);
            if (entry.type() != DRW_Variant::INTEGER64
                || value > (std::numeric_limits<std::uint32_t>::max)()
                || static_cast<std::uint64_t>(entry.i64_val()) != value
                || handles >= source.m_handleValues.size()
                || source.m_handleValues[handles].first != static_cast<int>(code)
                || source.m_handleValues[handles].second != value) return false;
            ++handles;
        } else if (code == 62u || code == 370u) {
            const auto value = payload.little(2u);
            const auto signedValue = value >= 32768u
                ? static_cast<std::int64_t>(value) - 65536
                : static_cast<std::int64_t>(value);
            if (entry.type() != DRW_Variant::INTEGER
                || entry.i_val() != signedValue) return false;
        } else if (code == 90u || code == 91u || code == 92u) {
            const auto value = payload.little(4u);
            const auto signedValue = value >= 0x80000000u
                ? static_cast<std::int64_t>(value) - 4294967296LL
                : static_cast<std::int64_t>(value);
            if (entry.type() != DRW_Variant::INTEGER
                || entry.i_val() != signedValue) return false;
        } else if (code == 290u) {
            const auto value = payload.take(8u);
            if (entry.type() != DRW_Variant::INTEGER || value > 1u
                || entry.i_val() != static_cast<int>(value)) return false;
        } else return false;
        if (!payload.good) return false;
    }
    if (!payload.good || payload.pos != payload.end
        || handles != source.m_handleValues.size()) return false;
    body.pos = dataEnd;
    if (body.shortValue() != static_cast<std::uint32_t>(source.m_cloning)
        || !body.good || bodyEnd < 1u || body.pos != bodyEnd - 1u
        || body.take(1u) != 0u || !body.good) return false;
    detail::Bits wire{raw.m_rawBytes.data(), bodyEnd, total};
    const auto owner = detail::handle(wire, 4u, source.handle);
    if (!wire.good || owner.role == 0u || owner.value != source.parentHandle
        || !linkSource(LinkKind::Owner, 0u, owner.role, owner.length)) return false;
    std::size_t index = 0u;
    for (const auto reactor : source.reactorHandles) {
        const auto token = detail::handle(wire, 4u, source.handle);
        if (!wire.good || reactor == 0u || reactor == DRW::NoHandle
            || token.value != reactor
            || !linkSource(LinkKind::Reactor, index++, token.role, token.length))
            return false;
    }
    if (source.xDictHandle != 0u) {
        const auto token = detail::handle(wire, 3u, source.handle);
        if (!wire.good || token.role != 3u || token.value != source.xDictHandle
            || !linkSource(LinkKind::XDictionary, 0u, token.role, token.length))
            return false;
    }
    const auto padding = wire.end - wire.pos;
    return padding < 8u && wire.skip(padding) && wire.good;
}

// Bounded AC1024 SCALE observer. classSource must bind the numeric class type
// and class identity/application to the same completed import; no class number
// is assumed globally. The witnessed SCALE typed model does not carry the
// dictionary's common-tail marker; validated raw evidence plus the complete
// handle observer are required. Names are ASCII UTF-16 and EED/xdict refused.
// This does not authorize a graph or verify source-to-DXF output transport.
template <class ClassSource, class LinkSource>
bool observeScale(const DRW_UnsupportedObject &raw, const DRW_Scale &source,
                  ClassSource classSource, LinkSource linkSource) noexcept {
    using namespace dx_raster_wire;
    static_assert(noexcept(classSource(std::uint32_t{},
                                       std::declval<const std::string &>())),
                  "class source verifier must not throw");
    static_assert(noexcept(linkSource(LinkKind::Owner, std::size_t{},
                                      std::uint8_t{}, std::uint8_t{})),
                  "link source verifier must not throw");
    if (!std::numeric_limits<double>::is_iec559 || sizeof(double) != 8u
        || raw.m_version != DRW::AC1024 || raw.m_objectType < 500
        || raw.m_handle != source.handle || source.handle == 0u
        || source.handle == DRW::NoHandle || source.tType != DRW::SCALE
        || raw.m_isEntity
        || !raw.m_isCustomClass || !raw.m_hasClassDefinition
        || !raw.m_commonHandleDataValidated
        || !drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence)
        || raw.m_recordName != "SCALE" || raw.m_className != "AcDbScale"
        || !classSource(static_cast<std::uint32_t>(raw.m_objectType),
                        raw.m_classAppName)
        || raw.m_hasDataStorage || raw.m_xDictFlag != 1u
        || raw.m_xDictHandle != 0u || source.xDictHandle != 0u
        || raw.m_parentHandle != source.parentHandle || raw.m_numReactors < 0
        || static_cast<std::size_t>(raw.m_numReactors) != source.reactorHandles.size()
        || raw.m_reactorHandles != source.reactorHandles
        || source.reactorHandles.size() > 256u || !source.appData.empty()
        || !source.extData.empty() || source.hasDataStorageBinaryData()
        || !source.pendingAppIdResolutions.empty()
        || !source.pendingLayerRefResolutions.empty()
        || source.name.empty() || !std::isfinite(source.paperUnits)
        || !std::isfinite(source.drawingUnits) || raw.m_rawBytes.empty()
        || raw.m_rawBytes.size() > 4096u || raw.m_rawBytes.size() != raw.m_objectSize
        || raw.m_bodyBitSize == 0u) return false;
    const auto total = static_cast<std::uint32_t>(raw.m_rawBytes.size() * 8u);
    if (raw.m_bodyBitSize >= total) return false;
    const auto bodyEnd = total - raw.m_bodyBitSize;
    if (bodyEnd < 17u) return false;
    detail::Bits body{raw.m_rawBytes.data(), 0u, bodyEnd};
    const auto tag = body.take(2u);
    const auto type = tag == 0u ? body.take(8u)
        : tag == 1u ? body.take(8u) + 0x1f0u : body.little(2u);
    const auto self = detail::handle(body, 0u, 0u, true);
    if (!body.good || type != static_cast<unsigned>(raw.m_objectType)
        || self.value != source.handle || body.shortValue() != 0u
        || body.longValue() != source.reactorHandles.size()
        || body.take(1u) != 1u || body.shortValue() != source.flag) return false;
    const auto bitDouble = [&body](double expected) noexcept {
        const auto code = body.take(2u);
        std::uint64_t bits = 0u, wanted = 0u;
        if (code == 0u) bits = body.little(8u);
        else if (code == 1u) bits = UINT64_C(0x3ff0000000000000);
        else if (code != 2u) return false;
        std::memcpy(&wanted, &expected, sizeof(wanted));
        return body.good && bits == wanted;
    };
    if (!bitDouble(source.paperUnits) || !bitDouble(source.drawingUnits)
        || body.take(1u) != static_cast<unsigned>(source.isUnitScale)
        || !body.good) return false;
    detail::Bits footer{raw.m_rawBytes.data(), bodyEnd - 17u, bodyEnd - 1u};
    const auto textBits = static_cast<std::uint32_t>(footer.little(2u));
    detail::Bits present{raw.m_rawBytes.data(), bodyEnd - 1u, bodyEnd};
    if (!footer.good || (textBits & 0x8000u) != 0u
        || textBits > bodyEnd - 17u || body.pos != bodyEnd - 17u - textBits
        || present.take(1u) != 1u || !present.good) return false;
    detail::Bits text{raw.m_rawBytes.data(), body.pos, bodyEnd - 17u};
    if (text.shortValue() != source.name.size()) return false;
    for (unsigned char c : source.name)
        if (c > 0x7fu || text.little(2u) != c) return false;
    if (!text.good || text.pos != text.end) return false;
    detail::Bits wire{raw.m_rawBytes.data(), bodyEnd, total};
    const auto owner = detail::handle(wire, 4u, source.handle);
    if (!wire.good || owner.role == 0u || owner.value != source.parentHandle
        || !linkSource(LinkKind::Owner, 0u, owner.role, owner.length)) return false;
    std::size_t index = 0u;
    for (const auto reactor : source.reactorHandles) {
        const auto token = detail::handle(wire, 4u, source.handle);
        if (!wire.good || reactor == 0u || reactor == DRW::NoHandle
            || token.value != reactor
            || !linkSource(LinkKind::Reactor, index++, token.role, token.length))
            return false;
    }
    const auto padding = wire.end - wire.pos;
    return padding < 8u && wire.skip(padding) && wire.good;
}
} // namespace dx_layerstates_dwg_wire
#endif
