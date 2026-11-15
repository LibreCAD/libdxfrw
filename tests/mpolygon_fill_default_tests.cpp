// SPDX-License-Identifier: GPL-2.0-or-later
// Generated DXF controls for MPOLYGON's omitted solid-fill flag.
#include "lc3_compat_check.cpp"
#include "intern/dxfwriter.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

namespace {
class LegacyConsumer final : public LC3CompatCheck {
public:
    void addHatch(const DRW_Hatch* hatch) override {
        if (hatch != nullptr)
            fills.push_back(hatch->solid);
    }
    std::vector<int> fills;
};

class TypedConsumer final : public LC3CompatCheck {
public:
    void addMPolygon(const DRW_MPolygon* polygon) override {
        if (polygon != nullptr)
            fills.push_back(polygon->solid);
    }
    std::vector<int> fills;
};

bool emitDrawing(std::ostream& stream, bool binary, bool invalidAnnotation) {
    if (binary) {
        static constexpr char signature[] = "AutoCAD Binary DXF\r\n\x1a\0";
        stream.write(signature, sizeof(signature) - 1);
    }
    std::unique_ptr<dxfWriter> writer = binary
        ? std::unique_ptr<dxfWriter>(std::make_unique<dxfWriterBinary>(&stream))
        : std::unique_ptr<dxfWriter>(std::make_unique<dxfWriterAscii>(&stream));
    const auto text = [&](int code, const char* value) { return writer->writeString(code, value); };
    const auto integer = [&](int code, int value) { return writer->writeInt16(code, value); };
    const auto count = [&](int code, int value) { return writer->writeInt32(code, value); };
    const auto real = [&](int code, double value) { return writer->writeDouble(code, value); };
    if (!text(0, "SECTION") || !text(2, "HEADER") || !text(9, "$ACADVER")
        || !text(1, "AC1021") || !text(0, "ENDSEC") || !text(0, "SECTION")
        || !text(2, "ENTITIES")) return false;
    const char* handles[] = {"30", "31", "32", "33"};
    const int flags[] = {-1, 1, 0, -1};
    for (int i = 0; i < (invalidAnnotation ? 1 : 4); ++i) {
        if (!text(0, "MPOLYGON") || !text(5, handles[i])
            || !text(100, "AcDbEntity") || !text(8, "0")
            || !text(100, "AcDbMPolygon") || !integer(70, 1)
            || !real(10, 0) || !real(20, 0) || !real(30, 0)
            || !real(210, 0) || !real(220, 0) || !real(230, 1)
            || !text(2, "ANSI31") || (flags[i] >= 0 && !integer(71, flags[i]))
            || !count(91, 1) || !count(92, 3) || !integer(73, 1)
            || !integer(72, 0) || !count(93, 3)
            || !real(10, 0) || !real(20, 0) || !real(10, 1)
            || !real(20, 0) || !real(10, 0) || !real(20, 1)
            || !integer(76, 1) || (invalidAnnotation && !integer(73, 42))
            || !real(52, 0) || !real(41, 1)
            || !integer(77, 0) || !integer(78, 0)
            || !real(11, 0) || !real(21, 0) || !count(99, 0)) return false;
    }
    return text(0, "ENDSEC") && text(0, "EOF") && !writer->hasWriteError();
}

bool run(bool binary, bool invalidAnnotation) {
    std::ostringstream output(std::ios::binary);
    if (!emitDrawing(output, binary, invalidAnnotation)) return false;
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-mpolygon-default-" + std::to_string(nonce) + ".dxf");
    {
        std::ofstream file(path, std::ios::binary);
        const std::string bytes = output.str();
        file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        if (!file) return false;
    }
    const std::vector<int> expected{0, 1, 0, 0};
    TypedConsumer typed;
    dxfRW typedReader(path.string().c_str());
    const bool typedRead = typedReader.read(&typed, false);
    const bool typedOk = invalidAnnotation ? !typedRead && typed.fills.empty()
                                                && typedReader.getError() == DRW::BAD_CODE_PARSED
                                           : typedRead && typed.fills == expected;
    LegacyConsumer legacy;
    dxfRW legacyReader(path.string().c_str());
    const bool legacyRead = legacyReader.read(&legacy, false);
    const bool legacyOk = invalidAnnotation ? !legacyRead && legacy.fills.empty()
                                                 && legacyReader.getError() == DRW::BAD_CODE_PARSED
                                            : legacyRead && legacy.fills == expected;
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    if (!typedOk || !legacyOk)
        std::cerr << (binary ? "binary" : "ASCII") << " MPOLYGON default/readback failed\n";
    return typedOk && legacyOk;
}
} // namespace

int main() {
    // Directly constructed objects keep their existing API defaults; only
    // DXF input without group 71 uses the MPOLYGON pattern default.
    if (DRW_Hatch{}.solid != 1 || DRW_MPolygon{}.solid != 1) return 1;
    return run(false, false) && run(true, false)
        && run(false, true) && run(true, true) ? 0 : 1;
}
