// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1014_MLINESTYLE_WIRE_H
#define DX_AC1014_MLINESTYLE_WIRE_H

#include "dx_ac1014_dictionaryvar_wire.h"
#include <cstring>

// Bounded R14 standard style observer. Caller binds source generation, owner
// dictionary, output graph and staged readback.
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
        || raw.m_handle != model.handle || raw.m_isEntity || raw.m_isCustomClass
        || raw.m_hasClassDefinition || raw.m_objectSize != raw.m_rawBytes.size()
        || raw.m_rawBytes.size() > UINT32_MAX / 8u || raw.m_bodyBitSize != 0u
        || raw.m_hasDataStorage || model.handle == 0u || model.parentHandle == 0u
        || (model.name != "STANDARD" && model.name != "Standard")
        || !model.description.empty() || model.flags != 0
        || model.reactorCount() != 1 || model.reactorHandles.size() != 1u
        || model.reactorHandles.front() != model.parentHandle || model.xDictHandle != 0u
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
    Bits b{raw.m_rawBytes.data(), 0u, static_cast<std::uint32_t>(raw.m_rawBytes.size() * 8u)};
    constexpr double halfPi = 1.5707963267948966;
    if (b.shortValue() != 73u || !reference(b, 0u, 0u, model.handle, true)
        || b.shortValue() != 0u) return false;
    const auto bodyEnd = b.little(4u);
    return b.longValue() == 1u && text(b, model.name) && text(b, "")
        && b.shortValue() == 0u && b.shortValue() == 256u
        && bitDouble(b, halfPi) && bitDouble(b, halfPi) && b.take(8u) == 2u
        && bitDouble(b, 0.5) && b.shortValue() == 256u
        && b.shortValue() == 32767u && bitDouble(b, -0.5)
        && b.shortValue() == 256u && b.shortValue() == 32767u
        && b.pos == bodyEnd && reference(b, 4u, model.handle, model.parentHandle)
        && reference(b, 4u, model.handle, model.parentHandle)
        && reference(b, 3u, model.handle, 0u) && padding(b);
}
} // namespace dx_ac1014_mlinestyle_wire
#endif
