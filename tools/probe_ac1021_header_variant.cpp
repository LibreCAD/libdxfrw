// SPDX-License-Identifier: GPL-2.0-or-later
// Reproduce a bounded R2007 header differential on a locally generated file.
#include "dwgutil.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>

static std::uint64_t get64(const std::uint8_t* p) {
    std::uint64_t v = 0;
    for (unsigned i = 0; i != 8; ++i) v |= std::uint64_t(p[i]) << (8u*i);
    return v;
}
static void put64(std::uint8_t* p, std::uint64_t v) {
    for (unsigned i = 0; i != 8; ++i) p[i] = std::uint8_t(v >> (8u*i));
}
int main(int argc, char** argv) {
    if (argc != 4) return 1;
    const int mask = std::atoi(argv[3]);
    if (mask < 1 || mask > 3) return 1;
    std::ifstream input(argv[1], std::ios::binary);
    std::vector<std::uint8_t> file((std::istreambuf_iterator<char>(input)), {});
    std::vector<std::uint8_t> decoded(717), encoded(765);
    if (file.size() < 2048 || !std::equal(file.begin() + 128, file.begin() + 893,
                                          file.end() - 1024)
        || !dwgRSCodec::decode239I(file.data() + 128, decoded.data(), 3)) return 2;
    // This diagnostic is intentionally restricted to our uncompressed 0x110
    // header and its exact repeated 304-byte block.
    if (decoded[24] != 0xf0 || decoded[25] != 0xfe || decoded[26] != 0xff
        || decoded[27] != 0xff || !std::equal(decoded.begin(), decoded.begin() + 304,
                                               decoded.begin() + 304)) return 2;
    auto* data = decoded.data() + 32;
    const auto expectedHeaderCrc = get64(data + 264);
    const auto expectedPayloadCrc = get64(decoded.data() + 16);
    put64(data + 264, 0);
    const auto actualHeaderCrc = dwgUtil::crc64Normal(dwgUtil::updateSeed2(0, 272), data, 272);
    put64(data + 264, expectedHeaderCrc);
    const auto actualPayloadCrc = dwgUtil::crc64Normal(dwgUtil::updateSeed1(0, 272), data, 272);
    if (expectedHeaderCrc != actualHeaderCrc || expectedPayloadCrc != actualPayloadCrc
        || get64(data) != 0x110 || get64(data+0x88) || get64(data+0x90) || get64(data+0x98)) return 2;
    if (mask & 1) put64(data, 0x70);
    if (mask & 2) { put64(data+0x88, 0xf800); put64(data+0x90, 4); put64(data+0x98, 1); }
    put64(data+264, 0);
    put64(data+264, dwgUtil::crc64Normal(dwgUtil::updateSeed2(0, 272), data, 272));
    put64(decoded.data()+16, dwgUtil::crc64Normal(dwgUtil::updateSeed1(0, 272), data, 272));
    std::copy_n(decoded.begin(), 304, decoded.begin()+304);
    if (!dwgRSCodec::encode239I(decoded.data(), encoded.data(), 3)) return 3;
    std::copy(encoded.begin(), encoded.end(), file.begin()+128);
    std::copy(encoded.begin(), encoded.end(), file.end()-1024);
    std::ofstream output(argv[2], std::ios::binary);
    output.write(reinterpret_cast<const char*>(file.data()), file.size());
    return output ? 0 : 3;
}
