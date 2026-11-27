// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_DXF_TEXT_SAFETY_H
#define DX_DXF_TEXT_SAFETY_H
#include <string_view>

namespace dx_dxf_text_safety {
// Limit opaque cross-encoding transport to text that cannot contain a DXF
// escape or depend on a code page. Broader text needs a separate proof.
inline bool safeOpaqueTranscode(std::string_view text) noexcept {
    for (const unsigned char c : text)
        if (c<0x20u || c>0x7eu || c=='^' || c=='\\') return false;
    return true;
}
}
#endif
