// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_RASTER_DWG_WIRE_H
#define DX_RASTER_DWG_WIRE_H

// Independent, allocation-free byte observer for ODA v5.4.1 sections 2,
// 20.1, 20.4.81,20.4.82 and28. No production buffer, parser, writer or decoder.
// Input is an already extracted object body, excluding MS/UMC/CRC. Container
// integrity, class-stream extraction and text decoding are separate evidence.
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace dx_raster_wire {
enum class Kind { Definition, Reactor };
enum class Status { Malformed, Unsupported, Proven };
enum class Field : std::size_t {
    Type, DataEnd, Self, EedSize, ReactorCount, XDictMissing, DataStorage,
    ClassVersion, ImageU, ImageV, PathLength, PathUnits, Loaded, Resolution,
    PixelU, PixelV, Owner, Reactors, XDictionary, StringFooter, ScalarPadding,
    StringPadding, HandlePadding, Eed, Count
};
struct Span {
    std::uint32_t begin{0}, end{0};
    bool present{false};
    std::uint32_t width() const noexcept { return end - begin; }
};
struct Proof {
    Status status{Status::Malformed};
    std::array<Span, static_cast<std::size_t>(Field::Count)> fields{};
    std::uint32_t objectType{0}, self{0}, dataEnd{0}, classVersion{0};
    std::uint32_t reactorCount{0}, owner{0}, dictionary{0}, pathUnits{0};
    std::array<std::uint64_t, 4> doubles{}; // U,V,pixel U,pixel V IEEE bits
    bool dictionaryMissing{false}, dataStorage{false}, loaded{false};
    bool noncanonicalTypeTag{false}, emptyStringTrailer{false};
    std::uint8_t resolution{0}, ownerRole{0}, dictionaryRole{0};
    std::uint32_t eedChunks{0}, eedItems{0}, eedVariants{0};
    bool eedLegacyStrings{false}, eedUnicodeStrings{false}, eedLayerEndianConflict{false};
    const Span& at(Field field) const noexcept { return fields[static_cast<std::size_t>(field)]; }
};
inline bool sameProof(const Proof& a, const Proof& b) noexcept {
    if (a.status != b.status || a.objectType != b.objectType || a.self != b.self
        || a.dataEnd != b.dataEnd || a.classVersion != b.classVersion
        || a.reactorCount != b.reactorCount || a.owner != b.owner || a.dictionary != b.dictionary
        || a.pathUnits != b.pathUnits || a.doubles != b.doubles
        || a.dictionaryMissing != b.dictionaryMissing || a.dataStorage != b.dataStorage
        || a.loaded != b.loaded || a.noncanonicalTypeTag != b.noncanonicalTypeTag
        || a.emptyStringTrailer != b.emptyStringTrailer || a.resolution != b.resolution
        || a.ownerRole != b.ownerRole || a.dictionaryRole != b.dictionaryRole) return false;
    if (a.eedChunks != b.eedChunks || a.eedItems != b.eedItems || a.eedVariants != b.eedVariants
        || a.eedLegacyStrings != b.eedLegacyStrings || a.eedUnicodeStrings != b.eedUnicodeStrings
        || a.eedLayerEndianConflict != b.eedLayerEndianConflict) return false;
    for (std::size_t n = 0; n < a.fields.size(); ++n)
        if (a.fields[n].begin != b.fields[n].begin || a.fields[n].end != b.fields[n].end
            || a.fields[n].present != b.fields[n].present) return false;
    return true;
}

// Every callback is positioned by independent wire order, including each
// synthetic1001 APPID entry. No decoded name or successful target lookup may
// decide byte order. Layer references retain both interpretations explicitly.
struct EedToken {
    std::uint32_t index{0}, chunk{0}, code{0};
    Span span, valueSpan;
    std::uint64_t value{0}, alternate{0};
    std::uint16_t codePageBE{0}, codePageLE{0};
    std::uint8_t role{0};
};
inline bool sameToken(const EedToken& a, const EedToken& b) noexcept {
    return a.index == b.index && a.chunk == b.chunk && a.code == b.code
        && a.span.begin == b.span.begin && a.span.end == b.span.end && a.span.present == b.span.present
        && a.valueSpan.begin == b.valueSpan.begin && a.valueSpan.end == b.valueSpan.end
        && a.valueSpan.present == b.valueSpan.present && a.value == b.value && a.alternate == b.alternate
        && a.codePageBE == b.codePageBE && a.codePageLE == b.codePageLE && a.role == b.role;
}
enum class Layout { LegacyInline, Split2007, Split2010 };
enum ProfileIssue : std::uint32_t {
    UnknownWire = 1u, UnknownBodyClass = 2u, UnboundCodepage = 4u,
    ClassEnvelope = 8u, ProxyOrUnknownClassMetadata = 16u,
    LayerByteOrderDisagreement = 32u, UnboundDependency = 64u,
    TextSemanticsNotIndependent = 128u, ExternalDataStorage = 256u
};
struct Profile {
    unsigned revision{0};
    Kind kind{Kind::Definition};
    Layout layout{Layout::LegacyInline};
    bool knownRevision{false}, knownBodyClass{false}, bytePath{false}, unicodePath{false};
    bool dictionaryFlag{false}, dataStorageFlag{false}, eedPresent{false};
};
inline Profile profile(unsigned revision, Kind kind, const Proof& proof) noexcept {
    Profile value;
    value.revision = revision; value.kind = kind;
    value.knownRevision = revision == 13u || revision == 14u || revision == 2000u
        || revision == 2004u || revision == 2007u || revision == 2010u
        || revision == 2013u || revision == 2018u;
    value.layout = revision >= 2010u ? Layout::Split2010
        : revision >= 2007u ? Layout::Split2007 : Layout::LegacyInline;
    value.knownBodyClass = proof.status == Status::Proven
        && proof.classVersion == (kind == Kind::Definition ? 0u : 2u);
    value.bytePath = kind == Kind::Definition && revision < 2007u;
    value.unicodePath = kind == Kind::Definition && revision >= 2007u;
    value.dictionaryFlag = revision >= 2004u;
    value.dataStorageFlag = revision >= 2013u;
    value.eedPresent = proof.eedChunks != 0u;
    return value;
}

namespace detail {
struct Bits {
    const std::uint8_t* bytes;
    std::uint32_t pos, end;
    bool good{true};
    std::uint64_t take(unsigned count) noexcept {
        if (!good || count > 64u || pos > end || count > end - pos) {
            good = false; return 0u;
        }
        std::uint64_t value = 0u;
        for (unsigned n = 0; n < count; ++n, ++pos)
            value = (value << 1u) | ((bytes[pos / 8u] >> (7u - pos % 8u)) & 1u);
        return value;
    }
    std::uint64_t little(unsigned count) noexcept {
        if (count > 8u) { good = false; return 0u; }
        std::uint64_t value = 0u;
        for (unsigned n = 0; n < count; ++n) value |= take(8u) << (8u * n);
        return value;
    }
    std::uint32_t shortValue() noexcept {
        switch (take(2u)) {
        case 0: return static_cast<std::uint32_t>(little(2u));
        case 1: return static_cast<std::uint32_t>(take(8u));
        case 2: return 0u;
        default: return 256u;
        }
    }
    std::uint32_t longValue() noexcept {
        switch (take(2u)) {
        case 0: return static_cast<std::uint32_t>(little(4u));
        case 1: return static_cast<std::uint32_t>(take(8u));
        case 2: return 0u;
        default: good = false; return 0u; // BL tag11 is unused, not BS's256
        }
    }
    bool skip(std::uint32_t count) noexcept {
        if (!good || pos > end || count > end - pos) { good = false; return false; }
        pos += count; return true;
    }
};
inline void span(Proof& proof, Field field, std::uint32_t begin, std::uint32_t end) noexcept {
    proof.fields[static_cast<std::size_t>(field)] = {begin, end, true};
}
struct Token {
    std::uint32_t value{0};
    std::uint8_t role{0}, length{0};
};
inline Token handle(Bits& bits, unsigned role, std::uint32_t base, bool own = false) noexcept {
    Token token;
    const auto header = static_cast<unsigned>(bits.take(8u));
    token.role = static_cast<std::uint8_t>(header >> 4u);
    token.length = static_cast<std::uint8_t>(header & 15u);
    if (!bits.good || token.length > 4u) { bits.good = false; return token; }
    for (unsigned n = 0; n < token.length; ++n)
        token.value = (token.value << 8u) | static_cast<std::uint32_t>(bits.take(8u));
    if (!bits.good) return token;
    if (own) {
        if (token.role != 0u || token.length == 0u || token.value == 0u) bits.good = false;
    } else if (token.role == 0u && token.length == 0u) {
        // Canonical NULL has no nonzero role to infer.
    } else if (token.role == role) {
        // Explicit absolute typed reference, including an explicit zero.
    } else if (role == 4u && (token.role == 6u || token.role == 8u
                              || token.role == 10u || token.role == 12u)) {
        // ODA offset handles are soft pointers only. Never relabel one as
        // hard ownership or let reference arithmetic wrap.
        std::uint64_t delta = token.value;
        if (token.role == 6u || token.role == 8u) {
            if (token.length != 0u) { bits.good = false; return token; }
            delta = 1u;
        }
        const bool add = token.role == 6u || token.role == 10u;
        if ((add && delta > (std::numeric_limits<std::uint32_t>::max)() - base)
            || (!add && delta > base)) { bits.good = false; return token; }
        token.value = add ? static_cast<std::uint32_t>(base + delta)
                          : static_cast<std::uint32_t>(base - delta);
    } else bits.good = false;
    return token;
}
inline bool supported(unsigned revision) noexcept {
    return revision == 13u || revision == 14u || revision == 2000u || revision == 2004u
        || revision == 2007u || revision == 2010u || revision == 2013u || revision == 2018u;
}
struct Ignore {
    bool operator()(std::uint32_t, std::uint32_t, Span, std::uint8_t) const noexcept { return true; }
};
struct IgnoreEed {
    bool operator()(const EedToken&) const noexcept { return true; }
};

template<class Visitor>
Status eed(Bits& body, unsigned revision, Proof& proof, Visitor visitor) noexcept {
    const auto start = body.pos;
    while (body.good) {
        const auto lengthStart = body.pos;
        const auto length = body.shortValue();
        if (!body.good) return Status::Malformed;
        if (!proof.at(Field::EedSize).present) span(proof, Field::EedSize, lengthStart, body.pos);
        if (length == 0u) { span(proof, Field::Eed, start, body.pos); return Status::Proven; }
        if (++proof.eedChunks > 65536u || length > 65535u) return Status::Malformed;
        EedToken token;
        token.index = proof.eedVariants++; token.chunk = proof.eedChunks - 1u; token.code = 1001u;
        const auto appStart = body.pos;
        const auto app = handle(body, 5u, 0u);
        token.value = app.value; token.role = app.role;
        token.span = token.valueSpan = {appStart, body.pos, true};
        // Explicit typed zero is a missing dependency, not proven membership
        // and not a reason to break the established unqualified fallback.
        if (!body.good || app.role != 5u || !visitor(token)) return Status::Malformed;
        const auto count = length * 8u;
        if (body.pos > body.end || count > body.end - body.pos) return Status::Malformed;
        Bits items{body.bytes, body.pos, body.pos + count};
        std::uint32_t chunkItems = 0u;
        while (items.pos < items.end && items.good) {
            if (++chunkItems > 65536u || ++proof.eedItems > 1000000u) return Status::Malformed;
            token = EedToken{};
            token.index = proof.eedVariants++; token.chunk = proof.eedChunks - 1u;
            const auto itemStart = items.pos;
            const auto code = static_cast<unsigned>(items.take(8u)); token.code = 1000u + code;
            const auto valueStart = items.pos;
            switch (code) {
            case 0: {
                const auto chars = static_cast<std::uint32_t>(revision >= 2007u ? items.little(2u) : items.take(8u));
                if (revision < 2007u) {
                    const auto first = items.take(8u), second = items.take(8u);
                    token.codePageBE = static_cast<std::uint16_t>((first << 8u) | second);
                    token.codePageLE = static_cast<std::uint16_t>(first | (second << 8u));
                    proof.eedLegacyStrings = true;
                } else proof.eedUnicodeStrings = true;
                token.value = chars;
                if (!items.skip(chars * (revision >= 2007u ? 16u : 8u))) return Status::Malformed;
                break;
            }
            case 2: token.value = items.take(8u); if (token.value > 1u) return Status::Malformed; break;
            case 3: {
                // The ODA text does not distinguish this byte order explicitly;
                // LibreDWG/local use LE and ACadSharp uses BE. Retain both.
                const auto at = items.pos;
                token.value = items.little(8u);
                Bits other{items.bytes, at, items.pos}; token.alternate = other.take(64u);
                if (token.value != token.alternate) proof.eedLayerEndianConflict = true;
                break;
            }
            case 4: { const auto bytes = static_cast<std::uint32_t>(items.take(8u));
                token.value = bytes; if (!items.skip(bytes * 8u)) return Status::Malformed; break; }
            case 5: token.value = items.take(64u); break;
            case 10: case 11: case 12: case 13: if (!items.skip(192u)) return Status::Malformed; break;
            case 40: case 41: case 42: if (!items.skip(64u)) return Status::Malformed; break;
            case 70: token.value = items.little(2u); break;
            case 71: token.value = items.little(4u); break;
            default: return items.good ? Status::Unsupported : Status::Malformed;
            }
            token.span = {itemStart, items.pos, true};
            token.valueSpan = {valueStart, items.pos, true};
            if (!items.good || !visitor(token)) return Status::Malformed;
        }
        if (!items.good || items.pos != items.end) return Status::Malformed;
        body.pos = items.end;
    }
    return Status::Malformed;
}
} // namespace detail

// reactorVisitor receives wire ordinal, resolved reference, exact span and
// literal role. Caller matching is separate from the independent role grammar.
template<class ReactorVisitor = detail::Ignore, class EedVisitor = detail::IgnoreEed>
Proof observe(const std::uint8_t* bytes, std::size_t size, unsigned revision,
              Kind kind, std::uint32_t handleBits = 0u,
              ReactorVisitor reactorVisitor = ReactorVisitor{}, EedVisitor eedVisitor = EedVisitor{},
              std::uint32_t definitionType = 102u) noexcept {
    Proof result;
    static_assert(noexcept(reactorVisitor(0u, 0u, Span{}, std::uint8_t{})),
                  "wire observer visitors must not throw");
    static_assert(noexcept(eedVisitor(EedToken{})), "EED observer visitors must not throw");
    if (!detail::supported(revision)) { result.status = Status::Unsupported; return result; }
    if (!bytes || size == 0u || size > 64u * 1024u * 1024u) return result;
    const auto total = static_cast<std::uint32_t>(size * 8u);
    detail::Bits body{bytes, 0u, total};
    auto begin = body.pos;
    if (revision >= 2010u) {
        const auto tag = body.take(2u);
        result.noncanonicalTypeTag = tag == 3u;
        result.objectType = static_cast<std::uint32_t>(tag == 0u ? body.take(8u)
            : tag == 1u ? body.take(8u) + 0x1F0u : body.little(2u));
    } else result.objectType = body.shortValue();
    detail::span(result, Field::Type, begin, body.pos);
    if (!body.good || (kind == Kind::Definition
        ? result.objectType != definitionType || (definitionType != 102u && definitionType < 500u)
        : result.objectType < 500u)) return result;
    if (revision >= 2010u) {
        if (handleBits == 0u || handleBits >= total) return result;
        result.dataEnd = total - handleBits;
    } else if (revision >= 2000u) {
        begin = body.pos;
        result.dataEnd = static_cast<std::uint32_t>(body.little(4u));
        detail::span(result, Field::DataEnd, begin, body.pos);
    }
    if (revision >= 2000u) {
        if (!body.good || result.dataEnd <= body.pos || result.dataEnd >= total) return result;
        body.end = result.dataEnd;
    }
    begin = body.pos;
    result.self = detail::handle(body, 0u, 0u, true).value;
    detail::span(result, Field::Self, begin, body.pos);
    const auto eedStatus = detail::eed(body, revision, result, eedVisitor);
    if (eedStatus != Status::Proven) { result.status = eedStatus; return result; }
    if (revision < 2000u) {
        begin = body.pos;
        result.dataEnd = static_cast<std::uint32_t>(body.little(4u));
        detail::span(result, Field::DataEnd, begin, body.pos);
        if (!body.good || result.dataEnd == 0u) { result.status = Status::Unsupported; return result; }
        if (result.dataEnd <= body.pos || result.dataEnd >= total) return result;
        body.end = result.dataEnd;
    }
    std::uint32_t stringsBegin = result.dataEnd, stringsEnd = result.dataEnd;
    bool hasStrings = false;
    if (revision >= 2007u) {
        detail::Bits tail{bytes, result.dataEnd - 1u, result.dataEnd};
        hasStrings = tail.take(1u) != 0u;
        std::uint32_t footer = result.dataEnd - 1u;
        if (hasStrings) {
            if (footer < 16u) return result;
            footer -= 16u; tail = {bytes, footer, result.dataEnd - 1u};
            auto length = static_cast<std::uint32_t>(tail.little(2u));
            if (length & 0x8000u) {
                if (footer < 16u) return result;
                footer -= 16u; tail = {bytes, footer, footer + 16u};
                length = (length & 0x7FFFu) | (static_cast<std::uint32_t>(tail.little(2u)) << 15u);
            }
            if (!tail.good || length > footer || length == 0u) return result;
            stringsEnd = footer; stringsBegin = footer - length;
            body.end = stringsBegin;
        } else body.end = footer;
        detail::span(result, Field::StringFooter, footer, result.dataEnd);
    }
    begin = body.pos;
    result.reactorCount = body.longValue();
    detail::span(result, Field::ReactorCount, begin, body.pos);
    if (!body.good || result.reactorCount > 1000000u) return result;
    if (revision >= 2004u) {
        begin = body.pos; result.dictionaryMissing = body.take(1u) != 0u;
        detail::span(result, Field::XDictMissing, begin, body.pos);
    }
    if (revision >= 2013u) {
        begin = body.pos; result.dataStorage = body.take(1u) != 0u;
        detail::span(result, Field::DataStorage, begin, body.pos);
    }
    begin = body.pos; result.classVersion = body.longValue();
    detail::span(result, Field::ClassVersion, begin, body.pos);
    if (!body.good || result.classVersion > 0x7FFFFFFFu) return result;
    if (kind == Kind::Definition) {
        for (unsigned n = 0; n < 2u; ++n) {
            begin = body.pos; result.doubles[n] = body.little(8u);
            detail::span(result, n ? Field::ImageV : Field::ImageU, begin, body.pos);
        }
        if (revision >= 2007u && !hasStrings) return result;
        detail::Bits strings{bytes, stringsBegin, stringsEnd};
        auto& path = revision >= 2007u ? strings : body;
        begin = path.pos; result.pathUnits = path.shortValue();
        detail::span(result, Field::PathLength, begin, path.pos);
        if (!path.good || result.pathUnits > 32767u) return result;
        begin = path.pos;
        if (!path.skip(result.pathUnits * (revision >= 2007u ? 16u : 8u))) return result;
        detail::span(result, Field::PathUnits, begin, path.pos);
        if (revision >= 2007u) {
            if (strings.pos > strings.end || strings.end - strings.pos > 14u) return result;
            detail::span(result, Field::StringPadding, strings.pos, strings.end);
        }
        begin = body.pos; result.loaded = body.take(1u) != 0u;
        detail::span(result, Field::Loaded, begin, body.pos);
        begin = body.pos; result.resolution = static_cast<std::uint8_t>(body.take(8u));
        detail::span(result, Field::Resolution, begin, body.pos);
        for (unsigned n = 2u; n < 4u; ++n) {
            begin = body.pos; result.doubles[n] = body.little(8u);
            detail::span(result, n == 2u ? Field::PixelU : Field::PixelV, begin, body.pos);
        }
        for (const auto bits : result.doubles)
            if ((bits & UINT64_C(0x7FF0000000000000)) == UINT64_C(0x7FF0000000000000)) return result;
    } else if (hasStrings) { result.status = Status::Unsupported; return result; }
    if (!body.good || body.pos > body.end) return result;
    if (revision >= 2007u && !hasStrings && body.end - body.pos > 7u) {
        // The existing native R2010+ writer emits an explicit 24-zero empty
        // trailer. Distinguish that observed profile, not arbitrary extra data.
        if (result.dataEnd < 24u || result.dataEnd - body.pos > 31u) return result;
        detail::Bits empty{bytes, result.dataEnd - 24u, result.dataEnd};
        if (empty.take(24u) != 0u || !empty.good) return result;
        body.end = result.dataEnd - 24u;
        result.emptyStringTrailer = true;
        detail::span(result, Field::StringFooter, body.end, result.dataEnd);
    }
    if (body.pos > body.end || body.end - body.pos > 7u) return result;
    detail::span(result, Field::ScalarPadding, body.pos, body.end);
    detail::Bits handles{bytes, result.dataEnd, total};
    begin = handles.pos;
    const auto owner = detail::handle(handles, kind == Kind::Definition ? 3u : 4u, result.self);
    result.owner = owner.value; result.ownerRole = owner.role;
    detail::span(result, Field::Owner, begin, handles.pos);
    if (!handles.good || result.reactorCount > (handles.end - handles.pos) / 8u) return result;
    begin = handles.pos;
    for (std::uint32_t n = 0; n < result.reactorCount; ++n) {
        const auto start = handles.pos;
        const auto token = detail::handle(handles, 4u, result.self);
        if (!handles.good || !reactorVisitor(n, token.value, Span{start, handles.pos, true}, token.role)) return result;
    }
    detail::span(result, Field::Reactors, begin, handles.pos);
    if (!result.dictionaryMissing) {
        begin = handles.pos;
        const auto dictionary = detail::handle(handles, 3u, result.self);
        result.dictionary = dictionary.value; result.dictionaryRole = dictionary.role;
        detail::span(result, Field::XDictionary, begin, handles.pos);
    }
    if (!handles.good || handles.end - handles.pos > 7u) return result;
    detail::span(result, Field::HandlePadding, handles.pos, handles.end);
    result.status = result.dataStorage ? Status::Unsupported : Status::Proven;
    return result;
}
} // namespace dx_raster_wire
#endif
