// SPDX-License-Identifier: GPL-2.0-or-later
// Source-specific extractor for the hash-bound AC1021 material witness.
// Uses libdxfrw's own RS decoder and decompressor; not independent evidence.
#include "intern/dwgutil.h"
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 4 || (std::string(argv[1]) != "source"
                       && std::string(argv[1]) != "generated")) return 2;
    const bool generated = std::string(argv[1]) == "generated";
    std::ifstream input(argv[2], std::ios::binary);
    if (!input) return 2;
    std::vector<std::uint8_t> source((std::istreambuf_iterator<char>(input)), {});
    const std::size_t offset = generated ? 0xf80u : 0x1140u;
    const std::size_t blocks = generated ? 6u : 28u;
    if (source.size() < offset + blocks * 255u) return 1;
    std::vector<std::uint8_t> compressed(blocks * 251u),
        page(generated ? 1667u : 63488u);
    if (!dwgRSCodec::decode251I(source.data() + offset, compressed.data(), blocks)) return 1;
    dwgCompressor compressor;
    if (!compressor.decompress21(compressed.data(), page.data(),
                                 generated ? 1298u : 6992u, page.size())) return 1;
    std::ofstream output(argv[3], std::ios::binary);
    output.write(reinterpret_cast<const char*>(page.data()),
                 static_cast<std::streamsize>(page.size()));
    return output ? 0 : 1;
}
