// SPDX-License-Identifier: GPL-2.0-or-later
#include "libdxfrw.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"
#include "lc3_compat_check.cpp"
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

namespace {
struct Capture final : LC3CompatCheck {
    void addMText(const DRW_MText& value) override {
        ++count; text = value.text; style = value.linespacingStyle;
    }
    void addAttDef(const DRW_Attdef& value) override {
        if (value.mtext) {
            ++embeddedCount; embeddedText = value.mtext->text;
            embeddedStyle = value.mtext->linespacingStyle;
            embeddedAngle = value.mtext->angle;
            embeddedDirection = value.mtext->secPoint;
            embeddedHasDirection = embeddedDirection.x != 0.0
                || embeddedDirection.y != 0.0 || embeddedDirection.z != 0.0;
        }
    }
    int count = 0;
    int embeddedCount = 0;
    std::uint16_t style = 0;
    std::uint16_t embeddedStyle = 0;
    double embeddedAngle = 0.0;
    DRW_Coord embeddedDirection;
    bool embeddedHasDirection = false;
    std::string text;
    std::string embeddedText;
};
struct Exporter final : LC3CompatCheck {
    Exporter(dxfRW& output, const std::string& content) : out(output) {
        value.text = content; value.height = 1.0; value.widthscale = 10.0;
    }
    void writeEntities() override { ok = out.writeMText(&value); }
    bool finalizeDxfWrite() override { return ok; }
    dxfRW& out;
    DRW_MText value;
    bool ok = false;
};
struct EmbeddedExporter final : LC3CompatCheck {
    EmbeddedExporter(dxfRW& output, const std::string& content) : out(output) {
        value.text = "fallback"; value.tag = "TAG"; value.prompt = "Prompt";
        value.height = 1.0; value.mtext = std::make_unique<DRW_MText>();
        value.mtext->text = content; value.mtext->height = 1.0;
        value.mtext->widthscale = 10.0;
    }
    void writeEntities() override { ok = out.writeAttdef(&value); }
    bool finalizeDxfWrite() override { return ok; }
    dxfRW& out; DRW_Attdef value; bool ok = false;
};
bool makeSource(const std::filesystem::path& path, bool binary,
                const std::string& text, const char* version) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) return false;
    if (binary) stream.write("AutoCAD Binary DXF\r\n\x1a\0", 22);
    std::unique_ptr<dxfWriter> w = binary
        ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&stream))
        : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&stream));
    w->setVersion(version, true);
    w->writeString(0, "SECTION"); w->writeString(2, "HEADER");
    w->writeString(9, "$ACADVER"); w->writeString(1, version);
    w->writeString(0, "ENDSEC"); w->writeString(0, "SECTION");
    w->writeString(2, "ENTITIES"); w->writeString(0, "MTEXT");
    w->writeString(100, "AcDbEntity"); w->writeString(8, "0");
    w->writeString(100, "AcDbMText"); w->writeDouble(10, 0.0);
    w->writeDouble(20, 0.0); w->writeDouble(40, 1.0);
    w->writeDouble(41, 10.0); w->writeInt16(71, 1);
    // Split immediately after a caret to exercise assembly before decoding.
    const auto split = text.find('^') + 1u;
    w->writeString(3, std::string_view(text).substr(0, split));
    w->writeString(1, std::string_view(text).substr(split));
    w->writeString(0, "ENDSEC"); w->writeString(0, "EOF");
    return !w->hasWriteError() && stream.good();
}
bool read(const std::filesystem::path& path, const std::string& expected) {
    dxfRW input(path.string().c_str()); Capture capture;
    return input.read(&capture, false) && capture.count == 1
        && capture.text == expected;
}
bool wireText(const std::filesystem::path& path, bool binary,
              const std::string& expected) {
    std::ifstream stream(path, std::ios::binary);
    if (binary) stream.seekg(22);
    std::unique_ptr<dxfReader> r = binary
        ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&stream))
        : std::unique_ptr<dxfReader>(new dxfReaderAscii(&stream));
    std::string text;
    bool inMText = false;
    int code = 0;
    while (r->readRec(&code)) {
        if (code == 0) inMText = r->getRawValue() == "MTEXT";
        else if (inMText && (code == 1 || code == 3)) {
            const auto chunk = r->getRawValue();
            if (code == 3 && !chunk.empty() && chunk.back() == '^') return false;
            text += chunk;
        }
    }
    return text == expected;
}
}

int main(int argc, char** argv) {
    if (argc != 1 && (argc != 3 || std::string(argv[1]) != "--output-dir"))
        return 2;
    const auto dir = argc == 3 ? std::filesystem::path(argv[2])
        : std::filesystem::temp_directory_path()
          / ("libdxfrw-mtext-caret-" + std::to_string(
              std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(dir);
    struct Cleanup { std::filesystem::path p; bool remove; ~Cleanup() {
        if (remove) { std::error_code ec; std::filesystem::remove_all(p, ec); }
    }} cleanup{dir, argc == 1};
    const std::string text = std::string(246u, 'A')
        + "\\S1^ 2;^Iitem\\P^Jline ^Z";
    for (DRW::Version version : {DRW::AC1018, DRW::AC1027}) {
      for (bool binary : {false, true}) {
        const std::string prefix = std::string(version == DRW::AC1018 ? "legacy-" : "modern-")
            + (binary ? "binary-" : "ascii-");
        const auto source = dir / (prefix + "source.dxf");
        if (!makeSource(source, binary, text,
                        version == DRW::AC1018 ? "AC1018" : "AC1027")
            || !read(source, text)) {
            std::cerr << "MTEXT source caret syntax was not preserved\n";
            return 1;
        }
        for (bool strict : {false, true}) {
            const auto target = dir / (prefix + (strict ? "strict.dxf" : "compat.dxf"));
            dxfRW output(target.string().c_str());
            if (strict) output.setDxfTextWritePolicy(DRW::DxfTextWritePolicy::RequireLossless);
            Exporter exporter(output, text);
            if (!output.write(&exporter, version, binary) || !exporter.ok
                || !wireText(target, binary, text) || !read(target, text)) {
                std::cerr << "MTEXT caret export/readback failed binary="
                          << binary << " strict=" << strict << '\n';
                return 1;
            }
            const auto newlineTarget = dir / (prefix + (strict ? "strict-newline.dxf" : "compat-newline.dxf"));
            dxfRW newlineOutput(newlineTarget.string().c_str());
            if (strict) newlineOutput.setDxfTextWritePolicy(DRW::DxfTextWritePolicy::RequireLossless);
            Exporter newlineExporter(newlineOutput, text + "\nnext");
            const std::string normalized = text + "\\Pnext";
            if (!newlineOutput.write(&newlineExporter, version, binary) || !newlineExporter.ok
                || !wireText(newlineTarget, binary, normalized) || !read(newlineTarget, normalized)) {
                std::cerr << "MTEXT caret/newline export failed binary=" << binary
                          << " strict=" << strict << '\n';
                return 1;
            }
        }
      }
    }
    for (bool binary : {false, true}) {
        for (bool strict : {false, true}) {
            const auto target = dir / (std::string(binary ? "embedded-binary-" : "embedded-ascii-")
                                       + (strict ? "strict.dxf" : "compat.dxf"));
            dxfRW output(target.string().c_str());
            if (strict) output.setDxfTextWritePolicy(DRW::DxfTextWritePolicy::RequireLossless);
            EmbeddedExporter exporter(output, text);
            if (!output.write(&exporter, DRW::AC1032, binary) || !exporter.ok) {
                std::cerr << "embedded MTEXT caret export failed\n";
                return 1;
            }
            dxfRW input(target.string().c_str()); Capture capture;
            if (!input.read(&capture, false) || capture.embeddedCount != 1
                || capture.embeddedText != text) {
                std::cerr << "embedded MTEXT caret readback failed\n";
                return 1;
            }
        }
    }
    for (bool binary : {false, true}) for (bool strict : {false, true}) {
        for (bool characterLimit : {false, true}) {
            std::ostringstream bytes(std::ios::binary);
            std::unique_ptr<dxfWriter> writer = binary
                ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&bytes))
                : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&bytes));
            writer->setVersion("AC1027", true);
            if (strict) writer->setTextWritePolicy(DRW::DxfTextWritePolicy::RequireLossless);
            if (writer->writeUtf8MTextChunks("^I", 3, 1,
                    characterLimit ? 250u : 1u, characterLimit ? 1u : 250u)
                || !bytes.str().empty() || !writer->hasWriteError()) {
                std::cerr << "oversized MTEXT caret pair was not rejected atomically\n";
                return 1;
            }
        }
    }
    for (const auto version : {DRW::AC1018, DRW::AC1021, DRW::AC1027, DRW::AC1032}) {
      for (bool binary : {false, true}) for (const std::uint16_t style : {0u, 1u, 2u, 7u, 65535u}) {
        const auto expected = static_cast<std::uint16_t>(style == 2u ? 2u : 1u);
        const auto prefix = "spacing-" + std::to_string(static_cast<int>(version))
            + (binary ? "-binary-" : "-ascii-") + std::to_string(style);
        const auto target = dir / (prefix + ".dxf");
        dxfRW output(target.string().c_str()); Exporter exporter(output, "spacing");
        exporter.value.linespacingStyle = style;
        if (!output.write(&exporter, version, binary) || !exporter.ok
            || exporter.value.linespacingStyle != style) return 1;
        dxfRW input(target.string().c_str()); Capture capture;
        if (!input.read(&capture, false) || capture.count != 1
            || capture.style != expected) {
            std::cerr << "standalone MTEXT line spacing mismatch\n"; return 1;
        }
        if (version != DRW::AC1032) continue;
        const auto nestedTarget = dir / (prefix + "-embedded.dxf");
        dxfRW nestedOutput(nestedTarget.string().c_str());
        EmbeddedExporter nested(nestedOutput, "spacing");
        nested.value.mtext->linespacingStyle = style;
        if (!nestedOutput.write(&nested, version, binary) || !nested.ok
            || nested.value.mtext->linespacingStyle != style) return 1;
        dxfRW nestedInput(nestedTarget.string().c_str()); Capture nestedCapture;
        if (!nestedInput.read(&nestedCapture, false) || nestedCapture.embeddedCount != 1
            || nestedCapture.embeddedStyle != expected) {
            std::cerr << "embedded MTEXT line spacing mismatch\n"; return 1;
        }
      }
    }
    for (bool binary : {false, true}) {
        for (int profile = 0; profile < 4; ++profile) {
            const auto target = dir / (std::string("orientation-")
                + (binary ? "binary-" : "ascii-") + std::to_string(profile) + ".dxf");
            dxfRW output(target.string().c_str()); EmbeddedExporter exporter(output, "angle");
            auto& textValue = *exporter.value.mtext;
            textValue.angle = profile == 3 ? 30.0 + 1e-11 : 30.0;
            textValue.secPoint = profile == 0 ? DRW_Coord{0.0, 1.0, 0.0}
                : profile == 2 ? DRW_Coord{0.0, 1.0, 2.0}
                               : DRW_Coord{0.8660254037844386, 0.5, 0.0};
            const double originalAngle = textValue.angle;
            const auto originalDirection = textValue.secPoint;
            if (!output.write(&exporter, DRW::AC1032, binary) || !exporter.ok
                || textValue.angle != originalAngle || textValue.secPoint.x != originalDirection.x
                || textValue.secPoint.y != originalDirection.y
                || textValue.secPoint.z != originalDirection.z) return 1;
            dxfRW input(target.string().c_str()); Capture capture;
            if (!input.read(&capture, false) || capture.embeddedCount != 1
                || std::abs(capture.embeddedAngle - (profile == 2 ? 90.0 : originalAngle)) > 1e-10
                || capture.embeddedHasDirection != (profile == 1 || profile == 2)
                || (profile == 2 && capture.embeddedDirection.z != 2.0)) {
                std::cerr << "embedded legacy MTEXT orientation mismatch\n"; return 1;
            }
        }
    }
    std::cout << "MTEXT caret source, ASCII/Binary output and typed readback PASS\n";
}
