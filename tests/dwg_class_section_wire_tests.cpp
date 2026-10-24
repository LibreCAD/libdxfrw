// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_dwg_class_section_wire.h"
#include <algorithm>
#include <iostream>
#include <vector>

namespace wire = dx_class_wire;
namespace {
std::size_t positives = 0u, refusals = 0u;
bool expect(bool value, int line) {
    if (!value) std::cerr << "class section check failed at line " << line << '\n';
    return value;
}
#define CHECK(value) do { if (!expect((value), __LINE__)) return false; } while (false)
struct Pack {
    std::vector<std::uint8_t> bytes;
    std::uint32_t bits{0};
    void put(std::uint32_t value, unsigned width) {
        for (unsigned n = width; n; --n, ++bits) {
            if (bits / 8u == bytes.size()) bytes.push_back(0u);
            bytes[bits / 8u] |= static_cast<std::uint8_t>(((value >> (n - 1u)) & 1u) << (7u - bits % 8u));
        }
    }
    void little(std::uint32_t value, unsigned count) {
        for (unsigned n = 0; n < count; ++n) put((value >> (8u * n)) & 255u, 8u);
    }
    void bs(std::uint32_t value) {
        if (!value) put(2u, 2u);
        else if (value == 256u) put(3u, 2u);
        else if (value < 256u) { put(1u, 2u); put(value, 8u); }
        else { put(0u, 2u); little(value, 2u); }
    }
    void bl(std::uint32_t value) {
        if (!value) put(2u, 2u);
        else if (value < 256u) { put(1u, 2u); put(value, 8u); }
        else { put(0u, 2u); little(value, 4u); }
    }
    void append(const Pack& other) {
        for (std::uint32_t n = 0; n < other.bits; ++n)
            put((other.bytes[n / 8u] >> (7u - n % 8u)) & 1u, 1u);
    }
};
// Scratch author's nibble recurrence, distinct from observer's bitwise loop.
std::uint16_t authorCrc(const std::uint8_t* bytes, std::size_t size, std::uint16_t value = 0xC0C1u) {
    constexpr std::uint16_t table[] = {0x0000,0xCC01,0xD801,0x1400,0xF001,0x3C00,0x2800,0xE401,
                                      0xA001,0x6C00,0x7800,0xB401,0x5000,0x9C01,0x8801,0x4400};
    for (std::size_t n = 0; n < size; ++n) {
        value ^= bytes[n];
        for (unsigned k = 0; k < 2u; ++k) value = static_cast<std::uint16_t>((value >> 4u) ^ table[value & 15u]);
    }
    return value;
}
struct Built {
    std::vector<std::uint8_t> bytes;
    wire::SectionProfile profile;
    std::uint32_t crcByte{0}, bitSizeByte{0}, flag{0}, low{0}, high{0}, stringsBegin{0}, recordsBegin{0};
};
void patch(Built& value, std::uint32_t bit, std::uint32_t data, unsigned width) {
    for (unsigned n = width; n; --n, ++bit) {
        auto& byte = value.bytes[bit / 8u];
        const auto mask = static_cast<std::uint8_t>(1u << (7u - bit % 8u));
        byte = static_cast<std::uint8_t>((byte & ~mask) | (((data >> (n - 1u)) & 1u) ? mask : 0u));
    }
}
void fixCrc(Built& value) {
    const auto crc = authorCrc(value.bytes.data() + 16u, value.crcByte - 16u);
    value.bytes[value.crcByte] = static_cast<std::uint8_t>(crc);
    value.bytes[value.crcByte + 1u] = static_cast<std::uint8_t>(crc >> 8u);
}
Built make(wire::SectionProfile profile, std::vector<unsigned> numbers = {500u},
           unsigned textUnits = 1u, unsigned version = 257u, unsigned declaredMax = 0u,
           bool extraStringBit = false, unsigned itemClass = 0x1F3u, unsigned classMaintenance = 329u) {
    Built out;
    out.profile = profile;
    const bool modern = profile.revision >= 2004u, split = profile.revision >= 2007u;
    const bool high = (profile.revision >= 2018u && profile.highSizeRule == wire::ClassHighSizeRule::R2018Always)
        || (profile.revision >= 2010u && profile.maintenance > 3u);
    Pack body, strings;
    if (split) body.little(0u, 4u);
    unsigned maximum = 499u;
    for (auto number : numbers) maximum = (std::max)(maximum, number);
    if (declaredMax) maximum = declaredMax;
    if (modern) {
        if (split) body.bl(maximum);
        else { body.bs(maximum); body.put(0u, 16u); }
        body.put(profile.requireTrueHeaderFlag, 1u);
    }
    out.recordsBegin = body.bits;
    for (auto number : numbers) {
        body.bs(number); body.bs(0u);
        auto& names = split ? strings : body;
        for (auto name : {'A', 'C', 'R'}) {
            names.bs(textUnits);
            for (unsigned n = 0; n < textUnits; ++n) names.little(name, split ? 2u : 1u);
        }
        body.put(0u, 1u); body.bs(itemClass);
        if (modern) {
            body.bl(1u);
            if (profile.trailer == wire::Trailer::OdaBitShort) { body.bs(version); body.bs(classMaintenance); }
            else { body.bl(version); body.bl(classMaintenance); }
            body.bl(0u); body.bl(0xFFFFFFFFu);
        }
    }
    if (split) {
        if (profile.scalarByteAlignment)
            while (body.bits % 8u) body.put(body.bits % 3u == 0u, 1u);
        out.stringsBegin = body.bits;
        body.append(strings);
        if (extraStringBit) { body.put(1u, 1u); ++strings.bits; }
        if (strings.bits != 0u) {
            if (strings.bits >= 0x8000u) { out.high = body.bits; body.little(strings.bits >> 15u, 2u); }
            out.low = body.bits;
            body.little((strings.bits & 0x7FFFu) | (strings.bits >= 0x8000u ? 0x8000u : 0u), 2u);
        }
        out.flag = body.bits;
        body.put(strings.bits != 0u, 1u);
        const auto bitSize = body.bits;
        for (unsigned n = 0; n < 4u; ++n) body.bytes[n] = static_cast<std::uint8_t>(bitSize >> (8u * n));
    }
    // Arbitrary unused bits are legal byte alignment, not zero-only fields.
    while (body.bits % 8u) body.put(body.bits % 3u == 0u, 1u);
    Pack section;
    for (auto byte : {0x8D,0xA1,0xC4,0xB8,0xC4,0xA9,0xF8,0xC5,0xC0,0xDC,0xF4,0x5F,0xE7,0xCF,0xB6,0x8A})
        section.put(byte, 8u);
    const auto includedHigh = high && profile.sizeLayout == wire::ClassSizeLayout::IncludesHighWord ? 4u : 0u;
    section.little(static_cast<std::uint32_t>(body.bytes.size()) + includedHigh, 4u);
    if (high) section.little(0u, 4u);
    out.bitSizeByte = section.bits / 8u;
    out.flag += section.bits; out.low += section.bits; out.high += section.bits;
    out.stringsBegin += section.bits; out.recordsBegin += section.bits;
    section.append(body);
    out.crcByte = section.bits / 8u;
    section.little(0u, 2u);
    const unsigned before = profile.tail == wire::ClassTail::BeforeOne ? 1u : profile.tail == wire::ClassTail::BeforeTwo ? 2u
        : profile.tail == wire::ClassTail::BeforeEight ? 8u : 0u;
    for (unsigned n = 0; n < before; ++n) section.put(0x55u + n, 8u);
    for (auto byte : {0x72,0x5E,0x3B,0x47,0x3B,0x56,0x07,0x3A,0x3F,0x23,0x0B,0xA0,0x18,0x30,0x49,0x75})
        section.put(byte, 8u);
    if (profile.tail == wire::ClassTail::AfterEight)
        for (unsigned n = 0; n < 8u; ++n) section.put(0xABu + n, 8u);
    if (profile.zeroEnvelopePadding) for (unsigned n = 0; n < 11u; ++n) section.put(0u, 8u);
    out.bytes = std::move(section.bytes);
    fixCrc(out);
    return out;
}
wire::SectionProof observe(const Built& built) {
    return wire::observeSection(built.bytes.data(), built.bytes.size(), built.profile);
}
bool anchors() {
    const std::uint8_t text[] = {'1','2','3','4','5','6','7','8','9'};
    const std::uint8_t zero[] = {0,0,0,0};
    CHECK(wire::section_detail::crc(text, 9u, 0u) == 0xBB3Du && authorCrc(text, 9u, 0u) == 0xBB3Du);
    CHECK(wire::section_detail::crc(zero, 4u) == 0xC03Du && authorCrc(zero, 4u) == 0xC03Du);
    const std::vector<std::uint8_t> literal = {
        0x8D,0xA1,0xC4,0xB8,0xC4,0xA9,0xF8,0xC5,0xC0,0xDC,0xF4,0x5F,0xE7,0xCF,0xB6,0x8A,
        0,0,0,0,0x3D,0xC0,
        0x72,0x5E,0x3B,0x47,0x3B,0x56,0x07,0x3A,0x3F,0x23,0x0B,0xA0,0x18,0x30,0x49,0x75};
    wire::SectionProfile profile; profile.revision = 2000u;
    CHECK(make(profile, {}).bytes == literal);
    const auto proof = wire::observeSection(literal.data(), literal.size(), profile);
    CHECK(proof.status == wire::Status::Observed && proof.records == 0u && proof.crcMatches
        && proof.denseNumbering && proof.sectionEndByte == 38u);
    ++positives;
    return true;
}
bool matrix() {
    unsigned alignments = 0u;
    for (auto revision : {13u,14u,2000u,2004u,2007u,2010u,2013u,2018u})
    for (auto trailer : {wire::Trailer::OdaBitShort, wire::Trailer::ProducerBitLong})
    for (auto maintenance : {0u,3u,4u})
    for (auto tail : {wire::ClassTail::SentinelOnly, wire::ClassTail::BeforeOne, wire::ClassTail::BeforeTwo,
                     wire::ClassTail::BeforeEight, wire::ClassTail::AfterEight}) {
        if (revision < 2004u && tail != wire::ClassTail::SentinelOnly) continue;
        wire::SectionProfile profile; profile.revision = revision;
        profile.trailer = trailer; profile.maintenance = static_cast<std::uint8_t>(maintenance); profile.tail = tail;
        profile.highSizeRule = wire::ClassHighSizeRule::R2018Always;
        const bool high = revision >= 2018u || (revision >= 2010u && maintenance > 3u);
        for (unsigned layout = 0; layout < (high ? 2u : 1u); ++layout) {
            profile.sizeLayout = layout ? wire::ClassSizeLayout::ExcludesHighWord : wire::ClassSizeLayout::IncludesHighWord;
            for (unsigned count = 0; count < 8u; ++count) {
                std::vector<unsigned> numbers;
                for (unsigned n = 0; n < count; ++n) numbers.push_back(500u + n);
                const auto built = make(profile, numbers);
                const auto proof = observe(built);
                CHECK(proof.status == wire::Status::Observed && proof.records == count && proof.crcMatches
                    && proof.denseNumbering && proof.highSizePresent == high
                    && proof.sectionEndByte == built.bytes.size() && proof.crcByte == built.crcByte
                    && proof.data.begin == built.recordsBegin && proof.textIssues == 0u);
                if (revision >= 2007u) alignments |= 1u << (built.flag % 8u);
                ++positives;
            }
        }
    }
    CHECK(alignments == 255u);
    return true;
}
bool failures() {
    for (auto revision : {13u,14u,2000u,2004u,2007u,2010u,2013u,2018u}) {
        wire::SectionProfile profile; profile.revision = revision; profile.maintenance = 4u;
        const auto built = make(profile, {500u,501u});
        for (std::size_t size = 0; size < built.bytes.size(); ++size) {
            CHECK(wire::observeSection(built.bytes.data(), size, profile).status != wire::Status::Observed);
            ++refusals;
        }
        for (std::size_t byte = 0; byte < built.bytes.size(); ++byte) for (unsigned bit = 0; bit < 8u; ++bit) {
            auto changed = built;
            changed.bytes[byte] ^= static_cast<std::uint8_t>(1u << bit);
            CHECK(observe(changed).status != wire::Status::Observed);
            ++refusals;
        }
        const auto duplicate = observe(make(profile, {500u,500u}));
        CHECK(duplicate.status == wire::Status::Malformed && duplicate.reason == wire::SectionReason::Duplicate);
        ++refusals;
        const auto sparse = observe(make(profile, {502u,500u}));
        CHECK(sparse.status == wire::Status::Observed && sparse.records == 2u
            && sparse.maximumClass == 502u && !sparse.denseNumbering);
        ++positives;
        if (revision >= 2004u) {
            CHECK(observe(make(profile, {}, 1u, 257u, 500u)).reason == wire::SectionReason::ClassNumber);
            CHECK(observe(make(profile, {500u}, 1u, 257u, 499u)).reason == wire::SectionReason::ClassNumber);
            refusals += 2u;
        }
        if (revision >= 2007u) {
            auto badFooter = built;
            patch(badFooter, built.low, 0xFF7Fu, 16u); fixCrc(badFooter);
            CHECK(observe(badFooter).reason == wire::SectionReason::Footer);
            auto noFlag = built;
            patch(noFlag, built.flag, 0u, 1u); fixCrc(noFlag);
            CHECK(observe(noFlag).status != wire::Status::Observed);
            CHECK(observe(make(profile, {500u}, 1u, 257u, 0u, true)).reason == wire::SectionReason::Consumption);
            refusals += 3u;
        }
        auto zero = built;
        zero.bytes[zero.crcByte] = zero.bytes[zero.crcByte + 1u] = 0u;
        CHECK(observe(zero).reason == wire::SectionReason::Crc && !observe(zero).crcMatches);
        ++refusals;
    }
    return true;
}
bool boundaries() {
    wire::SectionProfile profile; profile.revision = 2018u;
    profile.highSizeRule = wire::ClassHighSizeRule::R2018Always;
    auto padded = make(profile);
    padded.bytes.insert(padded.bytes.end(), 20u, 0u);
    CHECK(observe(padded).reason == wire::SectionReason::Envelope); ++refusals;
    padded.profile.zeroEnvelopePadding = true;
    CHECK(observe(padded).status == wire::Status::Observed); ++positives;
    padded.bytes.back() = 1u;
    CHECK(observe(padded).reason == wire::SectionReason::Envelope); ++refusals;
    const auto extended = make(profile, {500u}, 700u);
    CHECK(observe(extended).status == wire::Status::Observed && observe(extended).extendedFooter); ++positives;
    for (std::size_t size = 0; size < extended.bytes.size(); ++size) {
        CHECK(wire::observeSection(extended.bytes.data(), size, profile).status != wire::Status::Observed);
        ++refusals;
    }
    for (auto units : {0u,1u,681u,682u,65535u}) {
        const auto boundary = observe(make(profile, {500u}, units));
        CHECK(boundary.status == wire::Status::Observed && boundary.extendedFooter == (units >= 682u));
        ++positives;
    }
    auto footer = extended;
    patch(footer, footer.high, 0xFFFFu, 16u); fixCrc(footer);
    CHECK(observe(footer).reason == wire::SectionReason::Footer); ++refusals;
    auto high = extended;
    high.bytes[20] = 1u;
    CHECK(observe(high).reason == wire::SectionReason::HighSize
        && observe(high).status == wire::Status::Unsupported); ++refusals;
    auto noHighProfile = profile;
    noHighProfile.highSizeRule = wire::ClassHighSizeRule::MaintenanceGate;
    const auto noHigh = make(noHighProfile);
    CHECK(observe(noHigh).status == wire::Status::Observed && !observe(noHigh).highSizePresent); ++positives;
    auto wrongHighRule = noHigh;
    wrongHighRule.profile.highSizeRule = wire::ClassHighSizeRule::R2018Always;
    CHECK(observe(wrongHighRule).status != wire::Status::Observed); ++refusals;
    auto falseHeaderProfile = profile;
    falseHeaderProfile.requireTrueHeaderFlag = false;
    const auto falseHeader = make(falseHeaderProfile);
    CHECK(observe(falseHeader).status == wire::Status::Observed && !observe(falseHeader).headerFlag); ++positives;
    auto requiredTrue = falseHeader;
    requiredTrue.profile.requireTrueHeaderFlag = true;
    CHECK(observe(requiredTrue).reason == wire::SectionReason::Header); ++refusals;
    auto alignedProfile = profile;
    alignedProfile.scalarByteAlignment = true;
    const auto aligned = make(alignedProfile);
    CHECK(observe(aligned).status == wire::Status::Observed
        && observe(aligned).scalarPadding.end - observe(aligned).scalarPadding.begin <= 7u); ++positives;
    auto requiredTight = aligned;
    requiredTight.profile.scalarByteAlignment = false;
    CHECK(observe(requiredTight).status != wire::Status::Observed); ++refusals;
    auto shortProfile = make(profile);
    shortProfile.profile.trailer = wire::Trailer::ProducerBitLong;
    CHECK(observe(shortProfile).status != wire::Status::Observed); ++refusals;
    // The same small version/maintenance encodings cannot select a schema.
    auto ambiguous = make(profile, {500u}, 1u, 1u, 0u, false, 0x1F3u, 2u);
    CHECK(observe(ambiguous).status == wire::Status::Observed);
    ambiguous.profile.trailer = wire::Trailer::ProducerBitLong;
    CHECK(observe(ambiguous).status == wire::Status::Observed); positives += 2u;
    const auto unknown = observe(make(profile, {500u}, 1u, 257u, 0u, false, 0x123u));
    CHECK(unknown.status == wire::Status::Observed && unknown.unknownItemClass); ++positives;
    const auto maximum = observe(make(profile, {500u,65535u}));
    CHECK(maximum.status == wire::Status::Observed && maximum.records == 2u
        && maximum.maximumClass == 65535u && !maximum.denseNumbering); ++positives;
    CHECK(observe(make(profile, {500u}, 1u, 257u, 65536u)).reason == wire::SectionReason::Header); ++refusals;
    CHECK(wire::observeSection(nullptr, 100u, profile).status == wire::Status::Malformed); ++refusals;
    CHECK(wire::observeSection(extended.bytes.data(), wire::section_detail::MaxBytes + 1u, profile).status
        == wire::Status::Unsupported); ++refusals;
    profile.revision = 999u;
    CHECK(wire::observeSection(extended.bytes.data(), extended.bytes.size(), profile).reason
        == wire::SectionReason::Profile); ++refusals;
    return true;
}
}
int main() {
    if (!anchors() || !matrix() || !failures() || !boundaries()) return 1;
    std::cout << "independent class sections: " << positives << " positive, " << refusals
              << " refused/truncated; no support qualification\n";
}
