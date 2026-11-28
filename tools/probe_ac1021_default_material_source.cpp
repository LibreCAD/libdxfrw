// SPDX-License-Identifier: GPL-2.0-or-later
// Compare a hash-bound decompressed page from the documented AC1021 witness.
// This is a source-specific audit, not a general DWG or MATERIAL parser.
#include "intern/dwg_default_material.h"
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <string>

namespace {
void append(dwgBufferW& out, const dwgBufferW& in) {
    for (std::uint64_t bit = 0; bit < in.bitCount(); ++bit)
        out.putBit((in.data()[bit / 8] >> (7u - bit % 8u)) & 1u);
}
}
int main(int argc, char** argv) {
    if (argc != 3 || (std::string(argv[1]) != "source"
                       && std::string(argv[1]) != "generated")) {
        std::cerr << "usage: probe_ac1021_default_material_source source|generated DECOMPRESSED_PAGE\n";
        return 2;
    }
    const bool generated = std::string(argv[1]) == "generated";
    std::ifstream input(argv[2], std::ios::binary);
    if (!input) return 2;
    const std::vector<std::uint8_t> page((std::istreambuf_iterator<char>(input)), {});
    const std::array<std::size_t, 3> offsets = generated
        ? std::array<std::size_t, 3>{{1149u, 1258u, 1367u}}
        : std::array<std::size_t, 3>{{3664u, 3770u, 3876u}};
    const std::array<std::uint32_t, 3> boundaries = generated
        ? std::array<std::uint32_t, 3>{{790u, 790u, 774u}}
        : std::array<std::uint32_t, 3>{{782u, 782u, 766u}};
    const std::array<drwDwgDefaultMaterial::Kind, 3> kinds{{
        drwDwgDefaultMaterial::Kind::ByBlock,
        drwDwgDefaultMaterial::Kind::ByLayer,
        drwDwgDefaultMaterial::Kind::Global}};
    for (std::size_t i = 0; i < kinds.size(); ++i) {
        dwgBufferW body, strings, object;
        if (!drwDwgDefaultMaterial::build(DRW::AC1021, kinds[i], body, strings)) return 1;
        object.putObjType(DRW::AC1021, generated ? 505u : 503u);
        object.putRawLong32(boundaries[i]);
        dwgHandle self; self.ref = static_cast<std::uint32_t>((generated ? 0xb204u : 0x1fu) + i);
        object.putHandle(self);
        object.putBitShort(0u); // EED
        object.putBitLong(1); // one reactor
        object.putBit(1u); // no extension dictionary
        append(object, body);
        append(object, strings);
        object.putRawShort16(static_cast<std::uint16_t>(strings.bitCount()));
        object.putBit(1u); // strings present
        if (object.bitCount() != boundaries[i]) return 1;
        dwgHandle owner; owner.code = DRW::DwgSoftPointer;
        owner.ref = generated ? 0xb203u : 0x0eu;
        object.putHandle(owner); object.putHandle(owner); // owner, reactor
        object.alignToByte();
        if (!object.isGood() || offsets[i] > page.size()
            || object.size() > page.size() - offsets[i]) return 1;
        for (std::size_t j = 0; j < object.size(); ++j)
            if (object.data()[j] != page[offsets[i] + j]) {
                std::cerr << "material " << i << " differs at byte " << j << '\n';
                return 1;
            }
        std::cout << "material " << i << ": body, strings and handles match "
                  << object.bitCount() << " bits (handle boundary " << boundaries[i] << ")\n";
    }
    return 0;
}
