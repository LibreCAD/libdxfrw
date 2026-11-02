// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_raster_dwg_wire.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

namespace wire = dx_raster_wire;
namespace {
// Scratch-only author, not a production encoder. Literal primitive checks below
// anchor its representation before it is used to build composite test records.
struct Pack {
    std::vector<std::uint8_t> bytes;
    std::uint32_t n{0};
    void put(std::uint64_t value, unsigned width) {
        for (unsigned k = width; k; --k, ++n) {
            if (n / 8u == bytes.size()) bytes.push_back(0u);
            bytes[n / 8u] |= static_cast<std::uint8_t>(((value >> (k - 1u)) & 1u) << (7u - n % 8u));
        }
    }
    void little(std::uint64_t value, unsigned width) {
        for (unsigned k = 0; k != width; ++k) put((value >> (k * 8u)) & 255u, 8u);
    }
    void bs(unsigned value) {
        if (value == 0) put(2, 2); else if (value == 256) put(3, 2);
        else if (value < 256) { put(1, 2); put(value, 8); }
        else { put(0, 2); little(value, 2); }
    }
    void bl(unsigned value) {
        if (value == 0) put(2, 2); else if (value < 256) { put(1, 2); put(value, 8); }
        else { put(0, 2); little(value, 4); }
    }
    void handle(unsigned role, std::uint32_t value) {
        unsigned width = 0;
        for (auto v = value; v; v >>= 8u) ++width;
        put(value ? (role << 4u) | width : 0u, 8u);
        for (unsigned k = width; k; --k) put((value >> (8u * (k - 1u))) & 255u, 8u);
    }
    void append(const Pack& other) {
        for (std::uint32_t k = 0; k < other.n; ++k) put((other.bytes[k / 8u] >> (7u - k % 8u)) & 1u, 1u);
    }
    void align() { while (n % 8u) put(0u, 1u); }
    void patch(std::uint32_t pos, std::uint64_t value, unsigned width) {
        for (unsigned k = width; k; --k, ++pos) {
            auto& byte = bytes[pos / 8u]; const auto mask = static_cast<std::uint8_t>(1u << (7u - pos % 8u));
            byte = static_cast<std::uint8_t>((byte & ~mask) | (((value >> (k - 1u)) & 1u) ? mask : 0u));
        }
    }
    void patchLittle(std::uint32_t pos, std::uint32_t value) {
        for (unsigned k = 0; k != 4; ++k) patch(pos + 8u * k, (value >> (8u * k)) & 255u, 8u);
    }
};
struct Fixture {
    Pack bytes;
    std::array<wire::Span, static_cast<std::size_t>(wire::Field::Count)> spans{};
    unsigned revision{0}, objectType{0}; wire::Kind kind{wire::Kind::Definition};
    std::uint32_t handles{0};
};
Fixture make(unsigned revision, wire::Kind kind, bool missing = false, bool extended = false,
             unsigned longTag = 2u, unsigned customType = 532u, bool nullLinks = false,
             const Pack* eed = nullptr, bool customDefinition = false) {
    Fixture f; f.revision = revision; f.kind = kind;
    auto& out = f.bytes;
    const auto mark = [&](wire::Field field, auto emit) {
        const auto begin = out.n; emit();
        f.spans[static_cast<std::size_t>(field)] = {begin, out.n, true};
    };
    const unsigned type = kind == wire::Kind::Definition && !customDefinition ? 102u : customType;
    f.objectType = type;
    mark(wire::Field::Type, [&]() {
        if (revision < 2010u) out.bs(type);
        else if (type < 256u) { out.put(0u, 2u); out.put(type, 8u); }
        else if (type >= 0x1F0u && type <= 0x2EFu) { out.put(1u, 2u); out.put(type - 0x1F0u, 8u); }
        else { out.put(2u, 2u); out.little(type, 2u); }
    });
    std::uint32_t patchAt = 0u;
    const auto placeholder = [&]() { patchAt = out.n; out.little(0u, 4u); };
    if (revision >= 2000u && revision < 2010u) mark(wire::Field::DataEnd, placeholder);
    mark(wire::Field::Self, [&]() { out.handle(0u, 0x11223344u); });
    if (eed) mark(wire::Field::Eed, [&]() { out.append(*eed); });
    else mark(wire::Field::EedSize, [&]() { out.bs(0u); });
    if (revision < 2000u) mark(wire::Field::DataEnd, placeholder);
    mark(wire::Field::ReactorCount, [&]() { out.bl(nullLinks ? 0u : 2u); });
    if (revision >= 2004u) mark(wire::Field::XDictMissing, [&]() { out.put(missing ? 1u : 0u, 1u); });
    if (revision >= 2013u) mark(wire::Field::DataStorage, [&]() { out.put(0u, 1u); });
    mark(wire::Field::ClassVersion, [&]() {
        const auto value = kind == wire::Kind::Definition ? 0u : 2u;
        if (longTag == 0u) { out.put(0u, 2u); out.little(value, 4u); }
        else if (longTag == 1u) { out.put(1u, 2u); out.put(value, 8u); }
        else out.bl(value);
    });
    Pack strings;
    if (kind == wire::Kind::Definition) {
        mark(wire::Field::ImageU, [&]() { out.little(UINT64_C(0x4030000000000000), 8u); });
        mark(wire::Field::ImageV, [&]() { out.little(UINT64_C(0x4020000000000000), 8u); });
        auto& path = revision >= 2007u ? strings : out;
        const auto start = path.n;
        const auto length = extended ? 2200u : revision >= 2007u ? 2u : 1u;
        path.bs(length);
        f.spans[static_cast<std::size_t>(wire::Field::PathLength)] = {start, path.n, true};
        const auto content = path.n;
        for (unsigned k = 0; k != length; ++k) path.little(k ? 0x3A9u : 0x41u, revision >= 2007u ? 2u : 1u);
        f.spans[static_cast<std::size_t>(wire::Field::PathUnits)] = {content, path.n, true};
        mark(wire::Field::Loaded, [&]() { out.put(1u, 1u); });
        mark(wire::Field::Resolution, [&]() { out.put(5u, 8u); });
        mark(wire::Field::PixelU, [&]() { out.little(UINT64_C(0x3FD0000000000000), 8u); });
        mark(wire::Field::PixelV, [&]() { out.little(UINT64_C(0x3FE0000000000000), 8u); });
    }
    if (revision >= 2007u) {
        if (kind == wire::Kind::Definition) {
            const auto start = out.n;
            for (auto field : {wire::Field::PathLength, wire::Field::PathUnits}) {
                auto& s = f.spans[static_cast<std::size_t>(field)]; s.begin += start; s.end += start;
            }
            out.append(strings);
            while (out.n % 8u != 7u) out.put(0u, 1u);
            const auto stringBits = out.n - start;
            if (stringBits > 32767u) out.little(stringBits >> 15u, 2u);
            out.little((stringBits & 32767u) | (stringBits > 32767u ? 0x8000u : 0u), 2u);
            out.put(1u, 1u);
        } else {
            out.align();
            if (revision >= 2010u) out.put(0u, 24u); // observed empty-trailer profile
        }
    }
    const auto end = out.n;
    if (revision < 2010u) out.patchLittle(patchAt, end);
    mark(wire::Field::Owner, [&]() { out.handle(kind == wire::Kind::Definition ? 3u : 4u, nullLinks ? 0u : 0x2211u); });
    mark(wire::Field::Reactors, [&]() { if (!nullLinks) { out.handle(4u, 0x3344u); out.handle(4u, 0x5566u); } });
    if (revision < 2004u || !missing) mark(wire::Field::XDictionary, [&]() { out.handle(3u, nullLinks ? 0u : 0x7788u); });
    out.align(); f.handles = out.n - end;
    return f;
}
wire::Proof scan(const Fixture& f) {
    return wire::observe(f.bytes.bytes.data(), f.bytes.bytes.size(), f.revision, f.kind, f.handles,
        wire::detail::Ignore{}, wire::detail::IgnoreEed{}, f.objectType);
}
bool sameSpans(const Fixture& f, const wire::Proof& proof) {
    for (std::size_t n = 0; n < f.spans.size(); ++n) {
        const auto expected = f.spans[n];
        if (expected.present && (!proof.fields[n].present || expected.begin != proof.fields[n].begin
            || expected.end != proof.fields[n].end)) {
            std::cerr << "span mismatch revision=" << f.revision << " kind=" << static_cast<int>(f.kind)
                      << " field=" << n << '\n'; return false;
        }
    }
    return true;
}
struct EedFixture { Pack bytes; std::vector<wire::EedToken> tokens; };
EedFixture eed(unsigned revision) {
    EedFixture f;
    for (unsigned chunk = 0; chunk != 2u; ++chunk) {
        Pack data;
        std::vector<wire::EedToken> items;
        const auto item = [&](unsigned code, auto emit) {
            wire::EedToken token; token.chunk = chunk; token.code = 1000u + code;
            const auto begin = data.n; data.put(code, 8u); const auto value = data.n; emit();
            token.span = {begin, data.n, true}; token.valueSpan = {value, data.n, true};
            items.push_back(token);
        };
        item(3u, [&]() { data.little(chunk ? UINT64_C(0x0102030405060708) : 0x123u, 8u); });
        item(0u, [&]() {
            if (revision >= 2007u) { data.little(2u, 2u); data.little(0x41u, 2u); data.little(0x3A9u, 2u); }
            else { data.put(1u, 8u); data.put(0x04E4u, 16u); data.put(0xE9u, 8u); }
        });
        if (!chunk) {
            item(2u, [&]() { data.put(0u, 8u); });
            item(4u, [&]() { data.put(3u, 8u); data.put(0x030501u, 24u); });
            item(5u, [&]() { data.put(UINT64_C(0x0102030405060708), 64u); });
            for (auto code : {10u,11u,12u,13u}) item(code, [&]() { for (auto n : {1u,2u,3u}) data.little(n, 8u); });
            for (auto code : {40u,41u,42u}) item(code, [&]() { data.little(UINT64_C(0x3FF0000000000000), 8u); });
            item(70u, [&]() { data.little(0xFFFEu, 2u); });
            item(71u, [&]() { data.little(0xFFFFFFFDu, 4u); });
            item(2u, [&]() { data.put(1u, 8u); });
        }
        f.bytes.bs(data.n / 8u);
        wire::EedToken app; app.chunk = chunk; app.code = 1001u; app.index = static_cast<std::uint32_t>(f.tokens.size());
        const auto begin = f.bytes.n; f.bytes.handle(5u, 0x456u);
        app.span = app.valueSpan = {begin, f.bytes.n, true}; f.tokens.push_back(app);
        const auto offset = f.bytes.n; f.bytes.append(data);
        for (auto token : items) {
            token.index = static_cast<std::uint32_t>(f.tokens.size());
            token.span.begin += offset; token.span.end += offset;
            token.valueSpan.begin += offset; token.valueSpan.end += offset;
            f.tokens.push_back(token);
        }
    }
    f.bytes.bs(0u);
    return f;
}
bool run() {
    // Byte literals, not inverse author/observer agreement, anchor primitives.
    const std::uint8_t literals[]{0x40u, 0x55u, 0xAAu, 0xFFu, 0x01u, 0x00u};
    wire::detail::Bits bits{literals, 0u, 48u};
    if (bits.shortValue() != 1u || bits.pos != 10u) return false;
    wire::detail::Bits handleBits{literals, 8u, 48u};
    const auto h = wire::detail::handle(handleBits, 5u, 0u);
    if (h.role != 5u || h.length != 5u || handleBits.good) return false; // length>32-bit
    const std::uint8_t literalHandle[]{0x32u, 0x22u, 0x11u};
    wire::detail::Bits owner{literalHandle, 0u, 24u};
    if (wire::detail::handle(owner, 3u, 0u).value != 0x2211u || !owner.good) return false;
    // Frozen whole EED block: BS9, H5.2.456, code3, eight LE bytes08..01,
    // BS0 terminator. Exact108-bit extent and spans are hand-specified.
    const std::uint8_t literalEed[]{0x42u,0x54u,0x81u,0x15u,0x80u,0xC2u,0x01u,
                                   0xC1u,0x81u,0x41u,0x00u,0xC0u,0x80u,0x60u};
    wire::detail::Bits literalCursor{literalEed, 0u, 108u}; wire::Proof literalProof;
    unsigned literalItems = 0u;
    const auto literalVisitor = [&](const wire::EedToken& token) noexcept {
        ++literalItems;
        if (token.index == 0u) return token.code == 1001u && token.value == 0x456u && token.role == 5u
            && token.span.begin == 10u && token.span.end == 34u;
        return token.index == 1u && token.code == 1003u && token.span.begin == 34u && token.span.end == 106u
            && token.valueSpan.begin == 42u && token.valueSpan.end == 106u
            && token.value == UINT64_C(0x0102030405060708) && token.alternate == UINT64_C(0x0807060504030201);
    };
    if (wire::detail::eed(literalCursor, 2018u, literalProof, literalVisitor) != wire::Status::Proven
        || literalItems != 2u || literalCursor.pos != 108u || literalProof.eedChunks != 1u) return false;
    for (std::uint32_t end = 0; end < 108u; ++end) {
        wire::detail::Bits shortCursor{literalEed, 0u, end}; wire::Proof shortProof;
        if (wire::detail::eed(shortCursor, 2018u, shortProof, wire::detail::IgnoreEed{}) == wire::Status::Proven) return false;
    }
    Pack missingApp; missingApp.bs(2u); missingApp.put(0x50u, 8u);
    missingApp.put(2u, 8u); missingApp.put(0u, 8u); missingApp.bs(0u);
    wire::detail::Bits missingCursor{missingApp.bytes.data(), 0u, missingApp.n}; wire::Proof missingProof;
    bool explicitZero = false;
    const auto missingVisitor = [&](const wire::EedToken& token) noexcept {
        if (token.code == 1001u) explicitZero = token.role == 5u && token.value == 0u;
        return true;
    };
    if (wire::detail::eed(missingCursor, 2018u, missingProof, missingVisitor) != wire::Status::Proven
        || !explicitZero) return false;
    unsigned positive = 0u, negative = 0u;
    for (const auto revision : {13u, 14u, 2000u, 2004u, 2007u, 2010u, 2013u, 2018u})
        for (const auto kind : {wire::Kind::Definition, wire::Kind::Reactor})
            for (const bool missing : {false, true}) {
                const auto f = make(revision, kind, missing);
                const auto proof = scan(f);
                if (proof.status != wire::Status::Proven || !sameSpans(f, proof)
                    || proof.owner != 0x2211u || proof.reactorCount != 2u
                    || proof.ownerRole != (kind == wire::Kind::Definition ? 3u : 4u)) {
                    std::cerr << "positive revision=" << revision << " kind=" << static_cast<int>(kind) << '\n';
                    return false;
                }
                ++positive;
                const auto profile = wire::profile(revision, kind, proof);
                if (!profile.knownRevision || !profile.knownBodyClass || profile.eedPresent
                    || profile.dictionaryFlag != (revision >= 2004u)
                    || profile.dataStorageFlag != (revision >= 2013u)
                    || profile.bytePath != (kind == wire::Kind::Definition && revision < 2007u)
                    || profile.unicodePath != (kind == wire::Kind::Definition && revision >= 2007u)) return false;
                for (auto field : {wire::Field::Owner, wire::Field::Reactors, wire::Field::XDictionary}) {
                    if (!proof.at(field).present) continue;
                    auto broken = f;
                    broken.bytes.patch(proof.at(field).begin, 5u, 4u); // wrong role, same value/size
                    if (scan(broken).status == wire::Status::Proven) return false;
                    if (scan(f).status != wire::Status::Proven) return false;
                    ++negative;
                }
                for (std::size_t size = 0; size < f.bytes.bytes.size(); ++size) {
                    if (wire::observe(f.bytes.bytes.data(), size, revision, kind, f.handles).status == wire::Status::Proven)
                        return false;
                    ++negative;
                }
                auto invalid = f;
                invalid.bytes.patch(proof.at(wire::Field::ClassVersion).begin, 3u, 2u);
                if (scan(invalid).status != wire::Status::Malformed) return false;
                ++negative;
                if (kind == wire::Kind::Definition) {
                    invalid = f;
                    invalid.bytes.patch(proof.at(wire::Field::ImageU).begin + 48u, 0xF07Fu, 16u);
                    if (scan(invalid).status != wire::Status::Malformed) return false;
                    ++negative;
                }
                if (revision >= 2007u && kind == wire::Kind::Definition) {
                    invalid = f;
                    invalid.bytes.patch(proof.at(wire::Field::StringFooter).begin, 0xFFFFu, 16u);
                    if (scan(invalid).status != wire::Status::Malformed) return false;
                    ++negative;
                }
            }
    for (auto revision : {13u, 14u, 2000u, 2004u, 2007u, 2010u, 2013u, 2018u})
        for (auto kind : {wire::Kind::Definition, wire::Kind::Reactor}) {
            const auto f = make(revision, kind, false, false, 2u, 532u, true);
            const auto p = scan(f);
            if (p.status != wire::Status::Proven || p.owner != 0u || p.dictionary != 0u
                || p.reactorCount != 0u || p.ownerRole != 0u || !sameSpans(f, p)) return false;
            ++positive;
        }
    for (auto revision : {2007u, 2010u, 2013u, 2018u}) {
        const auto f = make(revision, wire::Kind::Definition, false, true);
        const auto proof = scan(f);
        if (proof.status != wire::Status::Proven || proof.at(wire::Field::StringFooter).width() != 33u
            || !sameSpans(f, proof)) return false;
        ++positive;
    }
    for (auto tag : {0u, 1u, 2u}) for (auto kind : {wire::Kind::Definition, wire::Kind::Reactor}) {
        const auto f = make(2018u, kind, true, false, tag, 0xBEEFu);
        if (scan(f).status != wire::Status::Proven || !sameSpans(f, scan(f))) return false;
        ++positive;
    }
    struct Offset { std::array<std::uint8_t, 5> bytes; unsigned role, base, expected; bool good; };
    const Offset offsets[]{
        {{{0x60u, 0u, 0u, 0u, 0u}}, 4u, 10u, 11u, true},
        {{{0x80u, 0u, 0u, 0u, 0u}}, 4u, 10u, 9u, true},
        {{{0xA1u, 2u, 0u, 0u, 0u}}, 4u, 10u, 12u, true},
        {{{0xC1u, 2u, 0u, 0u, 0u}}, 4u, 10u, 8u, true},
        {{{0x00u, 0u, 0u, 0u, 0u}}, 3u, 10u, 0u, true},
        {{{0x30u, 0u, 0u, 0u, 0u}}, 3u, 10u, 0u, true},
        {{{0x40u, 0u, 0u, 0u, 0u}}, 3u, 10u, 0u, false},
        {{{0x60u, 0u, 0u, 0u, 0u}}, 3u, 10u, 0u, false},
        {{{0x80u, 0u, 0u, 0u, 0u}}, 4u, 0u, 0u, false},
        {{{0x61u, 0u, 0u, 0u, 0u}}, 4u, 10u, 0u, false},
        {{{0xA4u, 0xFFu, 0xFFu, 0xFFu, 0xFFu}}, 4u, 10u, 0u, false},
        {{{0x60u, 0u, 0u, 0u, 0u}}, 4u, 0xFFFFFFFFu, 0u, false},
    };
    for (const auto& input : offsets) {
        wire::detail::Bits cursor{input.bytes.data(), 0u, 40u};
        const auto token = wire::detail::handle(cursor, input.role, input.base);
        if (cursor.good != input.good || (input.good && token.value != input.expected)) return false;
        if (input.good) ++positive; else ++negative;
    }
    unsigned eedPositive = 0u, eedNegative = 0u;
    for (auto revision : {13u,14u,2000u,2004u,2007u,2010u,2013u,2018u})
        for (auto kind : {wire::Kind::Definition, wire::Kind::Reactor}) {
            const auto authored = eed(revision);
            const auto f = make(revision, kind, false, false, 2u, 0xBEEFu, false, &authored.bytes);
            const auto start = f.spans[static_cast<std::size_t>(wire::Field::Eed)].begin;
            std::uint32_t visited = 0u;
            const auto visitor = [&](const wire::EedToken& token) noexcept {
                if (visited >= authored.tokens.size()) return false;
                const auto& expected = authored.tokens[visited++];
                if (token.index != expected.index || token.chunk != expected.chunk || token.code != expected.code
                    || token.span.begin != expected.span.begin + start || token.span.end != expected.span.end + start
                    || token.valueSpan.begin != expected.valueSpan.begin + start
                    || token.valueSpan.end != expected.valueSpan.end + start) return false;
                if (token.code == 1001u && (token.value != 0x456u || token.role != 5u)) return false;
                if (token.code == 1003u && (token.value != (token.chunk ? UINT64_C(0x0102030405060708) : 0x123u)
                    || token.alternate != (token.chunk ? UINT64_C(0x0807060504030201) : UINT64_C(0x2301000000000000)))) return false;
                if (token.code == 1000u && revision < 2007u && (token.codePageBE != 1252u || token.codePageLE != 0xE404u)) return false;
                return true;
            };
            const auto p = wire::observe(f.bytes.bytes.data(), f.bytes.bytes.size(), revision, kind, f.handles,
                                        wire::detail::Ignore{}, visitor);
            if (p.status != wire::Status::Proven || !sameSpans(f, p) || visited != authored.tokens.size()
                || p.eedChunks != 2u || p.eedVariants != visited || p.eedItems + 2u != visited
                || !p.eedLayerEndianConflict || !wire::profile(revision, kind, p).eedPresent) return false;
            ++eedPositive;
            for (const auto& token : authored.tokens) {
                auto broken = f;
                if (token.code == 1001u) broken.bytes.patch(start + token.span.begin, 4u, 4u);
                else if (token.code == 1000u) broken.bytes.patch(start + token.valueSpan.begin, 0xFFFFu, 16u);
                else if (token.code == 1002u) broken.bytes.patch(start + token.valueSpan.begin, 2u, 8u);
                else continue;
                if (scan(broken).status != wire::Status::Malformed) return false;
                ++eedNegative;
            }
            auto unknown = f;
            unknown.bytes.patch(start + authored.tokens[1].span.begin, 1u, 8u); // explicitly invalid1001 item
            if (scan(unknown).status != wire::Status::Unsupported) return false;
            ++eedNegative;
            for (std::size_t bytes = 0; bytes < f.bytes.bytes.size(); ++bytes) {
                if (wire::observe(f.bytes.bytes.data(), bytes, revision, kind, f.handles).status == wire::Status::Proven) return false;
                ++eedNegative;
            }
        }
    unsigned customProfiles = 0u;
    for (auto revision : {13u,14u,2000u,2004u,2007u,2010u,2013u,2018u}) for (bool withEed : {false,true}) {
        const auto authored = eed(revision);
        const auto f = make(revision, wire::Kind::Definition, false, false, 2u, 715u, false,
                            withEed ? &authored.bytes : nullptr, true);
        const auto p = scan(f);
        if (p.status != wire::Status::Proven || p.objectType != 715u || !sameSpans(f, p)
            || !wire::profile(revision, wire::Kind::Definition, p).knownBodyClass) return false;
        ++customProfiles;
    }
    auto unsupported = make(2018u, wire::Kind::Definition);
    auto proof = scan(unsupported);
    unsupported.bytes.patch(proof.at(wire::Field::EedSize).begin, 1u, 2u);
    if (scan(unsupported).status != wire::Status::Malformed) return false; // nonempty chunk lacks its framing
    unsupported = make(2018u, wire::Kind::Definition);
    unsupported.bytes.patch(scan(unsupported).at(wire::Field::DataStorage).begin, 1u, 1u);
    if (scan(unsupported).status != wire::Status::Unsupported) return false;
    for (auto kind : {wire::Kind::Definition, wire::Kind::Reactor}) {
        auto future = make(2018u, kind, false, false, 0u);
        future.bytes.patchLittle(scan(future).at(wire::Field::ClassVersion).begin + 2u, 9u);
        const auto p = scan(future);
        if (p.status != wire::Status::Proven || wire::profile(2018u, kind, p).knownBodyClass) return false;
    }
    if (wire::profile(2025u, wire::Kind::Definition, {}).knownRevision) return false;
    std::cout << "Independent raster wire proof: " << positive << " positive, " << negative
              << " malformed/truncation controls; EED " << eedPositive << " positive, " << eedNegative
              << " negative; custom IMAGEDEF profiles=" << customProfiles
              << "; DataStorage/text/ambiguous layer byte order stay unqualified\n";
    return true;
}
} // namespace
int main() { return run() ? 0 : 1; }
