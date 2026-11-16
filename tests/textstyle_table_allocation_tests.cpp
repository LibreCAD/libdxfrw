// SPDX-License-Identifier: GPL-2.0-or-later
// Generated TEXTSTYLE table-record allocation rollback/retry control.
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>
#include <vector>

namespace {
long allocationsBeforeFailure = -1;
std::size_t allocationAttempts = 0u;
bool allocationFailed = false;

void armAllocationFailure(long offset) noexcept {
    allocationsBeforeFailure = offset;
    allocationAttempts = 0u;
    allocationFailed = false;
}
void disarmAllocationFailure() noexcept { allocationsBeforeFailure = -1; }

DRW_Textstyle makeTextstyle() {
    DRW_Textstyle style;
    style.handle = 0xD740u;
    style.name = "Generated allocation retry TEXTSTYLE";
    style.height = 2.5;
    style.width = 0.85;
    style.oblique = 12.0;
    style.genFlag = 2;
    style.lastHeight = 3.25;
    style.font = "generated-font.ttf";
    style.bigFont = "generated-bigfont.shx";
    return style;
}

bool sameTextstyle(const DRW_Textstyle& a, const DRW_Textstyle& b,
                   bool readback) {
    return a.tType == DRW::STYLE
        && (readback ? a.handle != 0u : a.handle == b.handle)
        && a.name == b.name && a.height == b.height && a.width == b.width
        && a.oblique == b.oblique && a.genFlag == b.genFlag
        && a.lastHeight == b.lastHeight && a.font == b.font
        && a.bigFont == b.bigFont && a.fontFamily == b.fontFamily;
}

class Export final : public LC3CompatCheck {
public:
    explicit Export(dxfRW& codec) : codec(codec), style(makeTextstyle()) {}
    void writeHeader(DRW_Header&) override {}
    void writeTextstyles() override {
        if (failAt < 0) {
            accepted = codec.writeTextstyle(&style);
            return;
        }
        armAllocationFailure(failAt);
        try {
            accepted = codec.writeTextstyle(&style);
        } catch (const std::bad_alloc&) {
            accepted = false;
        }
        reached = static_cast<long>(allocationAttempts);
        disarmAllocationFailure();
        if (!accepted)
            (void)codec.writeRawDxfObject(nullptr);
    }
    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    DRW_Textstyle style;
    long failAt = -1;
    long reached = 0;
    bool accepted = false;
};

class Reader final : public LC3CompatCheck {
public:
    void addTextStyle(const DRW_Textstyle& value) override {
        if (value.name == "Generated allocation retry TEXTSTYLE")
            styles.push_back(value);
    }
    std::vector<DRW_Textstyle> styles;
};

const char* versionCode(DRW::Version version) {
    switch (version) {
    case DRW::AC1014: return "AC1014";
    case DRW::AC1015: return "AC1015";
    case DRW::AC1018: return "AC1018";
    case DRW::AC1021: return "AC1021";
    case DRW::AC1024: return "AC1024";
    case DRW::AC1027: return "AC1027";
    case DRW::AC1032: return "AC1032";
    default: return "unknown";
    }
}

bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

bool readbackMatches(const std::filesystem::path& path) {
    dxfRW codec(path.string().c_str());
    Reader reader;
    const DRW_Textstyle expected = makeTextstyle();
    return codec.read(&reader, false) && reader.styles.size() == 1u
        && sameTextstyle(reader.styles.front(), expected, true);
}

bool allocationSweep(const std::filesystem::path& directory) {
    const auto path = directory / "textstyle-table-allocation.dxf";
    const std::string prior = "prior TEXTSTYLE destination bytes\n";
    constexpr std::array<DRW::Version, 7> versions{{
        DRW::AC1014, DRW::AC1015, DRW::AC1018, DRW::AC1021,
        DRW::AC1024, DRW::AC1027, DRW::AC1032}};
    for (const DRW::Version version : versions) {
        for (const bool binary : {false, true}) {
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(path.string().c_str());
                Export exporter(codec);
                exporter.failAt = 1000000;
                const bool written = codec.write(&exporter, version, binary);
                const DRW_Textstyle expected = makeTextstyle();
                if (!written || !exporter.accepted
                    || !sameTextstyle(exporter.style, expected, false)
                    || !readbackMatches(path)
                    || !readBytes(path, expectedBytes)) {
                    std::cerr << "clean TEXTSTYLE write failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " written=" << written
                              << " accepted=" << exporter.accepted << '\n';
                    return false;
                }
                allocationCount = exporter.reached;
            }
            if (allocationCount <= 0)
                return false;
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream output(path,
                                         std::ios::binary | std::ios::trunc);
                    output << prior;
                    if (!output)
                        return false;
                }
                dxfRW codec(path.string().c_str());
                Export exporter(codec);
                exporter.failAt = offset;
                const bool written = codec.write(&exporter, version, binary);
                exporter.failAt = -1;
                std::string current;
                const DRW_Textstyle expected = makeTextstyle();
                if (written || exporter.accepted
                    || exporter.reached != offset + 1
                    || !allocationFailed || !readBytes(path, current)
                    || current != prior
                    || !sameTextstyle(exporter.style, expected, false)) {
                    std::cerr << "TEXTSTYLE rollback failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " offset=" << offset
                              << " reached=" << exporter.reached
                              << " failed=" << allocationFailed << '\n';
                    return false;
                }
                const bool retried = codec.write(&exporter, version, binary);
                std::string retryBytes;
                if (!retried || !exporter.accepted
                    || !sameTextstyle(exporter.style, expected, false)
                    || !readBytes(path, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(path)) {
                    std::cerr << "TEXTSTYLE retry failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " offset=" << offset << '\n';
                    return false;
                }
            }
            std::cout << "TEXTSTYLE " << versionCode(version)
                      << (binary ? " Binary" : " ASCII")
                      << " table-writer allocation offsets="
                      << allocationCount << '\n';
        }
    }
    return true;
}

class FontReader final : public LC3CompatCheck {
public:
    void addTextStyle(const DRW_Textstyle& value) override {
        if (value.name == "STYLEFONT") styles.push_back(value);
    }
    std::vector<DRW_Textstyle> styles;
};

bool legacyFontProjectionGuard(const std::filesystem::path& directory) {
    const auto path = directory / "style-font-legacy.dxf";
    const std::string prior = "previous destination bytes\n";
    for (const bool binary : {false, true}) {
        dxfRW codec(path.string().c_str());
        Export exporter(codec);
        exporter.style.name = "STYLEFONT";
        exporter.style.fontFamily = 50331682;
        const DRW_Textstyle original(exporter.style);
        {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output << prior;
            if (!output) return false;
        }
        const bool wrote = codec.write(&exporter, DRW::AC1009, binary);
        std::string current;
        if (wrote || exporter.accepted
            || !sameTextstyle(exporter.style, original, false)
            || !readBytes(path, current) || current != prior) {
            std::cerr << "STYLE font flags lost on R12 export: binary=" << binary << '\n';
            return false;
        }
        if (!codec.write(&exporter, DRW::AC1015, binary) || !exporter.accepted
            || !sameTextstyle(exporter.style, original, false))
            return false;
        FontReader modern; dxfRW modernReader(path.string().c_str());
        if (!modernReader.read(&modern, false) || modern.styles.size() != 1u
            || modern.styles.front().fontFamily != original.fontFamily)
            return false;
        exporter.style.fontFamily = 0;
        if (!codec.write(&exporter, DRW::AC1009, binary) || !exporter.accepted)
            return false;
        FontReader plain; dxfRW plainReader(path.string().c_str());
        if (!plainReader.read(&plain, false) || plain.styles.size() != 1u
            || plain.styles.front().fontFamily != 0)
            return false;
        exporter.style.fontFamily = original.fontFamily;
        exporter.style.extData.push_back(new DRW_Variant(1001, std::string("UNRELATED")));
        exporter.style.extData.push_back(new DRW_Variant(1071, 42));
        {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output << prior;
            if (!output) return false;
        }
        if (codec.write(&exporter, DRW::AC1009, binary) || exporter.accepted
            || exporter.style.fontFamily != original.fontFamily
            || exporter.style.extData.size() != 2u
            || exporter.style.extData[0]->code() != 1001
            || *exporter.style.extData[0]->content.s != "UNRELATED"
            || exporter.style.extData[1]->i_val() != 42
            || !readBytes(path, current) || current != prior)
            return false;
        exporter.style = original;
        exporter.style.extData.push_back(new DRW_Variant(1001, std::string("ACAD")));
        exporter.style.extData.push_back(new DRW_Variant(1000, std::string("SampleFont")));
        exporter.style.extData.push_back(new DRW_Variant(1071, 42));
        if (!codec.write(&exporter, DRW::AC1009, binary) || !exporter.accepted
            || exporter.style.extData.size() != 3u
            || exporter.style.extData[2]->i_val() != 42)
            return false;
        FontReader metadata; dxfRW metadataReader(path.string().c_str());
        if (!metadataReader.read(&metadata, false) || metadata.styles.size() != 1u
            || metadata.styles.front().fontFamily != original.fontFamily)
            return false;
    }
    return true;
}

} // namespace

void* operator new(std::size_t size) {
    if (allocationsBeforeFailure == 0) {
        ++allocationAttempts;
        allocationsBeforeFailure = -1;
        allocationFailed = true;
        throw std::bad_alloc();
    }
    if (allocationsBeforeFailure > 0) {
        --allocationsBeforeFailure;
        ++allocationAttempts;
    }
    if (void* value = std::malloc(size == 0u ? 1u : size))
        return value;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }
void operator delete(void* value, std::size_t) noexcept { std::free(value); }
void operator delete[](void* value, std::size_t) noexcept { std::free(value); }

int main() {
    const auto directory = std::filesystem::temp_directory_path()
        / ("textstyle-table-allocation-"
           + std::to_string(static_cast<long long>(
                 std::filesystem::file_time_type::clock::now()
                     .time_since_epoch().count())));
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return 1;
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
    } cleanup{directory};
    return legacyFontProjectionGuard(directory) && allocationSweep(directory) ? 0 : 2;
}
