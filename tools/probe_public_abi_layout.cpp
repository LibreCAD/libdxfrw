// SPDX-License-Identifier: GPL-2.0-or-later
// Compile once against each candidate's public headers with the same compiler.
// Class sizes alone establish differences, not complete binary compatibility.
#include "drw_interface.h"
#include "libdxfrw.h"
#include "libdwgr.h"

#include <iostream>

#define SHOW_LAYOUT(Type) \
    std::cout << #Type << ' ' << sizeof(Type) << ' ' << alignof(Type) << '\n'

int main() {
    SHOW_LAYOUT(DRW_Interface);
    SHOW_LAYOUT(DRW_Entity);
    SHOW_LAYOUT(DRW_Line);
    SHOW_LAYOUT(DRW_Viewport);
    SHOW_LAYOUT(DRW_Layer);
    SHOW_LAYOUT(DRW_Header);
    SHOW_LAYOUT(DRW_Material);
    SHOW_LAYOUT(dxfRW);
    SHOW_LAYOUT(dwgR);
    SHOW_LAYOUT(dwgRW);
}
