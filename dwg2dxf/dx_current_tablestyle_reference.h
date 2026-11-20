// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_CURRENT_TABLESTYLE_REFERENCE_H
#define DX_CURRENT_TABLESTYLE_REFERENCE_H

#include "drw_objects.h"
#include <cstdint>
#include <map>
#include <string_view>

// ASCII case folding matches the adapter's dictionary-name comparison. Bytes
// outside ASCII compare exactly; no Unicode case-equivalence is assumed.
namespace dx_current_tablestyle_reference {
inline bool sameName(std::string_view left, std::string_view right) noexcept {
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        const auto upper = [](unsigned char c) {
            return c >= 'a' && c <= 'z' ? static_cast<unsigned char>(c - 'a' + 'A') : c;
        };
        if (upper(static_cast<unsigned char>(left[i]))
            != upper(static_cast<unsigned char>(right[i]))) return false;
    }
    return true;
}
inline bool resolves(std::string_view value, const DRW_Dictionary& dictionary,
                     const std::map<std::uint32_t, DRW_TableStyle>& styles) noexcept {
    if (value.empty() || dictionary.handle == 0u || dictionary.handle == DRW::NoHandle)
        return false;
    const DRW_Dictionary::Entry* selected = nullptr;
    for (const auto& entry : dictionary.m_entries) {
        if (!sameName(entry.m_name, value)) continue;
        if (selected) return false;
        selected = &entry;
    }
    if (!selected || selected->m_handle == 0u || selected->m_handle == DRW::NoHandle)
        return false;
    const auto style = styles.find(selected->m_handle);
    return style != styles.end() && style->second.handle == selected->m_handle
        && style->second.parentHandle == dictionary.handle
        && sameName(style->second.m_name, value);
}
} // namespace dx_current_tablestyle_reference
#endif
