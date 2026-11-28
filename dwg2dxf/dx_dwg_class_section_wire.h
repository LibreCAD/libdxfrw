// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_DWG_CLASS_SECTION_WIRE_H
#define DX_DWG_CLASS_SECTION_WIRE_H

// Independent, allocation-free observer of a decompressed/physical CLASSES
// envelope. No production parser, buffer, codec, writer or class-row boundary.
// A successful explicit profile is byte framing, not profile selection, outer
// DWG/container integrity, text semantics or support admission.
#include "dx_dwg_class_wire.h"

namespace dx_class_wire {
enum class ClassSizeLayout { IncludesHighWord, ExcludesHighWord };
// ODA10.2's maintenance gate and the R2018-always producer convention remain
// distinct candidates, especially for low-maintenance R2018 envelopes.
enum class ClassHighSizeRule { MaintenanceGate, R2018Always };
enum class ClassTail { SentinelOnly, BeforeOne, BeforeTwo, BeforeEight, AfterEight };
enum class SectionReason {
    NoFailure, Input, Profile, BeginSentinel, Length, HighSize, Header,
    Footer, Record, Duplicate, ClassNumber, Consumption, Crc, EndSentinel, Envelope
};
struct SectionProfile {
    unsigned revision{0};
    std::uint8_t maintenance{0};
    Trailer trailer{Trailer::OdaBitShort};
    ClassSizeLayout sizeLayout{ClassSizeLayout::IncludesHighWord};
    ClassHighSizeRule highSizeRule{ClassHighSizeRule::MaintenanceGate};
    ClassTail tail{ClassTail::SentinelOnly};
    bool zeroEnvelopePadding{false};
    // ODA10.2 says true; ODA5.8 only says bool. Raw false-bit producer
    // captures can be framed explicitly without certifying their semantics.
    bool requireTrueHeaderFlag{true};
    bool scalarByteAlignment{false};
};
struct SectionProof {
    Status status{Status::Malformed};
    SectionReason reason{SectionReason::Input};
    SectionProfile profile;
    View data, strings;
    Span scalarPadding, bytePadding, stringFooter, extension, envelopePadding;
    std::uint32_t sizeField{0}, bitSize{0}, records{0}, maximumClass{499u};
    std::uint32_t highestClass{499u}, crcByte{0}, sentinelByte{0}, sectionEndByte{0};
    std::uint32_t dataCursor{0}, stringCursor{0};
    std::uint16_t storedCrc{0}, calculatedCrc{0};
    std::uint8_t headerByte0{0}, headerByte1{0};
    unsigned textIssues{0};
    bool split{false}, highSizePresent{false}, extendedFooter{false}, headerFlag{false};
    bool crcChecked{false}, crcMatches{false}, denseNumbering{false}, unknownItemClass{false};
};
namespace section_detail {
constexpr std::size_t MaxBytes = 64u * 1024u * 1024u;
constexpr std::array<std::uint8_t, 16> Begin{{
    0x8D,0xA1,0xC4,0xB8,0xC4,0xA9,0xF8,0xC5,0xC0,0xDC,0xF4,0x5F,0xE7,0xCF,0xB6,0x8A}};
constexpr std::array<std::uint8_t, 16> End{{
    0x72,0x5E,0x3B,0x47,0x3B,0x56,0x07,0x3A,0x3F,0x23,0x0B,0xA0,0x18,0x30,0x49,0x75}};
inline bool sentinel(const std::uint8_t* bytes, std::size_t size, std::size_t at,
                     const std::array<std::uint8_t, 16>& expected) noexcept {
    if (at > size || size - at < expected.size()) return false;
    for (std::size_t n = 0; n < expected.size(); ++n) if (bytes[at + n] != expected[n]) return false;
    return true;
}
// Bitwise reflected CRC-16 (0xA001). No production/peer lookup table. The
// section covers the RL length and data, starting immediately after sentinel.
inline std::uint16_t crc(const std::uint8_t* bytes, std::size_t size,
                         std::uint16_t seed = 0xC0C1u) noexcept {
    auto value = static_cast<std::uint32_t>(seed);
    for (std::size_t n = 0; n < size; ++n) {
        value ^= bytes[n];
        for (unsigned b = 0; b != 8u; ++b) value = (value >> 1u) ^ ((value & 1u) ? 0xA001u : 0u);
    }
    return static_cast<std::uint16_t>(value);
}
inline SectionProof fail(SectionProof proof, SectionReason reason, bool unsupported = false) noexcept {
    proof.reason = reason;
    proof.status = unsupported ? Status::Unsupported : Status::Malformed;
    return proof;
}
} // namespace section_detail

namespace section_detail {
// Internal record/CRC prerequisite. Only finishEnvelope may turn this into a
// complete envelope proof; this intermediate result is not a public observer.
inline SectionProof recordCore(const std::uint8_t* bytes, std::size_t size,
                               SectionProfile profile) noexcept {
    using namespace section_detail;
    SectionProof proof;
    proof.profile = profile;
    const auto revision = profile.revision;
    if ((revision != 13u && revision != 14u && revision != 2000u && revision != 2004u
         && revision != 2007u && revision != 2010u && revision != 2013u && revision != 2018u)
        || (profile.trailer != Trailer::OdaBitShort && profile.trailer != Trailer::ProducerBitLong)
        || (profile.sizeLayout != ClassSizeLayout::IncludesHighWord
            && profile.sizeLayout != ClassSizeLayout::ExcludesHighWord)
        || (profile.highSizeRule != ClassHighSizeRule::MaintenanceGate
            && profile.highSizeRule != ClassHighSizeRule::R2018Always)
        || static_cast<unsigned>(profile.tail) > static_cast<unsigned>(ClassTail::AfterEight))
        return fail(proof, SectionReason::Profile, true);
    if (size > MaxBytes) return fail(proof, SectionReason::Length, true);
    if (!bytes || size < 38u) return proof;
    proof.split = revision >= 2007u;
    proof.highSizePresent = (revision >= 2018u && profile.highSizeRule == ClassHighSizeRule::R2018Always)
        || (revision >= 2010u && profile.maintenance > 3u);
    if ((!proof.highSizePresent && profile.sizeLayout == ClassSizeLayout::ExcludesHighWord)
        || (revision < 2004u && profile.tail != ClassTail::SentinelOnly))
        return fail(proof, SectionReason::Profile, true);
    if (!sentinel(bytes, size, 0u, Begin)) return fail(proof, SectionReason::BeginSentinel);
    const auto bits = static_cast<std::uint32_t>(size * 8u);
    // Header reads need only a small fixed window, not a record-sized view of
    // the entire section. All later arithmetic is bounded by the 64 MiB cap.
    detail::Bits header({bytes, size, 128u, bits < 512u ? bits : 512u});
    proof.sizeField = header.little(4u);
    if (proof.highSizePresent && header.little(4u) != 0u)
        return fail(proof, SectionReason::HighSize, true);
    const auto sizeBase = header.pos;
    if (proof.split) proof.bitSize = header.little(4u);
    if (revision >= 2004u) {
        if (proof.split) proof.maximumClass = header.bl(); // ODA5.8; BS+RC0+RC0 encodes identically in range.
        else {
            proof.maximumClass = header.bs();
            proof.headerByte0 = static_cast<std::uint8_t>(header.take(8u));
            proof.headerByte1 = static_cast<std::uint8_t>(header.take(8u));
            if (proof.headerByte0 != 0u || proof.headerByte1 != 0u) return fail(proof, SectionReason::Header);
        }
        proof.headerFlag = header.take(1u) != 0u;
        if ((profile.requireTrueHeaderFlag && !proof.headerFlag) || !header.good || proof.maximumClass < 499u
            || proof.maximumClass > 65535u) return fail(proof, SectionReason::Header);
    }
    if (!header.good) return fail(proof, SectionReason::Header);
    const auto extraSize = proof.highSizePresent
        && profile.sizeLayout == ClassSizeLayout::ExcludesHighWord ? 4u : 0u;
    if (proof.sizeField > size - 38u || extraSize > size - 38u - proof.sizeField)
        return fail(proof, SectionReason::Length);
    proof.crcByte = 20u + extraSize + proof.sizeField;
    const auto crcBit = proof.crcByte * 8u;
    if (header.pos > crcBit) return fail(proof, SectionReason::Length);
    proof.data = {bytes, size, header.pos, crcBit};
    proof.strings = {bytes, size, crcBit, crcBit};
    if (proof.split) {
        if (proof.bitSize > crcBit - sizeBase || proof.bitSize == 0u)
            return fail(proof, SectionReason::Footer);
        const auto flagEnd = sizeBase + proof.bitSize;
        if (flagEnd <= header.pos || crcBit - flagEnd > 7u)
            return fail(proof, SectionReason::Footer);
        const auto flag = flagEnd - 1u;
        detail::Bits endFlag({bytes, size, flag, flagEnd});
        const bool hasStrings = endFlag.take(1u) != 0u;
        auto footerStart = flag;
        std::uint32_t stringBits = 0u;
        if (hasStrings) {
            if (footerStart < header.pos + 16u) return fail(proof, SectionReason::Footer);
            footerStart -= 16u;
            detail::Bits low({bytes, size, footerStart, flag});
            stringBits = low.little(2u);
            if ((stringBits & 0x8000u) != 0u) {
                proof.extendedFooter = true;
                if (footerStart < header.pos + 16u) return fail(proof, SectionReason::Footer);
                footerStart -= 16u;
                detail::Bits high({bytes, size, footerStart, footerStart + 16u});
                stringBits = (stringBits & 0x7FFFu) | (high.little(2u) << 15u);
            }
            if (stringBits > footerStart - header.pos) return fail(proof, SectionReason::Footer);
        }
        proof.stringFooter = {footerStart, flagEnd};
        proof.bytePadding = {flagEnd, crcBit}; // Unused alignment bits, not required to be zero (ODA2.14).
        proof.data.end = footerStart - stringBits;
        proof.strings = {bytes, size, proof.data.end, footerStart};
    }

    auto data = proof.data, strings = proof.strings;
    proof.dataCursor = data.begin; proof.stringCursor = strings.begin;
    std::array<std::uint8_t, 8192> seen{};
    bool alignedScalarTail = false;
    while (data.begin < data.end) {
        if (!proof.split && data.end - data.begin <= 7u) break;
        if (proof.split && profile.scalarByteAlignment && data.end - data.begin <= 7u
            && data.end % 8u == 0u && strings.begin == strings.end) {
            alignedScalarTail = true; break;
        }
        const auto record = scan(data, strings, revision, profile.trailer);
        if (record.status != Status::Observed || record.dataEnd <= data.begin)
            return fail(proof, SectionReason::Record);
        if ((seen[record.number / 8u] & (1u << (record.number % 8u))) != 0u)
            return fail(proof, SectionReason::Duplicate);
        seen[record.number / 8u] |= static_cast<std::uint8_t>(1u << (record.number % 8u));
        if (revision >= 2004u && record.number > proof.maximumClass)
            return fail(proof, SectionReason::ClassNumber);
        if (record.number > proof.highestClass) proof.highestClass = record.number;
        for (const auto& text : record.text) proof.textIssues |= text.issues;
        proof.unknownItemClass = proof.unknownItemClass || record.unknownItemClass;
        ++proof.records;
        data.begin = record.dataEnd;
        if (proof.split) strings.begin = record.stringEnd;
        proof.dataCursor = data.begin; proof.stringCursor = strings.begin;
    }
    if (proof.split && ((!alignedScalarTail && data.begin != data.end) || strings.begin != strings.end))
        return fail(proof, SectionReason::Consumption);
    proof.scalarPadding = {data.begin, data.end};
    proof.data.end = data.begin;
    if (revision < 2004u) proof.maximumClass = proof.highestClass;
    if (proof.maximumClass != proof.highestClass) return fail(proof, SectionReason::ClassNumber);
    proof.denseNumbering = proof.records == proof.maximumClass - 499u;

    proof.storedCrc = static_cast<std::uint16_t>(bytes[proof.crcByte]
        | (static_cast<unsigned>(bytes[proof.crcByte + 1u]) << 8u));
    proof.calculatedCrc = crc(bytes + 16u, proof.crcByte - 16u);
    proof.crcChecked = true;
    proof.crcMatches = proof.storedCrc == proof.calculatedCrc; // Zero is not an integrity waiver.
    if (!proof.crcMatches) return fail(proof, SectionReason::Crc);
    proof.status = Status::Observed;
    proof.reason = SectionReason::NoFailure;
    return proof;
}
inline SectionProof finishEnvelope(SectionProof proof, const std::uint8_t* bytes,
                                   std::size_t size, ClassTail tail) noexcept {
    proof.profile.tail = tail;
    if (proof.status != Status::Observed) return proof;
    if ((proof.profile.revision < 2004u && tail != ClassTail::SentinelOnly)
        || static_cast<unsigned>(tail) > static_cast<unsigned>(ClassTail::AfterEight))
        return fail(proof, SectionReason::Profile, true);
    const auto profile = proof.profile;
    const auto before = profile.tail == ClassTail::BeforeOne ? 1u : profile.tail == ClassTail::BeforeTwo ? 2u
        : profile.tail == ClassTail::BeforeEight ? 8u : 0u;
    const auto after = profile.tail == ClassTail::AfterEight ? 8u : 0u;
    proof.sentinelByte = proof.crcByte + 2u + before;
    if (!sentinel(bytes, size, proof.sentinelByte, End)) return fail(proof, SectionReason::EndSentinel);
    proof.sectionEndByte = proof.sentinelByte + 16u + after;
    if (proof.sectionEndByte > size) return fail(proof, SectionReason::Length);
    proof.extension = before ? Span{(proof.crcByte + 2u) * 8u, proof.sentinelByte * 8u}
        : Span{(proof.sentinelByte + 16u) * 8u, proof.sectionEndByte * 8u};
    proof.envelopePadding = {proof.sectionEndByte * 8u, static_cast<std::uint32_t>(size * 8u)};
    if (size != proof.sectionEndByte) {
        if (!profile.zeroEnvelopePadding) return fail(proof, SectionReason::Envelope);
        for (std::size_t n = proof.sectionEndByte; n < size; ++n)
            if (bytes[n] != 0u) return fail(proof, SectionReason::Envelope);
    }
    proof.status = Status::Observed;
    proof.reason = SectionReason::NoFailure;
    return proof;
}
} // namespace section_detail

// Explicit candidates only. Do not select by comparison with expected output.
inline SectionProof observeSection(const std::uint8_t* bytes, std::size_t size,
                                    SectionProfile profile) noexcept {
    return section_detail::finishEnvelope(section_detail::recordCore(bytes, size, profile), bytes, size, profile.tail);
}

// All five explicit tail candidates, with one record/CRC traversal per common
// profile. Every returned candidate still verifies its own END/extension/full
// envelope consumption. The input tail member is replaced by these candidates.
inline std::array<SectionProof, 5> observeSectionTails(const std::uint8_t* bytes, std::size_t size,
                                                     SectionProfile profile) noexcept {
    profile.tail = ClassTail::SentinelOnly;
    const auto core = section_detail::recordCore(bytes, size, profile);
    std::array<SectionProof, 5> proofs{};
    for (unsigned n = 0; n < proofs.size(); ++n) {
        const auto tail = static_cast<ClassTail>(n);
        if (profile.revision < 2004u && n != 0u) {
            profile.tail = tail;
            proofs[n] = observeSection(bytes, size, profile); // Unsupported before any record/CRC traversal.
        } else proofs[n] = section_detail::finishEnvelope(core, bytes, size, tail);
    }
    return proofs;
}
} // namespace dx_class_wire
#endif
