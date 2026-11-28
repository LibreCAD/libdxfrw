// SPDX-License-Identifier: GPL-2.0-or-later
// This source-specific probe takes the ODA-produced AC1024 witness documented
// in p6-ac1024-default-material-source-profile-2026-11-04.json. It checks its
// SHA-256 separately as described there, then compares exact object-frame bits.
#include "intern/dwg_default_material.h"
#include "intern/dwgutil.h"
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {
void appendBits(dwgBufferW& dst, const dwgBufferW& src) {
    for (std::uint64_t i = 0; i < src.bitCount(); ++i)
        dst.putBit((src.data()[static_cast<std::size_t>(i / 8u)] >> (7u - i % 8u)) & 1u);
}
}
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "usage: probe <documented AC1024 source.dwg>\n"; return 2; }
    std::ifstream f(argv[1], std::ios::binary);
    std::vector<std::uint8_t> source((std::istreambuf_iterator<char>(f)), {});
    // Independently captured from LibreDWG 0.14 -v9 page and object logs;
    // these constants are not general DWG offsets.
    constexpr std::size_t pageStart = 1696, compressedSize = 6499;
    if (source.size() < pageStart + compressedSize) return 2;
    std::vector<std::uint8_t> page(29696);
    dwgCompressor decompressor;
    if (!decompressor.decompress18(source.data() + pageStart, page.data(),
                                   compressedSize, page.size())) return 2;
    const std::array<drwDwgDefaultMaterial::Kind, 3> kinds{
        drwDwgDefaultMaterial::Kind::ByBlock, drwDwgDefaultMaterial::Kind::ByLayer,
        drwDwgDefaultMaterial::Kind::Global};
    const std::array<const char*, 3> names{"ByBlock", "ByLayer", "Global"};
    const std::array<std::size_t, 3> offsets{3476, 3578, 3680};
    bool allMatch = true;
    for (std::size_t k = 0; k < kinds.size(); ++k) {
        dwgBufferW body, strings, object;
        if (!drwDwgDefaultMaterial::build(DRW::AC1024, kinds[k], body, strings)) return 2;
        object.putObjType(DRW::AC1024, 503);
        dwgHandle handle; handle.ref = static_cast<std::uint32_t>(0x1Fu + k);
        object.putHandle(handle);
        object.putBitShort(0); object.putBitLong(1); object.putBit(1);
        appendBits(object, body); appendBits(object, strings);
        object.putRawShort16(static_cast<std::uint16_t>(strings.bitCount()));
        object.putBit(1);
        handle.code = DRW::DwgSoftPointer; handle.ref = 0xEu;
        object.putHandle(handle); object.putHandle(handle);
        object.putBit(0); object.putBit(0);
        if (!object.isGood() || offsets[k] * 8u + object.bitCount() > page.size() * 8u) return 2;
        std::size_t mismatches = 0;
        for (std::uint64_t bit = 0; bit < object.bitCount(); ++bit) {
            const std::uint64_t position = offsets[k] * 8u + bit;
            const unsigned expected = (page[position / 8u] >> (7u - position % 8u)) & 1u;
            const unsigned actual = (object.data()[bit / 8u] >> (7u - bit % 8u)) & 1u;
            mismatches += expected != actual ? 1u : 0u;
        }
        std::cout << names[k] << ": " << object.bitCount() << " frame bits, "
                  << mismatches << " mismatches\n";
        allMatch &= mismatches == 0;
    }
    return allMatch ? 0 : 1;
}
