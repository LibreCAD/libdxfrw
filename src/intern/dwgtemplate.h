/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)                **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**  SPDX-License-Identifier: GPL-2.0-or-later                                 **
******************************************************************************/

#ifndef DWGTEMPLATE_H
#define DWGTEMPLATE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include "../drw_header.h"

namespace dwgTemplate {

// MEASUREMENT is independent of INSUNITS (ODA section 22). Preserve the
// existing DWG writer's English default only when no explicit value exists.
// Both spellings are accepted by the header API; conflicting aliases are not
// silently resolved by unordered-map iteration or a numeric mask.
inline bool headerMeasurement(const DRW_Header* header,
                              std::uint16_t& measurement) {
    measurement = 0;
    if (header == nullptr)
        return true;
    bool present = false;
    for (const char* key : {"MEASUREMENT", "$MEASUREMENT"}) {
        const auto it = header->vars.find(key);
        if (it == header->vars.end())
            continue;
        if (it->second == nullptr || it->second->type() != DRW_Variant::INTEGER)
            return false;
        const int value = it->second->i_val();
        if ((value != 0 && value != 1)
            || (present && measurement != static_cast<std::uint16_t>(value)))
            return false;
        measurement = static_cast<std::uint16_t>(value);
        present = true;
    }
    return true;
}

// ODA section 22's canonical empty description: RS0 followed by RSmeasurement.
inline std::array<std::uint8_t, 4> emptyDescription(std::uint16_t measurement) {
    return {{0, 0, static_cast<std::uint8_t>(measurement), 0}};
}

enum class DecodeResult {
    Valid,
    Malformed,
    DescriptionPresent,
    UnsupportedDescription
};
constexpr std::size_t MaxSectionSize = 65538;

// The logical section boundary, not decompressed page capacity, owns every
// field. ODA section 22 describes a signed byte count followed by a UInt16.
// Retain the exact six-byte, one-unit UTF-16 NUL form previously emitted by
// this library as a narrow compatibility exception. LibreDWG's R2007+ T16
// path uses TU16, which conflicts with ODA's byte-count description; non-empty
// TU16-shaped data stays warning-only until producer evidence resolves the
// version-specific layout.
inline DecodeResult decode(const std::uint8_t* data, std::size_t size,
                           DRW::Version version, std::uint16_t& measurement,
                           std::string* description = nullptr) {
    if (description != nullptr)
        description->clear();
    if (data == nullptr || size < 4 || size > MaxSectionSize)
        return DecodeResult::Malformed;
    const std::uint16_t length = static_cast<std::uint16_t>(data[0])
        | static_cast<std::uint16_t>(data[1]) << 8;
    if (length > 0x7fffu)
        return DecodeResult::Malformed;
    if (length > size - 4)
        return DecodeResult::Malformed;
    std::size_t valueOffset = 2 + length;
    const bool legacyModernEmpty = version >= DRW::AC1021 && length == 1
        && size == 6 && data[2] == 0 && data[3] == 0;
    if (legacyModernEmpty)
        valueOffset = 4;
    else if (version >= DRW::AC1021 && length != 0
             && size - 4 != length)
        return DecodeResult::UnsupportedDescription;
    if (valueOffset > size || size - valueOffset != 2)
        return DecodeResult::Malformed;
    const std::uint16_t value = static_cast<std::uint16_t>(data[valueOffset])
        | static_cast<std::uint16_t>(data[valueOffset + 1]) << 8;
    if (value > 1)
        return DecodeResult::Malformed;
    measurement = value;
    bool hasDescription = false;
    for (std::size_t i = 0; i < length; ++i) {
        hasDescription = hasDescription || data[2 + i] != 0;
    }
    if (hasDescription) {
        if (description != nullptr)
            description->assign(reinterpret_cast<const char*>(data + 2), length);
        return DecodeResult::DescriptionPresent;
    }
    return DecodeResult::Valid;
}

} // namespace dwgTemplate

#endif
