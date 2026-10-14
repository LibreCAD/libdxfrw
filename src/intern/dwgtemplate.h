/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)                **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**  SPDX-License-Identifier: GPL-2.0-or-later                                 **
******************************************************************************/

#ifndef DWGTEMPLATE_H
#define DWGTEMPLATE_H

#include <array>
#include <cstdint>
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

} // namespace dwgTemplate

#endif
