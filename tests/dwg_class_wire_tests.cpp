// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_dwg_class_wire.h"
#include <iostream>
#include <string>
#include <vector>

namespace wire = dx_class_wire;
namespace {
struct Pack {
    std::vector<std::uint8_t> bytes;
    std::uint32_t bits{0};
    void put(std::uint32_t value, unsigned width) {
        for (unsigned n = width; n; --n, ++bits) {
            if (bits / 8u == bytes.size()) bytes.push_back(0u);
            bytes[bits / 8u] |= static_cast<std::uint8_t>(((value >> (n - 1u)) & 1u) << (7u - bits % 8u));
        }
    }
    void little(std::uint32_t value, unsigned width) {
        for (unsigned n = 0; n < width; ++n) put((value >> (n * 8u)) & 255u, 8u);
    }
    void bs(std::uint32_t value) {
        if (value == 0u) put(2u, 2u);
        else if (value == 256u) put(3u, 2u);
        else if (value < 256u) { put(1u, 2u); put(value, 8u); }
        else { put(0u, 2u); little(value, 2u); }
    }
    void bl(std::uint32_t value) {
        if (!value) put(2u, 2u);
        else if (value < 256u) { put(1u, 2u); put(value, 8u); }
        else { put(0u, 2u); little(value, 4u); }
    }
    void literal(const char* text) {
        for (; *text; ++text) if (*text == '0' || *text == '1') put(*text == '1', 1u);
    }
    void patch(std::uint32_t at, std::uint32_t value, unsigned width) {
        for (unsigned n = width; n; --n, ++at) {
            auto& byte = bytes[at / 8u];
            const auto mask = static_cast<std::uint8_t>(1u << (7u - at % 8u));
            byte = static_cast<std::uint8_t>((byte & ~mask) | (((value >> (n - 1u)) & 1u) ? mask : 0u));
        }
    }
};
struct Record {
    Pack data, strings;
    unsigned revision{0};
    wire::Trailer trailer{wire::Trailer::OdaBitShort};
    std::uint32_t start{0}, stringStart{0};
    wire::View dataView() const { return {data.bytes.data(), data.bytes.size(), start, data.bits}; }
    wire::View stringView() const { return {strings.bytes.data(), strings.bytes.size(), stringStart, strings.bits}; }
};
Record make(unsigned revision, wire::Trailer trailer, unsigned prefix = 0u,
            std::uint32_t version = 257u, std::uint32_t maintenance = 329u,
            const std::vector<std::uint16_t>& app = {'A'}, std::uint32_t instances = 0u) {
    Record value;
    value.revision = revision; value.trailer = trailer;
    value.data.put(0u, prefix); value.start = value.data.bits;
    value.strings.put(0u, prefix); value.stringStart = value.strings.bits;
    value.data.bs(500u); value.data.bs(0u);
    auto& names = revision >= 2007u ? value.strings : value.data;
    for (const auto& text : {app, std::vector<std::uint16_t>{'C'}, std::vector<std::uint16_t>{'R'}}) {
        names.bs(static_cast<std::uint32_t>(text.size()));
        for (auto unit : text) names.little(unit, revision >= 2007u ? 2u : 1u);
    }
    value.data.put(0u, 1u); value.data.bs(0x1F3u);
    if (revision >= 2004u) {
        value.data.bl(instances);
        if (trailer == wire::Trailer::OdaBitShort) { value.data.bs(version); value.data.bs(maintenance); }
        else { value.data.bl(version); value.data.bl(maintenance); }
        value.data.bl(0u); value.data.bl(0xFFFFFFFFu);
    }
    return value;
}
wire::Proof observe(const Record& value) {
    return wire::observe(value.dataView(), value.stringView(), value.revision, value.trailer);
}
std::size_t positives = 0u, refusals = 0u, texts = 0u;
bool require(bool condition, const char* message) {
    if (!condition) std::cerr << message << '\n';
    return condition;
}
bool literal() {
    Pack expected;
    // Manually specified: BS500, BS0, TV(A), TV(C), TV(R), B0, BS0x1F3.
    expected.literal("00 11110100 00000001 10 "
        "01 00000001 01000001 01 00000001 01000011 "
        "01 00000001 01010010 0 00 11110011 00000001");
    const auto value = make(2000u, wire::Trailer::OdaBitShort);
    if (!require(expected.bits == 93u && value.data.bits == 93u
        && expected.bytes == value.data.bytes, "literal class-record author anchor failed")) return false;
    const auto proof = wire::observe({expected.bytes.data(), expected.bytes.size(), 0u, 93u}, {}, 2000u);
    if (!require(proof.status == wire::Status::Observed && proof.number == 500u
        && proof.itemClass == 0x1F3u && proof.fields[0].begin == 0u && proof.fields[0].end == 18u
        && proof.text[0].length.begin == 20u && proof.text[0].length.end == 30u
        && proof.text[0].units.begin == 30u && proof.text[0].units.end == 38u,
        "literal independent spans/values failed")) return false;
    ++positives;
    return true;
}
bool matrix() {
    for (auto revision : {13u, 14u, 2000u, 2004u, 2007u, 2010u, 2013u, 2018u}) {
        for (auto trailer : {wire::Trailer::OdaBitShort, wire::Trailer::ProducerBitLong}) {
            for (unsigned prefix = 0; prefix < 8u; ++prefix) {
                const auto record = make(revision, trailer, prefix);
                const auto proof = observe(record);
                const auto names = proof.split ? record.stringView() : record.dataView();
                if (!require(proof.status == wire::Status::Observed && proof.trailer == trailer
                    && proof.number == 500u && proof.proxy == 0u && !proof.zombie
                    && !proof.unknownItemClass && proof.itemClass == 0x1F3u
                    && (revision < 2004u || (proof.instances == 0 && proof.dwgVersion == 257
                        && proof.maintenance == 329 && proof.unknown1 == 0 && proof.unknown2 == -1))
                    && wire::equalsUtf8(names, proof.text[0], proof.split, "A", 1u)
                    && wire::equalsUtf8(names, proof.text[1], proof.split, "C", 1u)
                    && wire::equalsUtf8(names, proof.text[2], proof.split, "R", 1u),
                    "revision/profile/unaligned matrix failed")) return false;
                ++positives;
                for (auto end = record.start; end < record.data.bits; ++end) {
                    auto view = record.dataView(); view.end = end;
                    if (!require(wire::observe(view, record.stringView(), revision, trailer).status
                        == wire::Status::Malformed, "short data prefix was accepted")) return false;
                    ++refusals;
                }
                if (proof.split) for (auto end = record.stringStart; end < record.strings.bits; ++end) {
                    auto view = record.stringView(); view.end = end;
                    if (!require(wire::observe(record.dataView(), view, revision, trailer).status
                        == wire::Status::Malformed, "short detached-string prefix was accepted")) return false;
                    ++refusals;
                }
            }
        }
    }
    return true;
}
bool profiles() {
    for (auto revision : {2004u, 2007u, 2010u, 2013u, 2018u}) {
        for (auto trailer : {wire::Trailer::OdaBitShort, wire::Trailer::ProducerBitLong}) {
            const auto record = make(revision, trailer, 3u);
            const auto other = trailer == wire::Trailer::OdaBitShort
                ? wire::Trailer::ProducerBitLong : wire::Trailer::OdaBitShort;
            if (!require(wire::observe(record.dataView(), record.stringView(), revision, other).status
                == wire::Status::Malformed, "different large-version profile was silently inferred")) return false;
            ++refusals;
        }
        const auto small = make(revision, wire::Trailer::OdaBitShort, 0u, 1u, 2u);
        for (auto trailer : {wire::Trailer::OdaBitShort, wire::Trailer::ProducerBitLong}) {
            const auto proof = wire::observe(small.dataView(), small.stringView(), revision, trailer);
            if (!require(proof.status == wire::Status::Observed && proof.dwgVersion == 1
                && proof.maintenance == 2 && proof.trailer == trailer,
                "small encodings must retain explicit profile ambiguity")) return false;
            ++positives;
        }
    }
    return true;
}
bool textCases() {
    struct Case { std::vector<std::uint16_t> units; const char* utf8; std::size_t size; unsigned issue; };
    const Case cases[] = {
        {{0x03A9u}, "\xCE\xA9", 2u, 0u},
        {{0xD83Du, 0xDE00u}, "\xF0\x9F\x98\x80", 4u, 0u},
        {{0xD800u}, "", 0u, wire::InvalidUnicode},
        {{0xDC00u}, "", 0u, wire::InvalidUnicode},
        {{0xD800u, 'A'}, "", 0u, wire::InvalidUnicode},
        {{0xD800u, 0xD800u, 0xDC00u}, "", 0u, wire::InvalidUnicode},
        {{0u}, "", 0u, wire::EmbeddedNull},
        {{}, "", 0u, 0u}
    };
    for (const auto& item : cases) {
        const auto record = make(2018u, wire::Trailer::OdaBitShort, 5u, 257u, 329u, item.units);
        const auto proof = observe(record);
        if (!require(proof.status == wire::Status::Observed && proof.text[0].issues == item.issue
            && wire::equalsUtf8(record.stringView(), proof.text[0], true, item.utf8, item.size) == (item.issue == 0u),
            "Unicode units/issues/canonical UTF-8 comparison failed")) return false;
        ++texts;
    }
    const auto legacy = make(2000u, wire::Trailer::OdaBitShort, 0u, 0u, 0u, {0xE9u});
    const auto proof = observe(legacy);
    if (!require(proof.status == wire::Status::Observed
        && proof.text[0].issues == wire::LegacyCodepageUnknown
        && !wire::equalsUtf8(legacy.dataView(), proof.text[0], false, "\xC3\xA9", 2u),
        "unknown legacy codepage acquired Unicode semantics")) return false;
    ++texts;
    return true;
}
bool invalidViews() {
    const auto record = make(2018u, wire::Trailer::OdaBitShort);
    auto data = record.dataView();
    if (!require(wire::observe(data, record.stringView(), 2099u).status == wire::Status::Unsupported
        && wire::observe(data, record.stringView(), 2018u, static_cast<wire::Trailer>(99)).status
            == wire::Status::Unsupported, "unknown revision/profile was accepted")) return false;
    refusals += 2u;
    auto null = data; null.bytes = nullptr;
    auto shortView = data; shortView.size = 0u;
    auto reversed = data; reversed.begin = data.end + 1u;
    for (auto bad : {null, shortView, reversed}) {
        if (!require(wire::observe(bad, record.stringView(), 2018u).status == wire::Status::Malformed,
                     "invalid input view was accepted")) return false;
        ++refusals;
    }
    if (!require(wire::observe(data, data, 2018u).status == wire::Status::Malformed,
                 "overlapping split streams were accepted")) return false;
    ++refusals;
    auto extra = record; extra.data.put(0u, 1u);
    if (!require(observe(extra).status == wire::Status::Malformed, "extra record data was ignored")) return false;
    ++refusals;
    return true;
}
bool primitiveAndFaultControls() {
    for (auto value : {0u, 1u, 255u, 256u, 257u, 32767u, 32768u, 65535u}) {
        Pack bits; bits.bs(value);
        wire::detail::Bits cursor({bits.bytes.data(), bits.bytes.size(), 0u, bits.bits});
        if (!require(cursor.bs() == value && cursor.good && cursor.pos == bits.bits,
                     "BS boundary value failed")) return false;
        ++positives;
    }
    for (auto value : {0u, 1u, 255u, 256u, 257u, 0x7FFFFFFFu, 0x80000000u, 0xFFFFFFFFu}) {
        Pack bits; bits.bl(value);
        wire::detail::Bits cursor({bits.bytes.data(), bits.bytes.size(), 0u, bits.bits});
        if (!require(cursor.bl() == value && cursor.good && cursor.pos == bits.bits,
                     "BL boundary value failed")) return false;
        ++positives;
    }
    Pack unicode;
    unicode.literal("01 00000001 10101001 00000011"); // BS1, UTF-16LE U+03A9
    const wire::View view{unicode.bytes.data(), unicode.bytes.size(), 0u, unicode.bits};
    wire::detail::Bits cursor(view);
    const auto text = wire::detail::text(cursor, true);
    if (!require(unicode.bits == 26u && cursor.good && text.units.begin == 10u
        && text.units.end == 26u && wire::equalsUtf8(view, text, true, "\xCE\xA9", 2u)
        && !wire::equalsUtf8(view, text, true, "\xCE\xA9", 1u)
        && !wire::equalsUtf8(view, text, true, nullptr, 1u), "literal Unicode/UTF-8 extent failed")) return false;
    ++texts;
    auto record = make(2018u, wire::Trailer::OdaBitShort);
    const auto good = observe(record);
    record.data.patch(2u, 0xF3u, 8u); // class499, not a legal custom-class ordinal
    if (!require(observe(record).status == wire::Status::Malformed, "class499 was accepted")) return false;
    ++refusals;
    record = make(2018u, wire::Trailer::OdaBitShort);
    const auto instance = good.fields[static_cast<std::size_t>(wire::Field::Instances)];
    record.data.patch(instance.begin, 3u, 2u); // BL tag11 is unused, not BS256
    if (!require(observe(record).status == wire::Status::Malformed, "invalid BL tag11 was accepted")) return false;
    ++refusals;
    const auto negative = make(2018u, wire::Trailer::OdaBitShort, 0u, 257u, 329u, {'A'}, 0xFFFFFFFFu);
    if (!require(observe(negative).status == wire::Status::Malformed, "negative instance count was accepted")) return false;
    ++refusals;
    auto huge = record.dataView(); huge.end = wire::detail::MaxRecordBits + 1u; huge.size = 0x40000000u;
    if (!require(wire::observe(huge, record.stringView(), 2018u).status == wire::Status::Malformed,
                 "oversized forged view was accepted")) return false;
    ++refusals;
    const auto invalid = make(2018u, wire::Trailer::OdaBitShort, 0u, 257u, 329u, {0xD800u});
    auto forged = observe(invalid).text[0]; forged.issues = 0u;
    if (!require(!wire::equalsUtf8(invalid.stringView(), forged, true, "A", 1u),
                 "cleared issue bits bypassed scalar validation")) return false;
    ++refusals;
    return true;
}
bool prefixStreams() {
    for (auto revision : {2000u, 2004u, 2007u, 2018u}) {
        for (auto trailer : {wire::Trailer::OdaBitShort, wire::Trailer::ProducerBitLong}) {
            const auto record = make(revision, trailer);
            auto joined = record;
            for (unsigned n = 0; n != record.data.bits; ++n)
                joined.data.put((record.data.bytes[n / 8u] >> (7u - n % 8u)) & 1u, 1u);
            if (revision >= 2007u) for (unsigned n = 0; n != record.strings.bits; ++n)
                joined.strings.put((record.strings.bytes[n / 8u] >> (7u - n % 8u)) & 1u, 1u);
            const auto first = wire::scan(joined.dataView(), joined.stringView(), revision, trailer);
            auto data = joined.dataView(), strings = joined.stringView();
            data.begin = first.dataEnd;
            if (revision >= 2007u) strings.begin = first.stringEnd;
            const auto second = wire::observe(data, strings, revision, trailer);
            if (!require(first.status == wire::Status::Observed && first.dataEnd == record.data.bits
                && first.stringEnd == (revision >= 2007u ? record.strings.bits : record.data.bits)
                && second.status == wire::Status::Observed && second.number == first.number
                && observe(joined).status == wire::Status::Malformed,
                "prefix iteration lost record cursors or claimed whole-section completion")) return false;
            ++positives;
            ++refusals;
        }
    }
    return true;
}
} // namespace
int main() {
    if (!literal() || !matrix() || !profiles() || !textCases() || !invalidViews()
        || !primitiveAndFaultControls() || !prefixStreams()) return 1;
    std::cout << "independent class records: " << positives << " positive, " << refusals
              << " refused/truncated, " << texts << " text controls\n";
}
