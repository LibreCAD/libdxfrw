#ifndef DX_SYMBOL_NAME_KEY_H
#define DX_SYMBOL_NAME_KEY_H

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
#include "intern/drw_symbol_casefold_data.h"

namespace dx_symbol_output {
// Ambiguity detection only: never rewrite semantic/output spelling. The
// pinned invariant Unicode closure includes simple-uppercase and full-fold
// equivalents without depending on process locale or an optional platform
// Unicode library. This is not a native consumer collation certificate.
inline bool nameKey(std::string_view text, std::string& key) {
    constexpr std::size_t maxInput = 512u * 1024u, maxKey = 3u * maxInput;
    key.clear();
    const auto fail = [&]() { key.clear(); return false; };
    if (text.size() > maxInput) return false;
    key.reserve(text.size());
    const auto append = [&](std::uint32_t scalar) {
        const std::size_t width = scalar <= 0x7Fu ? 1u : scalar <= 0x7FFu ? 2u : scalar <= 0xFFFFu ? 3u : 4u;
        if (width > maxKey - key.size()) return false;
        if (width == 1u) key.push_back(static_cast<char>(scalar));
        else {
            key.push_back(static_cast<char>((width == 2u ? 0xC0u : width == 3u ? 0xE0u : 0xF0u)
                | (scalar >> (6u * (width - 1u)))));
            for (std::size_t part = width - 1u; part != 0u; --part)
                key.push_back(static_cast<char>(0x80u | ((scalar >> (6u * (part - 1u))) & 0x3Fu)));
        }
        return true;
    };
    for (std::size_t at = 0u; at < text.size();) {
        const auto lead = static_cast<unsigned char>(text[at++]);
        if (lead < 0x80u) {
            if (!append(lead >= 'A' && lead <= 'Z' ? lead + ('a' - 'A') : lead)) return fail();
            continue;
        }
        const unsigned width = lead >= 0xC2u && lead <= 0xDFu ? 2u
            : lead >= 0xE0u && lead <= 0xEFu ? 3u : lead >= 0xF0u && lead <= 0xF4u ? 4u : 0u;
        if (width == 0u || width - 1u > text.size() - at) return fail();
        std::uint32_t scalar = lead & (0x7Fu >> width);
        for (unsigned part = 1u; part < width; ++part) {
            const auto byte = static_cast<unsigned char>(text[at++]);
            if ((byte & 0xC0u) != 0x80u) return fail();
            scalar = (scalar << 6u) | (byte & 0x3Fu);
        }
        if (scalar < (width == 2u ? 0x80u : width == 3u ? 0x800u : 0x10000u)
            || scalar > 0x10FFFFu || (scalar >= 0xD800u && scalar <= 0xDFFFu)) return fail();
        const auto* first = std::begin(drw_symbol_casefold_data::mappings);
        const auto* last = std::end(drw_symbol_casefold_data::mappings);
        while (first != last) {
            const auto* middle = first + (last - first) / 2;
            if (middle->source < scalar) first = middle + 1;
            else last = middle;
        }
        if (first != std::end(drw_symbol_casefold_data::mappings) && first->source == scalar) {
            for (unsigned part = 0u; part < first->count; ++part)
                if (!append(first->target[part])) return fail();
        } else if (!append(scalar)) return fail();
    }
    return true;
}
} // namespace dx_symbol_output
#endif
