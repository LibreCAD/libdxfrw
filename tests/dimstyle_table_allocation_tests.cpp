// SPDX-License-Identifier: GPL-2.0-or-later
// Generated DIMSTYLE table-record allocation rollback/retry control.
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <new>
#include <sstream>
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
    style.handle = 0xD760u;
    style.name = "Standard";
    style.height = 0.0;
    style.width = 1.0;
    style.oblique = 0.0;
    style.genFlag = 0;
    style.lastHeight = 1.0;
    style.font = "txt";
    return style;
}

DRW_Dimstyle makeDimstyle() {
    DRW_Dimstyle style;
    style.handle = 0xD761u;
    style.name = "Generated allocation retry DIMSTYLE";
    style.dimtxsty = "Standard";
    style.dimpost = "<> generated";
    style.dimscale = 2.0;
    style.dimasz = 2.75;
    style.dimtxt = 3.5;
    style.dimdec = 3;
    style.dimtad = 1;
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

bool sameDimstyle(const DRW_Dimstyle& a, const DRW_Dimstyle& b,
                  bool readback) {
    return a.tType == DRW::DIMSTYLE
        && (readback ? a.handle != 0u : a.handle == b.handle)
        && a.name == b.name && (readback || a.dimtxsty == b.dimtxsty)
        && a.dimpost == b.dimpost && a.dimscale == b.dimscale
        && a.dimasz == b.dimasz && a.dimtxt == b.dimtxt
        && a.dimdec == b.dimdec && a.dimtad == b.dimtad
        && a.dimunit == b.dimunit && a.dimfit == b.dimfit
        && a.dimfxl == b.dimfxl && a.dimaltrnd == b.dimaltrnd
        && a.dimfxlon == b.dimfxlon;
}

std::string handleHex(std::uint32_t handle) {
    std::ostringstream stream;
    stream << std::uppercase << std::hex << handle;
    return stream.str();
}

class Export final : public LC3CompatCheck {
public:
    explicit Export(dxfRW& codec)
        : codec(codec), textstyle(makeTextstyle()), dimstyle(makeDimstyle()) {}
    void writeHeader(DRW_Header&) override {}
    void writeTextstyles() override {
        styleAccepted = codec.writeTextstyle(&textstyle);
    }
    void writeDimstyles() override {
        if (failAt < 0) {
            accepted = codec.writeDimstyle(&dimstyle);
            return;
        }
        armAllocationFailure(failAt);
        try {
            accepted = codec.writeDimstyle(&dimstyle);
        } catch (const std::bad_alloc&) {
            accepted = false;
        }
        reached = static_cast<long>(allocationAttempts);
        disarmAllocationFailure();
        if (!accepted)
            (void)codec.writeRawDxfObject(nullptr);
    }
    bool finalizeDxfWrite() override { return styleAccepted && accepted; }

    dxfRW& codec;
    DRW_Textstyle textstyle;
    DRW_Dimstyle dimstyle;
    long failAt = -1;
    long reached = 0;
    bool styleAccepted = false;
    bool accepted = false;
};

class Reader final : public LC3CompatCheck {
public:
    void addDimStyle(const DRW_Dimstyle& value) override {
        if (value.name == "Generated allocation retry DIMSTYLE")
            dimstyles.push_back(value);
    }
    void addTextStyle(const DRW_Textstyle& value) override {
        if (value.name == "Standard")
            textstyles.push_back(value);
    }
    std::vector<DRW_Dimstyle> dimstyles;
    std::vector<DRW_Textstyle> textstyles;
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

bool asciiDimstyleHasGroup(const std::string& bytes, int wanted) {
    std::istringstream input(bytes);
    std::string code;
    std::string value;
    bool inDimstyle = false;
    while (std::getline(input, code) && std::getline(input, value)) {
        if (code == "0") {
            if (inDimstyle)
                return false;
            inDimstyle = value == "DIMSTYLE";
            continue;
        }
        if (inDimstyle && code == std::to_string(wanted))
            return true;
    }
    return false;
}

bool readbackMatches(const std::filesystem::path& path) {
    dxfRW codec(path.string().c_str());
    Reader reader;
    const DRW_Dimstyle expectedDimstyle = makeDimstyle();
    const DRW_Textstyle expectedTextstyle = makeTextstyle();
    const bool read = codec.read(&reader, false);
    const bool matches = read && reader.dimstyles.size() == 1u
        && reader.textstyles.size() == 1u
        && sameDimstyle(reader.dimstyles.front(), expectedDimstyle, true)
        && sameTextstyle(reader.textstyles.front(), expectedTextstyle, true);
    return matches && reader.textstyles.front().handle != 0u
        && reader.dimstyles.front().dimtxsty
            == handleHex(reader.textstyles.front().handle);
}

bool allocationSweep(const std::filesystem::path& directory) {
    const auto path = directory / "dimstyle-table-allocation.dxf";
    const std::string prior = "prior DIMSTYLE destination bytes\n";
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
                const DRW_Textstyle expectedTextstyle = makeTextstyle();
                const DRW_Dimstyle expectedDimstyle = makeDimstyle();
                if (!written || !exporter.styleAccepted
                    || !exporter.accepted
                    || !sameTextstyle(exporter.textstyle,
                                      expectedTextstyle, false)
                    || !sameDimstyle(exporter.dimstyle,
                                     expectedDimstyle, false)
                    || !readbackMatches(path)
                    || !readBytes(path, expectedBytes)) {
                    std::cerr << "clean DIMSTYLE write failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " written=" << written
                              << " style=" << exporter.styleAccepted
                              << " dimstyle=" << exporter.accepted
                              << " error=" << codec.getError() << '\n';
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
                const DRW_Textstyle expectedTextstyle = makeTextstyle();
                const DRW_Dimstyle expectedDimstyle = makeDimstyle();
                if (written || exporter.accepted
                    || exporter.reached != offset + 1
                    || !allocationFailed || !readBytes(path, current)
                    || current != prior
                    || !sameTextstyle(exporter.textstyle,
                                      expectedTextstyle, false)
                    || !sameDimstyle(exporter.dimstyle,
                                     expectedDimstyle, false)) {
                    std::cerr << "DIMSTYLE rollback failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " offset=" << offset
                              << " reached=" << exporter.reached
                              << " failed=" << allocationFailed << '\n';
                    return false;
                }
                const bool retried = codec.write(&exporter, version, binary);
                std::string retryBytes;
                if (!retried || !exporter.styleAccepted
                    || !exporter.accepted
                    || !sameTextstyle(exporter.textstyle,
                                      expectedTextstyle, false)
                    || !sameDimstyle(exporter.dimstyle,
                                     expectedDimstyle, false)
                    || !readBytes(path, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(path)) {
                    std::cerr << "DIMSTYLE retry failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " offset=" << offset << '\n';
                    return false;
                }
            }
            std::cout << "DIMSTYLE " << versionCode(version)
                      << (binary ? " Binary" : " ASCII")
                      << " table-writer allocation offsets="
                      << allocationCount << '\n';
        }
    }
    return true;
}

bool dimfxlRefusalRetry(const std::filesystem::path& directory) {
    const auto path = directory / "dimstyle-dimfxl.dxf";
    const auto cleanPath = directory / "dimstyle-dimfxl-clean.dxf";
    const std::string prior = "prior DIMSTYLE destination bytes\n";
    constexpr std::array<DRW::Version, 2> versions{{
        DRW::AC1014, DRW::AC1018}};
    for (const DRW::Version version : versions) {
        for (const bool binary : {false, true}) {
            dxfRW cleanCodec(cleanPath.string().c_str());
            Export cleanExporter(cleanCodec);
            if (!cleanCodec.write(&cleanExporter, version, binary)
                || !cleanExporter.accepted)
                return false;
            std::string expectedBytes;
            if (!readBytes(cleanPath, expectedBytes)
                || (!binary && asciiDimstyleHasGroup(expectedBytes, 49))) {
                std::cerr << "legacy DIMSTYLE emitted group 49: version="
                          << versionCode(version) << '\n';
                return false;
            }

            for (const bool useOverride : {false, true}) {
                {
                    std::ofstream output(path,
                                         std::ios::binary | std::ios::trunc);
                    output << prior;
                    if (!output)
                        return false;
                }
                dxfRW codec(path.string().c_str());
                Export exporter(codec);
                if (useOverride)
                    exporter.dimstyle.add("$DIMFXL", 49, 2.0);
                else
                    exporter.dimstyle.dimfxl = 2.0;
                if (codec.write(&exporter, version, binary)
                    || exporter.accepted) {
                    std::cerr << "legacy DIMFXL refusal failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " override=" << useOverride << '\n';
                    return false;
                }
                std::string current;
                if (!readBytes(path, current) || current != prior)
                    return false;

                if (useOverride)
                    exporter.dimstyle.add("$DIMFXL", 49, 1.0);
                else
                    exporter.dimstyle.dimfxl = 1.0;
                if (!codec.write(&exporter, version, binary)
                    || !exporter.accepted || !readBytes(path, current)
                    || current != expectedBytes || !readbackMatches(path)) {
                    std::cerr << "legacy DIMFXL retry failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " override=" << useOverride << '\n';
                    return false;
                }
            }
        }
    }
    return true;
}

bool dimaltrndRefusalRetry(const std::filesystem::path& directory) {
    const auto path = directory / "dimstyle-dimaltrnd.dxf";
    const auto cleanPath = directory / "dimstyle-dimaltrnd-clean.dxf";
    const std::string prior = "prior DIMSTYLE destination bytes\n";
    const DRW::Version version = DRW::AC1014;
    for (const bool binary : {false, true}) {
        dxfRW cleanCodec(cleanPath.string().c_str());
        Export cleanExporter(cleanCodec);
        if (!cleanCodec.write(&cleanExporter, version, binary)
            || !cleanExporter.accepted)
            return false;
        std::string expectedBytes;
        if (!readBytes(cleanPath, expectedBytes)
            || (!binary && asciiDimstyleHasGroup(expectedBytes, 148))) {
            std::cerr << "R14 DIMSTYLE emitted group 148\n";
            return false;
        }

        for (const bool useOverride : {false, true}) {
            {
                std::ofstream output(path,
                                     std::ios::binary | std::ios::trunc);
                output << prior;
                if (!output)
                    return false;
            }
            dxfRW codec(path.string().c_str());
            Export exporter(codec);
            if (useOverride)
                exporter.dimstyle.add("$DIMALTRND", 148, 0.5);
            else
                exporter.dimstyle.dimaltrnd = 0.5;
            if (codec.write(&exporter, version, binary)
                || exporter.accepted) {
                std::cerr << "R14 DIMALTRND refusal failed: binary="
                          << binary << " override=" << useOverride << '\n';
                return false;
            }
            std::string current;
            if (!readBytes(path, current) || current != prior)
                return false;

            if (useOverride)
                exporter.dimstyle.add("$DIMALTRND", 148, 0.0);
            else
                exporter.dimstyle.dimaltrnd = 0.0;
            if (!codec.write(&exporter, version, binary)
                || !exporter.accepted || !readBytes(path, current)
                || current != expectedBytes || !readbackMatches(path)) {
                std::cerr << "R14 DIMALTRND retry failed: binary="
                          << binary << " override=" << useOverride << '\n';
                return false;
            }
        }
    }
    return true;
}

bool dimfxlonRefusalRetry(const std::filesystem::path& directory) {
    const auto path = directory / "dimstyle-dimfxlon.dxf";
    const auto cleanPath = directory / "dimstyle-dimfxlon-clean.dxf";
    const std::string prior = "prior DIMSTYLE destination bytes\n";
    const DRW::Version version = DRW::AC1018;
    for (const bool binary : {false, true}) {
        dxfRW cleanCodec(cleanPath.string().c_str());
        Export cleanExporter(cleanCodec);
        if (!cleanCodec.write(&cleanExporter, version, binary)
            || !cleanExporter.accepted)
            return false;
        std::string expectedBytes;
        if (!readBytes(cleanPath, expectedBytes)
            || (!binary && asciiDimstyleHasGroup(expectedBytes, 290))) {
            std::cerr << "pre-2007 DIMSTYLE emitted group 290\n";
            return false;
        }

        for (const bool useOverride : {false, true}) {
            {
                std::ofstream output(path,
                                     std::ios::binary | std::ios::trunc);
                output << prior;
                if (!output)
                    return false;
            }
            dxfRW codec(path.string().c_str());
            Export exporter(codec);
            if (useOverride)
                exporter.dimstyle.add("$DIMFXLON", 290, 1);
            else
                exporter.dimstyle.dimfxlon = 1;
            if (codec.write(&exporter, version, binary)
                || exporter.accepted) {
                std::cerr << "pre-2007 DIMFXLON refusal failed: binary="
                          << binary << " override=" << useOverride << '\n';
                return false;
            }
            std::string current;
            if (!readBytes(path, current) || current != prior)
                return false;

            if (useOverride)
                exporter.dimstyle.add("$DIMFXLON", 290, 0);
            else
                exporter.dimstyle.dimfxlon = 0;
            if (!codec.write(&exporter, version, binary)
                || !exporter.accepted || !readBytes(path, current)
                || current != expectedBytes || !readbackMatches(path)) {
                std::cerr << "pre-2007 DIMFXLON retry failed: binary="
                          << binary << " override=" << useOverride << '\n';
                return false;
            }
        }
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
        / ("dimstyle-table-allocation-"
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
    return allocationSweep(directory)
        && dimfxlRefusalRetry(directory)
        && dimaltrndRefusalRetry(directory)
        && dimfxlonRefusalRetry(directory) ? 0 : 2;
}
