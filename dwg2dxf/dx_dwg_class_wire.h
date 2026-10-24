// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_DWG_CLASS_WIRE_H
#define DX_DWG_CLASS_WIRE_H

// Allocation-free observer for one bounded CLASSES record (ODA5.4.1, 2/10).
// No libdxfrw parser/buffer/codec dependency. Source section extraction, record
// boundaries, CRC/count coverage and producer codepages are separate evidence.
#include <array>
#include <cstddef>
#include <cstdint>

namespace dx_class_wire {
enum class Status { Malformed, Unsupported, Observed };
// Do not infer the profile from successful parsing: common compact values
// have identical representations. The long variant is an observed producer
// compatibility layout for ODA10.2's BS fields. ODA5.8 separately documents
// BL version fields for R2007; do not flatten that distinction into one schema.
enum class Trailer { OdaBitShort, ProducerBitLong };
enum class Field : std::size_t {
    Number, Proxy, App, Cpp, Record, Zombie, ItemClass, Instances,
    DwgVersion, Maintenance, Unknown1, Unknown2, Count
};
struct Span { std::uint32_t begin{0}, end{0}; };
struct View {
    const std::uint8_t* bytes{nullptr};
    std::size_t size{0};
    std::uint32_t begin{0}, end{0};
};
enum TextIssue : unsigned { LegacyCodepageUnknown = 1u, InvalidUnicode = 2u, EmbeddedNull = 4u };
struct Text {
    Span length, units;
    std::uint16_t count{0}; // encoded units, not Unicode scalar count
    unsigned issues{0};
};
struct Proof {
    Status status{Status::Malformed};
    Trailer trailer{Trailer::OdaBitShort};
    std::array<Span, static_cast<std::size_t>(Field::Count)> fields{};
    std::array<Text, 3> text{}; // app, C++ class, DXF record
    std::uint16_t number{0}, proxy{0}, itemClass{0};
    std::int32_t instances{0}, dwgVersion{0}, maintenance{0}, unknown1{0}, unknown2{0};
    std::uint32_t dataEnd{0}, stringEnd{0};
    bool zombie{false}, split{false}, unknownItemClass{false};
};
namespace detail {
constexpr std::uint32_t MaxRecordBits = 3u * 65535u * 16u + 512u;
inline bool extentValid(View v) noexcept {
    return v.begin <= v.end
        && (static_cast<std::uint64_t>(v.end) + 7u) / 8u <= v.size
        && (v.bytes != nullptr || (v.size == 0u && v.end == 0u));
}
inline bool valid(View v) noexcept {
    return extentValid(v) && v.end - v.begin <= MaxRecordBits;
}
inline View recordWindow(View v) noexcept {
    if (v.end - v.begin > MaxRecordBits) v.end = v.begin + MaxRecordBits;
    return v;
}
struct Bits {
    View view;
    std::uint32_t pos;
    bool good;
    explicit Bits(View v) noexcept : view(v), pos(v.begin), good(valid(v)) {}
    std::uint32_t take(unsigned width) noexcept {
        if (!good || width > 32u || pos > view.end || width > view.end - pos) {
            good = false; return 0u;
        }
        std::uint32_t value = 0u;
        for (unsigned n = 0; n < width; ++n, ++pos)
            value = (value << 1u) | ((view.bytes[pos / 8u] >> (7u - pos % 8u)) & 1u);
        return value;
    }
    std::uint32_t little(unsigned bytes) noexcept {
        if (bytes > 4u) { good = false; return 0u; }
        std::uint32_t value = 0u;
        for (unsigned n = 0; n < bytes; ++n) value |= take(8u) << (8u * n);
        return value;
    }
    std::uint32_t bs() noexcept {
        const auto tag = take(2u);
        return tag == 0u ? little(2u) : tag == 1u ? take(8u) : tag == 2u ? 0u : 256u;
    }
    std::uint32_t bl() noexcept {
        const auto tag = take(2u);
        if (tag == 3u) { good = false; return 0u; }
        return tag == 0u ? little(4u) : tag == 1u ? take(8u) : 0u;
    }
};
inline std::int32_t signed32(std::uint32_t value) noexcept {
    return static_cast<std::int32_t>(value < 0x80000000u ? static_cast<std::int64_t>(value)
        : static_cast<std::int64_t>(value) - INT64_C(0x100000000));
}
inline std::int32_t versionValue(Bits& bits, Trailer trailer) noexcept {
    if (trailer == Trailer::ProducerBitLong) return signed32(bits.bl());
    const auto value = bits.bs();
    return value < 0x8000u ? static_cast<std::int32_t>(value)
        : static_cast<std::int32_t>(value) - 65536;
}
inline Text text(Bits& bits, bool unicode) noexcept {
    Text value;
    value.length.begin = bits.pos;
    value.count = static_cast<std::uint16_t>(bits.bs());
    value.length.end = value.units.begin = bits.pos;
    const auto width = unicode ? 16u : 8u;
    if (!bits.good || bits.pos > bits.view.end
        || value.count > (bits.view.end - bits.pos) / width) {
        bits.good = false; return value;
    }
    bool high = false;
    for (unsigned n = 0; n < value.count; ++n) {
        const auto unit = bits.little(unicode ? 2u : 1u);
        if (unit == 0u) value.issues |= EmbeddedNull;
        if (!unicode) {
            if (unit > 127u) value.issues |= LegacyCodepageUnknown;
            continue;
        }
        if (high) {
            high = false;
            if (unit >= 0xDC00u && unit <= 0xDFFFu) continue;
            value.issues |= InvalidUnicode;
        }
        if (unit >= 0xD800u && unit <= 0xDBFFu) high = true;
        else if (unit >= 0xDC00u && unit <= 0xDFFFu) value.issues |= InvalidUnicode;
    }
    if (high) value.issues |= InvalidUnicode;
    value.units.end = bits.pos;
    return value;
}
} // namespace detail

// Consume exactly one record prefix from already separated section streams.
// Returned end cursors allow an independent section iterator; remaining data
// is NOT implicitly padding or a complete/valid CLASSES section.
inline Proof scan(View data, View strings, unsigned revision,
                  Trailer trailer = Trailer::OdaBitShort) noexcept {
    Proof proof;
    proof.trailer = trailer;
    if ((revision != 13u && revision != 14u && revision != 2000u && revision != 2004u
         && revision != 2007u && revision != 2010u && revision != 2013u && revision != 2018u)
        || (trailer != Trailer::OdaBitShort && trailer != Trailer::ProducerBitLong)) {
        proof.status = Status::Unsupported; return proof;
    }
    proof.split = revision >= 2007u;
    if (!detail::extentValid(data) || (proof.split && (!detail::extentValid(strings)
        || (data.bytes == strings.bytes && data.begin < strings.end && strings.begin < data.end))))
        return proof;
    detail::Bits body(detail::recordWindow(data));
    detail::Bits detached(proof.split ? detail::recordWindow(strings) : View{});
    auto& names = proof.split ? detached : body;
    const auto scalar = [&](Field field, bool longValue) noexcept {
        auto& span = proof.fields[static_cast<std::size_t>(field)];
        span.begin = body.pos;
        const auto value = longValue ? body.bl() : body.bs();
        span.end = body.pos;
        return value;
    };
    proof.number = static_cast<std::uint16_t>(scalar(Field::Number, false));
    proof.proxy = static_cast<std::uint16_t>(scalar(Field::Proxy, false));
    for (unsigned n = 0; n != 3u; ++n) {
        proof.text[n] = detail::text(names, proof.split);
        proof.fields[static_cast<std::size_t>(Field::App) + n] =
            {proof.text[n].length.begin, proof.text[n].units.end};
    }
    auto& zombie = proof.fields[static_cast<std::size_t>(Field::Zombie)];
    zombie.begin = body.pos; proof.zombie = body.take(1u) != 0u; zombie.end = body.pos;
    proof.itemClass = static_cast<std::uint16_t>(scalar(Field::ItemClass, false));
    proof.unknownItemClass = proof.itemClass != 0x1F2u && proof.itemClass != 0x1F3u;
    if (revision >= 2004u) {
        proof.instances = detail::signed32(scalar(Field::Instances, true));
        auto& dwg = proof.fields[static_cast<std::size_t>(Field::DwgVersion)];
        dwg.begin = body.pos; proof.dwgVersion = detail::versionValue(body, trailer); dwg.end = body.pos;
        auto& maint = proof.fields[static_cast<std::size_t>(Field::Maintenance)];
        maint.begin = body.pos; proof.maintenance = detail::versionValue(body, trailer); maint.end = body.pos;
        proof.unknown1 = detail::signed32(scalar(Field::Unknown1, true));
        proof.unknown2 = detail::signed32(scalar(Field::Unknown2, true));
    }
    proof.dataEnd = body.pos; proof.stringEnd = names.pos;
    if (body.good && names.good && proof.number >= 500u && proof.instances >= 0)
        proof.status = Status::Observed;
    return proof;
}

// Exact bounded-record form: reject any unconsumed scalar or string bits.
inline Proof observe(View data, View strings, unsigned revision,
                     Trailer trailer = Trailer::OdaBitShort) noexcept {
    auto proof = scan(data, strings, revision, trailer);
    if (proof.status == Status::Observed
        && (proof.dataEnd != data.end || proof.stringEnd != (proof.split ? strings.end : data.end)))
        proof.status = Status::Malformed;
    return proof;
}

// Canonical UTF-8 comparison, not a legacy codepage decoder. Structural text
// observations with unknown legacy bytes, NULs or invalid surrogates cannot
// bind a semantic name. No allocation and no production Unicode helper.
inline bool equalsUtf8(View source, const Text& text, bool unicode,
                       const char* expected, std::size_t size) noexcept {
    if (!detail::extentValid(source) || text.issues || (!expected && size) || text.units.begin < source.begin
        || text.units.end > source.end || text.units.end < text.units.begin
        || text.units.end - text.units.begin != static_cast<std::uint32_t>(text.count) * (unicode ? 16u : 8u))
        return false;
    detail::Bits bits({source.bytes, source.size, text.units.begin, text.units.end});
    std::size_t pos = 0u;
    const auto emit = [&](std::uint32_t byte) noexcept {
        if (pos >= size || static_cast<unsigned char>(expected[pos]) != byte) return false;
        ++pos; return true;
    };
    while (bits.good && bits.pos < bits.view.end) {
        auto scalar = bits.little(unicode ? 2u : 1u);
        if (scalar == 0u || (!unicode && scalar > 127u)) return false;
        if (unicode && scalar >= 0xD800u && scalar <= 0xDBFFu) {
            const auto low = bits.little(2u);
            if (low < 0xDC00u || low > 0xDFFFu) return false;
            scalar = 0x10000u + ((scalar - 0xD800u) << 10u) + low - 0xDC00u;
        } else if (unicode && scalar >= 0xDC00u && scalar <= 0xDFFFu) return false;
        if (scalar < 0x80u) { if (!emit(scalar)) return false; }
        else if (scalar < 0x800u) {
            if (!emit(0xC0u | (scalar >> 6u)) || !emit(0x80u | (scalar & 63u))) return false;
        } else if (scalar < 0x10000u) {
            if (!emit(0xE0u | (scalar >> 12u)) || !emit(0x80u | ((scalar >> 6u) & 63u))
                || !emit(0x80u | (scalar & 63u))) return false;
        } else if (!emit(0xF0u | (scalar >> 18u)) || !emit(0x80u | ((scalar >> 12u) & 63u))
                   || !emit(0x80u | ((scalar >> 6u) & 63u)) || !emit(0x80u | (scalar & 63u))) return false;
    }
    return bits.good && pos == size;
}

// Explicit single-terminal-NUL convention, not a tolerant trim or a codec.
// The original count and final encoded unit are independently checked; the
// prefix comparator revalidates every unit, refusing interior/repeated NULs,
// invalid Unicode and unknown legacy codepage bytes even if issue bits change.
inline bool equalsTerminatedUtf8(View source, const Text& text, bool unicode,
                                 const char* expected, std::size_t size) noexcept {
    const auto width = unicode ? 16u : 8u;
    if (!detail::extentValid(source) || text.count == 0u || text.units.begin < source.begin
        || text.units.end > source.end || text.units.end < text.units.begin
        || text.units.end - text.units.begin != static_cast<std::uint32_t>(text.count) * width)
        return false;
    detail::Bits last({source.bytes, source.size, text.units.end - width, text.units.end});
    if (last.little(unicode ? 2u : 1u) != 0u || !last.good) return false;
    auto prefix = text;
    --prefix.count;
    prefix.units.end -= width;
    prefix.issues &= ~EmbeddedNull;
    return equalsUtf8(source, prefix, unicode, expected, size);
}
} // namespace dx_class_wire
#endif
