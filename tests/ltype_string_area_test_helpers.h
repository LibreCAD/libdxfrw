#ifndef LIBDXFRW_LTYPE_STRING_AREA_TEST_HELPERS_H
#define LIBDXFRW_LTYPE_STRING_AREA_TEST_HELPERS_H

#include <cstdint>
#include <string>
#include <vector>

#include "drw_objects.h"

namespace ltype_area_test {

inline DRW_LType make(const std::vector<std::string>& texts) {
    DRW_LType value;
    value.name = "LTYPE_AREA";
    value.desc = "fixed text area";
    value.size = static_cast<int>(texts.size());
    for (const std::string& text : texts) {
        DRW_LTypeSegment segment;
        segment.length = 2.0;
        segment.shapeFlags = 2; // Existing text-carrier convention, unchanged.
        segment.text = text;
        value.path.push_back(segment.length);
        value.segments.push_back(segment);
        value.length += segment.length;
    }
    return value;
}

// Independent field inspection: no production dwgBuffer/decoder or inverse
// encoder is used to inspect offsets and the fixed text-area bytes. This is
// a bounded wire projection, not an independent CAD-reader qualification.
class BitReader {
public:
    explicit BitReader(const std::vector<std::uint8_t>& bytes,
                       std::uint64_t bit = 0) : m_bytes(bytes), m_bit(bit) {}

    bool bits(unsigned count, std::uint64_t& value) {
        if (count > 64u || m_bit > m_bytes.size() * 8u
            || count > m_bytes.size() * 8u - m_bit)
            return false;
        value = 0;
        for (unsigned i = 0; i < count; ++i, ++m_bit)
            value = (value << 1) | ((m_bytes[static_cast<std::size_t>(m_bit / 8u)]
                >> (7u - static_cast<unsigned>(m_bit % 8u))) & 1u);
        return true;
    }

    bool skip(std::uint64_t count) {
        if (m_bit > m_bytes.size() * 8u || count > m_bytes.size() * 8u - m_bit)
            return false;
        m_bit += count;
        return true;
    }

    bool byte(std::uint8_t& value) {
        std::uint64_t raw = 0;
        if (!bits(8, raw))
            return false;
        value = static_cast<std::uint8_t>(raw);
        return true;
    }

    bool bitShort(std::uint16_t& value) {
        std::uint64_t code = 0;
        if (!bits(2, code))
            return false;
        if (code == 2 || code == 3) {
            value = code == 2 ? 0 : 256;
            return true;
        }
        std::uint8_t low = 0;
        std::uint8_t high = 0;
        if (!byte(low) || (code == 0 && !byte(high)))
            return false;
        value = static_cast<std::uint16_t>(low)
            | static_cast<std::uint16_t>(static_cast<std::uint16_t>(high) << 8);
        return true;
    }

    bool bitDouble() {
        std::uint64_t code = 0;
        return bits(2, code) && code < 3 && (code != 0 || skip(64));
    }

    bool text(bool unicode) {
        std::uint16_t count = 0;
        return bitShort(count)
            && skip(static_cast<std::uint64_t>(count) * (unicode ? 16u : 8u));
    }

private:
    const std::vector<std::uint8_t>& m_bytes;
    std::uint64_t m_bit;
};

struct WireArea {
    bool valid = false;
    std::vector<std::uint16_t> offsets;
    std::vector<std::uint16_t> flags;
    std::vector<std::uint8_t> bytes;
};

inline WireArea inspect(DRW::Version version,
                        const std::vector<std::uint8_t>& bytes,
                        std::uint64_t startBit = 0) {
    BitReader input(bytes, startBit);
    WireArea result;
    std::uint16_t ignored = 0;
    if (version <= DRW::AC1018 && !input.text(false))
        return result;
    if (version > DRW::AC1018) {
        if (!input.bitShort(ignored))
            return result;
    } else if (!input.skip(1) || !input.bitShort(ignored) || !input.skip(1)
               || !input.text(false)) {
        return result;
    }
    std::uint8_t alignment = 0;
    std::uint8_t count = 0;
    if (!input.bitDouble() || !input.byte(alignment) || alignment != 'A'
        || !input.byte(count))
        return result;
    bool areaPresent = version < DRW::AC1021;
    for (unsigned i = 0; i < count; ++i) {
        std::uint16_t offset = 0;
        std::uint16_t flags = 0;
        if (!input.bitDouble() || !input.bitShort(offset) || !input.skip(128)
            || !input.bitDouble() || !input.bitDouble() || !input.bitShort(flags))
            return result;
        result.offsets.push_back(offset);
        result.flags.push_back(flags);
        // ODA §20.4.58: DWG and DXF group 74 both use bit 2 for embedded
        // text; bit 4 indicates an embedded shape and does not add a text area.
        areaPresent = areaPresent || (flags & 2u) != 0;
    }
    if (areaPresent) {
        result.bytes.resize(version >= DRW::AC1021 ? 512u : 256u);
        for (std::uint8_t& value : result.bytes)
            if (!input.byte(value))
                return result;
    }
    result.valid = true;
    return result;
}

inline std::vector<std::uint8_t> asciiBytes(const std::string& text,
                                           bool unicode) {
    std::vector<std::uint8_t> result;
    for (const unsigned char value : text) {
        result.push_back(value);
        if (unicode)
            result.push_back(0);
    }
    return result;
}

inline bool matches(const WireArea& wire, DRW::Version version,
                    const std::vector<std::vector<std::uint8_t>>& texts) {
    if (!wire.valid || wire.offsets.size() != texts.size())
        return false;
    const std::size_t terminator = version >= DRW::AC1021 ? 2u : 1u;
    std::size_t offset = version <= DRW::AC1014 ? 1u : 0u;
    std::vector<std::uint8_t> expected(version >= DRW::AC1021 ? 512u : 256u, 0);
    for (std::size_t i = 0; i < texts.size(); ++i) {
        const auto& text = texts[i];
        if (offset > expected.size() || terminator > expected.size() - offset
            || text.size() > expected.size() - offset - terminator
            || wire.offsets[i] != offset)
            return false;
        for (const std::uint8_t byte : text)
            expected[offset++] = byte;
        offset += terminator;
    }
    return wire.bytes == expected;
}

} // namespace ltype_area_test

#endif
