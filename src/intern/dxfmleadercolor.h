#ifndef DXF_MLEADER_COLOR_H
#define DXF_MLEADER_COLOR_H

#include <cstdint>
#include <cstring>

namespace dxfMLeaderColor {

struct Value {
    int index{0};
    std::int32_t rgb24{-1};
    std::uint8_t method{0};
};

inline bool isValid(const Value& value) noexcept {
    if (value.rgb24 >= 0)
        return value.index == 256 && value.rgb24 <= 0x00FFFFFF
            && (value.method == 0 || value.method == 0xC2u);
    if (value.rgb24 != -1)
        return false;
    if (value.method == 0)
        return value.index >= 0 && value.index <= 256;
    switch (value.method) {
    case 0xC0u: return value.index == 256;
    case 0xC1u: return value.index == 0;
    case 0xC3u: return value.index >= 0 && value.index <= 255;
    default: return false;
    }
}

inline bool decodeDxf(std::int32_t raw, Value& value) noexcept {
    if (raw >= 0 && raw <= 256) {
        value = {static_cast<int>(raw), -1, 0};
        return true;
    }

    std::uint32_t bits = 0;
    static_assert(sizeof(bits) == sizeof(raw), "DXF color word width");
    std::memcpy(&bits, &raw, sizeof(bits));
    const std::uint8_t method = static_cast<std::uint8_t>(bits >> 24);
    const std::uint32_t payload = bits & 0x00FFFFFFu;
    switch (method) {
    case 0xC0u:
        value = {256, -1, method};
        return true;
    case 0xC1u:
        value = {0, -1, method};
        return true;
    case 0xC2u:
        value = {256, static_cast<std::int32_t>(payload), method};
        return true;
    case 0xC3u:
        value = {static_cast<int>(payload & 0xFFu), -1, method};
        return true;
    default:
        return false;
    }
}

inline bool encodeDxf(const Value& value, std::int32_t& raw) noexcept {
    if (!isValid(value))
        return false;
    if (value.method == 0u && value.rgb24 < 0) {
        raw = value.index;
        return true;
    }

    std::uint32_t payload = 0u;
    std::uint8_t method = value.method;
    if (method == 0u && value.rgb24 >= 0)
        method = 0xC2u;
    switch (method) {
    case 0xC0u:
    case 0xC1u:
        break;
    case 0xC2u:
        payload = static_cast<std::uint32_t>(value.rgb24);
        break;
    case 0xC3u:
        payload = static_cast<std::uint32_t>(value.index);
        break;
    default:
        return false;
    }
    const std::uint32_t bits = (static_cast<std::uint32_t>(method) << 24)
        | payload;
    std::memcpy(&raw, &bits, sizeof(raw));
    return true;
}

} // namespace dxfMLeaderColor

#endif // DXF_MLEADER_COLOR_H
