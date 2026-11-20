// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1014_MLINESTYLE_WIRE_H
#define DX_AC1014_MLINESTYLE_WIRE_H

#include "dx_ac1014_dictionaryvar_wire.h"
#include <cstring>

// Exact R14 STANDARD style observed in LibreDWG's r14/v.dwg. Caller binds
// source generation, owner dictionary, output graph and staged readback.
// ODA v5.4.1 section 20.4.73.
namespace dx_ac1014_mlinestyle_wire {
inline bool bitDouble(dx_raster_wire::detail::Bits& bits, double expected) noexcept {
    const auto tag = bits.take(2u);
    double value = 0.0;
    if (tag == 0u) {
        const auto raw = bits.little(8u);
        std::memcpy(&value, &raw, sizeof value);
    } else if (tag == 1u) value = 1.0;
    else if (tag != 2u) return false;
    return bits.good && value == expected;
}

inline bool style(const DRW_UnsupportedObject& raw,
                  const DRW_MLineStyle& model) noexcept {
    using namespace dx_ac1014_dictionaryvar_wire;
    if (raw.m_version != DRW::AC1014 || raw.m_objectType != 73
        || raw.m_handle != 0x1bu || raw.m_isEntity || raw.m_isCustomClass
        || raw.m_hasClassDefinition || raw.m_objectSize != 62u
        || raw.m_rawBytes.size() != 62u || raw.m_bodyBitSize != 0u
        || raw.m_hasDataStorage || model.handle != 0x1bu
        || model.parentHandle != 0x1au || model.name != "STANDARD"
        || !model.description.empty() || model.flags != 0
        || model.reactorCount() != 1 || model.reactorHandles.size() != 1u
        || model.reactorHandles.front() != 0x1au || model.xDictHandle != 0u
        || !model.appData.empty() || !model.extData.empty()
        || model.hasDataStorageBinaryData() || model.elements.size() != 2u
        || !model.pendingAppIdResolutions.empty()
        || !model.pendingLayerRefResolutions.empty()
        || model.fillColor != 256 || model.startAngle != 90.0
        || model.endAngle != 90.0 || model.elements[0].offset != 0.5
        || model.elements[1].offset != -0.5) return false;
    for (const auto& element : model.elements)
        if (element.color != 256 || element.linetypeIndex != 32767
            || element.linetypeHandle != 0u || !element.linetype.empty())
            return false;
    Bits b{raw.m_rawBytes.data(), 0u, 496u};
    constexpr double halfPi = 1.5707963267948966;
    return b.shortValue() == 73u && token(b, 0u, 1u, 0u, 0x1bu, true)
        && b.shortValue() == 0u && b.little(4u) == 462u
        && b.longValue() == 1u && text(b, "STANDARD") && text(b, "")
        && b.shortValue() == 0u && b.shortValue() == 256u
        && bitDouble(b, halfPi) && bitDouble(b, halfPi) && b.take(8u) == 2u
        && bitDouble(b, 0.5) && b.shortValue() == 256u
        && b.shortValue() == 32767u && bitDouble(b, -0.5)
        && b.shortValue() == 256u && b.shortValue() == 32767u
        && b.pos == 462u && token(b, 8u, 0u, 0x1bu, 0x1au)
        && token(b, 4u, 1u, 0x1bu, 0x1au)
        && token(b, 3u, 0u, 0x1bu, 0u) && b.take(2u) == 3u
        && b.good && b.pos == b.end;
}
} // namespace dx_ac1014_mlinestyle_wire
#endif
