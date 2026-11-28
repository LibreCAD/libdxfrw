// Read-only research aid: extract a bounded AC1032 OBJECTS section to scratch.
// This uses libdxfrw's production section decoder; it is not an independent
// decompression oracle and makes no source-integrity or support decision.
#include "intern/dwgreader32.h"
#include "libdwgr.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

class ObjectSectionProbe final : public dwgReader32 {
public:
    using dwgReader32::dwgReader32;
    bool extract(const char* output) {
        if (!readMetaData() || !readFileHeader()) return false;
        const auto it = sections.find(secEnum::OBJECTS);
        if (it == sections.end() || it->second.size == 0u
            || it->second.size > 20u * 1024u * 1024u) return false;
        std::unique_ptr<std::uint8_t[]> bytes;
        std::uint64_t size = 0;
        if (!parseDataPage(it->second, bytes, size) || !bytes
            || size < it->second.size) return false;
        if (std::filesystem::exists(output)) return false;
        std::ofstream result(output, std::ios::binary | std::ios::out);
        result.write(reinterpret_cast<const char*>(bytes.get()),
                     static_cast<std::streamsize>(it->second.size));
        return result.good();
    }
};

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: dwg_ac1032_object_section_probe input.dwg output.bin\n";
        return 2;
    }
    std::ifstream source(argv[1], std::ios::binary | std::ios::ate);
    if (!source) return 1;
    const auto size = source.tellg();
    if (size <= 0 || size > 20 * 1024 * 1024) return 1;
    source.seekg(0);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    if (!source.read(reinterpret_cast<char*>(bytes.data()), size)) return 1;
    dwgRW owner(argv[1]);
    (void)owner.getPreview();
    if (owner.getVersion() != DRW::AC1032) return 1;
    ObjectSectionProbe probe(std::unique_ptr<dwgBuffer>(
        new dwgBuffer(bytes.data(), bytes.size())), &owner);
    return probe.extract(argv[2]) ? 0 : 1;
}
