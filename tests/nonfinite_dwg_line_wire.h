#ifndef LIBDXFRW_TESTS_NONFINITE_DWG_LINE_WIRE_H
#define LIBDXFRW_TESTS_NONFINITE_DWG_LINE_WIRE_H

#include "drw_entities.h"
#include "intern/dwgbufferw.h"

#include <cstdint>
#include <cstring>
#include <limits>

/// Test-only LINE encoder that first emits a valid body, then changes exactly
/// one raw start-X field to a quiet NaN. This creates malformed wire input
/// without weakening production writer validation or committing a DWG file.
class NonFiniteDwgLineWire final : public DRW_Line {
public:
    [[nodiscard]] bool encodeDwg(DRW::Version version, dwgBufferW *buf,
                                 std::uint32_t bs = 0,
                                 dwgBufferW *strBuf = nullptr,
                                 dwgBufferW *handleBuf = nullptr) override {
        if (buf == nullptr)
            return false;

        constexpr double marker = 1234567.125;
        const double originalX = basePoint.x;
        basePoint.x = marker;
        const bool encoded = DRW_Line::encodeDwg(version, buf, bs, strBuf,
                                                  handleBuf);
        basePoint.x = originalX;
        if (!encoded)
            return false;

        std::uint64_t markerBits = 0;
        std::uint64_t nonFiniteBits = 0;
        const double nonFinite = std::numeric_limits<double>::quiet_NaN();
        std::memcpy(&markerBits, &marker, sizeof(markerBits));
        std::memcpy(&nonFiniteBits, &nonFinite, sizeof(nonFiniteBits));
        std::uint8_t markerBytes[sizeof(markerBits)];
        std::uint8_t nonFiniteBytes[sizeof(nonFiniteBits)];
        for (std::size_t i = 0; i < sizeof(markerBits); ++i) {
            markerBytes[i] = static_cast<std::uint8_t>(markerBits >> (i * 8));
            nonFiniteBytes[i] = static_cast<std::uint8_t>(nonFiniteBits >> (i * 8));
        }

        auto& bytes = buf->data();
        const std::size_t totalBits = bytes.size() * 8;
        constexpr std::size_t markerBitCount = sizeof(markerBytes) * 8;
        const auto bitAt = [&bytes](std::size_t bit) {
            return static_cast<std::uint8_t>(
                (bytes[bit / 8] >> (7 - (bit % 8))) & 1u);
        };
        std::size_t found = totalBits;
        for (std::size_t start = 0; start + markerBitCount <= totalBits; ++start) {
            bool matches = true;
            for (std::size_t bit = 0; bit < markerBitCount; ++bit) {
                const std::uint8_t expected = static_cast<std::uint8_t>(
                    (markerBytes[bit / 8] >> (7 - (bit % 8))) & 1u);
                if (bitAt(start + bit) != expected) {
                    matches = false;
                    break;
                }
            }
            if (!matches)
                continue;
            if (found != totalBits)
                return false;
            found = start;
        }
        if (found == totalBits)
            return false;
        for (std::size_t bit = 0; bit < markerBitCount; ++bit) {
            const std::uint8_t mask = static_cast<std::uint8_t>(
                1u << (7 - ((found + bit) % 8)));
            const std::size_t byte = (found + bit) / 8;
            const bool value = (nonFiniteBytes[bit / 8]
                >> (7 - (bit % 8))) & 1u;
            bytes[byte] = static_cast<std::uint8_t>(
                (bytes[byte] & static_cast<std::uint8_t>(~mask))
                | (value ? mask : 0u));
        }
        return true;
    }
};

#endif
