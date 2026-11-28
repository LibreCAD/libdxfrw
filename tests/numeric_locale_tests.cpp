#include "dx_iface.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"
#include "intern/dxfnumber.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/drw_textcodec.h"

#include <array>
#include <clocale>
#include <cmath>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <locale>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) std::cerr << message << '\n';
    return condition;
}

class CommaPunctuation final : public std::numpunct<char> {
    char do_decimal_point() const override { return ','; }
    char do_thousands_sep() const override { return '.'; }
    std::string do_grouping() const override { return "\3"; }
};

bool readDouble(const std::string& text, double& value) {
    std::istringstream stream("10\n" + text + "\n");
    dxfReaderAscii reader(&stream);
    int code = 0;
    if (!reader.readRec(&code) || code != 10) return false;
    value = reader.getDouble();
    return true;
}

bool testTypedInput() {
    double value = 0;
    bool ok = expect(readDouble(" \t+1.2345e3 \t", value) && value == 1234.5,
                     "typed reader rejected dot-decimal/sign/exponent whitespace")
        && expect(readDouble("-0", value) && std::signbit(value), "signed zero changed")
        && expect(readDouble("1e-9999", value) && value == 0,
                  "typed finite-underflow policy changed")
        && expect(readDouble("4.9406564584124654e-324", value)
                      && value == std::numeric_limits<double>::denorm_min(),
                  "typed subnormal-underflow policy changed")
        && expect(readDouble("0x1.4p+1", value) && value == 2.5,
                  "existing C-strtod hexadecimal compatibility changed");
    for (const char* token : {"1,25", "nan", "inf", "1e9999", "1.5trailing", ""})
        ok = expect(!readDouble(token, value), "malformed/nonfinite/overflow numeric token accepted") && ok;
    ok = expect(!readDouble(std::string("1.5\0trailing", 12), value),
                "embedded NUL bypassed full numeric token consumption") && ok;
    std::istringstream integers(" +70 \n +65535 \t\n90\n-2147483648\n160\n9223372036854775807\n290\n2\n");
    dxfReaderAscii reader(&integers);
    int code = 0;
    ok = expect(reader.readRec(&code) && code == 70 && reader.getInt32() == 65535,
                "integer16 unsigned-pattern/sign/whitespace policy changed") && ok;
    ok = expect(reader.readRec(&code) && code == 90 && reader.getInt32() == -2147483647 - 1,
                "integer32 boundary policy changed") && ok;
    ok = expect(reader.readRec(&code) && code == 160
                    && reader.getInt64() == std::numeric_limits<std::int64_t>::max(),
                "integer64 boundary policy changed") && ok;
    ok = expect(reader.readRec(&code) && code == 290 && reader.getInt32() == 2,
                "existing permissive typed Boolean policy changed") && ok;
    std::istringstream invalidInteger(std::string("90\n12\0trailing\n", 15));
    dxfReaderAscii invalidReader(&invalidInteger);
    ok = expect(!invalidReader.readRec(&code),
                "embedded NUL bypassed full integer token consumption") && ok;
    return ok;
}

bool testFiniteNumericToken() {
    double value = 17.0;
    bool ok = expect(DxfNumeric::parseFiniteToken(" \t+1.25e1 \r", value)
                         && value == 12.5,
                     "strict numeric token rejected surrounding whitespace")
        && expect(DxfNumeric::parseFiniteToken("0x1.4p+1", value)
                      && value == 2.5,
                  "strict numeric token changed existing C-strtod compatibility");
    for (const std::string& token : {
             std::string("2.25tail"), std::string("1,25"),
             std::string("nan"), std::string("inf"),
             std::string("1e9999"), std::string("1e-9999"),
             std::string("2.25\0tail", 9u)}) {
        value = 17.0;
        ok = expect(!DxfNumeric::parseFiniteToken(token, value)
                        && value == 17.0,
                    "strict numeric token accepted a suffix or invalid range")
            && ok;
    }
    const double subnormal = std::numeric_limits<double>::denorm_min();
    return expect(DxfNumeric::parseFiniteToken(
                      "4.9406564584124654e-324", value)
                      && value == subnormal,
                  "strict numeric token rejected a representable subnormal")
        && ok;
}

bool testOutput() {
    const std::locale comma(std::locale::classic(), new CommaPunctuation);
    std::ostringstream stream;
    stream.imbue(comma);
    stream.precision(7);
    stream.width(9);
    const auto flags = stream.flags();
    dxfWriterAscii writer(&stream);
    bool ok = writer.writeDouble(10, 1234.5)
        && writer.writeInt32(90, 1234567)
        && writer.writeString(1001, "LOCAL_APP")
        && writer.writeBool(290, true)
        && writer.writeDouble(20, std::nextafter(1.0, 2.0));
    ok = expect(ok && stream.str() == " 10\n1234.5\n 90\n1234567\n1001\nLOCAL_APP\n290\n1\n 20\n1.0000000000000002\n",
                "ASCII numeric output inherited decimal/grouping punctuation or lost binary64 precision") && ok;
    ok = expect(stream.getloc() == comma && stream.flags() == flags
                    && stream.precision() == 7 && stream.width() == 9,
                "ASCII writer changed caller stream locale/format state") && ok;

    const struct {
        int code;
        double value;
    } values[] = {
        {10, 2419318.197152937},
        {20, -3154161.355029621},
        {30, std::nextafter(1.0, 2.0)},
        {40, std::nextafter(-1.0, -2.0)},
        {41, (std::numeric_limits<double>::max)()},
        {42, (std::numeric_limits<double>::min)()},
        {43, std::numeric_limits<double>::denorm_min()},
        {44, -0.0}
    };
    std::ostringstream coordinateStream;
    dxfWriterAscii coordinateWriter(&coordinateStream);
    bool coordinateOk = true;
    for (const auto& value : values)
        coordinateOk = coordinateWriter.writeDouble(value.code, value.value) && coordinateOk;
    std::istringstream coordinateInput(coordinateStream.str());
    dxfReaderAscii coordinateReader(&coordinateInput);
    for (const auto& value : values) {
        int code = 0;
        const bool read = coordinateReader.readRec(&code);
        const double actual = coordinateReader.getDouble();
        coordinateOk = read && code == value.code && actual == value.value
            && std::signbit(actual) == std::signbit(value.value) && coordinateOk;
    }
    ok = expect(coordinateOk,
                "ASCII DXF changed a finite double during writer/reader round trip") && ok;

    std::ostringstream ordinary, localized;
    localized.imbue(comma);
    dxfWriterBinary binaryA(&ordinary), binaryB(&localized);
    ok = binaryA.writeDouble(10, 1234.5) && binaryA.writeInt32(90, 1234567)
        && binaryB.writeDouble(10, 1234.5) && binaryB.writeInt32(90, 1234567) && ok;
    ok = expect(ordinary.str() == localized.str() && localized.getloc() == comma,
                "binary numeric output or caller locale changed") && ok;
    std::ostringstream binaryBoundary;
    dxfWriterBinary boundaryWriter(&binaryBoundary);
    return expect(boundaryWriter.writeDouble(30, std::nextafter(1.0, 2.0))
                      && binaryBoundary.str() == std::string("\x1e\x00\x01\x00\x00\x00\x00\x00\xf0\x3f", 10),
                  "binary64 boundary encoding changed") && ok;
}

class RawSink final : public dx_iface {
public:
    explicit RawSink(std::string value) : token(std::move(value)) {}
    void writeObjects() override {
        DRW_RawDxfObject raw;
        raw.name = "LOCAL_NUMERIC"; raw.handle = 0xA0; raw.m_version = DRW::AC1027;
        raw.hasRawValues = true;
        const double typedValue = token == "1e-9999" ? 0.0
            : token == "4.9406564584124654e-324"
                ? std::numeric_limits<double>::denorm_min() : 1234.5;
        raw.groups = {DRW_Variant(5, std::string("A0")), DRW_Variant(10, typedValue)};
        raw.rawValues = {"A0", token};
        dxfW->writeRawDxfObject(&raw);
    }
private:
    std::string token;
};

bool testRawInput(const std::filesystem::path& directory) {
    unsigned index = 0;
    for (const std::string& token : {
             std::string(" +1.234500e3 "), std::string("1e-9999"),
             std::string("4.9406564584124654e-324"), std::string("1,25"),
             std::string("1e9999")}) {
        const auto path = directory / ("raw-" + std::to_string(index++) + ".dxf");
        const std::string sentinel = "original destination\n";
        { std::ofstream stream(path); stream << sentinel; }
        dx_data data;
        RawSink sink(token);
        const bool written = sink.fileExport(path.string(), DRW::AC1027, false, &data, false);
        std::ifstream stream(path);
        const std::string bytes((std::istreambuf_iterator<char>(stream)), {});
        const bool finiteUnderflow = token == "1e-9999"
            || token == "4.9406564584124654e-324";
        const bool lexicalPositive = token.front() == ' ';
        if (lexicalPositive || finiteUnderflow) {
            if (!expect(written && bytes.find(token + "\n") != std::string::npos,
                        "raw numeric validation rejected/rewrote valid finite underflow spelling")) return false;
        } else if (!expect(!written && bytes == sentinel,
                           "raw invalid/overflow policy changed or failed output replaced destination")) return false;
    }
    return true;
}

class ArcText final : public DRW_ArcAlignedText {
public:
    using DRW_ArcAlignedText::parseCode;
    using DRW_ArcAlignedText::encodeDwg;
};

class ArcSink final : public dx_iface {
public:
    explicit ArcSink(std::string textSize = "2.5",
                     std::string xScale = "1.25")
        : textSize_(std::move(textSize)), xScale_(std::move(xScale)) {}

    void setTextSize(std::string textSize) {
        textSize_ = std::move(textSize);
    }

    void writeEntities() override {
        DRW_ArcAlignedText text;
        text.text = "locale sentinel"; text.m_radius = 12.5;
        text.m_textSize = textSize_; text.m_xScale = xScale_;
        dxfW->writeArcAlignedText(&text);
    }

private:
    std::string textSize_;
    std::string xScale_;
};

bool testStringBackedNumeric(const std::filesystem::path& directory) {
    std::stringstream bytes;
    dxfWriterBinary writer(&bytes);
    if (!writer.writeDouble(42, 2.5)) return false;
    std::unique_ptr<dxfReader> reader = std::make_unique<dxfReaderBinary>(&bytes);
    int code = 0;
    ArcText text;
    if (!reader->readRec(&code) || !text.parseCode(code, reader)) return false;
    text.applyArcApproximation();
    if (!expect(text.m_textSize == "2.5" && text.height == 2.5,
                "ARCALIGNEDTEXT string-backed numeric formatting/parsing depends on locale")) return false;
    dx_data data;
    ArcSink sink;
    const auto path = directory / "arc text.dxf";
    if (!sink.fileExport(path.string(), DRW::AC1027, false, &data, false)) return false;
    std::ifstream input(path);
    dxfReaderAscii output(&input);
    bool arc = false, foundSize = false, foundScale = false;
    output.setAllowDimstyleNames(true);
    while (output.readRec(&code)) {
        if (code == 0) arc = output.getString() == "ARCALIGNEDTEXT";
        else if (arc && code == 42) foundSize = output.getDouble() == 2.5;
        else if (arc && code == 41) foundScale = output.getDouble() == 1.25;
    }
    if (!expect(foundSize && foundScale,
                "ARCALIGNEDTEXT writer truncated dot-decimal string fields"))
        return false;

    const auto rangePath = directory / "arc text range.dxf";
    const std::string sentinel = "preserve D2T destination on underflow refusal";
    {
        std::ofstream output(rangePath, std::ios::binary | std::ios::trunc);
        output.write(sentinel.data(),
                     static_cast<std::streamsize>(sentinel.size()));
        if (!output)
            return expect(false,
                "ARCALIGNEDTEXT D2T range sentinel setup failed");
    }
    ArcSink rangeSink("1e-9999", "1.25");
    if (rangeSink.fileExport(rangePath.string(), DRW::AC1027, false, &data,
                             false))
        return expect(false,
            "ARCALIGNEDTEXT D2T underflow-to-zero output was admitted");
    {
        std::ifstream preserved(rangePath, std::ios::binary);
        const std::string contents(
            (std::istreambuf_iterator<char>(preserved)),
            std::istreambuf_iterator<char>());
        if (!expect(contents == sentinel,
                    "ARCALIGNEDTEXT D2T refusal changed its destination"))
            return false;
    }

    const double subnormal = std::numeric_limits<double>::denorm_min();
    rangeSink.setTextSize(DxfNumeric::formatDouble(subnormal));
    if (!rangeSink.fileExport(rangePath.string(), DRW::AC1027, false, &data,
                              false))
        return expect(false,
            "ARCALIGNEDTEXT D2T subnormal corrected retry failed");
    {
        std::ifstream input(rangePath);
        dxfReaderAscii output(&input);
        int code = 0;
        bool inArcText = false;
        bool foundSubnormal = false;
        while (output.readRec(&code)) {
            if (code == 0) {
                inArcText = output.getString() == "ARCALIGNEDTEXT";
            } else if (inArcText && code == 42) {
                foundSubnormal = output.getDouble() == subnormal;
                break;
            }
        }
        if (!expect(foundSubnormal,
                    "ARCALIGNEDTEXT D2T subnormal DXF readback changed value"))
            return false;
    }

    const std::array<std::pair<int, double>, 6> d2tFields{{
        {41, std::nextafter(1.0, 2.0)},
        {42, std::nextafter(2.0, 3.0)},
        {43, std::nextafter(0.75, 1.0)},
        {44, -std::numeric_limits<double>::max()},
        {45, std::nextafter(-1.0, 0.0)},
        {46, std::nextafter(16.0, 17.0)}}};
    for (const auto& field : d2tFields) {
        std::stringstream encoded(std::ios::in | std::ios::out
                                  | std::ios::binary);
        dxfWriterBinary encodedWriter(&encoded);
        if (!encodedWriter.writeDouble(field.first, field.second))
            return expect(false, "ARCALIGNEDTEXT D2T precision input encode failed");

        std::unique_ptr<dxfReader> fieldReader =
            std::make_unique<dxfReaderBinary>(&encoded);
        int fieldCode = 0;
        if (!fieldReader->readRec(&fieldCode) || fieldCode != field.first)
            return expect(false, "ARCALIGNEDTEXT D2T precision input read failed");

        ArcText parsed;
        std::shared_ptr<DxfReadRetainedBudget> budget;
        if (field.first == 44) {
            // Formatting reserves the 24-byte worst-case bound and transfers
            // that reservation into the retained value without double charge.
            // One byte less must still reject before changing the field.
            auto shortBudget = std::make_shared<DxfReadRetainedBudget>(23u);
            fieldReader->setRetainedByteBudget(shortBudget);
            if (parsed.parseCode(fieldCode, fieldReader)
                || !parsed.m_offsetFromArc.empty()
                || !shortBudget->exceeded() || shortBudget->used() != 0u)
                return expect(false,
                    "ARCALIGNEDTEXT D2T short-budget input was not refused atomically");
            budget = std::make_shared<DxfReadRetainedBudget>(24u);
            fieldReader->setRetainedByteBudget(budget);
        }
        if (!parsed.parseCode(fieldCode, fieldReader))
            return expect(false, "ARCALIGNEDTEXT D2T precision retry failed");

        const UTF8STRING* spelling = nullptr;
        switch (field.first) {
        case 41: spelling = &parsed.m_xScale; break;
        case 42: spelling = &parsed.m_textSize; break;
        case 43: spelling = &parsed.m_charSpacing; break;
        case 44: spelling = &parsed.m_offsetFromArc; break;
        case 45: spelling = &parsed.m_rightOffset; break;
        case 46: spelling = &parsed.m_leftOffset; break;
        default: break;
        }
        double recovered = 0.0;
        if (spelling == nullptr
            || !DxfNumeric::parseFinitePrefix(*spelling, recovered)
            || recovered != field.second)
            return expect(false,
                "ARCALIGNEDTEXT D2T conversion lost a finite binary64 value");
        if (field.first == 44
            && (!expect(spelling->size() == 24u,
                        "ARCALIGNEDTEXT D2T worst-case spelling changed")
                || budget == nullptr || budget->used() != spelling->size()))
            return expect(false,
                "ARCALIGNEDTEXT D2T exact retained-byte accounting failed");
    }

    std::stringstream subnormalInput(std::ios::in | std::ios::out
                                     | std::ios::binary);
    dxfWriterBinary subnormalWriter(&subnormalInput);
    if (!subnormalWriter.writeDouble(42, subnormal))
        return expect(false,
            "ARCALIGNEDTEXT subnormal input encode failed");
    std::unique_ptr<dxfReader> subnormalReader =
        std::make_unique<dxfReaderBinary>(&subnormalInput);
    int subnormalCode = 0;
    ArcText subnormalText;
    if (!subnormalReader->readRec(&subnormalCode)
        || subnormalCode != 42
        || !subnormalText.parseCode(subnormalCode, subnormalReader))
        return expect(false,
            "ARCALIGNEDTEXT typed subnormal field parse failed");
    subnormalText.applyArcApproximation();
    if (!expect(subnormalText.height == subnormal,
                "ARCALIGNEDTEXT approximation dropped a representable subnormal height"))
        return false;

    ArcText dwgText;
    dwgText.handle = 0x42u;
    dwgText.text = "D2T precision";
    dwgText.height = std::nextafter(2.0, 3.0);
    dwgText.widthscale = 1.0;
    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    if (!dwgText.encodeDwg(DRW::AC1027, &body, 0u, &strings, &handles))
        return expect(false, "ARCALIGNEDTEXT DWG D2T fallback encode failed");
    DRW_TextCodec codec;
    codec.setVersion("AC1027", false);
    dwgBuffer stringReader(strings.data().data(), strings.data().size(),
                           &codec);
    const UTF8STRING encodedTextSize =
        stringReader.getVariableText(DRW::AC1027);
    double encodedHeight = 0.0;
    if (!stringReader.isGood()
        || !DxfNumeric::parseFinitePrefix(encodedTextSize, encodedHeight)
        || encodedHeight != dwgText.height)
        return expect(false,
            "ARCALIGNEDTEXT DWG D2T fallback lost binary64 precision");
    return true;
}

bool generate(const std::string& path, bool binary) {
    std::ofstream stream(path, std::ios::binary);
    std::unique_ptr<dxfWriter> writer;
    if (binary) {
        stream.write("AutoCAD Binary DXF\r\n\x1a\0", 22);
        writer = std::make_unique<dxfWriterBinary>(&stream);
    } else writer = std::make_unique<dxfWriterAscii>(&stream);
    writer->setVersion("AC1027", true);
    writer->writeString(0, "SECTION"); writer->writeString(2, "HEADER");
    writer->writeString(9, "$ACADVER"); writer->writeString(1, "AC1027");
    writer->writeString(9, "$LTSCALE"); writer->writeDouble(40, 1234.5);
    writer->writeString(0, "ENDSEC"); writer->writeString(0, "SECTION");
    writer->writeString(2, "ENTITIES"); writer->writeString(0, "LINE");
    writer->writeString(5, "A0"); writer->writeString(330, "0");
    writer->writeString(100, "AcDbEntity"); writer->writeString(8, "0");
    writer->writeString(100, "AcDbLine");
    writer->writeDouble(10, 2419318.197152937); writer->writeDouble(20, -3154161.355029621);
    writer->writeDouble(30, std::nextafter(1.0, 2.0)); writer->writeDouble(11, -4.5);
    writer->writeDouble(21, 5.25); writer->writeDouble(31, -6.75);
    writer->writeString(0, "ENDSEC"); writer->writeString(0, "EOF");
    return !writer->hasWriteError() && stream.good();
}

bool verify(const std::string& path) {
    dx_data data;
    dx_iface iface;
    if (!iface.fileImport(path, &data, false)) return false;
    auto scale = data.headerC.vars.find("$LTSCALE");
    if (scale == data.headerC.vars.end()) scale = data.headerC.vars.find("LTSCALE");
    if (!expect(scale != data.headerC.vars.end() && scale->second != nullptr
                    && scale->second->type() == DRW_Variant::DOUBLE
                    && scale->second->d_val() == 1234.5
                    && data.mBlock->ent.size() == 1, "header/coordinate control changed")) return false;
    const auto* line = dynamic_cast<DRW_Line*>(data.mBlock->ent.front());
    return expect(line && line->basePoint.x == 2419318.197152937
                      && line->basePoint.y == -3154161.355029621
                      && line->basePoint.z == std::nextafter(1.0, 2.0)
                      && line->secPoint.x == -4.5
                      && line->secPoint.y == 5.25 && line->secPoint.z == -6.75,
                  "coordinate sentinel changed");
}

bool run(const std::filesystem::path& directory) {
    bool ok = testOutput();
    ok = testTypedInput() && ok;
    ok = testFiniteNumericToken() && ok;
    ok = testRawInput(directory) && ok;
    ok = testStringBackedNumeric(directory) && ok;
    for (bool binary : {false, true}) {
        const auto path = directory / (binary ? "header binary.dxf" : "header ascii.dxf");
        ok = generate(path.string(), binary) && verify(path.string()) && ok;
    }
    const std::locale previous = std::locale();
    const std::string numericBefore = std::setlocale(LC_NUMERIC, nullptr);
    const std::locale globalComma(std::locale::classic(), new CommaPunctuation);
    std::locale::global(globalComma);
    const auto localized = directory / "global C++ locale header.dxf";
    dx_data localizedData;
    dx_iface localizedIface;
    const auto localizedOutput = directory / "global C++ locale output.dxf";
    const bool localizedOk = generate(localized.string(), false) && verify(localized.string())
        && localizedIface.fileImport(localized.string(), &localizedData, false)
        && localizedIface.fileExport(localizedOutput.string(), DRW::AC1027, false,
                                     &localizedData, false)
        && verify(localizedOutput.string())
        && expect(std::locale() == globalComma
                      && std::setlocale(LC_NUMERIC, nullptr) == numericBefore,
                  "library changed a caller global locale during header conversion");
    std::locale::global(previous);
    // Restoring a named C++ locale is specified to set the C locale too.
    // Restore this test's independent LC_NUMERIC override, not the library's.
    std::setlocale(LC_NUMERIC, numericBefore.c_str());
    ok = localizedOk && ok;
    return ok;
}

} // namespace

int main(int argc, char** argv) {
    if (argc == 3) {
        const std::string mode = argv[1];
        if (mode == "--generate") return generate(argv[2], false) ? 0 : 1;
        if (mode == "--generate-binary") return generate(argv[2], true) ? 0 : 1;
        if (mode == "--verify") return verify(argv[2]) ? 0 : 1;
        return 2;
    }
    const std::string originalLocale = std::setlocale(LC_NUMERIC, nullptr);
    const bool processLocale = argc == 2 && std::string(argv[1]) == "--process-locale";
    if (processLocale) {
        bool available = false;
        for (const char* name : {"de_DE.UTF-8", "de_DE.utf8", "German_Germany.1252", "fr_FR.UTF-8"}) {
            if (std::setlocale(LC_NUMERIC, name) && std::localeconv()->decimal_point[0] == ',') {
                available = true;
                break;
            }
        }
        if (!available) {
            std::setlocale(LC_NUMERIC, originalLocale.c_str());
            std::cout << "SKIP: optional comma-decimal OS locale is unavailable\n";
            return 77;
        }
    }
    const std::string activeLocale = std::setlocale(LC_NUMERIC, nullptr);
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-numeric-locale-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    bool ok = run(directory);
    ok = expect(std::setlocale(LC_NUMERIC, nullptr) == activeLocale,
                "library mutated the caller process locale") && ok;
    std::setlocale(LC_NUMERIC, originalLocale.c_str());
    if (ok) std::filesystem::remove_all(directory);
    else std::cerr << "Evidence: " << directory << '\n';
    return ok ? 0 : 1;
}
