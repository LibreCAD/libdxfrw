// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1024_AERIAL_APPID_WIRE_H
#define DX_AC1024_AERIAL_APPID_WIRE_H

// Bounded source-wire observers for the aerial and mechanical AC1024 ACAD
// APPID/APPID_CONTROL profiles. Exact positions and terminal bits are source
// observations, not a general rule for other AC1024 files. Container/frame
// integrity, producer semantics and whole-file output remain separate gates.
#include "drw_objects.h"
#include "dx_raster_dwg_wire.h"

#include <array>
#include <cstdint>

namespace dx_ac1024_aerial_appid_wire {
namespace detail {
using dx_raster_wire::detail::Bits;
using dx_raster_wire::detail::handle;

inline bool common(Bits& body, std::uint32_t type,
                   std::uint32_t selfHandle) noexcept {
    if (body.take(2u) != 0u || body.take(8u) != type || body.pos != 10u)
        return false;
    const auto self = handle(body, 0u, 0u, true);
    if (!body.good || self.role != 0u || self.length != 1u
        || self.value != selfHandle || body.pos != 26u
        || body.shortValue() != 0u || body.pos != 28u
        || body.longValue() != 0u || body.pos != 30u
        || body.take(1u) != 1u || body.pos != 31u)
        return false;
    return body.good;
}

inline bool appId(const DRW_UnsupportedObject& raw) noexcept {
    if (raw.m_version != DRW::AC1024 || raw.m_handle != 0x12u
        || raw.m_objectType != DRW::DwgAppIdObjectType
        || raw.m_rawBytes.size() != 20u || raw.m_objectSize != 20u
        || raw.m_bodyBitSize != 28u)
        return false;
    const auto* bytes = raw.m_rawBytes.data();
    Bits body{bytes, 0u, 132u};
    if (!common(body, DRW::DwgAppIdObjectType, 0x12u)
        || body.shortValue() != 0u || body.pos != 33u
        || body.take(8u) != 0u || body.pos != 41u
        || body.shortValue() != 4u || body.pos != 51u)
        return false;
    constexpr std::array<std::uint16_t, 4> name{{'A', 'C', 'A', 'D'}};
    for (const auto unit : name)
        if (body.little(2u) != unit) return false;
    if (body.pos != 115u || body.little(2u) != 74u
        || body.take(1u) != 1u || !body.good || body.pos != 132u)
        return false;
    Bits handles{bytes, 132u, 160u};
    const auto owner = handle(handles, 4u, 0x12u);
    const auto xref = handle(handles, 5u, 0x12u);
    return handles.good && owner.role == 4u && owner.length == 1u
        && owner.value == 0x09u && xref.role == 5u && xref.length == 0u
        && xref.value == 0u && handles.pos == 156u
        && handles.take(4u) == 15u && handles.good && handles.pos == 160u;
}

template <std::size_t N>
inline bool controlProfile(const DRW_UnsupportedObject& raw,
                           const DRW_DwgFramePublication& publication,
                           const std::array<std::uint32_t, N>& members,
                           std::uint32_t byteSize,
                           std::uint32_t handleBits) noexcept {
    if (raw.m_version != DRW::AC1024 || raw.m_handle != 0x09u
        || raw.m_objectType != DRW::DwgAppIdControlObjectType
        || raw.m_rawBytes.size() != byteSize || raw.m_objectSize != byteSize
        || raw.m_bodyBitSize != handleBits)
        return false;
    if (publication.m_controlHandles.size() != members.size()) return false;
    const auto* bytes = raw.m_rawBytes.data();
    Bits body{bytes, 0u, 42u};
    if (!common(body, DRW::DwgAppIdControlObjectType, 0x09u)
        || body.longValue() != members.size() || body.pos != 41u
        || body.take(1u) != 0u || !body.good || body.pos != 42u)
        return false;
    Bits handles{bytes, 42u, byteSize * 8u};
    const auto owner = handle(handles, 4u, 0x09u);
    if (!handles.good || owner.role != 4u || owner.length != 0u
        || owner.value != 0u || handles.pos != 50u)
        return false;
    for (std::size_t i = 0u; i < members.size(); ++i) {
        const auto member = handle(handles, 2u, 0x09u);
        const auto expectedLength = members[i] <= 0xffu ? 1u : 2u;
        if (!handles.good || member.role != 2u || member.length != expectedLength
            || member.value != members[i]
            || publication.m_controlHandles[i] != members[i])
            return false;
    }
    return handles.pos == byteSize * 8u - 6u && handles.take(6u) == 63u
        && handles.good && handles.pos == byteSize * 8u;
}

inline bool control(const DRW_UnsupportedObject& raw,
                    const DRW_DwgFramePublication& publication) noexcept {
    constexpr std::array<std::uint32_t, 9> members{{
        0x12u, 0xf8u, 0x14fu, 0x150u, 0x151u,
        0x152u, 0x1a8u, 0x23bu, 0x2a3u
    }};
    return controlProfile(raw, publication, members, 32u, 214u);
}

inline bool mechanicalControl(const DRW_UnsupportedObject& raw,
                              const DRW_DwgFramePublication& publication) noexcept {
    constexpr std::array<std::uint32_t, 14> members{{
        0x12u, 0xddu, 0xdeu, 0xdfu, 0xe0u, 0x107u, 0x1b5u,
        0x1bau, 0x237u, 0x28eu, 0x385u, 0x41eu, 0x41fu, 0x48eu
    }};
    return controlProfile(raw, publication, members, 44u, 310u);
}
} // namespace detail

inline bool observe(const DRW_UnsupportedObject& appId,
                    const DRW_UnsupportedObject& control,
                    const DRW_DwgFramePublication& controlPublication) noexcept {
    return detail::appId(appId) && detail::control(control, controlPublication);
}

inline bool observeMechanical(const DRW_UnsupportedObject& appId,
                              const DRW_UnsupportedObject& control,
                              const DRW_DwgFramePublication& publication) noexcept {
    return detail::appId(appId) && detail::mechanicalControl(control, publication);
}
} // namespace dx_ac1024_aerial_appid_wire

#endif
