// SPDX-License-Identifier: GPL-2.0-or-later
// End-to-end generated controls for duplicate DIMASSOC singleton fields.
#include "lc3_compat_check.cpp"
#include "intern/dxfwriter.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

namespace {
class Capture final : public LC3CompatCheck {
public:
    void addDimensionAssociation(const DRW_DimensionAssociation& value) override {
        association = value;
        ++typedCount;
    }
    void addRawDxfObject(const DRW_RawDxfObject& value) override {
        if (value.name == "DIMASSOC") {
            raw = value;
            ++rawCount;
        }
    }
    DRW_DimensionAssociation association;
    DRW_RawDxfObject raw;
    int typedCount = 0;
    int rawCount = 0;
};

class Emitter final : public LC3CompatCheck {
public:
    void writeObjects() override { result = writer->writeDimensionAssociation(&association); }
    dxfRW* writer = nullptr;
    DRW_DimensionAssociation association;
    bool result = true;
};

bool emitInput(std::ostream& out, bool binary, int duplicatedCode) {
    if (binary) {
        static constexpr char signature[] = "AutoCAD Binary DXF\r\n\x1a\0";
        out.write(signature, sizeof(signature) - 1);
    }
    std::unique_ptr<dxfWriter> w = binary
        ? std::unique_ptr<dxfWriter>(std::make_unique<dxfWriterBinary>(&out))
        : std::unique_ptr<dxfWriter>(std::make_unique<dxfWriterAscii>(&out));
    const auto str = [&](int code, const char* value) { return w->writeString(code, value); };
    const auto i16 = [&](int code, int value) { return w->writeInt16(code, value); };
    const auto i32 = [&](int code, int value) { return w->writeInt32(code, value); };
    if (!str(0, "SECTION") || !str(2, "HEADER") || !str(9, "$ACADVER")
        || !str(1, "AC1021") || !str(0, "ENDSEC") || !str(0, "SECTION")
        || !str(2, "OBJECTS") || !str(0, "DIMASSOC") || !str(5, "A0")
        || !str(330, "C") || !str(100, "AcDbDimAssoc")
        || !str(330, "A1") || (duplicatedCode == 330 && !str(330, "A2"))
        || !i32(90, 0) || (duplicatedCode == 90 && !i32(90, 1))
        || !i16(70, 0) || (duplicatedCode == 70 && !i16(70, 1))
        || !i16(71, 0) || (duplicatedCode == 71 && !i16(71, 1)))
        return false;
    return str(0, "ENDSEC") && str(0, "EOF") && !w->hasWriteError();
}

bool run(bool binary, int code) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto root = std::filesystem::temp_directory_path();
    const auto source = root / ("libdxfrw-dimassoc-source-" + std::to_string(nonce) + ".dxf");
    const auto destination = root / ("libdxfrw-dimassoc-output-" + std::to_string(nonce) + ".dxf");
    {
        std::ofstream out(source, std::ios::binary);
        if (!emitInput(out, binary, code) || !out) return false;
    }
    Capture capture;
    dxfRW reader(source.string().c_str());
    bool ok = reader.read(&capture, false) && capture.typedCount == 1
        && capture.rawCount == 1 && capture.association.m_hasUnrepresentableDetail;
    int occurrences = 0;
    bool inBody = false;
    for (const DRW_Variant& group : capture.raw.groups) {
        if (group.code() == 100) inBody = true;
        else if (inBody && group.code() == code) {
            if (code == 330)
                ok = ok && group.stringEquals(occurrences == 0 ? "A1" : "A2");
            else
                ok = ok && group.type() == DRW_Variant::INTEGER
                    && group.i_val() == occurrences;
            ++occurrences;
        }
    }
    ok = ok && occurrences == 2;
    {
        std::ofstream out(destination, std::ios::binary);
        out << "sentinel";
    }
    Emitter emitter;
    emitter.association = capture.association;
    dxfRW writer(destination.string().c_str());
    emitter.writer = &writer;
    ok = !writer.write(&emitter, DRW::AC1021, binary) && !emitter.result && ok;
    std::ifstream in(destination, std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ok = ok && bytes == "sentinel";
    in.close();
    std::error_code ignored;
    std::filesystem::remove(source, ignored);
    std::filesystem::remove(destination, ignored);
    if (!ok) std::cerr << "DIMASSOC duplicate " << code << (binary ? " binary" : " ASCII") << " failed\n";
    return ok;
}
} // namespace

int main() {
    bool ok = true;
    for (bool binary : {false, true})
        for (int code : {330, 90, 70, 71})
            ok = run(binary, code) && ok;
    return ok ? 0 : 1;
}
