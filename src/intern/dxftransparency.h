/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
******************************************************************************/

#ifndef LIBDXFRW_DXFTRANSPARENCY_H
#define LIBDXFRW_DXFTRANSPARENCY_H

#include <cstdint>

namespace DRW_Transparency {

// DRW_Entity::transparency keeps the DWG ENC alpha_raw representation so
// existing consumers (including LibreCAD) can distinguish ByLayer, ByBlock,
// and explicit alpha. DXF group 440 uses a different type byte for explicit
// alpha: 2 in DXF, 3 in the DWG ENC stream.
constexpr std::uint32_t kByBlock = 0x01000000u;
constexpr std::uint32_t kDxfAlphaType = 0x02000000u;
constexpr std::uint32_t kDwgAlphaType = 0x03000000u;
constexpr std::uint32_t kTypeMask = 0xFF000000u;
constexpr std::uint32_t kAlphaMask = 0x000000FFu;
constexpr std::uint32_t kReservedMask = 0x00FFFF00u;

inline bool toDxfGroup440(int entityValue, std::uint32_t& value) noexcept {
    const std::uint32_t raw = static_cast<std::uint32_t>(entityValue);
    if (entityValue == -1) { // legacy DRW::Transparent sentinel
        value = kDxfAlphaType;
        return true;
    }
    switch (raw & kTypeMask) {
    case 0u: // ByLayer is represented by an omitted group 440.
        if ((raw & kReservedMask) != 0u)
            return false;
        value = 0u;
        return true;
    case kByBlock:
        if ((raw & kReservedMask) != 0u)
            return false;
        value = kByBlock;
        return true;
    case kDxfAlphaType: // accept values created directly by older clients
    case kDwgAlphaType:
        if ((raw & kReservedMask) != 0u)
            return false;
        value = kDxfAlphaType | (raw & kAlphaMask);
        return true;
    default:
        return false;
    }
}

inline bool fromDxfGroup440(std::uint32_t value, int& entityValue) noexcept {
    switch (value & kTypeMask) {
    case kByBlock:
        if ((value & ~kTypeMask) != 0u)
            return false;
        entityValue = static_cast<int>(kByBlock);
        return true;
    case kDxfAlphaType:
        if ((value & ~(kTypeMask | kAlphaMask)) != 0u)
            return false;
        entityValue = static_cast<int>(kDwgAlphaType | (value & kAlphaMask));
        return true;
    default:
        return false;
    }
}

inline bool toDwgAlphaRaw(int entityValue, std::uint32_t& value) noexcept {
    const std::uint32_t raw = static_cast<std::uint32_t>(entityValue);
    if (entityValue == -1) { // legacy DRW::Transparent sentinel
        value = kDwgAlphaType;
        return true;
    }
    switch (raw & kTypeMask) {
    case 0u:
        if ((raw & kReservedMask) != 0u)
            return false;
        value = 0u;
        return true;
    case kByBlock:
        if ((raw & kReservedMask) != 0u)
            return false;
        value = kByBlock;
        return true;
    case kDxfAlphaType: // accept values created directly by older clients
    case kDwgAlphaType:
        if ((raw & kReservedMask) != 0u)
            return false;
        value = kDwgAlphaType | (raw & kAlphaMask);
        return true;
    default:
        return false;
    }
}

} // namespace DRW_Transparency

#endif // LIBDXFRW_DXFTRANSPARENCY_H
