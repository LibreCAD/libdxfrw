/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**                                                                           **
**  This library is distributed in the hope that it will be useful,          **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of           **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the             **
**  GNU General Public License for more details.                             **
******************************************************************************/

#ifndef DWGHEADERSECTION_H
#define DWGHEADERSECTION_H

#include "dwgbuffer.h"
#include "dwgsafety.h"
#include <limits>

// ODA v5.4.1 section 9, pp.72/83. This strict envelope is implemented/tested
// only for AC1015 here. Modern boundaries remain gated.
enum class DwgHeaderEnvelopeError { None, Bounds, BeginSentinel, EndSentinel };

struct DwgHeaderEnvelope {
    DwgHeaderEnvelopeError error{DwgHeaderEnvelopeError::Bounds};
    bool compact{false};
    std::uint64_t start{0}, payloadEnd{0}, trailerEnd{0}, offset{0};
    std::uint64_t expected{0}, observed{0};
    bool hasValues{false};
    std::uint16_t storedCrc{0};
};

inline DwgHeaderEnvelope inspectLegacyDwgHeader(const dwgBuffer& input) {
    DwgHeaderEnvelope result;
    result.start = result.offset = input.getPosition();
    if (!input.isGood() || input.getBitPos() != 0u
        || input.size() > dwgSafety::MaxBufferSize)
        return result;
    auto probe = input.forkIndependent();
    const std::uint32_t count = probe.getRawLong32();
    if (!probe.isGood())
        return result;
    std::uint64_t dataStart = 0;
    if (!dwgSafety::add(result.start, 4u, dataStart)
        || !dwgSafety::add(dataStart, count, result.payloadEnd))
        return result;
    if (result.start == 0u && result.payloadEnd == input.size()) {
        result.compact = true;
        result.trailerEnd = result.payloadEnd;
        result.error = DwgHeaderEnvelopeError::None;
        return result;
    }
    if (result.start != 16u || count == 0u
        || !dwgSafety::add(result.payloadEnd, 18u, result.trailerEnd)
        || result.trailerEnd > input.size()
        || result.payloadEnd > std::numeric_limits<std::int32_t>::max())
        return result;
    probe.setPosition(0u);
    for (std::size_t i = 0; i < 16u; ++i) {
        const auto value = probe.getRawChar8();
        if (!probe.isGood() || value != dwgSentinels::HEADER_BEGIN[i]) {
            result.error = DwgHeaderEnvelopeError::BeginSentinel;
            result.offset = i;
            result.expected = dwgSentinels::HEADER_BEGIN[i];
            result.observed = value;
            result.hasValues = probe.isGood();
            return result;
        }
    }
    if (!probe.setPosition(result.payloadEnd))
        return result;
    result.storedCrc = probe.getRawShort16();
    for (std::size_t i = 0; i < 16u; ++i) {
        const auto value = probe.getRawChar8();
        if (!probe.isGood() || value != dwgSentinels::HEADER_END[i]) {
            result.error = DwgHeaderEnvelopeError::EndSentinel;
            result.offset = result.payloadEnd + 2u + i;
            result.expected = dwgSentinels::HEADER_END[i];
            result.observed = value;
            result.hasValues = probe.isGood();
            return result;
        }
    }
    result.error = DwgHeaderEnvelopeError::None;
    return result;
}

#endif
