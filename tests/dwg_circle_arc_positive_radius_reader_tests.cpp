// SPDX-License-Identifier: GPL-2.0-or-later
#include "drw_entities.h"
#include "drw_header.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgwriter21.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

class DwgBlockRecordXrefWireTestAccess {
public:
    static bool frame(dwgWriter21& writer, std::uint32_t handle,
                      std::vector<std::uint8_t>& body) {
        const auto it = std::find_if(
            writer.m_objectMap.cbegin(), writer.m_objectMap.cend(),
            [handle](const auto& item) { return item.first == handle; });
        if (it == writer.m_objectMap.cend())
            return false;
        const auto& bytes = writer.m_buf.data();
        if (it->second >= bytes.size())
            return false;
        dwgBuffer frameBytes(const_cast<std::uint8_t*>(
            bytes.data() + it->second), bytes.size() - it->second);
        const std::int32_t length = frameBytes.getModularShort();
        const std::size_t offset = frameBytes.getPosition();
        if (!frameBytes.isGood() || length <= 0
            || offset + static_cast<std::size_t>(length) + 2u
                > bytes.size() - it->second)
            return false;
        body.assign(bytes.begin() + static_cast<std::ptrdiff_t>(
                        it->second + offset),
                    bytes.begin() + static_cast<std::ptrdiff_t>(
                        it->second + offset + static_cast<std::size_t>(length)));
        return true;
    }
};

struct CircleProbe : DRW_Circle {
    using DRW_Circle::parseDwg;
};

struct ArcProbe : DRW_Arc {
    using DRW_Arc::parseDwg;
};

static bool putBitField(std::vector<std::uint8_t>& body, double from,
                        double to) {
    dwgBufferW source;
    dwgBufferW replacement;
    source.putBitDouble(from);
    replacement.putBitDouble(to);
    if (!source.isGood() || !replacement.isGood()
        || source.bitCount() != replacement.bitCount())
        return false;
    const std::size_t fieldBits = source.bitCount();
    const auto bitAt = [](const std::vector<std::uint8_t>& bytes,
                          std::size_t bit) {
        return (bytes[bit / 8u] >> (7u - bit % 8u)) & 1u;
    };
    for (std::size_t start = 0; start + fieldBits <= body.size() * 8u;
         ++start) {
        bool matches = true;
        for (std::size_t offset = 0; offset < fieldBits; ++offset) {
            if (bitAt(body, start + offset) != bitAt(source.data(), offset)) {
                matches = false;
                break;
            }
        }
        if (!matches)
            continue;
        for (std::size_t offset = 0; offset < fieldBits; ++offset) {
            const std::size_t target = start + offset;
            const auto mask = static_cast<std::uint8_t>(
                0x80u >> (target % 8u));
            if (bitAt(replacement.data(), offset) != 0u)
                body[target / 8u] |= mask;
            else
                body[target / 8u] &= static_cast<std::uint8_t>(~mask);
        }
        return true;
    }
    return false;
}

static bool run() {
    DRW_Header header;
    dwgWriter21 writer(nullptr, &header);
    if (!writer.prepareDwgClassManifest() || !writer.writeFileHeaderStub()
        || !writer.writeDwgHeader() || !writer.writeDwgClasses()
        || !writer.writeDwgObjects())
        return false;

    DRW_Circle circle;
    circle.basePoint = {11.0, 12.0, 13.0};
    circle.radious = 2.5;
    if (!writer.encodeEntity(&circle))
        return false;
    DRW_Arc arc;
    arc.basePoint = {21.0, 22.0, 23.0};
    arc.radious = 2.5;
    arc.staangle = 0.25;
    arc.endangle = 1.0;
    if (!writer.encodeEntity(&arc))
        return false;

    std::vector<std::uint8_t> circleBody;
    std::vector<std::uint8_t> arcBody;
    if (!DwgBlockRecordXrefWireTestAccess::frame(writer, circle.handle, circleBody)
        || !DwgBlockRecordXrefWireTestAccess::frame(writer, arc.handle, arcBody)
        || !putBitField(circleBody, 2.5, -2.5)
        || !putBitField(arcBody, 2.5, -2.5))
        return false;

    CircleProbe parsedCircle;
    dwgBuffer badCircle(circleBody.data(), circleBody.size());
    if (parsedCircle.parseDwg(DRW::AC1021, &badCircle, 0u)
        || parsedCircle.radious != 0.0
        || parsedCircle.basePoint.x != 0.0)
        return false;
    std::vector<std::uint8_t> validCircle;
    if (!DwgBlockRecordXrefWireTestAccess::frame(writer, circle.handle, validCircle))
        return false;
    dwgBuffer retryCircle(validCircle.data(), validCircle.size());
    if (!parsedCircle.parseDwg(DRW::AC1021, &retryCircle, 0u)
        || parsedCircle.radious != 2.5 || parsedCircle.basePoint.x != 11.0)
        return false;

    ArcProbe parsedArc;
    dwgBuffer badArc(arcBody.data(), arcBody.size());
    if (parsedArc.parseDwg(DRW::AC1021, &badArc, 0u)
        || parsedArc.radious != 0.0 || parsedArc.basePoint.x != 0.0
        || parsedArc.staangle != 0.0 || parsedArc.endangle != 0.0)
        return false;
    std::vector<std::uint8_t> validArc;
    if (!DwgBlockRecordXrefWireTestAccess::frame(writer, arc.handle, validArc))
        return false;
    dwgBuffer retryArc(validArc.data(), validArc.size());
    if (!parsedArc.parseDwg(DRW::AC1021, &retryArc, 0u)
        || parsedArc.radious != 2.5 || parsedArc.basePoint.x != 21.0
        || parsedArc.staangle != 0.25 || parsedArc.endangle != 1.0)
        return false;
    return true;
}

int main() {
    const bool passed = run();
    std::cout << "DWG CIRCLE/ARC positive-radius parser refusal and retry: "
              << (passed ? "PASS" : "FAIL") << '\n';
    return passed ? 0 : 1;
}
