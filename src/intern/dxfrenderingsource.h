// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFRENDERINGSOURCE_H
#define DXFRENDERINGSOURCE_H
#include "drw_objects.h"
#include <utility>

namespace dxfRendering {
class Observation {
public:
    static std::pair<bool,bool> visualStyle(const DRW_VisualStyle& value) noexcept {
        return {value.m_dxfInVisualStyle,value.m_dxfInPropertyTail};
    }
};
}
#endif
