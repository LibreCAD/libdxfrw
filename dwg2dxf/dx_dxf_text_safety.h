// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_DXF_TEXT_SAFETY_H
#define DX_DXF_TEXT_SAFETY_H
#include <string_view>

namespace dx_dxf_text_safety {
// Limit opaque cross-encoding transport to text that cannot contain a DXF
// escape or depend on a code page. Only separately qualified fields may
// admit literal backslashes; broader text needs a separate proof.
inline bool safeOpaqueTranscode(std::string_view text, bool allowLiteralBackslash = false) noexcept {
    for (std::size_t i = 0; i < text.size(); ++i) {
        const auto c = static_cast<unsigned char>(text[i]);
        if (c < 0x20u || c > 0x7eu || c == '^') return false;
        if (c == '\\') {
            if (!allowLiteralBackslash) return false;
            // Refuse CIF/MIF escape prefixes even if malformed or lower-case:
            // peers differ in which spellings they interpret.
            if (i + 2u < text.size() && text[i + 2u] == '+' &&
                (text[i + 1u] == 'U' || text[i + 1u] == 'u' ||
                 text[i + 1u] == 'M' || text[i + 1u] == 'm')) return false;
        }
    }
    return true;
}
}
#endif
