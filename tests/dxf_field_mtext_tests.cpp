// SPDX-License-Identifier: GPL-2.0-or-later
#include "libdxfrw.h"
#include "intern/dxfwriter.h"
#include "lc3_compat_check.cpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>

namespace {
struct Capture final : LC3CompatCheck {
    void addField(const DRW_Field& value) override { field = value; ++count; }
    DRW_Field field; int count = 0;
};
struct Exporter final : LC3CompatCheck {
    Exporter(dxfRW& output, const DRW_Field& source) : out(output), field(source) {}
    void writeObjects() override { ok = out.writeField(&field); }
    bool finalizeDxfWrite() override { return ok; }
    dxfRW& out; DRW_Field field; bool ok = false;
};
bool create(const std::filesystem::path& path, bool binary, bool mtext,
            bool lateEvaluator, const std::string& text,
            std::size_t forcedSplit = std::string::npos,
            const char* version = "AC1027", const char* codepage = nullptr) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) return false;
    if (binary) stream.write("AutoCAD Binary DXF\r\n\x1a\0", 22);
    std::unique_ptr<dxfWriter> w = binary
        ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&stream))
        : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&stream));
    w->setVersion(version, true);
    w->writeString(0,"SECTION"); w->writeString(2,"HEADER");
    w->writeString(9,"$ACADVER"); w->writeString(1,version);
    if (codepage) { w->writeString(9,"$DWGCODEPAGE"); w->writeString(3,codepage); }
    w->writeString(0,"ENDSEC"); w->writeString(0,"SECTION");
    w->writeString(2,"OBJECTS"); w->writeString(0,"FIELD");
    w->writeString(5,"50"); w->writeString(330,"C");
    w->writeString(100,"AcDbField");
    if (!lateEvaluator) w->writeString(1,mtext?"_text":"Other");
    const auto split = forcedSplit != std::string::npos ? forcedSplit
        : mtext ? text.find('^') + 1u : text.size();
    w->writeString(3,std::string_view(text).substr(0,split));
    w->writeString(2,std::string_view(text).substr(split));
    if (lateEvaluator) w->writeString(1,mtext?"_text":"Other");
    w->writeInt32(90,0); w->writeInt32(97,0);
    w->writeInt32(91,63); w->writeInt32(92,0); w->writeInt32(94,1);
    w->writeInt32(95,2); w->writeInt32(96,0); w->writeString(300,"");
    w->writeInt32(93,0); w->writeString(7,"ACFD_FIELD_VALUE");
    w->writeInt32(93,0); w->writeInt32(90,4);
    w->writeString(2,std::string_view(text).substr(0,split));
    w->writeString(1,std::string_view(text).substr(split));
    w->writeInt32(94,0); w->writeString(300,""); w->writeString(302,"");
    w->writeString(304,"ACVALUE_END");
    w->writeString(9,std::string_view(text).substr(0,split));
    w->writeString(301,std::string_view(text).substr(split));
    w->writeInt32(98,static_cast<int>(text.size()));
    w->writeString(0,"ENDSEC"); w->writeString(0,"EOF");
    return !w->hasWriteError() && stream.good();
}
bool matches(const Capture& capture, const std::string& expected) {
    const auto& f = capture.field;
    return capture.count == 1 && f.m_fieldCode == expected
        && f.m_valueString == expected && f.m_value.m_value.type() == DRW_Variant::STRING
        && f.m_value.m_value.content.s != nullptr
        && *f.m_value.m_value.content.s == expected;
}
}

int main(int argc, char** argv) {
    if (argc == 5 && std::string(argv[1]) == "--source-record") {
        dxfRW input(argv[2]); Capture source;
        if (!input.read(&source,false) || source.count != 1
            || source.field.m_evaluatorId != "_text"
            || source.field.m_value.m_value.type() != DRW_Variant::STRING)
            return 1;
        for (int index = 0; index < 2; ++index) {
            dxfRW output(argv[3 + index]);
            output.setDxfTextWritePolicy(DRW::DxfTextWritePolicy::RequireLossless);
            Exporter exporter(output,source.field);
            if (!output.write(&exporter,DRW::AC1027,index == 1) || !exporter.ok)
                return 1;
            dxfRW reader(argv[3 + index]); Capture result;
            if (!reader.read(&result,false) || result.count != 1
                || result.field.m_fieldCode != source.field.m_fieldCode
                || result.field.m_valueString != source.field.m_valueString
                || result.field.m_valueStringLength != source.field.m_valueStringLength
                || result.field.parentHandle != source.field.parentHandle
                || result.field.m_childHandles != source.field.m_childHandles
                || result.field.m_objectHandles != source.field.m_objectHandles
                || result.field.m_value.m_value.type() != DRW_Variant::STRING
                || std::string(result.field.m_value.m_value.c_str())
                    != std::string(source.field.m_value.m_value.c_str()))
                return 1;
        }
        std::cout << "isolated source FIELD typed readback PASS\n";
        return 0;
    }
    if (argc != 1 && (argc != 3 || std::string(argv[1]) != "--output-dir")) return 2;
    const auto dir = argc == 3 ? std::filesystem::path(argv[2])
        : std::filesystem::temp_directory_path()
          / ("libdxfrw-field-mtext-" + std::to_string(
              std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(dir);
    struct Cleanup { std::filesystem::path p; bool remove; ~Cleanup() {
        if (remove) { std::error_code ec; std::filesystem::remove_all(p, ec); }
    }} cleanup{dir, argc == 1};
    const std::string text = std::string(246u,'A') + "\\S1^ 2;^Iitem";
    for (bool binary : {false,true}) {
        const auto source = dir / (binary?"source-b.dxf":"source-a.dxf");
        if (!create(source,binary,true,false,text)) return 1;
        dxfRW input(source.string().c_str()); Capture capture;
        if (!input.read(&capture,false) || !matches(capture,text)) {
            std::cerr << "_text FIELD source lost MTEXT syntax\n"; return 1;
        }
        const std::string splitWire = std::string("u") + "\xE2\x82\xAC" + "\\U+0041^ 2";
        const std::string splitExpected = std::string("u") + "\xE2\x82\xAC" + "A^ 2";
        for (std::size_t split = 2; split <= 10; ++split) {
            if (!create(source,binary,true,false,splitWire,split)) return 1;
            dxfRW splitReader(source.string().c_str()); Capture splitCapture;
            if (!splitReader.read(&splitCapture,false) || !matches(splitCapture,splitExpected)) {
                std::cerr << "split _text FIELD UTF-8/escape failed at " << split
                          << " binary=" << binary << '\n'; return 1;
            }
        }
        const std::string legacyWire = std::string("u") + "\x82\xA0" + "^ 2";
        const std::string legacyExpected = std::string("u") + "\xE3\x81\x82" + "^ 2";
        if (!create(source,binary,true,false,legacyWire,2,"AC1018","ANSI_932")) return 1;
        dxfRW legacyReader(source.string().c_str()); Capture legacyCapture;
        if (!legacyReader.read(&legacyCapture,false) || !matches(legacyCapture,legacyExpected)) {
            std::cerr << "split _text FIELD CP932 failed binary=" << binary << '\n'; return 1;
        }
        if (!create(source,binary,true,true,splitWire,2)) return 1;
        dxfRW ambiguousReader(source.string().c_str()); Capture ambiguous;
        if (ambiguousReader.read(&ambiguous,false) || ambiguous.count != 0) {
            std::cerr << "late _text evaluator accepted ambiguous multibyte text\n"; return 1;
        }
        if (!create(source,binary,true,true,"\\U+0041^ 2",3)) return 1;
        dxfRW escapedReader(source.string().c_str()); Capture ambiguousEscape;
        if (escapedReader.read(&ambiguousEscape,false) || ambiguousEscape.count != 0) {
            std::cerr << "late _text evaluator accepted ambiguous escape text\n"; return 1;
        }
        for (const auto version : {DRW::AC1018, DRW::AC1021, DRW::AC1027, DRW::AC1032}) {
          for (bool strict : {false,true}) {
            const auto output = dir / (std::string(binary?"output-b-":"output-a-")
                + std::to_string(static_cast<int>(version)) + (strict?"-strict.dxf":"-compatible.dxf"));
            dxfRW target(output.string().c_str());
            if (strict) target.setDxfTextWritePolicy(DRW::DxfTextWritePolicy::RequireLossless);
            Exporter exporter(target,capture.field);
            if (!target.write(&exporter,version,binary) || !exporter.ok) {
                std::cerr << "_text FIELD output failed\n"; return 1;
            }
            dxfRW reread(output.string().c_str()); Capture roundtrip;
            if (!reread.read(&roundtrip,false) || !matches(roundtrip,text)) {
                std::cerr << "_text FIELD readback lost chunked MTEXT syntax\n"; return 1;
            }
          }
        }
        const auto rollback = dir / (binary?"rollback-b.dxf":"rollback-a.dxf");
        { std::ofstream saved(rollback,std::ios::binary); saved << "sentinel"; }
        dxfRW retryWriter(rollback.string().c_str());
        Exporter invalid(retryWriter,capture.field);
        invalid.field.m_value.m_value = DRW_Variant();
        if (retryWriter.write(&invalid,DRW::AC1027,binary)) return 1;
        std::ifstream saved(rollback,std::ios::binary);
        const std::string preserved((std::istreambuf_iterator<char>(saved)), {});
        if (preserved != "sentinel") return 1;
        saved.close();
        Exporter repaired(retryWriter,capture.field);
        if (!retryWriter.write(&repaired,DRW::AC1027,binary) || !repaired.ok) return 1;
        dxfRW repairedReader(rollback.string().c_str()); Capture repairedCapture;
        if (!repairedReader.read(&repairedCapture,false) || !matches(repairedCapture,text)) return 1;
        if (!create(source,binary,false,false,"A^ B")) return 1;
        dxfRW generic(source.string().c_str()); Capture genericCapture;
        const std::string genericExpected = binary ? "A^ B" : "A^B";
        const bool genericRead = generic.read(&genericCapture,false);
        if (!genericRead || genericCapture.count != 1
            || genericCapture.field.m_fieldCode != genericExpected
            || genericCapture.field.m_valueString != "A^ B"
            || genericCapture.field.m_value.m_value.type() != DRW_Variant::STRING
            || std::string(genericCapture.field.m_value.m_value.c_str()) != genericExpected) {
            std::cerr << "non-MTEXT FIELD caret decoding changed: read=" << genericRead
                      << " count=" << genericCapture.count << " code="
                      << genericCapture.field.m_fieldCode << " cache="
                      << genericCapture.field.m_valueString << " value="
                      << genericCapture.field.m_value.m_value.c_str() << '\n'; return 1;
        }
        const auto genericOutput = dir / (binary?"generic-b.dxf":"generic-a.dxf");
        dxfRW genericTarget(genericOutput.string().c_str());
        Exporter genericExporter(genericTarget,genericCapture.field);
        if (!genericTarget.write(&genericExporter,DRW::AC1027,binary) || !genericExporter.ok)
            return 1;
        dxfRW genericReread(genericOutput.string().c_str()); Capture genericRoundtrip;
        if (!genericReread.read(&genericRoundtrip,false) || genericRoundtrip.count != 1
            || genericRoundtrip.field.m_fieldCode != genericExpected
            || genericRoundtrip.field.m_valueString != "A^ B"
            || genericRoundtrip.field.m_value.m_value.type() != DRW_Variant::STRING
            || std::string(genericRoundtrip.field.m_value.m_value.c_str()) != genericExpected) {
            std::cerr << "non-MTEXT FIELD output/readback changed\n"; return 1;
        }
        if (!create(source,binary,false,true,"A^ B")) return 1;
        dxfRW lateGeneric(source.string().c_str()); Capture lateGenericCapture;
        if (!lateGeneric.read(&lateGenericCapture,false) || lateGenericCapture.count != 1
            || lateGenericCapture.field.m_fieldCode != genericExpected) {
            std::cerr << "late non-MTEXT FIELD evaluator changed\n"; return 1;
        }
        if (!create(source,binary,true,true,"A^ B")) return 1;
        dxfRW retry(source.string().c_str()); Capture rejected;
        const bool lateAccepted = retry.read(&rejected,false);
        if ((!binary && (lateAccepted || rejected.count != 0))
            || (binary && (!lateAccepted || !matches(rejected,"A^ B")))) {
            std::cerr << "late _text evaluator disposition mismatch\n"; return 1;
        }
        if (!create(source,binary,true,false,"A^ B")) return 1;
        Capture corrected;
        if (!retry.read(&corrected,false) || !matches(corrected,"A^ B")) {
            std::cerr << "corrected FIELD retry failed\n"; return 1;
        }
    }
    std::cout << "_text FIELD expression/value/cache and retry PASS\n";
}
