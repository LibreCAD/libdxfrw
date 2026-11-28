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
#include <limits>
#include <new>
#include <sstream>
#include <string>
#include <string_view>
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
        && a.dimfxlon == b.dimfxlon
        && a.dimtxtdirection == b.dimtxtdirection
        && a.dimtfill == b.dimtfill && a.dimarcsym == b.dimarcsym
        && a.dimjogang == b.dimjogang;
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

bool addDimstyleFillColor(std::string& bytes, int color) {
    std::istringstream input(bytes);
    std::ostringstream output;
    std::string code;
    std::string value;
    bool inDimstyle = false;
    bool duplicated = false;
    while (std::getline(input, code) && std::getline(input, value)) {
        if (!code.empty() && code.back() == '\r')
            code.pop_back();
        if (!value.empty() && value.back() == '\r')
            value.pop_back();
        const auto first = code.find_first_not_of(" \t");
        const std::string codeValue = first == std::string::npos
            ? std::string{} : code.substr(first);
        if (codeValue == "0")
            inDimstyle = value == "DIMSTYLE";
        output << code << '\n' << value << '\n';
        if (inDimstyle && !duplicated && codeValue == "70") {
            output << "70\n" << color << '\n';
            duplicated = true;
        }
    }
    if (!input.eof() || !duplicated)
        return false;
    bytes = output.str();
    return true;
}

bool asciiDimstyleHasGroup(const std::string& bytes, int wanted) {
    std::istringstream input(bytes);
    std::string code;
    std::string value;
    bool inDimstyle = false;
    while (std::getline(input, code) && std::getline(input, value)) {
        const auto first = code.find_first_not_of(" \t\r");
        const std::string_view codeValue = first == std::string::npos
            ? std::string_view{} : std::string_view(code).substr(first);
        if (codeValue == "0") {
            if (inDimstyle)
                return false;
            inDimstyle = value == "DIMSTYLE";
            continue;
        }
        if (inDimstyle) {
            if (codeValue == std::to_string(wanted))
                return true;
        }
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

bool dimstyleTextFillColorTypedOutput(const std::filesystem::path& directory) {
    const auto path = directory / "dimstyle-text-fill-color.dxf";
    const auto malformedPath = directory / "dimstyle-text-fill-color-third-70.dxf";
    for (const bool binary : {false, true}) {
        dxfRW codec(path.string().c_str());
        Export exporter(codec);
        exporter.dimstyle.flags = 64;
        exporter.dimstyle.dimtfill = 2;
        exporter.dimstyle.dimtfillclr = 42;
        if (!codec.write(&exporter, DRW::AC1021, binary)
            || !exporter.accepted)
            return false;

        dxfRW validCodec(path.string().c_str());
        Reader validReader;
        if (!validCodec.read(&validReader, false)
            || validReader.dimstyles.size() != 1u
            || validReader.dimstyles.front().flags != 64
            || validReader.dimstyles.front().dimtfill != 2
            || validReader.dimstyles.front().dimtfillclr != 42) {
            std::cerr << "DIMSTYLE second group-70 color readback failed: binary="
                      << binary << '\n';
            return false;
        }

        std::string bytes;
        if (!binary) {
            if (!readBytes(path, bytes) || !addDimstyleFillColor(bytes, 7))
                return false;
            std::ofstream output(malformedPath,
                                 std::ios::binary | std::ios::trunc);
            output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
            if (!output)
                return false;
            output.close();
            // This now has three group-70 values: entry flags, DIMTFILLCLR,
            // and an unassigned third occurrence.
            dxfRW malformedCodec(malformedPath.string().c_str());
            Reader malformedReader;
            if (malformedCodec.read(&malformedReader, false)
                || malformedCodec.getError() != DRW::BAD_CODE_PARSED) {
                std::cerr << "third DIMSTYLE group-70 occurrence was accepted\n";
                return false;
            }
        }
    }

    // The dynamic compatibility view must take precedence over the typed
    // value while still emitting exactly one second group-70 occurrence.
    {
        dxfRW codec(path.string().c_str());
        Export exporter(codec);
        exporter.dimstyle.flags = 64;
        exporter.dimstyle.dimtfill = 2;
        exporter.dimstyle.dimtfillclr = 42;
        exporter.dimstyle.add("$DIMTFILLCLR", 70, 13);
        if (!codec.write(&exporter, DRW::AC1021, false)
            || !exporter.accepted)
            return false;
        dxfRW readerCodec(path.string().c_str());
        Reader reader;
        if (!readerCodec.read(&reader, false) || reader.dimstyles.size() != 1u
            || reader.dimstyles.front().flags != 64
            || reader.dimstyles.front().dimtfillclr != 13)
            return false;
    }
    return true;
}

bool dimstyleTextFillColorAllocationSweep(
    const std::filesystem::path& directory) {
    const auto path = directory / "dimstyle-text-fill-color-allocation.dxf";
    const std::string prior = "prior DIMSTYLE fill-color bytes\n";
    for (const bool binary : {false, true}) {
        for (const bool dynamicOverride : {false, true}) {
            const int expectedColor = dynamicOverride ? 13 : 42;
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(path.string().c_str());
                Export exporter(codec);
                exporter.dimstyle.flags = 64;
                exporter.dimstyle.dimtfill = 2;
                exporter.dimstyle.dimtfillclr = 42;
                if (dynamicOverride)
                    exporter.dimstyle.add("$DIMTFILLCLR", 70, 13);
                exporter.failAt = 1000000;
                if (!codec.write(&exporter, DRW::AC1021, binary)
                    || !exporter.styleAccepted || !exporter.accepted
                    || exporter.dimstyle.dimtfillclr != 42
                    || (dynamicOverride
                        && (exporter.dimstyle.get("$DIMTFILLCLR") == nullptr
                            || exporter.dimstyle.get("$DIMTFILLCLR")->code() != 70
                            || exporter.dimstyle.get("$DIMTFILLCLR")->i_val()
                                != 13))
                    || !readBytes(path, expectedBytes)) {
                    std::cerr << "DIMTFILLCLR allocation baseline failed: binary="
                              << binary << '\n';
                    return false;
                }
                allocationCount = exporter.reached;
                dxfRW readerCodec(path.string().c_str());
                Reader reader;
                if (!readerCodec.read(&reader, false)
                    || reader.dimstyles.size() != 1u
                    || reader.dimstyles.front().flags != 64
                    || reader.dimstyles.front().dimtfill != 2
                    || reader.dimstyles.front().dimtfillclr != expectedColor)
                    return false;
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
                exporter.dimstyle.flags = 64;
                exporter.dimstyle.dimtfill = 2;
                exporter.dimstyle.dimtfillclr = 42;
                if (dynamicOverride)
                    exporter.dimstyle.add("$DIMTFILLCLR", 70, 13);
                exporter.failAt = offset;
                const bool written = codec.write(&exporter, DRW::AC1021, binary);
                exporter.failAt = -1;
                std::string current;
                if (written || exporter.accepted
                    || exporter.reached != offset + 1
                    || !allocationFailed || !readBytes(path, current)
                    || current != prior || exporter.dimstyle.flags != 64
                    || exporter.dimstyle.dimtfill != 2
                    || exporter.dimstyle.dimtfillclr != 42
                    || (dynamicOverride
                        && (exporter.dimstyle.get("$DIMTFILLCLR") == nullptr
                            || exporter.dimstyle.get("$DIMTFILLCLR")->code() != 70
                            || exporter.dimstyle.get("$DIMTFILLCLR")->i_val()
                                != 13))) {
                    std::cerr << "DIMTFILLCLR allocation rollback failed: binary="
                              << binary << " offset=" << offset << '\n';
                    return false;
                }

                if (!codec.write(&exporter, DRW::AC1021, binary)
                    || !exporter.styleAccepted || !exporter.accepted
                    || exporter.dimstyle.dimtfillclr != 42
                    || (dynamicOverride
                        && (exporter.dimstyle.get("$DIMTFILLCLR") == nullptr
                            || exporter.dimstyle.get("$DIMTFILLCLR")->code() != 70
                            || exporter.dimstyle.get("$DIMTFILLCLR")->i_val()
                                != 13))
                    || !readBytes(path, current) || current != expectedBytes) {
                    std::cerr << "DIMTFILLCLR same-writer retry failed: binary="
                              << binary << " offset=" << offset << '\n';
                    return false;
                }
                dxfRW readerCodec(path.string().c_str());
                Reader reader;
                if (!readerCodec.read(&reader, false)
                    || reader.dimstyles.size() != 1u
                    || reader.dimstyles.front().flags != 64
                    || reader.dimstyles.front().dimtfill != 2
                    || reader.dimstyles.front().dimtfillclr != expectedColor)
                    return false;
            }
            std::cout << "DIMTFILLCLR AC1021 " << (binary ? "Binary" : "ASCII")
                      << (dynamicOverride ? " dynamic" : " typed")
                      << " allocation offsets=" << allocationCount << '\n';
        }
    }
    return true;
}

bool dimstyleTextFillColorUnsupportedCmcRetry(
    const std::filesystem::path& directory) {
    const auto path = directory / "dimstyle-text-fill-color-cmc.dxf";
    const auto cleanPath =
        directory / "dimstyle-text-fill-color-cmc-clean.dxf";
    const std::string prior = "prior DIMSTYLE destination bytes\n";
    for (const bool binary : {false, true}) {
        dxfRW cleanCodec(cleanPath.string().c_str());
        Export cleanExporter(cleanCodec);
        cleanExporter.dimstyle.dimtfill = 2;
        cleanExporter.dimstyle.dimtfillclr = 42;
        if (!cleanCodec.write(&cleanExporter, DRW::AC1021, binary)
            || !cleanExporter.accepted)
            return false;
        std::string expected;
        if (!readBytes(cleanPath, expected))
            return false;

        {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output << prior;
            if (!output)
                return false;
        }
        dxfRW codec(path.string().c_str());
        Export exporter(codec);
        exporter.dimstyle.dimtfill = 2;
        exporter.dimstyle.dimtfillclr = 256;
        exporter.dimstyle.dimtfillclrRgb = 0x0000FF;
        if (codec.write(&exporter, DRW::AC1021, binary)
            || exporter.accepted) {
            std::cerr << "unprojected DIMTFILLCLR RGB was accepted: binary="
                      << binary << '\n';
            return false;
        }
        std::string current;
        if (!readBytes(path, current) || current != prior)
            return false;

        exporter.dimstyle.dimtfillclr = 42;
        exporter.dimstyle.dimtfillclrRgb = -1;
        if (!codec.write(&exporter, DRW::AC1021, binary)
            || !exporter.accepted || !readBytes(path, current)
            || current != expected) {
            std::cerr << "DIMTFILLCLR ACI retry after RGB refusal failed: binary="
                      << binary << '\n';
            return false;
        }
        dxfRW readerCodec(path.string().c_str());
        Reader reader;
        if (!readerCodec.read(&reader, false) || reader.dimstyles.size() != 1u
            || reader.dimstyles.front().dimtfillclr != 42)
            return false;
    }
    return true;
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
            || !cleanExporter.accepted) {
            std::cerr << "pre-2010 DIMTXTDIRECTION baseline failed\n";
            return false;
        }
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

bool dimtxtdirectionRefusalRetry(const std::filesystem::path& directory) {
    const auto path = directory / "dimstyle-dimtxtdirection.dxf";
    const auto cleanPath = directory / "dimstyle-dimtxtdirection-clean.dxf";
    const std::string prior = "prior DIMSTYLE destination bytes\n";
    const DRW::Version version = DRW::AC1021;
    for (const bool binary : {false, true}) {
        dxfRW cleanCodec(cleanPath.string().c_str());
        Export cleanExporter(cleanCodec);
        if (!cleanCodec.write(&cleanExporter, version, binary)
            || !cleanExporter.accepted)
            return false;
        std::string expectedBytes;
        if (!readBytes(cleanPath, expectedBytes)
            || (!binary && asciiDimstyleHasGroup(expectedBytes, 295))) {
            std::cerr << "pre-2010 DIMSTYLE emitted group 295\n";
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
                exporter.dimstyle.add("$DIMTXTDIRECTION", 295, 1);
            else
                exporter.dimstyle.dimtxtdirection = 1;
            if (codec.write(&exporter, version, binary)
                || exporter.accepted) {
                std::cerr << "pre-2010 DIMTXTDIRECTION refusal failed: binary="
                          << binary << " override=" << useOverride << '\n';
                return false;
            }
            std::string current;
            if (!readBytes(path, current) || current != prior)
                return false;

            if (useOverride)
                exporter.dimstyle.add("$DIMTXTDIRECTION", 295, 0);
            else
                exporter.dimstyle.dimtxtdirection = 0;
            if (!codec.write(&exporter, version, binary)
                || !exporter.accepted || !readBytes(path, current)
                || current != expectedBytes || !readbackMatches(path)) {
                std::cerr << "pre-2010 DIMTXTDIRECTION retry failed: binary="
                          << binary << " override=" << useOverride << '\n';
                return false;
            }
        }
    }
    return true;
}

bool dimtxtdirectionTypedOutput(const std::filesystem::path& directory) {
    const auto path = directory / "dimstyle-dimtxtdirection-typed.dxf";
    for (const bool binary : {false, true}) {
        dxfRW codec(path.string().c_str());
        Export exporter(codec);
        exporter.dimstyle.dimtxtdirection = 1;
        if (!codec.write(&exporter, DRW::AC1024, binary)
            || !exporter.accepted) {
            std::cerr << "AC1024 DIMTXTDIRECTION typed write failed: binary="
                      << binary << " error=" << codec.getError() << '\n';
            return false;
        }
        std::string bytes;
        if (!readBytes(path, bytes)
            || (!binary && !asciiDimstyleHasGroup(bytes, 295))) {
            std::cerr << "AC1024 DIMTXTDIRECTION output group missing: binary="
                      << binary << '\n';
            return false;
        }
        dxfRW readerCodec(path.string().c_str());
        Reader reader;
        if (!readerCodec.read(&reader, false)
            || reader.dimstyles.size() != 1u
            || reader.dimstyles.front().dimtxtdirection != 1) {
            std::cerr << "AC1024 DIMTXTDIRECTION typed readback failed: binary="
                      << binary << '\n';
            return false;
        }
    }
    return true;
}

bool dimstyleTextFillAndArcSymbolTypedOutput(
    const std::filesystem::path& directory) {
    const auto path = directory / "dimstyle-text-fill-arc-symbol.dxf";
    for (const bool binary : {false, true}) {
        dxfRW codec(path.string().c_str());
        Export exporter(codec);
        exporter.dimstyle.dimtfill = 2;
        exporter.dimstyle.dimarcsym = 1;
        if (!codec.write(&exporter, DRW::AC1021, binary)
            || !exporter.accepted) {
            std::cerr << "AC1021 DIMTFILL/DIMARCSYM typed write failed: binary="
                      << binary << " error=" << codec.getError() << '\n';
            return false;
        }
        std::string bytes;
        if (!readBytes(path, bytes)
            || (!binary && (!asciiDimstyleHasGroup(bytes, 69)
                            || !asciiDimstyleHasGroup(bytes, 90)))) {
            std::cerr << "AC1021 DIMTFILL/DIMARCSYM output missing: binary="
                      << binary << '\n';
            return false;
        }
        dxfRW readerCodec(path.string().c_str());
        Reader reader;
        if (!readerCodec.read(&reader, false)
            || reader.dimstyles.size() != 1u
            || reader.dimstyles.front().dimtfill != 2
            || reader.dimstyles.front().dimarcsym != 1) {
            std::cerr << "AC1021 DIMTFILL/DIMARCSYM readback failed: binary="
                      << binary << '\n';
            return false;
        }
    }
    return true;
}

bool dimstyleTextFillAndArcSymbolDowngradeRetry(
    const std::filesystem::path& directory) {
    const auto path = directory / "dimstyle-text-fill-arc-symbol-old.dxf";
    const auto cleanPath =
        directory / "dimstyle-text-fill-arc-symbol-old-clean.dxf";
    const std::string prior = "prior DIMSTYLE destination bytes\n";
    for (const bool binary : {false, true}) {
        dxfRW cleanCodec(cleanPath.string().c_str());
        Export cleanExporter(cleanCodec);
        if (!cleanCodec.write(&cleanExporter, DRW::AC1018, binary)
            || !cleanExporter.accepted)
            return false;
        std::string expectedBytes;
        if (!readBytes(cleanPath, expectedBytes)
            || (!binary && (asciiDimstyleHasGroup(expectedBytes, 69)
                            || asciiDimstyleHasGroup(expectedBytes, 90))))
            return false;

        for (const int code : {69, 90}) {
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
                    exporter.dimstyle.add(code == 69 ? "$DIMTFILL"
                                                     : "$DIMARCSYM",
                                          code, code == 69 ? 2 : 1);
                else if (code == 69)
                    exporter.dimstyle.dimtfill = 2;
                else
                    exporter.dimstyle.dimarcsym = 1;
                if (codec.write(&exporter, DRW::AC1018, binary)
                    || exporter.accepted) {
                    std::cerr << "pre-2007 DIMSTYLE field downgrade was accepted: code="
                              << code << " binary=" << binary
                              << " override=" << useOverride << '\n';
                    return false;
                }
                std::string current;
                if (!readBytes(path, current) || current != prior)
                    return false;

                if (useOverride)
                    exporter.dimstyle.add(code == 69 ? "$DIMTFILL"
                                                     : "$DIMARCSYM",
                                          code, 0);
                else if (code == 69)
                    exporter.dimstyle.dimtfill = 0;
                else
                    exporter.dimstyle.dimarcsym = 0;
                if (!codec.write(&exporter, DRW::AC1018, binary)
                    || !exporter.accepted || !readBytes(path, current)
                    || current != expectedBytes || !readbackMatches(path)) {
                    std::cerr << "pre-2007 DIMSTYLE field retry failed: code="
                              << code << " binary=" << binary
                              << " override=" << useOverride << '\n';
                    return false;
                }
            }
        }
    }
    return true;
}

bool dimstyleTextFillColorDowngradeRetry(
    const std::filesystem::path& directory) {
    const auto path = directory / "dimstyle-text-fill-color.dxf";
    const auto cleanPath =
        directory / "dimstyle-text-fill-color-clean.dxf";
    const std::string prior = "prior DIMSTYLE destination bytes\n";
    for (const bool binary : {false, true}) {
        dxfRW cleanCodec(cleanPath.string().c_str());
        Export cleanExporter(cleanCodec);
        if (!cleanCodec.write(&cleanExporter, DRW::AC1018, binary)
            || !cleanExporter.accepted)
            return false;
        std::string expectedBytes;
        if (!readBytes(cleanPath, expectedBytes))
            return false;

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
            if (useOverride) {
                exporter.dimstyle.add("$DIMTFILLCLR", 70, 42);
            } else {
                exporter.dimstyle.dimtfillclr = 42;
            }
            if (codec.write(&exporter, DRW::AC1018, binary)
                || exporter.accepted) {
                std::cerr << "pre-2007 DIMTFILLCLR downgrade was accepted: binary="
                          << binary << " override=" << useOverride << '\n';
                return false;
            }
            std::string current;
            if (!readBytes(path, current) || current != prior)
                return false;

            if (useOverride) {
                const auto field = exporter.dimstyle.vars.find("$DIMTFILLCLR");
                if (field == exporter.dimstyle.vars.end())
                    return false;
                delete field->second;
                exporter.dimstyle.vars.erase(field);
            } else {
                exporter.dimstyle.dimtfillclr = 0;
                exporter.dimstyle.dimtfillclrMethod = 0;
            }
            if (!codec.write(&exporter, DRW::AC1018, binary)
                || !exporter.accepted || !readBytes(path, current)
                || current != expectedBytes || !readbackMatches(path)) {
                std::cerr << "DIMTFILLCLR downgrade retry failed: binary="
                          << binary << " override=" << useOverride << '\n';
                return false;
            }
        }
        // A DWG CMC can carry the documented default index 0 with explicit
        // BYBLOCK method (0xC1). An older DXF has no DIMTFILLCLR field, but
        // omitting this default is equivalent. Keep nondefault fill refused
        // and verify atomic failure followed by the same-writer retry.
        {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output << prior;
            if (!output) return false;
        }
        dxfRW codec(path.string().c_str());
        Export exporter(codec);
        exporter.dimstyle.dimtfillclr = 0;
        exporter.dimstyle.dimtfillclrMethod = 0xC1u;
        exporter.dimstyle.dimtfill = 2;
        if (codec.write(&exporter, DRW::AC1018, binary) || exporter.accepted)
            return false;
        std::string current;
        if (!readBytes(path, current) || current != prior)
            return false;
        exporter.dimstyle.dimtfill = 0;
        if (!codec.write(&exporter, DRW::AC1018, binary)
            || !exporter.accepted || !readBytes(path, current)
            || current != expectedBytes || !readbackMatches(path)
            || exporter.dimstyle.dimtfillclrMethod != 0xC1u) {
            std::cerr << "default BYBLOCK DIMTFILLCLR downgrade/retry failed: binary="
                      << binary << '\n';
            return false;
        }
    }
    return true;
}

bool dimstyleJogAngleTypedOutput(const std::filesystem::path& directory) {
    const auto path = directory / "dimstyle-jog-angle.dxf";
    for (const bool binary : {false, true}) {
        dxfRW codec(path.string().c_str());
        Export exporter(codec);
        exporter.dimstyle.dimjogang = 45.0;
        if (!codec.write(&exporter, DRW::AC1021, binary)
            || !exporter.accepted) {
            std::cerr << "AC1021 DIMJOGANG typed write failed: binary="
                      << binary << " error=" << codec.getError() << '\n';
            return false;
        }
        std::string bytes;
        if (!readBytes(path, bytes)
            || (!binary && !asciiDimstyleHasGroup(bytes, 50)))
            return false;
        dxfRW readerCodec(path.string().c_str());
        Reader reader;
        if (!readerCodec.read(&reader, false)
            || reader.dimstyles.size() != 1u
            || reader.dimstyles.front().dimjogang != 45.0) {
            std::cerr << "AC1021 DIMJOGANG typed readback failed: binary="
                      << binary << '\n';
            return false;
        }
    }
    return true;
}

bool dimstyleJogAngleDowngradeRetry(
    const std::filesystem::path& directory) {
    const auto path = directory / "dimstyle-jog-angle-old.dxf";
    const auto cleanPath = directory / "dimstyle-jog-angle-old-clean.dxf";
    const std::string prior = "prior DIMSTYLE destination bytes\n";
    for (const bool binary : {false, true}) {
        dxfRW cleanCodec(cleanPath.string().c_str());
        Export cleanExporter(cleanCodec);
        if (!cleanCodec.write(&cleanExporter, DRW::AC1018, binary)
            || !cleanExporter.accepted)
            return false;
        std::string expectedBytes;
        if (!readBytes(cleanPath, expectedBytes)
            || (!binary && asciiDimstyleHasGroup(expectedBytes, 50)))
            return false;

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
                exporter.dimstyle.add("$DIMJOGANG", 50, 45.0);
            else
                exporter.dimstyle.dimjogang = 45.0;
            if (codec.write(&exporter, DRW::AC1018, binary)
                || exporter.accepted) {
                std::cerr << "pre-2007 DIMJOGANG downgrade accepted: binary="
                          << binary << " override=" << useOverride << '\n';
                return false;
            }
            std::string current;
            if (!readBytes(path, current) || current != prior)
                return false;

            if (useOverride)
                exporter.dimstyle.add("$DIMJOGANG", 50, 0.0);
            else
                exporter.dimstyle.dimjogang = 0.0;
            if (!codec.write(&exporter, DRW::AC1018, binary)
                || !exporter.accepted || !readBytes(path, current)
                || current != expectedBytes || !readbackMatches(path)) {
                std::cerr << "pre-2007 DIMJOGANG retry failed: binary="
                          << binary << " override=" << useOverride << '\n';
                return false;
            }
        }
    }
    return true;
}

bool dimstyleJogAngleDomainRefusalRetry(
    const std::filesystem::path& directory) {
    const auto path = directory / "dimstyle-jog-angle-domain.dxf";
    const auto cleanPath = directory / "dimstyle-jog-angle-domain-clean.dxf";
    const std::string prior = "prior DIMSTYLE destination bytes\n";
    for (const bool binary : {false, true}) {
        dxfRW cleanCodec(cleanPath.string().c_str());
        Export cleanExporter(cleanCodec);
        cleanExporter.dimstyle.dimjogang = 45.0;
        if (!cleanCodec.write(&cleanExporter, DRW::AC1021, binary)
            || !cleanExporter.accepted)
            return false;
        std::string expectedBytes;
        if (!readBytes(cleanPath, expectedBytes))
            return false;

        const std::array<double, 3> invalidValues{{
            4.0, 91.0, std::numeric_limits<double>::quiet_NaN()}};
        for (const double invalid : invalidValues) {
            {
                std::ofstream output(path,
                                     std::ios::binary | std::ios::trunc);
                output << prior;
                if (!output)
                    return false;
            }
            dxfRW codec(path.string().c_str());
            Export exporter(codec);
            exporter.dimstyle.dimjogang = invalid;
            if (codec.write(&exporter, DRW::AC1021, binary)
                || exporter.accepted) {
                std::cerr << "invalid DIMJOGANG was accepted: binary="
                          << binary << '\n';
                return false;
            }
            std::string current;
            if (!readBytes(path, current) || current != prior)
                return false;
            exporter.dimstyle.dimjogang = 45.0;
            if (!codec.write(&exporter, DRW::AC1021, binary)
                || !exporter.accepted || !readBytes(path, current)
                || current != expectedBytes) {
                std::cerr << "invalid DIMJOGANG same-writer retry failed: binary="
                          << binary << '\n';
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
        && dimstyleTextFillColorTypedOutput(directory)
        && dimstyleTextFillColorAllocationSweep(directory)
        && dimstyleTextFillColorUnsupportedCmcRetry(directory)
        && dimfxlRefusalRetry(directory)
        && dimaltrndRefusalRetry(directory)
        && dimfxlonRefusalRetry(directory)
        && dimtxtdirectionRefusalRetry(directory)
        && dimtxtdirectionTypedOutput(directory)
        && dimstyleTextFillAndArcSymbolTypedOutput(directory)
        && dimstyleTextFillAndArcSymbolDowngradeRetry(directory)
        && dimstyleTextFillColorDowngradeRetry(directory)
        && dimstyleJogAngleTypedOutput(directory)
        && dimstyleJogAngleDowngradeRetry(directory)
        && dimstyleJogAngleDomainRefusalRetry(directory) ? 0 : 2;
}
