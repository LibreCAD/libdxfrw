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

class EllipseProbe : public DRW_Ellipse {
public:
    using DRW_Ellipse::parseDwg;
};

static bool replaceBitDouble(std::vector<std::uint8_t>& body, double from,
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

    DRW_Ellipse ellipse;
    ellipse.basePoint = {10.0, 20.0, 30.0};
    ellipse.secPoint = {4.0, 0.0, 0.0};
    ellipse.extPoint = {0.0, 0.0, 1.0};
    ellipse.ratio = 0.5;
    ellipse.staparam = 0.25;
    ellipse.endparam = 1.5;
    if (!writer.encodeEntity(&ellipse))
        return false;

    std::vector<std::uint8_t> validBody;
    if (!DwgBlockRecordXrefWireTestAccess::frame(writer, ellipse.handle,
                                                  validBody))
        return false;
    EllipseProbe parsed;
    dwgBuffer valid(validBody.data(), validBody.size());
    if (!parsed.parseDwg(DRW::AC1021, &valid, 0u)
        || parsed.ratio != 0.5 || parsed.basePoint.x != 10.0
        || parsed.secPoint.x != 4.0 || parsed.extPoint.z != 1.0)
        return false;

    std::vector<std::uint8_t> invalidBody = validBody;
    if (!replaceBitDouble(invalidBody, 0.5, 1.5))
        return false;
    dwgBuffer invalid(invalidBody.data(), invalidBody.size());
    if (parsed.parseDwg(DRW::AC1021, &invalid, 0u)
        || parsed.ratio != 1.0 || parsed.basePoint.x != 0.0
        || parsed.secPoint.x != 0.0 || parsed.extPoint.z != 1.0)
        return false;

    dwgBuffer retry(validBody.data(), validBody.size());
    return parsed.parseDwg(DRW::AC1021, &retry, 0u)
        && parsed.ratio == 0.5 && parsed.basePoint.x == 10.0;
}

int main() {
    const bool passed = run();
    std::cout << "DWG ELLIPSE invalid-ratio parser refusal/reset/retry: "
              << (passed ? "PASS" : "FAIL") << '\n';
    return passed ? 0 : 1;
}
