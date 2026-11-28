// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

namespace {

class ViewportCapture final : public LC3CompatCheck {
public:
    void addViewport(const DRW_Viewport& value) override {
        viewport = value;
        ++count;
    }

    DRW_Viewport viewport;
    unsigned count{0};
};

class ViewportExport final : public LC3CompatCheck {
public:
    explicit ViewportExport(dxfRW& output) : output(output) {}

    void writeEntities() override {
        writeSucceeded = output.writeViewport(&viewport);
    }

    dxfRW& output;
    DRW_Viewport viewport;
    bool writeSucceeded{false};
};

class ViewportParserProbe final : public DRW_Viewport {
public:
    using DRW_Viewport::parseCode;
    using DRW_Viewport::validateDxf;
};

bool writeSource(const std::filesystem::path& path, bool binary, int status,
                 int shadePlotMode) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.good()) return false;
    if (binary) file << "AutoCAD Binary DXF\r\n" << char(26) << '\0';
    std::unique_ptr<dxfWriter> writer = binary
        ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&file))
        : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&file));
    writer->setVersion("AC1027", true);
    return writer->writeString(0, "SECTION")
        && writer->writeString(2, "HEADER")
        && writer->writeString(9, "$ACADVER")
        && writer->writeString(1, "AC1027")
        && writer->writeString(0, "ENDSEC")
        && writer->writeString(0, "SECTION")
        && writer->writeString(2, "ENTITIES")
        && writer->writeString(0, "VIEWPORT")
        && writer->writeString(5, "30")
        && writer->writeString(100, "AcDbEntity")
        && writer->writeString(8, "0")
        && writer->writeString(100, "AcDbViewport")
        && writer->writeDouble(10, 0.0)
        && writer->writeDouble(20, 0.0)
        && writer->writeDouble(30, 0.0)
        && writer->writeDouble(40, 10.0)
        && writer->writeDouble(41, 10.0)
        && writer->writeInt16(68, status)
        && writer->writeInt16(69, 2)
        && writer->writeInt16(170, shadePlotMode)
        && writer->writeString(0, "ENDSEC")
        && writer->writeString(0, "EOF") && !writer->hasWriteError();
}

std::string readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

bool roundTrip(const std::filesystem::path& directory, bool binary,
               int status, int shadePlotMode) {
    const auto sourcePath = directory / (binary ? "source-b.dxf" : "source-a.dxf");
    const auto outputPath = directory / (binary ? "output-b.dxf" : "output-a.dxf");
    if (!writeSource(sourcePath, binary, status, shadePlotMode)) return false;

    dxfRW importer(sourcePath.string().c_str());
    ViewportCapture source;
    if (!importer.read(&source, false) || source.count != 1
        || source.viewport.vpstatus != status
        || source.viewport.shadePlotMode != shadePlotMode)
        return false;

    dxfRW exporter(outputPath.string().c_str());
    ViewportExport adapter(exporter);
    adapter.viewport = source.viewport;
    if (!exporter.write(&adapter, DRW::AC1027, binary)
        || !adapter.writeSucceeded)
        return false;

    dxfRW verifier(outputPath.string().c_str());
    ViewportCapture readback;
    return verifier.read(&readback, false) && readback.count == 1
        && readback.viewport.vpstatus == status
        && readback.viewport.shadePlotMode == shadePlotMode;
}

int invalidViewportFieldIsToleratedAndRefused(
        const std::filesystem::path& directory, bool binary,
        int code, int value, int correctedValue) {
    const auto outputPath = directory
        / (std::string(code == 68 ? "status-" : "shade-")
           + (binary ? "binary.dxf" : "ascii.dxf"));
    std::istringstream field(std::to_string(code) + "\n"
                             + std::to_string(value) + "\n");
    std::unique_ptr<dxfReader> reader =
        std::make_unique<dxfReaderAscii>(&field);
    ViewportParserProbe parsed;
    int parsedCode = 0;
    if (!reader->readRec(&parsedCode) || parsedCode != code
        || !parsed.parseCode(parsedCode, reader)
        || (code == 68 && parsed.vpstatus != value)
        || (code == 170 && parsed.shadePlotMode != value)
        || parsed.validateDxf())
        return 11;

    {
        std::ofstream sentinel(outputPath, std::ios::binary | std::ios::trunc);
        sentinel << "viewport-status-sentinel";
    }
    dxfRW exporter(outputPath.string().c_str());
    ViewportExport adapter(exporter);
    adapter.viewport = parsed;
    if (exporter.write(&adapter, DRW::AC1027, binary)
        || adapter.writeSucceeded
        || readFile(outputPath) != "viewport-status-sentinel")
        return 12;

    if (code == 68)
        adapter.viewport.vpstatus = correctedValue;
    else
        adapter.viewport.shadePlotMode = correctedValue;
    if (!exporter.write(&adapter, DRW::AC1027, binary)
        || !adapter.writeSucceeded)
        return 13;
    dxfRW verifier(outputPath.string().c_str());
    ViewportCapture readback;
    if (!verifier.read(&readback, false) || readback.count != 1)
        return 14;
    return code == 68
        ? (readback.viewport.vpstatus == correctedValue ? 0 : 15)
        : (readback.viewport.shadePlotMode == correctedValue ? 0 : 16);
}

} // namespace

int main() {
    const auto directory = std::filesystem::temp_directory_path()
        / ("viewport-status-domain-"
           + std::to_string(std::chrono::steady_clock::now()
                                .time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
    } cleanup{directory};

    for (const bool binary : {false, true}) {
        for (const int status : {-1, 0, 1, 32767})
            for (const int shadePlotMode : {0, 1, 2, 3})
                if (!roundTrip(directory, binary, status, shadePlotMode))
                    return 1;
        for (const auto invalidField : {
                 std::pair<int, int>{68, -2},
                 std::pair<int, int>{170, 4}}) {
            const int corrected = invalidField.first == 68 ? -1 : 3;
            const int result = invalidViewportFieldIsToleratedAndRefused(
                directory, binary, invalidField.first, invalidField.second,
                corrected);
            if (result != 0) {
                std::cerr << "invalid-viewport-field stage=" << result
                          << " code=" << invalidField.first
                          << " binary=" << binary << '\n';
                return result;
            }
        }
    }
    return 0;
}
