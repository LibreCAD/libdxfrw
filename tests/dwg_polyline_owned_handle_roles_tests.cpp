// SPDX-License-Identifier: GPL-2.0-or-later
// Literal parent handle-stream controls for classic POLYLINE encoders.
#include "drw_entities.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgwriter21.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

class DwgPolylineTraceTestAccess {
public:
    static std::vector<std::uint32_t> vertices(const DRW_Polyline& polyline) {
        return {polyline.hadlesList.begin(), polyline.hadlesList.end()};
    }
    static std::uint8_t sequenceRole(const DRW_Polyline& polyline) {
        return polyline.seqEndH.code;
    }
};

namespace {
struct Probe : DRW_Polyline {
    using DRW_Polyline::encodeDwg;
    using DRW_Polyline::parseDwg;
};

bool check(DRW::Version version, int flags) {
    Probe polyline;
    polyline.handle = 0x120u;
    polyline.flags = flags;
    for (const auto value : {0x130u, 0x451u, 0x999u}) {
        auto vertex = std::make_shared<DRW_Vertex>();
        vertex->handle = value;
        polyline.appendVertex(vertex);
    }
    polyline.setDwgSeqEndHandle(0xa50u);
    dwgBufferW body, handles;
    if (!polyline.encodeDwg(version, &body, 0u, nullptr, &handles)
        || !body.isGood() || !handles.isGood()) return false;
    const auto& occurrences = handles.handleOccurrences();
    const bool modern = version >= DRW::AC1018;
    const std::size_t count = modern ? 4u : 3u;
    if (occurrences.size() < count) return false;
    const auto first = occurrences.size() - count;
    const std::array<std::uint32_t, 4> refs{{0x130u, 0x451u, 0x999u, 0xa50u}};
    const bool standard = (flags & (16 | 64)) == 0;
    const std::uint8_t role = modern && standard ? DRW::DwgHardOwnership : DRW::DwgHardPointer;
    const auto start = occurrences[first].startBit;
    if ((start & 7u) != 0u) return false;
    dwgHandle sentinel; sentinel.code = DRW::DwgSoftPointer; sentinel.ref = 0xbeefu;
    handles.putHandle(sentinel);
    dwgBuffer reader(handles.data().data(), handles.size());
    if (!reader.setPosition(start / 8u)) return false;
    for (std::size_t i = 0; i < count; ++i) {
        const auto expected = modern ? refs[i] : (i == 0u ? refs[0] : i == 1u ? refs[2] : refs[3]);
        const auto token = reader.getHandle();
        if (!reader.isGood() || token.code != role || token.ref != expected) return false;
    }
    const auto after = reader.getHandle();
    return reader.isGood() && after.code == DRW::DwgSoftPointer && after.ref == 0xbeefu;
}

bool checkHistoricalReader(int flags) {
    DRW_Header header;
    dwgWriter21 writer(nullptr, &header);
    Probe polyline;
    polyline.handle = 0x120u;
    polyline.flags = flags;
    for (const auto value : {0x130u, 0x451u, 0x999u}) {
        auto vertex = std::make_shared<DRW_Vertex>();
        vertex->handle = value;
        polyline.appendVertex(vertex);
    }
    polyline.setDwgSeqEndHandle(0xa50u);
    if (!writer.encodeEntity(&polyline)) return false;
    const auto& frame = writer.buffer();
    dwgBuffer framed(frame.data(), frame.size());
    const auto length = framed.getModularShort();
    const auto offset = framed.getPosition();
    if (!framed.isGood() || length <= 0 || offset > frame.size()
        || static_cast<std::size_t>(length) > frame.size() - offset) return false;
    const std::vector<std::uint8_t> data(frame.begin() + offset, frame.begin() + offset + length);
    const std::array<std::array<std::uint8_t, 3>, 4> tokens{{
        {{0x32u, 0x01u, 0x30u}}, {{0x32u, 0x04u, 0x51u}},
        {{0x32u, 0x09u, 0x99u}}, {{0x32u, 0x0au, 0x50u}}}};
    for (const bool oldRole : {false, true}) {
        auto candidate = data;
        for (const auto& token : tokens) {
            const auto match = std::search(candidate.begin(), candidate.end(), token.begin(), token.end());
            if (match == candidate.end()
                || std::search(match + 1, candidate.end(), token.begin(), token.end()) != candidate.end())
                return false;
            if (oldRole) *match = 0x52u;
        }
        dwgBuffer buffer(candidate.data(), candidate.size());
        Probe parsed;
        if (!parsed.parseDwg(DRW::AC1021, &buffer, 0u) || !buffer.isGood()
            || DwgPolylineTraceTestAccess::vertices(parsed)
                != std::vector<std::uint32_t>{0x130u, 0x451u, 0x999u}
            || parsed.dwgSeqEndHandle() != 0xa50u
            || DwgPolylineTraceTestAccess::sequenceRole(parsed)
                != (oldRole ? DRW::DwgHardPointer : DRW::DwgHardOwnership)) return false;
    }
    return true;
}
}
int main() {
    for (const auto version : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                                DRW::AC1024, DRW::AC1027, DRW::AC1032})
        for (const int flags : {0, 8, 16, 64})
            if (!check(version, flags)) {
                std::cerr << "POLYLINE owned-handle role mismatch version="
                          << int(version) << " flags=" << flags << '\n';
                return 1;
            }
    if (!checkHistoricalReader(0) || !checkHistoricalReader(8)) {
        std::cerr << "historical POLYLINE handle-role readback failed\n";
        return 1;
    }
    std::cout << "POLYLINE owned-handle roles PASS\n";
    return 0;
}
