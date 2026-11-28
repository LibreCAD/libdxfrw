#include "lc3_compat_check.cpp"
#include "libdwgr.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#ifdef DWG_LAYOUT_VALIDATION_TESTS
#include "intern/dwgwriterlayoutvalidation.h"
#endif

namespace {
#ifdef DWG_LAYOUT_VALIDATION_TESTS
std::uint32_t crc32(const std::uint8_t* p, std::size_t n);
int mutation = 0;
void corrupt(dwgWriterLayoutValidation::Snapshot& snapshot) {
    auto* state = std::get_if<dwgWriterLayoutValidation::R2004Snapshot>(&snapshot);
    if (!state) return;
    if (mutation == 1) state->assembled.back() ^= 1;
    if (mutation == 3) state->assembled.pop_back();
    if (mutation == 2 || mutation == 4) {
        std::array<std::uint8_t, 0x6c> raw{}, mask{};
        std::uint32_t seed = 1;
        for (std::size_t i = 0; i < raw.size(); ++i) {
            seed = seed * 0x343fdu + 0x269ec3u;
            mask[i] = static_cast<std::uint8_t>(seed >> 16);
            raw[i] = state->fileHeader[0x80 + i] ^ mask[i];
        }
        if (mutation == 2) {
            raw[0x34] ^= 1;
            raw[0x68] = raw[0x69] = raw[0x6a] = raw[0x6b] = 0;
            const auto crc = crc32(raw.data(), raw.size());
            for (unsigned i = 0; i < 4; ++i)
                raw[0x68 + i] = static_cast<std::uint8_t>(crc >> (8 * i));
        } else {
            raw[0x68] ^= 1;
        }
        for (std::size_t i = 0; i < raw.size(); ++i) {
            const auto encrypted = raw[i] ^ mask[i];
            state->fileHeader[0x80 + i] = encrypted;
            state->assembled[0x80 + i] = encrypted;
            state->assembled[state->receipt.declaredFileSize + i] = encrypted;
        }
    }
}
#endif
std::uint32_t get32(const std::uint8_t* p) {
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8)
        | (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}
std::uint64_t get64(const std::uint8_t* p) {
    return std::uint64_t(get32(p)) | (std::uint64_t(get32(p + 4)) << 32);
}
std::uint32_t crc32(const std::uint8_t* p, std::size_t n) {
    std::uint32_t crc = 0xffffffffu;
    for (std::size_t i = 0; i < n; ++i) {
        crc ^= p[i];
        for (unsigned j = 0; j < 8; ++j)
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}
std::vector<std::uint8_t> read(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
class Writer final : public LC3CompatCheck {
public:
    explicit Writer(dwgRW& output) : out(output) {}
    void writeEntities() override {
        if (fail) {
            DRW_Polyline p;
            p.addVertex(DRW_Vertex(1, 2, 0, 0));
            p.addVertex(DRW_Vertex(3, 4, 0, 0));
            p.vertlist[0]->handle = p.vertlist[1]->handle = 0x90;
            ok = out.writePolyline(&p);
        } else {
            DRW_Line line;
            line.basePoint = {1, 2, 3}; line.secPoint = {4, 5, 6};
            ok = out.writeLine(&line);
        }
    }
    bool finalizeDwgWrite() override { return ok; }
    dwgRW& out;
    bool fail = true, ok = false;
};
bool verify(const std::vector<std::uint8_t>& bytes) {
    if (bytes.size() < 0x100) return false;
    std::array<std::uint8_t, 0x6c> header{};
    std::uint32_t seed = 1;
    for (std::size_t i = 0; i < header.size(); ++i) {
        seed = seed * 0x343fdu + 0x269ec3u;
        header[i] = bytes[0x80 + i] ^ static_cast<std::uint8_t>(seed >> 16);
    }
    const auto stored = get32(header.data() + 0x68);
    header[0x68] = header[0x69] = header[0x6a] = header[0x6b] = 0;
    if (crc32(header.data(), header.size()) != stored) return false;
    const auto address = get64(header.data() + 0x34);
    if (address != bytes.size() - header.size()) return false;
    for (std::size_t i = 0; i < header.size(); ++i)
        if (bytes[address + i] != bytes[0x80 + i]) return false;
    return true;
}
}

int main(int argc, char** argv) {
    const bool keep = argc == 3 && std::string(argv[1]) == "--keep";
    if (argc != 1 && !keep) return 2;
    auto root = keep ? std::filesystem::path(argv[2]) : std::filesystem::temp_directory_path() /
        ("modern-repeated-header-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    struct Cleanup { std::filesystem::path p; bool keep; ~Cleanup() {
        if (!keep) { std::error_code e; std::filesystem::remove_all(p,e); }
    }} cleanup{root,keep};
    for (auto version : {DRW::AC1018, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        const auto path = root / (std::to_string(static_cast<int>(version)) + ".dwg");
        const std::string sentinel = "prior destination";
        { std::ofstream f(path, std::ios::binary); f << sentinel; }
        dwgRW output(path.string().c_str()); Writer writer(output);
        if (output.write(&writer, version, true) || read(path) != std::vector<std::uint8_t>(sentinel.begin(),sentinel.end())) {
            std::cerr << "failed write published output\n"; return 1;
        }
        writer.fail = false;
#ifdef DWG_LAYOUT_VALIDATION_TESTS
        {
            for (mutation = 1; mutation <= 4; ++mutation) {
                dwgWriterLayoutValidation::setTestSnapshotObserver(corrupt);
                const bool written = output.write(&writer, version, true);
                dwgWriterLayoutValidation::setTestSnapshotObserver(nullptr);
                if (written || read(path) != std::vector<std::uint8_t>(sentinel.begin(),sentinel.end())) {
                    std::cerr << "corrupt layout published output\n"; return 1;
                }
            }
        }
#endif
        if (!output.write(&writer, version, true) || !writer.ok) {
            std::cerr << "retry/header verification failed for " << static_cast<int>(version) << '\n'; return 1;
        }
        const auto bytes = read(path);
        if (!verify(bytes)) return 1;
        {
            auto corrupted = bytes;
            corrupted.back() ^= 1;
            if (verify(corrupted)) return 1;
            corrupted = bytes;
            corrupted[0x80 + 0x34] ^= 1;
            if (verify(corrupted)) return 1;
            corrupted = bytes;
            corrupted.pop_back();
            if (verify(corrupted)) return 1;
        }
    }
}
