// Extract one known R2004-family compressed HEADER data page for independent
// envelope checks. Offsets, sizes and input hashes are supplied by the caller.
#include "intern/dwgutil.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
int main(int argc,char** argv) {
    if (argc != 5 || std::filesystem::exists(argv[4])) return 2;
    try {
        std::ifstream input(argv[1],std::ios::binary);
        if (!input) return 2;
        const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),{});
        const auto start=std::stoull(argv[2]), length=std::stoull(argv[3]);
        if (start>bytes.size() || length>bytes.size()-start) return 2;
        std::vector<std::uint8_t> decoded(0x7400);
        dwgCompressor compressor;
        if (!compressor.decompress18(bytes.data()+start,decoded.data(),length,decoded.size())
            || compressor.decompressedBytes()!=decoded.size()) return 2;
        std::ofstream output(argv[4],std::ios::binary|std::ios::out);
        if (!output) return 2;
        output.write(reinterpret_cast<const char*>(decoded.data()),decoded.size());
        return output.good()?0:2;
    } catch (...) { return 2; }
}
