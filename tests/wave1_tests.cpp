#include <cstdint>
#include <chrono>
#include <array>
#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <functional>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <streambuf>
#include <vector>

#include "drw_base.h"
#include "drw_header.h"
#include "handle_allocator.h"
#define private public
#include "libdxfrw.h"
#include "dx_iface.h"
#undef private
#include "intern/drw_textcodec.h"
#include "intern/dwg_fixed_handles.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/dxfcode.h"
#include "intern/dwgreaderR11.h"
#include "intern/dwgutil.h"
#include "intern/dxfparserlimits.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"
#include "dx_standard_output.h"
#include "intern/rscodec.h"

// Link the production adapter: inherited publication callbacks and the
// write/short-append probes require its actual implementation.

// Keep access to the protected header codec limited to this test translation
// unit. The production API remains unchanged.
class DwgHandseedTestAccess {
public:
    static bool encode(DRW_Header& header, DRW::Version version,
                       dwgBufferW* data, dwgBufferW* handles) {
        return header.encodeDwg(version, data, handles);
    }
};

namespace {

struct TestContext {
    int failures {0};

    void expect(bool condition, const char* label) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << label << '\n';
        }
    }
};

class RejectingStreambuf final : public std::streambuf {
public:
    std::size_t acceptedBytes() const { return 0; }

protected:
    std::streamsize xsputn(const char*, std::streamsize) override { return 0; }

    int_type overflow(int_type) override { return traits_type::eof(); }
};

class ProfileProbeInterface final : public dx_iface {
public:
    ProfileProbeInterface() {
        cData = &storage;
        currentBlock = storage.mBlock;
    }

    void addRawDxfObject(const DRW_RawDxfObject& data) override {
        objects.push_back(data);
    }

    void addRawDxfSection(const DRW_RawDxfSection& data) override {
        sections.push_back(data);
    }

    void addRawDxfEntity(const DRW_RawDxfObject& data) override {
        entities.push_back(data);
    }

    dx_data storage;
    std::vector<DRW_RawDxfObject> objects;
    std::vector<DRW_RawDxfSection> sections;
    std::vector<DRW_RawDxfObject> entities;
};

std::vector<std::uint8_t> literalRunHeader(std::uint32_t count) {
    std::vector<std::uint8_t> result;
    std::uint32_t remainder = count - 3u - 0x0Fu;
    result.push_back(0x00);
    while (remainder > 0xFFu) {
        result.push_back(0x00);
        remainder -= 0xFFu;
    }
    result.push_back(static_cast<std::uint8_t>(remainder));
    return result;
}

void testBufferRoundTrip(TestContext& t) {
    dwgBufferW writer;
    writer.putBoolBit(true);
    writer.putBit(0);
    writer.put2Bits(2);
    writer.put3Bits(5);
    writer.putBitShort(0x1234);
    writer.putSBitShort(-42);
    writer.putBitLong(-123456);
    writer.alignToByte();
    const std::size_t rawStart = writer.size();
    writer.putRawChar8(0xA5);
    writer.putRawShort16(0xBEEF);
    writer.putRawLong32(0x12345678u);
    writer.putRawLong64(0x0123456789ABCDEFULL);
    writer.putRawDouble(3.5);
    dwgHandle expected;
    expected.code = 4;
    expected.ref = 0x1234;
    writer.putHandle(expected);

    auto bytes = writer.data();
    dwgBuffer reader(bytes.data(), bytes.size());
    t.expect(reader.getBoolBit(), "buffer bool bit");
    t.expect(reader.getBit() == 0, "buffer bit");
    t.expect(reader.get2Bits() == 2, "buffer two bits");
    t.expect(reader.get3Bits() == 5, "buffer three bits");
    t.expect(reader.getBitShort() == 0x1234, "buffer bit short");
    t.expect(reader.getSBitShort() == -42, "buffer signed bit short");
    t.expect(reader.getBitLong() == -123456, "buffer bit long");
    reader.setPosition(rawStart);
    t.expect(reader.getRawChar8() == 0xA5, "buffer raw char");
    t.expect(reader.getRawShort16() == 0xBEEF, "buffer raw short");
    t.expect(reader.getRawLong32() == 0x12345678u, "buffer raw long");
    t.expect(reader.getRawLong64() == 0x0123456789ABCDEFULL,
             "buffer raw long long");
    t.expect(reader.getRawDouble() == 3.5, "buffer raw double");
    const dwgHandle actual = reader.getHandle();
    t.expect(actual.code == 4 && actual.ref == 0x1234 && actual.size == 2,
             "buffer handle");
    t.expect(reader.isGood(), "buffer round trip remains good");
}

void testTextAndPreR13(TestContext& t) {
    t.expect(dxfValueKindForCode(79) == DxfValueKind::I16
                 && dxfValueKindForCode(80) == DxfValueKind::Unknown
                 && dxfValueKindForCode(89) == DxfValueKind::Unknown
                 && dxfValueKindForCode(90) == DxfValueKind::I32,
             "DXF undocumented 80-89 span is unknown between assigned integer ranges");
    t.expect(dxfValueKindForCode(260) == DxfValueKind::Unknown
                 && dxfValueKindForCode(269) == DxfValueKind::Unknown,
             "DXF undocumented 260-269 span remains unclassified");
    t.expect(dxfValueKindForCode(482) == DxfValueKind::Unknown
                 && dxfValueKindForCode(998) == DxfValueKind::Unknown,
             "DXF unassigned 482-998 span is unknown");
    t.expect(dxfValueKindForCode(999) == DxfValueKind::Str
                 && dxfValueKindForCode(1004) == DxfValueKind::Bin,
             "DXF comment and binary chunk boundaries are explicit");
    std::stringstream opaqueAsciiRecords("80\nlegacy-looking\n89\n3.5\n");
    dxfReaderAscii opaqueAsciiReader(&opaqueAsciiRecords);
    int opaqueCode = 0;
    t.expect(opaqueAsciiReader.readRec(&opaqueCode) && opaqueCode == 80
                 && opaqueAsciiReader.type == dxfReader::STRING
                 && opaqueAsciiReader.getRawValue() == "legacy-looking"
                 && opaqueAsciiReader.readRec(&opaqueCode) && opaqueCode == 89
                 && opaqueAsciiReader.type == dxfReader::STRING
                 && opaqueAsciiReader.getRawValue() == "3.5",
             "DXF standalone ASCII retains unknown 80-89 value spellings");
    std::stringstream records("260\n2147483647\n482\n3.14\n999\ncomment\n1004\nAB\n1071\n-7\n");
    dxfReaderAscii reader(&records);
    int code = 0;
    t.expect(reader.readRec(&code) && code == 260
                 && reader.type == dxfReader::STRING
                 && reader.getRawValue() == "2147483647",
             "DXF standalone ASCII preserves unknown code 260 spelling");
    t.expect(reader.readRec(&code) && code == 482
                 && reader.type == dxfReader::STRING
                 && reader.getString() == "3.14"
                 && reader.getRawValue() == "3.14",
             "DXF ASCII reader preserves unknown code source spelling");
    t.expect(reader.readRec(&code) && code == 999
                 && reader.type == dxfReader::STRING
                 && reader.getString() == "comment",
             "DXF ASCII reader retains comment records");
    t.expect(reader.readRec(&code) && code == 1004
                 && reader.type == dxfReader::BINARY,
             "DXF ASCII reader recognizes binary chunk boundary");
    t.expect(reader.readRec(&code) && code == 1071
                 && reader.type == dxfReader::INT32 && reader.getInt32() == -7,
             "DXF ASCII reader preserves code 1071 as INT32");
    DRW_TextCodec codec;
    codec.setCodePage("ANSI_1252", false);
    t.expect(codec.fromUtf8("\xE4\xB8\x80") == "\\U+4E00",
             "text codec four-digit escape");
    t.expect(codec.fromUtf8("\xF0\xA0\x80\xA1") == "?",
             "text codec supplementary fallback");

    std::vector<std::uint8_t> field(32, 0);
    field[0] = 'A';
    field[1] = 'B';
    dwgBuffer buffer(field.data(), field.size());
    t.expect(preR13FixedText(buffer, 32, codec) == "AB",
             "pre-R13 fixed text decode");
    t.expect(buffer.getPosition() == 32, "pre-R13 fixed text consumes width");
    t.expect(preR13SectionSize(0x41585E43u) == 0x01585E43u,
             "pre-R13 30-bit section size");
    t.expect(preR13StyleRecordMinSize(DRW::AC1009) == 196,
             "pre-R13 R11 style width");
    const PreR13VertexLayout layout = preR13VertexLayout(0x0008);
    t.expect(layout.hasPoint && layout.hasFlag && !layout.hasBulge,
             "pre-R13 vertex layout");
}

void testDxfClassifierBoundaryMatrix(TestContext& t) {
    t.expect(dxfCodeRangesAreCanonical(),
             "DXF classifier ranges are contiguous and complete");
    t.expect(dxfValueKindForCode(79) == DxfValueKind::I16
                 && dxfValueKindForCode(80) == DxfValueKind::Unknown
                 && dxfValueKindForCode(89) == DxfValueKind::Unknown
                 && dxfValueKindForCode(90) == DxfValueKind::I32,
             "DXF 79-90 boundary leaves undocumented 80-89 unclassified");
    t.expect(dxfValueKindForCode(149) == DxfValueKind::Dbl
                 && dxfValueKindForCode(150) == DxfValueKind::Unknown
                 && dxfValueKindForCode(159) == DxfValueKind::Unknown
                 && dxfValueKindForCode(160) == DxfValueKind::I64,
             "DXF 149-160 boundary leaves undocumented 150-159 unclassified");
    t.expect(dxfValueKindForCode(179) == DxfValueKind::I16
                 && dxfValueKindForCode(180) == DxfValueKind::Unknown
                 && dxfValueKindForCode(209) == DxfValueKind::Unknown
                 && dxfValueKindForCode(210) == DxfValueKind::Dbl,
             "DXF 179-210 boundary leaves undocumented 180-209 unclassified");
    t.expect(dxfValueKindForCode(239) == DxfValueKind::Dbl
                 && dxfValueKindForCode(240) == DxfValueKind::Unknown
                 && dxfValueKindForCode(242) == DxfValueKind::Unknown
                 && dxfValueKindForCode(259) == DxfValueKind::Unknown
                 && dxfValueKindForCode(260) == DxfValueKind::Unknown
                 && dxfValueKindForCode(269) == DxfValueKind::Unknown
                 && dxfValueKindForCode(270) == DxfValueKind::I16,
             "DXF 239-270 leaves undocumented 240-269 unclassified");
    t.expect(dxfValueKindForCode(481) == DxfValueKind::Str
                 && dxfValueKindForCode(482) == DxfValueKind::Unknown
                 && dxfValueKindForCode(998) == DxfValueKind::Unknown
                 && dxfValueKindForCode(999) == DxfValueKind::Str,
             "DXF 481-999 unknown span is non-overlapping");
    t.expect(dxfValueKindForCode(1003) == DxfValueKind::Str
                 && dxfValueKindForCode(1004) == DxfValueKind::Bin
                 && dxfValueKindForCode(1005) == DxfValueKind::Str
                 && dxfValueKindForCode(1006) == DxfValueKind::Unknown
                 && dxfValueKindForCode(1009) == DxfValueKind::Unknown
                 && dxfValueKindForCode(1010) == DxfValueKind::Dbl
                 && dxfValueKindForCode(1059) == DxfValueKind::Dbl
                 && dxfValueKindForCode(1071) == DxfValueKind::I32
                 && dxfValueKindForCode(1072) == DxfValueKind::Unknown,
             "DXF binary/XDATA boundaries are explicit");
    t.expect(dxfValueKindForCode(240, DxfClassifierProfile::StandaloneSafe)
                 == DxfValueKind::Unknown
                 && dxfValueKindForCode(260, DxfClassifierProfile::StandaloneSafe)
                 == DxfValueKind::Unknown
                 && dxfValueKindForCode(80,
                                        DxfClassifierProfile::StandaloneSafe)
                        == DxfValueKind::Unknown
                 && dxfValueKindForCode(150,
                                        DxfClassifierProfile::StandaloneSafe)
                        == DxfValueKind::Unknown
                 && dxfValueKindForCode(180,
                                        DxfClassifierProfile::StandaloneSafe)
                        == DxfValueKind::Unknown
                 && dxfValueKindForCode(482,
                                        DxfClassifierProfile::StandaloneSafe)
                        == DxfValueKind::Unknown,
             "standalone-safe classifier profile remains the default");
    t.expect(dxfValueKindForCode(260,
                                 DxfClassifierProfile::LibreCadMasterLegacy)
                 == DxfValueKind::Bln
                 && dxfValueKindForCode(
                        269, DxfClassifierProfile::LibreCadMasterLegacy)
                        == DxfValueKind::Bln
                 && dxfValueKindForCode(
                        240, DxfClassifierProfile::LibreCadMasterLegacy)
                        == DxfValueKind::Dbl
                 && dxfValueKindForCode(
                        259, DxfClassifierProfile::LibreCadMasterLegacy)
                        == DxfValueKind::Dbl
                 && dxfValueKindForCode(
                        80, DxfClassifierProfile::LibreCadMasterLegacy)
                        == DxfValueKind::Str
                 && dxfValueKindForCode(
                        150, DxfClassifierProfile::LibreCadMasterLegacy)
                        == DxfValueKind::Str
                 && dxfValueKindForCode(
                        180, DxfClassifierProfile::LibreCadMasterLegacy)
                        == DxfValueKind::Str
                 && dxfValueKindForCode(
                        209, DxfClassifierProfile::LibreCadMasterLegacy)
                        == DxfValueKind::Str
                 && dxfValueKindForCode(
                        482, DxfClassifierProfile::LibreCadMasterLegacy)
                        == DxfValueKind::Dbl
                 && dxfValueKindForCode(
                        1009, DxfClassifierProfile::LibreCadMasterLegacy)
                        == DxfValueKind::Str,
             "target legacy classifier profile is explicit and opt-in");

    std::stringstream records(
        "149\n9.5\n150\nopaque150\n159\nopaque159\n"
        "160\n1234567890123\n179\n7\n180\nopaque180\n"
        "209\nopaque209\n210\n2.5\n"
        "240\nopaque240\n242\nopaque242\n259\nopaque259\n"
        "260\n2147483647\n269\n-7\n270\n7\n"
        "481\nABCD\n482\n3.14\n998\nopaque-after\n"
        "999\ncomment\n1004\nAB\n1005\nABC\n"
        "1009\nxdata-string\n1010\n1.25\n1059\n2.5\n1071\n9\n");
    dxfReaderAscii reader(&records);
    int code = 0;
    const std::vector<dxfReader::TYPE> expectedTypes {
        dxfReader::DOUBLE, dxfReader::STRING, dxfReader::STRING,
        dxfReader::INT64, dxfReader::INT32, dxfReader::STRING,
        dxfReader::STRING, dxfReader::DOUBLE,
        dxfReader::STRING, dxfReader::STRING, dxfReader::STRING,
        dxfReader::STRING, dxfReader::STRING,
        dxfReader::INT32, dxfReader::STRING, dxfReader::STRING,
        dxfReader::STRING, dxfReader::STRING, dxfReader::BINARY,
        dxfReader::STRING, dxfReader::STRING, dxfReader::DOUBLE,
        dxfReader::DOUBLE, dxfReader::INT32};
    std::size_t index = 0;
    while (reader.readRec(&code)) {
        t.expect(index < expectedTypes.size() && reader.type == expectedTypes[index],
                 "DXF parser boundary type agrees with classifier");
        if (code == 150 || code == 159 || code == 180 || code == 209
            || code == 240 || code == 242 || code == 259)
            t.expect(reader.getRawValue().rfind("opaque", 0) == 0,
                     "DXF ASCII preserves opaque spelling for unassigned ranges");
        if (code == 260 || code == 269)
            t.expect(reader.getRawValue() == (code == 260 ? "2147483647" : "-7"),
                     "DXF ASCII preserves numeric-looking unknown spelling");
        if (code == 1009)
            t.expect(reader.getRawValue() == "xdata-string",
                     "DXF ASCII preserves the unassigned code-1009 spelling");
        ++index;
    }
    t.expect(index == expectedTypes.size(),
             "DXF parser consumes every boundary vector");

    std::stringstream capturedRecords(
        "260\n2147483647\n482\n3.14\n998\nopaque\n"
        "1009\nxdata-string\n1010\n1.25\n");
    dxfRW owner("");
    owner.binFile = false;
    owner.reader = std::make_unique<dxfReaderAscii>(&capturedRecords);
    DRW_RawDxfObject object;
    index = 0;
    while (owner.reader->readRec(&code)) {
        t.expect(owner.captureRawGroup(object, code),
                 "raw capture accepts classifier boundary vector");
        ++index;
    }
    t.expect(index == 5 && object.groups.size() == 5
                 && object.groups[0].type() == DRW_Variant::STRING
                 && object.groups[1].type() == DRW_Variant::STRING
                 && object.groups[2].type() == DRW_Variant::STRING
                 && object.rawValues.size() == object.groups.size(),
             "raw capture preserves typed/opaque boundary values");
    t.expect(index == 5 && object.groups.size() == 5
                 && object.groups[3].code() == 1009
                 && object.groups[3].type() == DRW_Variant::STRING
                 && object.groups[3].c_str() != nullptr
                 && std::string(object.groups[3].c_str()) == "xdata-string"
                 && object.groups[4].code() == 1010
                 && object.groups[4].type() == DRW_Variant::DOUBLE,
             "raw capture keeps code 1009 opaque and separates it from real 1010");

    std::ostringstream binaryBytes;
    dxfWriterBinary binaryWriter(&binaryBytes);
    t.expect(binaryWriter.writeString(1005, "1A")
                 && binaryWriter.writeDouble(1010, 1.25)
                 && binaryWriter.writeString(0, "EOF"),
             "binary XDATA handle boundary control is emitted");

    for (const bool binary : {false, true}) {
        std::ostringstream typedXdataBytes;
        dxfRW typedXdataWriter("");
        typedXdataWriter.version = DRW::AC1027;
        typedXdataWriter.binFile = binary;
        if (binary)
            typedXdataWriter.writer =
                std::make_unique<dxfWriterBinary>(&typedXdataBytes);
        else
            typedXdataWriter.writer =
                std::make_unique<dxfWriterAscii>(&typedXdataBytes);
        DRW_Variant appId(1001, std::string("LOCAL_1009"));
        DRW_Variant recognizedString(1000, std::string("before"));
        DRW_Variant genericStringCode(1009, std::string("unclassified-role"));
        const std::vector<DRW_Variant*> typedValues{
            &appId, &recognizedString, &genericStringCode};
        t.expect(!typedXdataWriter.writeExtData(typedValues)
                     && typedXdataBytes.str().empty(),
                 "typed XDATA refuses a generic string code without emitting the valid prefix");
    }

    std::istringstream binaryRecords(binaryBytes.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    t.expect(binaryReader.readRec(&code) && code == 1005
                 && binaryReader.type == dxfReader::STRING
                 && binaryReader.getString() == "1A"
                 && binaryReader.readRec(&code) && code == 1010
                 && binaryReader.type == dxfReader::DOUBLE
                 && binaryReader.getDouble() == 1.25
                 && binaryReader.readRec(&code) && code == 0
                 && binaryReader.type == dxfReader::STRING
                 && binaryReader.getString() == "EOF"
                 && !binaryReader.readRec(&code),
             "binary XDATA group 1005 stays aligned through following EOF sentinel");
}

void testDxfBinaryWriterTypeGuard(TestContext& t) {
    std::ostringstream validBytes;
    dxfWriterBinary validWriter(&validBytes);
    t.expect(validWriter.writeString(100, "AcDbEntity")
                 && validWriter.writeDouble(10, 1.25)
                 && validWriter.writeInt16(70, 7)
                 && validWriter.writeInt32(90, 70001)
                 && validWriter.writeInt64(160, 0x123456789LL)
                 && validWriter.writeBool(290, true)
                 && validWriter.writeString(1004, "A1")
                 && validWriter.writeString(310, "A1B2C3")
                 && validWriter.writeString(319, "FE")
                 && validWriter.writeString(0, "EOF"),
             "binary DXF writer accepts canonical group-code value kinds");

    struct WireTag {
        int code;
        std::size_t fixedPayloadBytes;
        bool nulTerminatedString;
        bool binaryChunk;
        const char* chunkHex;
    };
    // Independent test oracle copied from the published DXF value-kind table;
    // it intentionally does not call the production classifier.
    const WireTag expectedTags[] = {
        {100, 0u, true, false, nullptr}, {10, 8u, false, false, nullptr},
        {70, 2u, false, false, nullptr}, {90, 4u, false, false, nullptr},
        {160, 8u, false, false, nullptr}, {290, 1u, false, false, nullptr},
        {1004, 0u, false, true, "A1"},
        {310, 0u, false, true, "A1B2C3"},
        {319, 0u, false, true, "FE"},
        {0, 0u, true, false, nullptr}};
    const std::string wireBytes = validBytes.str();
    const auto readLittleEndian = [&wireBytes](std::size_t offset,
                                                std::size_t width) {
        std::uint64_t value = 0;
        for (std::size_t index = 0; index < width; ++index)
            value |= static_cast<std::uint64_t>(
                static_cast<unsigned char>(wireBytes[offset + index]))
                << (index * 8u);
        return value;
    };
    std::size_t wireOffset = 0;
    bool wireWalkValid = true;
    for (const WireTag& expected : expectedTags) {
        if (wireOffset + 2u > wireBytes.size()) {
            wireWalkValid = false;
            break;
        }
        const unsigned char low = static_cast<unsigned char>(wireBytes[wireOffset]);
        const unsigned char high = static_cast<unsigned char>(wireBytes[wireOffset + 1u]);
        const unsigned int actualCode = low | (static_cast<unsigned int>(high) << 8u);
        wireOffset += 2u;
        if (actualCode != static_cast<unsigned int>(expected.code)) {
            wireWalkValid = false;
            break;
        }
        if (expected.nulTerminatedString) {
            const std::size_t terminator = wireBytes.find('\0', wireOffset);
            if (terminator == std::string::npos) {
                wireWalkValid = false;
                break;
            }
            if (expected.code == 100
                && wireBytes.compare(wireOffset, terminator - wireOffset,
                                     "AcDbEntity") != 0) {
                wireWalkValid = false;
                break;
            }
            if (expected.code == 0
                && wireBytes.compare(wireOffset, terminator - wireOffset, "EOF") != 0) {
                wireWalkValid = false;
                break;
            }
            wireOffset = terminator + 1u;
        } else if (expected.binaryChunk) {
            if (wireOffset >= wireBytes.size()) {
                wireWalkValid = false;
                break;
            }
            const std::size_t chunkBytes = static_cast<unsigned char>(wireBytes[wireOffset++]);
            const std::size_t expectedChunkBytes = std::strlen(expected.chunkHex) / 2u;
            if (chunkBytes != expectedChunkBytes
                || chunkBytes > wireBytes.size() - wireOffset) {
                wireWalkValid = false;
                break;
            }
            const auto hexNibble = [](char value) -> int {
                if (value >= '0' && value <= '9')
                    return value - '0';
                if (value >= 'A' && value <= 'F')
                    return value - 'A' + 10;
                if (value >= 'a' && value <= 'f')
                    return value - 'a' + 10;
                return -1;
            };
            for (std::size_t index = 0; index < chunkBytes; ++index) {
                const int high = hexNibble(expected.chunkHex[index * 2u]);
                const int low = hexNibble(expected.chunkHex[index * 2u + 1u]);
                if (high < 0 || low < 0
                    || static_cast<unsigned char>(wireBytes[wireOffset + index])
                        != static_cast<unsigned char>((high << 4) | low)) {
                    wireWalkValid = false;
                    break;
                }
            }
            if (!wireWalkValid)
                break;
            wireOffset += chunkBytes;
        } else {
            if (expected.fixedPayloadBytes > wireBytes.size() - wireOffset) {
                wireWalkValid = false;
                break;
            }
            const std::uint64_t actualValue = readLittleEndian(
                wireOffset, expected.fixedPayloadBytes);
            const std::uint64_t expectedValue = expected.code == 10
                ? 0x3FF4000000000000ULL
                : expected.code == 70 ? 7u
                : expected.code == 90 ? 70001u
                : expected.code == 160 ? 0x123456789ULL
                : expected.code == 290 ? 1u : 0u;
            if (actualValue != expectedValue) {
                wireWalkValid = false;
                break;
            }
            wireOffset += expected.fixedPayloadBytes;
        }
    }
    t.expect(wireWalkValid && wireOffset == wireBytes.size(),
             "independent binary byte walker checks ordered widths, chunk, and EOF sentinel");

    const auto rejectsWithoutBytes = [](int code, DxfValueKind kind) {
        std::ostringstream bytes;
        dxfWriterBinary writer(&bytes);
        bool accepted = false;
        switch (kind) {
        case DxfValueKind::Str:
            accepted = writer.writeString(code, "wrong");
            break;
        case DxfValueKind::Dbl:
            accepted = writer.writeDouble(code, 1.25);
            break;
        case DxfValueKind::I16:
            accepted = writer.writeInt16(code, 7);
            break;
        case DxfValueKind::I32:
            accepted = writer.writeInt32(code, 7);
            break;
        case DxfValueKind::I64:
            accepted = writer.writeInt64(code, 7);
            break;
        case DxfValueKind::Bln:
            accepted = writer.writeBool(code, true);
            break;
        case DxfValueKind::Bin:
            accepted = writer.writeString(code, "A1");
            break;
        case DxfValueKind::Unknown:
            accepted = writer.writeDouble(code, 1.25);
            break;
        }
        return !accepted && bytes.str().empty() && writer.hasWriteError();
    };
    t.expect(rejectsWithoutBytes(260, DxfValueKind::Bln)
                 && rejectsWithoutBytes(70, DxfValueKind::I32)
                 && rejectsWithoutBytes(90, DxfValueKind::I16)
                 && rejectsWithoutBytes(160, DxfValueKind::I32)
                 && rejectsWithoutBytes(482, DxfValueKind::Dbl)
                 && rejectsWithoutBytes(310, DxfValueKind::Dbl)
                 && rejectsWithoutBytes(1004, DxfValueKind::I32)
                 && rejectsWithoutBytes(999, DxfValueKind::Str),
             "binary DXF writer rejects wrong-width and wrong-kind routes before emission");

    std::ostringstream unknownTypedBytes;
    dxfWriterBinary unknownTypedWriter(&unknownTypedBytes);
    t.expect(!unknownTypedWriter.writeString(80, "opaque")
                 && !unknownTypedWriter.writeString(482, "opaque")
                 && !unknownTypedWriter.writeString(1009, "opaque")
                 && unknownTypedBytes.str().empty(),
             "binary DXF typed writer rejects unclassified string values");
    std::ostringstream unknownRawBytes;
    dxfWriterBinary unknownRawWriter(&unknownRawBytes);
    t.expect(!unknownRawWriter.writeRawString(80, "opaque")
                 && !unknownRawWriter.writeRawString(150, "opaque")
                 && !unknownRawWriter.writeRawString(180, "opaque")
                 && !unknownRawWriter.writeRawString(240, "opaque")
                 && !unknownRawWriter.writeRawString(242, "opaque")
                 && !unknownRawWriter.writeRawString(482, "opaque")
                 && !unknownRawWriter.writeRawString(1009, "opaque")
                 && unknownRawBytes.str().empty(),
             "binary DXF raw replay refuses every unclassified value width");

    std::ostringstream legacyBytes;
    dxfWriterBinary legacyWriter(&legacyBytes);
    legacyWriter.setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    t.expect(legacyWriter.writeBool(260, true)
                 && legacyWriter.writeDouble(240, 2.25)
                 && legacyWriter.writeDouble(482, 3.5),
             "binary DXF writer retains explicit legacy-profile kinds");
    std::ostringstream legacyWalkBytes;
    dxfWriterBinary legacyWalkWriter(&legacyWalkBytes);
    legacyWalkWriter.setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    t.expect(legacyWalkWriter.writeBool(260, true)
                 && legacyWalkWriter.writeDouble(240, 2.25)
                 && legacyWalkWriter.writeDouble(482, 3.5)
                 && legacyWalkWriter.writeString(0, "EOF"),
             "legacy-profile wire vector writes through the typed sink");
    const std::string legacyExpected(
        "\x04\x01\x01\xF0\x00\x00\x00\x00\x00\x00\x00\x02\x40"
        "\xE2\x01\x00\x00\x00\x00\x00\x00\x0C\x40"
        "\x00\x00" "EOF\0", 29u);
    const std::string legacyActual = legacyWalkBytes.str();
    t.expect(legacyActual == legacyExpected,
             "independent legacy-profile byte expectation checks Boolean, double, and EOF widths");
    std::ostringstream legacyMismatchBytes;
    dxfWriterBinary legacyMismatchWriter(&legacyMismatchBytes);
    legacyMismatchWriter.setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    t.expect(!legacyMismatchWriter.writeString(482, "opaque")
                 && legacyMismatchBytes.str().empty(),
             "legacy-profile binary writer rejects safe-only unknown string route");

    std::ostringstream r12Bytes;
    dxfWriterBinaryR12 r12Writer(&r12Bytes);
    t.expect(r12Writer.writeInt32(90, 70001)
                 && r12Bytes.str().size() == 5u
                 && static_cast<unsigned char>(r12Bytes.str()[0]) == 90u,
             "binary group-type guard preserves R12 code framing");

    std::ostringstream r12ExtendedBytes;
    dxfWriterBinaryR12 r12ExtendedWriter(&r12ExtendedBytes);
    t.expect(r12ExtendedWriter.writeString(1005, "1A")
                 && r12ExtendedWriter.writeDouble(1010, 1.25)
                 && r12ExtendedWriter.writeString(0, "EOF"),
             "R12 binary writer emits extended XDATA string and double codes");
    std::vector<unsigned char> expectedR12Extended{
        0xffu, 0xedu, 0x03u};
    const std::string xdataText = "1A";
    expectedR12Extended.insert(expectedR12Extended.end(), xdataText.begin(),
                               xdataText.end());
    expectedR12Extended.push_back(0u);
    expectedR12Extended.insert(expectedR12Extended.end(),
                               {0xffu, 0xf2u, 0x03u,
                                0x00u, 0x00u, 0x00u, 0x00u,
                                0x00u, 0x00u, 0xf4u, 0x3fu,
                                0x00u, 'E', 'O', 'F', 0u});
    const std::string actualR12Extended = r12ExtendedBytes.str();
    bool exactR12ExtendedBytes =
        actualR12Extended.size() == expectedR12Extended.size();
    for (std::size_t index = 0;
         exactR12ExtendedBytes && index < expectedR12Extended.size();
         ++index) {
        exactR12ExtendedBytes = static_cast<unsigned char>(
            actualR12Extended[index]) == expectedR12Extended[index];
    }
    t.expect(exactR12ExtendedBytes,
             "R12 binary byte walk checks 0xFF escape, code bytes, values, and EOF sentinel");

    std::istringstream r12ExtendedInput(actualR12Extended);
    dxfReaderBinaryR12 r12ExtendedReader(&r12ExtendedInput);
    int r12ExtendedCode = 0;
    t.expect(r12ExtendedReader.readRec(&r12ExtendedCode)
                 && r12ExtendedCode == 1005
                 && r12ExtendedReader.type == dxfReader::STRING
                 && r12ExtendedReader.getString() == "1A"
                 && r12ExtendedReader.readRec(&r12ExtendedCode)
                 && r12ExtendedCode == 1010
                 && r12ExtendedReader.type == dxfReader::DOUBLE
                 && r12ExtendedReader.getDouble() == 1.25
                 && r12ExtendedReader.readRec(&r12ExtendedCode)
                 && r12ExtendedCode == 0
                 && r12ExtendedReader.type == dxfReader::STRING
                 && r12ExtendedReader.getString() == "EOF"
                 && !r12ExtendedReader.readRec(&r12ExtendedCode),
             "R12 binary reader independently consumes extended codes and following record");
}

void testDxfAsciiWriterTypeGuard(TestContext& t) {
    std::ostringstream encoded;
    dxfWriterAscii writer(&encoded);
    t.expect(writer.writeString(100, "AcDbEntity")
                 && writer.writeDouble(10, 1.25)
                 && writer.writeInt16(70, 7)
                 && writer.writeInt32(90, 70001)
                 && writer.writeInt64(160, 123456789012LL)
                 && writer.writeBool(290, true)
                 && writer.writeString(1004, "A1")
                 && writer.writeString(310, "A1B2C3")
                 && writer.writeString(0, "EOF"),
             "ASCII DXF writer accepts canonical group-code value kinds");

    std::stringstream input(encoded.str());
    dxfReaderAscii reader(&input);
    struct ExpectedTag {
        int code;
        dxfReader::TYPE type;
    };
    const ExpectedTag expected[] = {
        {100, dxfReader::STRING}, {10, dxfReader::DOUBLE},
        {70, dxfReader::INT32}, {90, dxfReader::INT32},
        {160, dxfReader::INT64}, {290, dxfReader::BOOL},
        {1004, dxfReader::BINARY}, {310, dxfReader::BINARY},
        {0, dxfReader::STRING}
    };
    int code = 0;
    bool sequenceValid = true;
    for (const ExpectedTag& tag : expected) {
        if (!reader.readRec(&code) || code != tag.code
            || reader.type != tag.type) {
            sequenceValid = false;
            break;
        }
        if (code == 10 && reader.getDouble() != 1.25)
            sequenceValid = false;
        if (code == 70 && reader.getInt32() != 7)
            sequenceValid = false;
        if (code == 90 && reader.getInt32() != 70001)
            sequenceValid = false;
        if (code == 160 && reader.getInt64() != 123456789012LL)
            sequenceValid = false;
        if (code == 290 && !reader.getBool())
            sequenceValid = false;
        if (code == 100 && reader.getString() != "AcDbEntity")
            sequenceValid = false;
        if (code == 1004 && reader.getString() != "A1")
            sequenceValid = false;
        if (code == 310 && reader.getString() != "A1B2C3")
            sequenceValid = false;
        if (code == 0 && reader.getString() != "EOF")
            sequenceValid = false;
    }
    t.expect(sequenceValid && !reader.readRec(&code),
             "ASCII DXF typed wire sequence remains readable through EOF sentinel");

    const auto rejectsStringWithoutBytes = [](int code,
                                               std::string_view value) {
        std::ostringstream bytes;
        dxfWriterAscii candidate(&bytes);
        return !candidate.writeString(code, value) && bytes.str().empty()
            && candidate.hasWriteError();
    };
    t.expect(rejectsStringWithoutBytes(90, "70001")
                 && rejectsStringWithoutBytes(70, "7")
                 && rejectsStringWithoutBytes(290, "1")
                 && rejectsStringWithoutBytes(482, "opaque")
                 && rejectsStringWithoutBytes(1009, "opaque"),
             "ASCII DXF typed string writer rejects numeric and unknown codes before emission");

    std::ostringstream rawUnknownBytes;
    dxfWriterAscii rawUnknownWriter(&rawUnknownBytes);
    t.expect(rawUnknownWriter.writeRawString(482, "opaque")
                 && !rawUnknownBytes.str().empty(),
             "ASCII DXF unknown string requires the explicit raw replay route");

    const auto rejectsChunkWithoutBytes = [](int code,
                                              std::string_view value) {
        std::ostringstream bytes;
        dxfWriterAscii candidate(&bytes);
        return !candidate.writeString(code, value) && bytes.str().empty()
            && candidate.hasWriteError();
    };
    t.expect(rejectsChunkWithoutBytes(310, "ABC")
                 && rejectsChunkWithoutBytes(1004, "G1")
                 && rejectsChunkWithoutBytes(310, std::string(256u, 'A')),
             "ASCII DXF binary chunks require bounded even-length hexadecimal text");

    for (const bool mtext : {false, true}) {
        std::ostringstream bytes;
        dxfWriterAscii candidate(&bytes);
        const bool accepted = mtext
            ? candidate.writeUtf8MTextChunks("AB", 90, 1, 1u, 1u)
            : candidate.writeUtf8Chunks("AB", 90, 1, 1u);
        t.expect(!accepted && bytes.str().empty() && candidate.hasWriteError(),
                 "ASCII UTF-8 chunk helper rejects dynamically selected numeric continuation code");
    }
    std::ostringstream utf8StringBytes;
    dxfWriterAscii utf8StringWriter(&utf8StringBytes);
    t.expect(!utf8StringWriter.writeUtf8String(90, "wrong")
                 && utf8StringBytes.str().empty(),
             "ASCII UTF-8 wrapper inherits the typed string group-code guard");

    const auto rejectsWithoutBytes = [](int code, DxfValueKind kind) {
        std::ostringstream bytes;
        dxfWriterAscii candidate(&bytes);
        bool accepted = false;
        switch (kind) {
        case DxfValueKind::Dbl:
            accepted = candidate.writeDouble(code, 1.25);
            break;
        case DxfValueKind::I16:
            accepted = candidate.writeInt16(code, 7);
            break;
        case DxfValueKind::I32:
            accepted = candidate.writeInt32(code, 7);
            break;
        case DxfValueKind::I64:
            accepted = candidate.writeInt64(code, 7);
            break;
        case DxfValueKind::Bln:
            accepted = candidate.writeBool(code, true);
            break;
        case DxfValueKind::Str:
        case DxfValueKind::Bin:
        case DxfValueKind::Unknown:
            return false;
        }
        return !accepted && bytes.str().empty() && candidate.hasWriteError();
    };
    t.expect(rejectsWithoutBytes(10, DxfValueKind::I16)
                 && rejectsWithoutBytes(70, DxfValueKind::I32)
                 && rejectsWithoutBytes(90, DxfValueKind::I16)
                 && rejectsWithoutBytes(160, DxfValueKind::I32)
                 && rejectsWithoutBytes(290, DxfValueKind::I16)
                 && rejectsWithoutBytes(482, DxfValueKind::Dbl)
                 && rejectsWithoutBytes(80, DxfValueKind::I16),
             "ASCII DXF writer rejects wrong-kind and unassigned numeric routes before emission");

    std::ostringstream legacyBytes;
    dxfWriterAscii legacyWriter(&legacyBytes);
    legacyWriter.setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    t.expect(legacyWriter.writeBool(260, true)
                 && legacyWriter.writeDouble(482, 3.5),
             "ASCII DXF writer preserves explicit legacy-profile kinds");
    std::ostringstream legacyMismatchBytes;
    dxfWriterAscii legacyMismatchWriter(&legacyMismatchBytes);
    legacyMismatchWriter.setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    t.expect(!legacyMismatchWriter.writeInt32(260, 1)
                 && legacyMismatchBytes.str().empty(),
             "ASCII DXF writer rejects safe-profile integer kind under legacy Boolean profile");

    std::ostringstream legacyStringBytes;
    dxfWriterAscii legacyStringWriter(&legacyStringBytes);
    legacyStringWriter.setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    t.expect(!legacyStringWriter.writeString(260, "1")
                 && legacyStringBytes.str().empty(),
             "ASCII DXF typed string route honors the active legacy classifier profile");
    std::ostringstream legacyRawBytes;
    dxfWriterAscii legacyRawWriter(&legacyRawBytes);
    legacyRawWriter.setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    t.expect(legacyRawWriter.writeRawString(260, "2")
                 && legacyRawWriter.writeRawString(482, "3.5"),
             "ASCII DXF raw replay preserves valid legacy-profile scalar spellings");
}

void testDxfClassifierProfileProbe(TestContext& t) {
    std::stringstream asciiRecords("260\n7\n482\n3.5\n");
    dxfReaderAscii safeReader(&asciiRecords);
    t.expect(safeReader.classifierProfile()
                 == DxfClassifierProfile::StandaloneSafe,
             "DXF reader defaults to standalone-safe classifier profile");
    int code = 0;
    t.expect(safeReader.readRec(&code) && code == 260
                 && safeReader.type == dxfReader::STRING
                 && safeReader.getRawValue() == "7",
             "standalone-safe probe preserves code 260 as opaque ASCII");
    t.expect(safeReader.readRec(&code) && code == 482
                 && safeReader.type == dxfReader::STRING
                 && safeReader.getString() == "3.5",
             "standalone-safe probe retains opaque ASCII spelling");

    std::stringstream legacyAsciiRecords("260\n7\n482\n3.5\n");
    dxfReaderAscii legacyReader(&legacyAsciiRecords);
    legacyReader.setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    t.expect(legacyReader.classifierProfile()
                 == DxfClassifierProfile::LibreCadMasterLegacy,
             "legacy classifier profile is explicit on the reader");
    t.expect(legacyReader.readRec(&code) && code == 260
                 && legacyReader.type == dxfReader::BOOL
                 && legacyReader.getInt32() == 7,
             "legacy probe reproduces code 260 boolean route");
    t.expect(legacyReader.readRec(&code) && code == 482
                 && legacyReader.type == dxfReader::DOUBLE
                 && legacyReader.getDouble() == 3.5,
             "legacy probe reproduces code 482 double route");

    std::ostringstream binaryBytes;
    dxfWriterBinary binaryWriter(&binaryBytes);
    binaryWriter.setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    t.expect(binaryWriter.writeBool(260, true)
                 && binaryWriter.writeDouble(482, 3.5),
             "legacy binary probe fixture writes locally");
    std::stringstream legacyBinaryRecords(binaryBytes.str());
    dxfReaderBinary legacyBinaryReader(&legacyBinaryRecords);
    legacyBinaryReader.setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    t.expect(legacyBinaryReader.readRec(&code) && code == 260
                 && legacyBinaryReader.type == dxfReader::BOOL
                 && legacyBinaryReader.getBool(),
             "legacy binary probe reproduces code 260 width");
    t.expect(legacyBinaryReader.readRec(&code) && code == 482
                 && legacyBinaryReader.type == dxfReader::DOUBLE
                 && legacyBinaryReader.getDouble() == 3.5,
             "legacy binary probe reproduces code 482 width");

    std::ostringstream legacyString80Bytes;
    dxfWriterBinary legacyString80Writer(&legacyString80Bytes);
    legacyString80Writer.setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    t.expect(legacyString80Writer.writeString(80, "legacy-value"),
             "legacy binary profile writes its historical code-80 string route");
    std::stringstream safeString80Records(legacyString80Bytes.str());
    dxfReaderBinary safeString80Reader(&safeString80Records);
    t.expect(!safeString80Reader.readRec(&code) && code == 80
                 && safeString80Reader.type == dxfReader::INVALID,
             "standalone binary profile refuses an undocumented code-80 width");
    std::stringstream legacyString80Records(legacyString80Bytes.str());
    dxfReaderBinary legacyString80Reader(&legacyString80Records);
    legacyString80Reader.setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    t.expect(legacyString80Reader.readRec(&code) && code == 80
                 && legacyString80Reader.type == dxfReader::STRING
                 && legacyString80Reader.getString() == "legacy-value",
             "legacy binary profile preserves its explicit code-80 string route");

    std::ostringstream legacyString1009Bytes;
    dxfWriterBinary legacyString1009Writer(&legacyString1009Bytes);
    legacyString1009Writer.setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    t.expect(legacyString1009Writer.writeString(1009, "legacy-value"),
             "legacy profile retains the old 1000-1009 string assignment");
    std::stringstream safeString1009Records(legacyString1009Bytes.str());
    dxfReaderBinary safeString1009Reader(&safeString1009Records);
    t.expect(!safeString1009Reader.readRec(&code) && code == 1009
                 && safeString1009Reader.type == dxfReader::INVALID,
             "standalone binary profile refuses the unassigned code-1009 width");
    std::stringstream legacyString1009Records(legacyString1009Bytes.str());
    dxfReaderBinary legacyString1009Reader(&legacyString1009Records);
    legacyString1009Reader.setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    t.expect(legacyString1009Reader.readRec(&code) && code == 1009
                 && legacyString1009Reader.type == dxfReader::STRING
                 && legacyString1009Reader.getString() == "legacy-value",
             "legacy binary profile reads code 1009 with its historical string width");

    std::ostringstream legacyStringRangesBytes;
    dxfWriterBinary legacyStringRangesWriter(&legacyStringRangesBytes);
    legacyStringRangesWriter.setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    t.expect(legacyStringRangesWriter.writeString(150, "legacy-150")
                 && legacyStringRangesWriter.writeString(180, "legacy-180")
                 && legacyStringRangesWriter.writeString(209, "legacy-209"),
             "legacy binary profile retains historical strings in omitted ranges");
    std::stringstream safeStringRangesRecords(legacyStringRangesBytes.str());
    dxfReaderBinary safeStringRangesReader(&safeStringRangesRecords);
    t.expect(!safeStringRangesReader.readRec(&code) && code == 150
                 && safeStringRangesReader.type == dxfReader::INVALID,
             "standalone binary reader refuses an undocumented 150-range width");
    std::stringstream legacyStringRangesRecords(legacyStringRangesBytes.str());
    dxfReaderBinary legacyStringRangesReader(&legacyStringRangesRecords);
    legacyStringRangesReader.setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    const std::vector<std::pair<int, std::string>> legacyStringRanges{
        {150, "legacy-150"}, {180, "legacy-180"}, {209, "legacy-209"}};
    bool legacyStringRangesRead = true;
    for (const auto& item : legacyStringRanges) {
        legacyStringRangesRead = legacyStringRangesRead
            && legacyStringRangesReader.readRec(&code) && code == item.first
            && legacyStringRangesReader.type == dxfReader::STRING
            && legacyStringRangesReader.getString() == item.second;
    }
    t.expect(legacyStringRangesRead
                 && !legacyStringRangesReader.readRec(&code),
             "legacy binary reader preserves exact historical range strings");

    std::stringstream captureRecords("5\n1A\n260\n7\n482\n3.5\n");
    std::ostringstream replayBytes;
    dxfRW owner("");
    owner.setDxfCompatibilityProfile(
        dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy);
    owner.version = DRW::AC1027;
    owner.binFile = false;
    owner.reader = std::make_unique<dxfReaderAscii>(&captureRecords);
    owner.reader->setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    DRW_RawDxfObject object;
    object.name = "RAW_PROFILE";
    object.m_version = DRW::AC1027;
    int captured = 0;
    while (owner.reader->readRec(&code)) {
        t.expect(owner.captureRawGroup(object, code, true),
                 "legacy profile capture accepts its typed groups");
        ++captured;
    }
    t.expect(captured == 3 && object.groups.size() == 3
                 && object.groups[1].type() == DRW_Variant::INTEGER
                 && object.groups[2].type() == DRW_Variant::DOUBLE,
             "legacy profile capture aligns typed raw variants");
    owner.writer = std::make_unique<dxfWriterAscii>(&replayBytes);
    t.expect(owner.writeRawDxfObject(&object),
             "legacy profile raw capture replays through matching writer");
    std::stringstream replayRecords(replayBytes.str());
    dxfReaderAscii replayReader(&replayRecords);
    replayReader.setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    t.expect(replayReader.readRec(&code) && code == 0
                 && replayReader.getString() == "RAW_PROFILE"
                 && replayReader.readRec(&code) && code == 5,
             "legacy profile raw replay preserves object framing");
    t.expect(replayReader.readRec(&code) && code == 260
                 && replayReader.type == dxfReader::BOOL
                 && replayReader.getInt32() == 7,
             "legacy profile raw replay preserves code 260 value");
    t.expect(replayReader.readRec(&code) && code == 482
                 && replayReader.type == dxfReader::DOUBLE
                 && replayReader.getDouble() == 3.5,
             "legacy profile raw replay preserves code 482 value");
}

void testDxfBinaryLegacyProfileReplay(TestContext& t) {
    DRW_RawDxfObject object;
    object.name = "RAW_LEGACY_BINARY";
    object.m_version = DRW::AC1027;
    object.groups = {
        DRW_Variant(5, std::string("2A")),
        DRW_Variant(260, static_cast<std::int32_t>(1)),
        DRW_Variant(482, 3.5)};

    std::ostringstream legacyBytes;
    dxfRW legacyOwner("");
    legacyOwner.setDxfCompatibilityProfile(
        dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy);
    legacyOwner.version = DRW::AC1027;
    legacyOwner.binFile = true;
    legacyOwner.writer = std::make_unique<dxfWriterBinary>(&legacyBytes);
    t.expect(legacyOwner.writeRawDxfObject(&object),
             "legacy profile binary raw object writes");
    std::stringstream legacyRecords(legacyBytes.str());
    dxfReaderBinary legacyReader(&legacyRecords);
    legacyReader.setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    int code = 0;
    t.expect(legacyReader.readRec(&code) && code == 0
                 && legacyReader.getString() == "RAW_LEGACY_BINARY"
                 && legacyReader.readRec(&code) && code == 5,
             "legacy profile binary object framing round-trips");
    t.expect(legacyReader.readRec(&code) && code == 260
                 && legacyReader.type == dxfReader::BOOL
                 && legacyReader.getBool(),
             "legacy profile binary object preserves one-byte code 260");
    t.expect(legacyReader.readRec(&code) && code == 482
                 && legacyReader.type == dxfReader::DOUBLE
                 && legacyReader.getDouble() == 3.5,
             "legacy profile binary object preserves eight-byte code 482");

    DRW_RawDxfSection section;
    section.m_name = "LOCAL_LEGACY_BINARY";
    section.m_version = DRW::AC1027;
    section.m_groups = {
        DRW_Variant(260, static_cast<std::int32_t>(1)),
        DRW_Variant(482, 3.5)};
    std::ostringstream sectionBytes;
    dxfRW sectionOwner("");
    sectionOwner.setDxfCompatibilityProfile(
        dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy);
    sectionOwner.version = DRW::AC1027;
    sectionOwner.binFile = true;
    sectionOwner.writer = std::make_unique<dxfWriterBinary>(&sectionBytes);
    t.expect(sectionOwner.writeRawDxfSection(section),
             "legacy profile binary raw section writes");
    std::stringstream sectionRecords(sectionBytes.str());
    dxfReaderBinary sectionReader(&sectionRecords);
    sectionReader.setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    const std::vector<int> expectedCodes {0, 2, 260, 482, 0};
    std::size_t index = 0;
    while (sectionReader.readRec(&code)) {
        t.expect(index < expectedCodes.size()
                     && code == expectedCodes[index],
                 "legacy profile binary section framing round-trips");
        ++index;
    }
    t.expect(index == expectedCodes.size(),
             "legacy profile binary section consumes complete frame");

    std::ostringstream safeBytes;
    dxfRW safeOwner("");
    safeOwner.version = DRW::AC1027;
    safeOwner.binFile = true;
    safeOwner.writer = std::make_unique<dxfWriterBinary>(&safeBytes);
    t.expect(!safeOwner.writeRawDxfObject(&object) && safeBytes.str().empty(),
             "standalone-safe binary writer rejects legacy-only double route");
}

void testDxfFacadeClassifierProfile(TestContext& t) {
    const std::string content =
        "0\nSECTION\n2\nCUSTOM_PROFILE\n5\n1A\n260\n7\n"
        "482\n3.5\n0\nENDSEC\n0\nEOF\n";

    ProfileProbeInterface safeInterface;
    dxfRW safeOwner("");
    t.expect(safeOwner.dxfCompatibilityProfile()
                 == dxfRW::DxfCompatibilityProfile::StandaloneSafe,
             "DXF facade defaults to standalone-safe compatibility profile");
    std::string safeContent = content;
    t.expect(safeOwner.readAscii(&safeInterface, false, safeContent),
             "safe profile reaches DXF facade raw section");
    t.expect(safeInterface.sections.size() == 1
                 && safeInterface.sections.front().m_groups.size() == 3
                 && safeInterface.sections.front().m_groups[1].type()
                        == DRW_Variant::STRING
                 && safeInterface.sections.front().m_groups[2].type()
                        == DRW_Variant::STRING,
             "safe facade profile publishes safe raw carrier types");

    ProfileProbeInterface legacyInterface;
    dxfRW legacyOwner("");
    legacyOwner.setDxfCompatibilityProfile(
        dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy);
    t.expect(legacyOwner.dxfCompatibilityProfile()
                 == dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy,
             "DXF facade legacy profile is explicit and observable");
    std::string legacyContent = content;
    t.expect(legacyOwner.readAscii(&legacyInterface, false, legacyContent),
             "legacy profile reaches DXF facade raw section");
    t.expect(legacyInterface.sections.size() == 1
                 && legacyInterface.sections.front().m_groups.size() == 3
                 && legacyInterface.sections.front().m_groups[1].type()
                        == DRW_Variant::INTEGER
                 && legacyInterface.sections.front().m_groups[2].type()
                        == DRW_Variant::DOUBLE,
             "legacy facade profile publishes matching raw carrier types");
}

void testDxfProfilePromotionPolicy(TestContext& t) {
    dxfRW codec("");
    codec.setBinary(true);
    t.expect(codec.dxfCompatibilityProfile()
                 == dxfRW::DxfCompatibilityProfile::StandaloneSafe,
             "binary-format selection does not silently promote DXF profile");
    codec.setDxfCompatibilityProfile(
        dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy);
    t.expect(codec.dxfCompatibilityProfile()
                 == dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy,
             "legacy DXF profile requires explicit adapter selection");
    codec.setBinary(false);
    t.expect(codec.dxfCompatibilityProfile()
                 == dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy,
             "format changes do not discard explicit DXF profile");
    codec.setDxfCompatibilityProfile(
        dxfRW::DxfCompatibilityProfile::StandaloneSafe);
    t.expect(codec.dxfCompatibilityProfile()
                 == dxfRW::DxfCompatibilityProfile::StandaloneSafe,
             "adapter can restore standalone-safe DXF profile explicitly");
}

void testDxfProfileMatrix(TestContext& t) {
    struct ProfileCase {
        DxfClassifierProfile profile;
        dxfRW::DxfCompatibilityProfile facadeProfile;
        dxfReader::TYPE code260Type;
        dxfReader::TYPE code482Type;
        bool legacy;
    };
    const ProfileCase cases[] = {
        {DxfClassifierProfile::StandaloneSafe,
         dxfRW::DxfCompatibilityProfile::StandaloneSafe,
         dxfReader::STRING, dxfReader::STRING, false},
        {DxfClassifierProfile::LibreCadMasterLegacy,
         dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy,
         dxfReader::BOOL, dxfReader::DOUBLE, true}};

    for (const ProfileCase &profileCase : cases) {
        std::stringstream asciiRecords("260\n7\n482\n3.5\n");
        dxfReaderAscii asciiReader(&asciiRecords);
        asciiReader.setClassifierProfile(profileCase.profile);
        int code = 0;
        const bool code260Read = asciiReader.readRec(&code);
        t.expect(code260Read && code == 260
                     && asciiReader.type == profileCase.code260Type,
                 "DXF profile matrix agrees on ASCII code 260");
        const bool code482Read = asciiReader.readRec(&code);
        t.expect(code482Read && code == 482
                     && asciiReader.type == profileCase.code482Type,
                 "DXF profile matrix agrees on ASCII code 482");

        if (profileCase.legacy) {
            std::ostringstream binaryBytes;
            dxfWriterBinary binaryWriter(&binaryBytes);
            binaryWriter.setClassifierProfile(profileCase.profile);
            const bool wroteLegacy = binaryWriter.writeBool(260, true)
                && binaryWriter.writeDouble(482, 3.5);
            std::stringstream binaryRecords(binaryBytes.str());
            dxfReaderBinary binaryReader(&binaryRecords);
            binaryReader.setClassifierProfile(profileCase.profile);
            int code = 0;
            t.expect(wroteLegacy
                         && binaryReader.readRec(&code) && code == 260
                         && binaryReader.type == profileCase.code260Type
                         && binaryReader.getBool()
                         && binaryReader.readRec(&code) && code == 482
                         && binaryReader.type == profileCase.code482Type
                         && binaryReader.getDouble() == 3.5,
                     "DXF legacy profile retains explicit binary 260/482 widths");
        } else {
            std::ostringstream rejectedBytes;
            dxfWriterBinary binaryWriter(&rejectedBytes);
            binaryWriter.setClassifierProfile(profileCase.profile);
            t.expect(!binaryWriter.writeInt32(260, 7)
                         && rejectedBytes.str().empty()
                         && binaryWriter.hasWriteError(),
                     "DXF standalone binary writer refuses undocumented 260 width");

            // Deliberately malformed reader input: bypass the writer guard
            // to verify that the binary reader independently refuses to
            // guess a payload width for code 260.
            const unsigned char malformedRecord[] = {
                0x04u, 0x01u, 0x07u, 0x00u, 0x00u, 0x00u};
            std::string malformedBytes(
                reinterpret_cast<const char*>(malformedRecord),
                sizeof(malformedRecord));
            std::stringstream binaryRecords(malformedBytes);
            dxfReaderBinary binaryReader(&binaryRecords);
            binaryReader.setClassifierProfile(profileCase.profile);
            int code = 0;
            t.expect(!binaryReader.readRec(&code) && code == 260
                         && binaryReader.type == dxfReader::INVALID,
                     "DXF standalone binary reader refuses undocumented 260 width");
        }

        dxfRW facade("");
        facade.setDxfCompatibilityProfile(profileCase.facadeProfile);
        t.expect(facade.dxfCompatibilityProfile()
                     == profileCase.facadeProfile,
                 "DXF profile matrix preserves façade selection");
    }
}

void testDxfFacadeProfileReplayAgreement(TestContext& t) {
    struct ReplayCase {
        dxfRW::DxfCompatibilityProfile facadeProfile;
        DxfClassifierProfile readerProfile;
        DRW_Variant code260;
        DRW_Variant code482;
        dxfReader::TYPE code260Type;
        dxfReader::TYPE code482Type;
    };
    const ReplayCase cases[] = {
        {dxfRW::DxfCompatibilityProfile::StandaloneSafe,
         DxfClassifierProfile::StandaloneSafe,
         DRW_Variant(260, std::string("7")),
         DRW_Variant(482, std::string("3.5")), dxfReader::STRING,
         dxfReader::STRING},
        {dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy,
         DxfClassifierProfile::LibreCadMasterLegacy,
         DRW_Variant(260, static_cast<std::int32_t>(7)),
         DRW_Variant(482, 3.5), dxfReader::BOOL, dxfReader::DOUBLE}};

    for (const ReplayCase &replayCase : cases) {
        DRW_RawDxfObject object;
        object.name = "PROFILE_REPLAY";
        object.m_version = DRW::AC1027;
        object.groups = {DRW_Variant(5, std::string("2A")),
                         replayCase.code260,
                         replayCase.code482};
        std::ostringstream objectBytes;
        dxfRW objectWriter("");
        objectWriter.setDxfCompatibilityProfile(replayCase.facadeProfile);
        objectWriter.version = DRW::AC1027;
        objectWriter.binFile = false;
        objectWriter.writer = std::make_unique<dxfWriterAscii>(&objectBytes);
        t.expect(objectWriter.writeRawDxfObject(&object),
                 "profile façade writes matching raw object");
        std::stringstream objectRecords(objectBytes.str());
        dxfReaderAscii objectReader(&objectRecords);
        objectReader.setClassifierProfile(replayCase.readerProfile);
        int code = 0;
        t.expect(objectReader.readRec(&code) && code == 0
                     && objectReader.readRec(&code) && code == 5
                     && objectReader.readRec(&code) && code == 260
                     && objectReader.type == replayCase.code260Type,
                 "profile façade raw object framing and code 260 agree");
        t.expect(objectReader.readRec(&code) && code == 482
                     && objectReader.type == replayCase.code482Type,
                 "profile façade raw object code 482 agrees");

        DRW_RawDxfSection section;
        section.m_name = "PROFILE_REPLAY_SECTION";
        section.m_version = DRW::AC1027;
        section.m_groups = {replayCase.code260,
                            replayCase.code482};
        std::ostringstream sectionBytes;
        dxfRW sectionWriter("");
        sectionWriter.setDxfCompatibilityProfile(replayCase.facadeProfile);
        sectionWriter.version = DRW::AC1027;
        sectionWriter.binFile = false;
        sectionWriter.writer = std::make_unique<dxfWriterAscii>(&sectionBytes);
        t.expect(sectionWriter.writeRawDxfSection(section),
                 "profile façade writes matching raw section");
        std::stringstream sectionRecords(sectionBytes.str());
        dxfReaderAscii sectionReader(&sectionRecords);
        sectionReader.setClassifierProfile(replayCase.readerProfile);
        t.expect(sectionReader.readRec(&code) && code == 0
                     && sectionReader.readRec(&code) && code == 2
                     && sectionReader.readRec(&code) && code == 260
                     && sectionReader.type == replayCase.code260Type
                     && sectionReader.readRec(&code) && code == 482
                     && sectionReader.type == replayCase.code482Type
                     && sectionReader.readRec(&code) && code == 0,
                 "profile façade raw section framing and types agree");
    }

    DRW_RawDxfObject legacyOnly;
    legacyOnly.name = "PROFILE_MISMATCH";
    legacyOnly.m_version = DRW::AC1027;
    legacyOnly.groups = {DRW_Variant(5, std::string("2A")),
                         DRW_Variant(260, static_cast<std::int32_t>(7)),
                         DRW_Variant(482, 3.5)};
    std::ostringstream safeBytes;
    dxfRW safeWriter("");
    safeWriter.version = DRW::AC1027;
    safeWriter.binFile = false;
    safeWriter.writer = std::make_unique<dxfWriterAscii>(&safeBytes);
    t.expect(!safeWriter.writeRawDxfObject(&legacyOnly)
                 && safeBytes.str().empty(),
             "safe façade rejects legacy double raw object transactionally");

    DRW_RawDxfObject safeOnly = legacyOnly;
    safeOnly.groups.back() = DRW_Variant(482, std::string("opaque"));
    std::ostringstream legacyBytes;
    dxfRW legacyWriter("");
    legacyWriter.setDxfCompatibilityProfile(
        dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy);
    legacyWriter.version = DRW::AC1027;
    legacyWriter.binFile = false;
    legacyWriter.writer = std::make_unique<dxfWriterAscii>(&legacyBytes);
    t.expect(!legacyWriter.writeRawDxfObject(&safeOnly)
                 && legacyBytes.str().empty(),
             "legacy façade rejects safe opaque raw object transactionally");
}

void testDxfProfileDiagnostics(TestContext& t) {
    struct FailureCase {
        dxfRW::DxfCompatibilityProfile facadeProfile;
        const char *value;
    };
    const FailureCase cases[] = {
        {dxfRW::DxfCompatibilityProfile::StandaloneSafe, "not-an-int"},
        {dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy, "opaque"}};
    for (const FailureCase &failureCase : cases) {
        const bool legacy = failureCase.facadeProfile
            == dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy;
        const std::string content = legacy
            ? "0\nSECTION\n2\nPROFILE_FAILURE\n482\nopaque\n"
              "0\nENDSEC\n0\nEOF\n"
            : "0\nSECTION\n2\nPROFILE_FAILURE\n90\nnot-an-int\n"
              "0\nENDSEC\n0\nEOF\n";
        ProfileProbeInterface interface_;
        dxfRW reader("");
        reader.setDxfCompatibilityProfile(failureCase.facadeProfile);
        std::string input = content;
        t.expect(!reader.readAscii(&interface_, false, input)
                     && reader.getError() == DRW::BAD_READ_SECTION,
                 "profile malformed ASCII keeps section error precedence");
        const DRW_OperationDiagnostic diagnostic = reader.getLastDiagnostic();
        t.expect(diagnostic.operation == DRW::OperationKind::Read
                     && diagnostic.phase == DRW::OperationPhase::RawSection
                     && diagnostic.cause == DRW::OperationCause::ReadFailure
                     && diagnostic.code == "read-section",
                 "profile malformed ASCII preserves structured section diagnostic");
        t.expect(interface_.sections.empty() && interface_.objects.empty(),
                 "profile malformed ASCII publishes no callbacks");
    }

    std::ostringstream malformedBytes;
    const unsigned char malformedRecord[] = {
        0xE2u, 0x01u, 0x07u, 0x00u, 0x00u, 0x00u};
    malformedBytes.write(reinterpret_cast<const char*>(malformedRecord),
                          sizeof(malformedRecord));
    t.expect(malformedBytes.good(),
             "profile malformed binary vector is created independently");
    std::stringstream malformedRecords(malformedBytes.str());
    dxfReaderBinary malformedReader(&malformedRecords);
    malformedReader.setClassifierProfile(
        DxfClassifierProfile::LibreCadMasterLegacy);
    int code = 0;
    t.expect(!malformedReader.readRec(&code) && code == 482
                 && malformedReader.type == dxfReader::INVALID,
             "legacy profile rejects malformed binary double width");
}

DRW_RawDxfObject rawBoundaryObject() {
    DRW_RawDxfObject object;
    object.name = "RAW_BOUNDARY";
    object.handle = 0x1Au;
    object.m_version = DRW::AC1027;
    object.hasRawValues = true;
    object.groups = {
        DRW_Variant(5, std::string("1A")),
        DRW_Variant(260, std::string("2147483647")),
        DRW_Variant(269, std::string("-7")),
        DRW_Variant(482, std::string("3.14")),
        DRW_Variant(998, std::string("opaque-after")),
        DRW_Variant(1004, std::string("AB"))};
    object.rawValues = {"1A", "2147483647", "-7", "3.14",
                        "opaque-after", "AB"};
    return object;
}

bool writeRawBoundaryObject(const DRW_RawDxfObject& source,
                            std::string& output) {
    std::ostringstream stream;
    dxfRW owner("");
    owner.version = DRW::AC1027;
    owner.binFile = false;
    owner.writer = std::make_unique<dxfWriterAscii>(&stream);
    DRW_RawDxfObject object = source;
    const bool written = owner.writeRawDxfObject(&object);
    output = stream.str();
    return written;
}

void testDxfRawBoundaryReplay(TestContext& t) {
    const DRW_RawDxfObject object = rawBoundaryObject();
    std::string output;
    t.expect(writeRawBoundaryObject(object, output),
             "DXF raw boundary object writes through ASCII replay");
    t.expect(!output.empty(), "DXF raw boundary replay commits a record");

    std::stringstream records(output);
    dxfReaderAscii reader(&records);
    const std::vector<int> expectedCodes {0, 5, 260, 269, 482, 998, 1004};
    const std::vector<dxfReader::TYPE> expectedTypes {
        dxfReader::STRING, dxfReader::STRING, dxfReader::STRING,
        dxfReader::STRING, dxfReader::STRING, dxfReader::STRING,
        dxfReader::BINARY};
    const std::vector<std::string> expectedRaw {
        "RAW_BOUNDARY", "1A", "2147483647", "-7", "3.14",
        "opaque-after", "AB"};
    int code = 0;
    std::size_t index = 0;
    while (reader.readRec(&code)) {
        t.expect(index < expectedCodes.size() && code == expectedCodes[index],
                 "DXF raw replay code order matches source");
        t.expect(index < expectedTypes.size() && reader.type == expectedTypes[index],
                 "DXF raw replay type matches canonical classifier");
        t.expect(index < expectedRaw.size() && reader.getRawValue() == expectedRaw[index],
                 "DXF raw replay preserves source spelling");
        ++index;
    }
    t.expect(index == expectedCodes.size(),
             "DXF raw replay parser consumes complete record");

    DRW_RawDxfObject badNumeric = object;
    badNumeric.groups[1] = DRW_Variant(260, static_cast<std::int32_t>(7));
    badNumeric.rawValues[1] = "not-an-int";
    output.clear();
    t.expect(!writeRawBoundaryObject(badNumeric, output)
                 && output.empty(),
             "DXF raw replay rejects malformed typed numeric boundary");

    DRW_RawDxfObject badOpaque = object;
    badOpaque.groups[3] = DRW_Variant(482, static_cast<std::int32_t>(7));
    badOpaque.rawValues[3] = "7";
    output.clear();
    t.expect(!writeRawBoundaryObject(badOpaque, output)
                 && output.empty(),
             "DXF raw replay rejects non-string opaque boundary");

    DRW_RawDxfObject missingHandle = object;
    missingHandle.groups.erase(missingHandle.groups.begin());
    missingHandle.rawValues.erase(missingHandle.rawValues.begin());
    output.clear();
    t.expect(!writeRawBoundaryObject(missingHandle, output)
                 && output.empty(),
             "DXF raw replay rejects a missing self handle");

    DRW_RawDxfObject zeroHandle = object;
    zeroHandle.groups[0] = DRW_Variant(5, std::string("0"));
    zeroHandle.rawValues[0] = "0";
    output.clear();
    t.expect(!writeRawBoundaryObject(zeroHandle, output)
                 && output.empty(),
             "DXF raw replay rejects a zero self handle");

    DRW_RawDxfObject wideHandle = object;
    wideHandle.groups[0] = DRW_Variant(5, std::string("123456789ABCDEF0"));
    wideHandle.rawValues[0] = "123456789ABCDEF0";
    output.clear();
    t.expect(writeRawBoundaryObject(wideHandle, output)
                 && !output.empty(),
             "DXF raw replay accepts a bounded wide self handle");

    ProfileProbeInterface duplicateInterface;
    std::string duplicateContent =
        "0\nSECTION\n2\nOBJECTS\n0\nLOCAL_DUPLICATE\n"
        "5\n1A\n5\n1A\n0\nENDSEC\n0\nEOF\n";
    dxfRW duplicateReader("");
    t.expect(!duplicateReader.readAscii(
                  &duplicateInterface, false, duplicateContent)
                 && duplicateInterface.objects.empty(),
             "DXF raw object capture rejects duplicate self handles");
}

DRW_RawDxfSection rawBoundarySection() {
    DRW_RawDxfSection section;
    section.m_name = "LOCAL_RAW_BOUNDARY";
    section.m_version = DRW::AC1027;
    section.m_hasRawValues = true;
    section.m_groups = {
        DRW_Variant(260, std::string("2147483647")),
        DRW_Variant(269, std::string("-7")),
        DRW_Variant(482, std::string("3.14")),
        DRW_Variant(998, std::string("opaque-after")),
        DRW_Variant(1004, std::string("AB"))};
    section.m_rawValues = {"2147483647", "-7", "3.14", "opaque-after",
                           "AB"};
    return section;
}

bool writeRawBoundarySection(const DRW_RawDxfSection& source,
                             std::string& output) {
    std::ostringstream stream;
    dxfRW owner("");
    owner.version = DRW::AC1027;
    owner.binFile = false;
    owner.writer = std::make_unique<dxfWriterAscii>(&stream);
    const bool written = owner.writeRawDxfSection(source);
    output = stream.str();
    return written;
}

void testDxfRawSectionBoundaryReplay(TestContext& t) {
    const DRW_RawDxfSection section = rawBoundarySection();
    std::string output;
    t.expect(writeRawBoundarySection(section, output),
             "DXF raw boundary section writes through ASCII replay");
    t.expect(!output.empty(), "DXF raw boundary section commits a record");

    std::stringstream records(output);
    dxfReaderAscii reader(&records);
    const std::vector<int> expectedCodes {0, 2, 260, 269, 482, 998, 1004, 0};
    const std::vector<dxfReader::TYPE> expectedTypes {
        dxfReader::STRING, dxfReader::STRING, dxfReader::STRING,
        dxfReader::STRING, dxfReader::STRING, dxfReader::STRING,
        dxfReader::BINARY, dxfReader::STRING};
    const std::vector<std::string> expectedRaw {
        "SECTION", "LOCAL_RAW_BOUNDARY", "2147483647", "-7", "3.14",
        "opaque-after", "AB", "ENDSEC"};
    int code = 0;
    std::size_t index = 0;
    while (reader.readRec(&code)) {
        t.expect(index < expectedCodes.size() && code == expectedCodes[index],
                 "DXF raw section code order matches framing");
        t.expect(index < expectedTypes.size() && reader.type == expectedTypes[index],
                 "DXF raw section type matches canonical classifier");
        t.expect(index < expectedRaw.size() && reader.getRawValue() == expectedRaw[index],
                 "DXF raw section preserves source spelling");
        ++index;
    }
    t.expect(index == expectedCodes.size(),
             "DXF raw section parser consumes SECTION and ENDSEC");

    DRW_RawDxfSection badNumeric = section;
    badNumeric.m_groups[0] = DRW_Variant(260, static_cast<std::int32_t>(7));
    badNumeric.m_rawValues[0] = "not-an-int";
    output.clear();
    t.expect(!writeRawBoundarySection(badNumeric, output)
                 && output.empty(),
             "DXF raw section rejects malformed typed numeric boundary");

    DRW_RawDxfSection badOpaque = section;
    badOpaque.m_groups[2] = DRW_Variant(482, static_cast<std::int32_t>(7));
    badOpaque.m_rawValues[2] = "7";
    output.clear();
    t.expect(!writeRawBoundarySection(badOpaque, output)
                 && output.empty(),
             "DXF raw section rejects non-string opaque boundary");
}

void testDxfRawSectionCaptureReplay(TestContext& t) {
    const std::string content =
        "0\nSECTION\n2\nLOCAL_CAPTURE\n260\n2147483647\n"
        "269\n-7\n482\n3.5\n1004\nAB\n"
        "0\nENDSEC\n0\nEOF\n";
    const dxfRW::DxfCompatibilityProfile profiles[] = {
        dxfRW::DxfCompatibilityProfile::StandaloneSafe,
        dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy};
    for (const dxfRW::DxfCompatibilityProfile profile : profiles) {
        ProfileProbeInterface interface_;
        dxfRW reader("");
        reader.setDxfCompatibilityProfile(profile);
        std::string input = content;
        t.expect(reader.readAscii(&interface_, false, input)
                     && interface_.sections.size() == 1,
                 "DXF raw section capture publishes one section");
        if (interface_.sections.size() != 1)
            continue;
        const DRW_RawDxfSection& captured = interface_.sections.front();
        t.expect(captured.m_hasRawValues && captured.m_groups.size() == 4
                     && captured.m_rawValues.size() == 4
                     && captured.m_rawValues[0] == "2147483647"
                     && captured.m_rawValues[1] == "-7"
                     && captured.m_rawValues[2] == "3.5"
                     && captured.m_rawValues[3] == "AB",
                 "DXF raw section capture retains source spellings");
        if (captured.m_groups.size() == 4) {
            const bool legacy = profile
                == dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy;
            t.expect(captured.m_groups[2].type()
                         == (legacy ? DRW_Variant::DOUBLE
                                    : DRW_Variant::STRING),
                     "DXF profile capture retains canonical code-482 type");
        }

        std::ostringstream output;
        dxfRW writer("");
        writer.setDxfCompatibilityProfile(profile);
        writer.version = DRW::AC1027;
        writer.binFile = false;
        writer.writer = std::make_unique<dxfWriterAscii>(&output);
        t.expect(writer.writeRawDxfSection(captured),
                 "DXF captured raw section replays through matching profile");
        const std::string replay = output.str();
        t.expect(replay.find("260\n2147483647\n") != std::string::npos
                     && replay.find("269\n-7\n") != std::string::npos
                     && replay.find("482\n3.5\n") != std::string::npos
                     && replay.find("1004\nAB\n") != std::string::npos,
                 "DXF captured raw section replay preserves source text");
    }
}

void testDxfAcdsDataOpaqueSectionCaptureReplay(TestContext& t) {
    // Locally generated, unknown ACDSDATA-shaped tags exercise only the
    // generic raw-section carrier. An ASM_Data record instead requires an
    // emitted modeler owner; check that orphan separately below.
    const std::string content =
        "0\nSECTION\n2\nACDSDATA\n"
        "0\nACDSRECORD\n90\n1\n2\nAcDbDs::ID\n280\n10\n"
        "320\nD2\n2\nOpaqueData\n280\n15\n94\n4\n310\n41424344\n"
        "0\nENDSEC\n0\nEOF\n";
    ProfileProbeInterface sourceInterface;
    dxfRW reader("");
    std::string input = content;
    const bool sourceRead = reader.readAscii(
        &sourceInterface, false, input);
    t.expect(sourceRead && sourceInterface.sections.size() == 1,
             "ACDSDATA DXF is delivered through raw-section callback");
    if (!sourceRead || sourceInterface.sections.size() != 1)
        return;

    const DRW_RawDxfSection& captured = sourceInterface.sections.front();
    const std::vector<int> expectedCodes {
        0, 90, 2, 280, 320, 2, 280, 94, 310};
    const std::vector<std::string> expectedRaw {
        "ACDSRECORD", "1", "AcDbDs::ID", "10", "D2", "OpaqueData",
        "15", "4", "41424344"};
    bool payloadMatches = captured.m_name == "ACDSDATA"
        && captured.m_hasRawValues
        && captured.m_groups.size() == expectedCodes.size()
        && captured.m_rawValues == expectedRaw;
    for (std::size_t i = 0;
         payloadMatches && i < expectedCodes.size(); ++i) {
        payloadMatches = captured.m_groups[i].code() == expectedCodes[i];
    }
    t.expect(payloadMatches,
             "ACDSDATA opaque capture preserves ordered tags and raw values");
    t.expect(sourceInterface.storage.mBlock->ent.empty(),
             "orphaned ACDS record creates no typed modeler entity");
    if (!payloadMatches)
        return;

    DRW_RawDxfSection orphanedAsm = captured;
    orphanedAsm.m_groups[5].addString(2, "ASM_Data");
    orphanedAsm.m_rawValues[5] = "ASM_Data";
    std::ostringstream rejected;
    dxfRW rejectingWriter("");
    rejectingWriter.version = DRW::AC1027;
    rejectingWriter.writer = std::make_unique<dxfWriterAscii>(&rejected);
    t.expect(!rejectingWriter.writeRawDxfSection(orphanedAsm)
                 && rejected.str().empty(),
             "orphaned ASM_Data rejects transactionally without weakening owner association");

    std::ostringstream output;
    dxfRW writer("");
    writer.version = DRW::AC1027;
    writer.binFile = false;
    writer.writer = std::make_unique<dxfWriterAscii>(&output);
    const bool replayWritten = writer.writeRawDxfSection(captured);
    std::string replay = output.str() + "0\nEOF\n";
    ProfileProbeInterface replayInterface;
    dxfRW replayReader("");
    const bool replayRead = replayWritten
        && replayReader.readAscii(&replayInterface, false, replay);
    t.expect(replayRead && replayInterface.sections.size() == 1,
             "ACDSDATA opaque section replays and is readable again");
    if (!replayRead || replayInterface.sections.size() != 1)
        return;

    const DRW_RawDxfSection& replayed = replayInterface.sections.front();
    bool replayMatches = replayed.m_name == captured.m_name
        && replayed.m_hasRawValues
        && replayed.m_rawValues == captured.m_rawValues
        && replayed.m_groups.size() == captured.m_groups.size();
    for (std::size_t i = 0;
         replayMatches && i < captured.m_groups.size(); ++i) {
        replayMatches = replayed.m_groups[i].code()
                == captured.m_groups[i].code()
            && replayed.m_groups[i].type()
                == captured.m_groups[i].type();
    }
    t.expect(replayMatches,
             "ACDSDATA raw replay preserves ordered group types and spellings");
    t.expect(replayInterface.storage.mBlock->ent.empty(),
             "ACDSDATA raw replay remains distinct from entity association");
}

DRW_RawDxfObject rawBinaryBoundaryObject() {
    DRW_RawDxfObject object;
    object.name = "RAW_BINARY_BOUNDARY";
    object.handle = 0x2Au;
    object.m_version = DRW::AC1027;
    object.groups = {
        DRW_Variant(5, std::string("2A")),
        DRW_Variant(90, static_cast<std::int32_t>(2147483647)),
        DRW_Variant(91, static_cast<std::int32_t>(-7)),
        DRW_Variant(1004, std::string("ABCD"))};
    return object;
}

bool writeRawBinaryBoundaryObject(const DRW_RawDxfObject& source,
                                  std::string& output) {
    std::ostringstream stream;
    dxfRW owner("");
    owner.version = DRW::AC1027;
    owner.binFile = true;
    owner.writer = std::make_unique<dxfWriterBinary>(&stream);
    DRW_RawDxfObject object = source;
    const bool written = owner.writeRawDxfObject(&object);
    output = stream.str();
    return written;
}

DRW_RawDxfSection rawBinaryBoundarySection() {
    DRW_RawDxfSection section;
    section.m_name = "LOCAL_RAW_BINARY";
    section.m_version = DRW::AC1027;
    section.m_groups = {
        DRW_Variant(90, static_cast<std::int32_t>(2147483647)),
        DRW_Variant(91, static_cast<std::int32_t>(-7)),
        DRW_Variant(1004, std::string("ABCD"))};
    return section;
}

bool writeRawBinaryBoundarySection(const DRW_RawDxfSection& source,
                                   std::string& output) {
    std::ostringstream stream;
    dxfRW owner("");
    owner.version = DRW::AC1027;
    owner.binFile = true;
    owner.writer = std::make_unique<dxfWriterBinary>(&stream);
    const bool written = owner.writeRawDxfSection(source);
    output = stream.str();
    return written;
}

void testViewFlagWireKinds(TestContext& t) {
    for (const DRW::Version version : {DRW::AC1014, DRW::AC1015,
                                       DRW::AC1021, DRW::AC1032}) {
        for (const bool binary : {false, true}) {
            for (const bool hasUcs : {false, true}) {
                std::ostringstream output;
                dxfRW owner("");
                owner.version = version;
                owner.binFile = binary;
                if (binary)
                    owner.writer = std::make_unique<dxfWriterBinary>(&output);
                else
                    owner.writer = std::make_unique<dxfWriterAscii>(&output);
                DRW_View view;
                view.name = "WIRE_VIEW";
                view.hasUCS = hasUcs;
                view.cameraPlottable = !hasUcs;
                view.ucsOrigin = DRW_Coord(11.0, 12.0, 13.0);
                view.m_brightness = 1.25;
                t.expect(owner.writeView(&view)
                             && owner.writer->writeString(0, "ENDTAB"),
                         "VIEW flag sentinel writes without a full document");
                std::istringstream input(output.str());
                std::unique_ptr<dxfReader> reader;
                if (binary)
                    reader = std::make_unique<dxfReaderBinary>(&input);
                else
                    reader = std::make_unique<dxfReaderAscii>(&input);
                int code = 0;
                int ucsFlags = 0;
                int cameraFlags = 0;
                int origins = 0;
                int brightness = 0;
                bool ended = false;
                while (reader->readRec(&code)) {
                    if (code == 72) {
                        ++ucsFlags;
                        t.expect(reader->type == dxfReader::INT32
                                     && reader->getInt32() == (hasUcs ? 1 : 0),
                                 "VIEW 72 is a 16-bit integer flag on the wire");
                    } else if (code == 73) {
                        ++cameraFlags;
                        t.expect(reader->type == dxfReader::INT32
                                     && reader->getInt32() == (hasUcs ? 0 : 1),
                                 "VIEW 73 is a 16-bit integer flag on the wire");
                    } else if (code == 110) {
                        ++origins;
                        t.expect(reader->getDouble() == 11.0,
                                 "VIEW flag width preserves following UCS data");
                    } else if (code == 141) {
                        ++brightness;
                        t.expect(reader->getDouble() == 1.25,
                                 "VIEW camera width preserves later light data");
                    } else if (code == 0 && reader->getString() == "ENDTAB") {
                        ended = true;
                        break;
                    }
                }
                const bool modern = version > DRW::AC1018;
                t.expect(ucsFlags == 1 && cameraFlags == (modern ? 1 : 0)
                             && origins == (hasUcs ? 1 : 0)
                             && brightness == (modern ? 1 : 0) && ended,
                         "VIEW flags preserve complete ASCII/binary framing");
            }
        }
    }
}

void testIntegerValuedBooleanWireKinds(TestContext& t) {
    // Preserve integer-valued DXF fields and their following-code framing.
    // DIMASSOC specifically checks that group 70 is trans-space and group 71
    // is the rotated-dimension type.
    for (int kind = 0; kind != 5; ++kind) {
        for (const bool binary : {false, true}) {
            for (const bool flag : {false, true}) {
                std::ostringstream output;
                dxfRW owner("");
                owner.version = kind == 4 ? DRW::AC1021 : DRW::AC1032;
                owner.binFile = binary;
                if (binary)
                    owner.writer = std::make_unique<dxfWriterBinary>(&output);
                else
                    owner.writer = std::make_unique<dxfWriterAscii>(&output);
                bool written = false;
                int flagCode = 0;
                int followingCode = 0;
                if (kind < 2) {
                    DRW_PointCloudDef definition;
                    definition.m_kind = kind == 0
                        ? DRW_PointCloudDef::Definition
                        : DRW_PointCloudDef::DefinitionEx;
                    definition.handle = 0x201u;
                    definition.m_isLoaded = flag;
                    definition.m_pointCount = 0x123456789abcdefULL;
                    written = owner.writePointCloudDef(&definition);
                    flagCode = 280;
                    followingCode = 160;
                } else if (kind == 2) {
                    DRW_Section section;
                    section.handle = 0x202u;
                    section.m_kind = DRW_Section::Manager;
                    section.m_isLive = flag;
                    section.m_sectionCount = 1;
                    section.m_sectionHandles.push_back(0x456u);
                    written = owner.writeSection(&section, "SECTIONMANAGER");
                    flagCode = 70;
                    followingCode = 90;
                } else if (kind == 3) {
                    DRW_DimensionAssociation association;
                    association.handle = 0x203u;
                    association.m_isTransSpace = flag;
                    association.m_rotatedDimensionType = 1;
                    association.m_associativityFlags = 1;
                    DRW_DimensionAssociationOsnapRef ref;
                    ref.m_className = "AcDbOsnapPointRef";
                    ref.m_objectHandle = 0x456u;
                    association.m_osnapRefs.push_back(ref);
                    written = owner.writeDimensionAssociation(&association);
                    flagCode = 70;
                    followingCode = 71;
                } else {
                    DRW_MLeader leader;
                    leader.styleRightAttach = 3;
                    leader.styleTextAngleType = 2;
                    DRW_MLeader::ArrowHeadEntry arrow;
                    arrow.dxfIndex = flag ? 1 : 0;
                    arrow.hasDxfIndex = true;
                    arrow.handle.ref = 0x456u;
                    arrow.handle.ref64 = arrow.handle.ref;
                    leader.arrowHeads.push_back(arrow);
                    written = owner.writeMultiLeader(&leader);
                    flagCode = 94;
                    followingCode = 345;
                }
                t.expect(written
                             && owner.writer->writeString(0, "WIRE_END"),
                         "integer-valued Boolean sentinel writes a full record");
                std::istringstream input(output.str());
                std::unique_ptr<dxfReader> reader;
                if (binary)
                    reader = std::make_unique<dxfReaderBinary>(&input);
                else
                    reader = std::make_unique<dxfReaderAscii>(&input);
                int code = 0;
                int seenFlags = 0;
                bool needFollowing = false;
                bool sawFollowing = false;
                bool needAttachmentFollowing = false;
                bool sawAttachment = false;
                bool sawAttachmentFollowing = false;
                bool ended = false;
                while (reader->readRec(&code)) {
                    if (needAttachmentFollowing) {
                        sawAttachmentFollowing = code == 174
                            && reader->getInt32() == 2;
                        t.expect(sawAttachmentFollowing,
                                 "MLEADER 95 width preserves the next attachment tag");
                        needAttachmentFollowing = false;
                    }
                    if (kind == 4 && code == 95) {
                        sawAttachment = reader->type == dxfReader::INT32
                            && reader->getInt32() == 3;
                        t.expect(sawAttachment,
                                 "MLEADER 95 retains its literal 32-bit value");
                        needAttachmentFollowing = true;
                    }
                    if (needFollowing) {
                        t.expect(code == followingCode,
                                 "integer flag width preserves the next group code");
                        const bool valueMatches = kind < 2
                            ? reader->type == dxfReader::INT64
                                && reader->getInt64() == 0x123456789abcdefLL
                            : kind == 2
                                ? reader->type == dxfReader::INT32
                                    && reader->getInt32() == 1
                            : kind == 3
                                ? reader->type == dxfReader::INT32
                                    && reader->getInt32() == 1
                                : reader->type == dxfReader::STRING
                                    && reader->getString()
                                        == "456";
                        t.expect(valueMatches,
                                 "integer flag width preserves the next value");
                        sawFollowing = code == followingCode && valueMatches;
                        needFollowing = false;
                    }
                    if (code == flagCode) {
                        ++seenFlags;
                        t.expect(reader->type == dxfReader::INT32
                                     && reader->getInt32() == (flag ? 1 : 0),
                                 kind == 4
                                     ? "MLEADER code 94 retains its integer index"
                                     : "Boolean-valued integer tag retains its literal value");
                        needFollowing = true;
                    } else if (code == 0 && reader->getString() == "WIRE_END") {
                        ended = true;
                        break;
                    }
                }
                if (seenFlags != 1 || !sawFollowing || !ended)
                    std::cerr << "integer flag framing: kind=" << kind
                              << " binary=" << binary << " flag=" << flag
                              << " flags=" << seenFlags
                              << " next=" << sawFollowing
                              << " end=" << ended << '\n';
                t.expect(seenFlags == 1 && sawFollowing && ended,
                         "integer-valued Boolean preserves ASCII/binary framing");
                if (kind == 4)
                    t.expect(sawAttachment && sawAttachmentFollowing,
                             "MLEADER integer attachment framing is complete");
            }
        }
    }

    const auto verifyDimensionAssociationReferences =
        [&t](bool binary, bool useResolver) {
            std::ostringstream output;
            dxfRW owner("");
            owner.version = DRW::AC1032;
            owner.binFile = binary;
            owner.setHandleRemap({
                {0x203u, 0xA03u}, {0x110u, 0xA10u},
                {0x220u, 0xA20u}, {0x330u, 0xA30u}, {0x440u, 0xA40u}});
            if (useResolver) {
                owner.setReferenceResolver([](std::uint32_t source) {
                    switch (source) {
                    case 0x220u: return std::uint32_t{0xB20u};
                    case 0x330u: return std::uint32_t{0xB30u};
                    case 0x440u: return std::uint32_t{0xB40u};
                    default: return source;
                    }
                });
            }
            owner.writer = binary
                ? std::unique_ptr<dxfWriter>(
                    std::make_unique<dxfWriterBinary>(&output))
                : std::unique_ptr<dxfWriter>(
                    std::make_unique<dxfWriterAscii>(&output));

            DRW_DimensionAssociation association;
            association.handle = 0x203u;
            association.parentHandle = 0x110u;
            association.m_dimensionHandle = 0x220u;
            association.m_associativityFlags = 3;
            for (const std::uint32_t handle : {0x330u, 0x440u}) {
                DRW_DimensionAssociationOsnapRef ref;
                ref.m_className = "AcDbOsnapPointRef";
                ref.m_objectHandle = handle;
                association.m_osnapRefs.push_back(ref);
            }
            const bool wrote = owner.writeDimensionAssociation(&association)
                && owner.writer->writeString(0, "WIRE_END");
            t.expect(wrote,
                     "DIMASSOC writes after resolving every association handle");
            if (!wrote)
                return;

            std::istringstream input(output.str());
            std::unique_ptr<dxfReader> reader = binary
                ? std::unique_ptr<dxfReader>(
                    std::make_unique<dxfReaderBinary>(&input))
                : std::unique_ptr<dxfReader>(
                    std::make_unique<dxfReaderAscii>(&input));
            std::uint32_t own = 0;
            std::uint32_t objectOwner = 0;
            std::uint32_t dimension = 0;
            std::vector<std::uint32_t> osnapHandles;
            bool inAssociationBody = false;
            bool ended = false;
            int code = 0;
            while (reader->readRec(&code)) {
                if (code == 0) {
                    const std::string recordName = reader->getString();
                    if (recordName == "WIRE_END") {
                        ended = true;
                        break;
                    }
                } else if (code == 5) {
                    own = reader->getHandleString();
                } else if (code == 100) {
                    inAssociationBody = reader->getString() == "AcDbDimAssoc";
                } else if (code == 330) {
                    if (inAssociationBody)
                        dimension = reader->getHandleString();
                    else
                        objectOwner = reader->getHandleString();
                } else if (code == 331) {
                    osnapHandles.push_back(reader->getHandleString());
                }
            }
            const std::vector<std::uint32_t> expectedOsnaps = useResolver
                ? std::vector<std::uint32_t>{0xB30u, 0xB40u}
                : std::vector<std::uint32_t>{0xA30u, 0xA40u};
            t.expect(ended && own == 0xA03u && objectOwner == 0xA10u
                         && dimension == (useResolver ? 0xB20u : 0xA20u)
                         && osnapHandles == expectedOsnaps,
                     useResolver
                         ? "DIMASSOC uses final resolver identities without remapping twice"
                         : "DIMASSOC uses source-handle remaps for identity and links");
        };

    for (const bool binary : {false, true}) {
        verifyDimensionAssociationReferences(binary, false);
        verifyDimensionAssociationReferences(binary, true);

        for (const std::uint32_t missingHandle : {0x220u, 0x440u}) {
            std::ostringstream output;
            output << "sentinel";
            dxfRW owner("");
            owner.version = DRW::AC1032;
            owner.binFile = binary;
            owner.setReferenceResolver([missingHandle](std::uint32_t source) {
                return source == missingHandle ? 0u : source;
            });
            owner.writer = binary
                ? std::unique_ptr<dxfWriter>(
                    std::make_unique<dxfWriterBinary>(&output))
                : std::unique_ptr<dxfWriter>(
                    std::make_unique<dxfWriterAscii>(&output));
            DRW_DimensionAssociation association;
            association.handle = 0x203u;
            association.m_dimensionHandle = 0x220u;
            association.m_associativityFlags = 3;
            for (const std::uint32_t handle : {0x330u, 0x440u}) {
                DRW_DimensionAssociationOsnapRef ref;
                ref.m_className = "AcDbOsnapPointRef";
                ref.m_objectHandle = handle;
                association.m_osnapRefs.push_back(ref);
            }
            t.expect(!owner.writeDimensionAssociation(&association)
                         && output.str() == "sentinel",
                     "DIMASSOC rejects a dropped required link before publishing bytes");
        }
    }
}

void testDimensionAssociationGraphClosure(TestContext& t) {
    const auto readAssociationTargets = [&t](const std::string& bytes,
                                              bool binary,
                                              std::uint32_t expectedDimension,
                                              std::uint32_t expectedOsnap,
                                              std::uint32_t expectedIntersection,
                                              const std::string& expectedXref,
                                              const std::string& expectedIntersectionXref) {
        std::istringstream input(bytes);
        std::unique_ptr<dxfReader> reader = binary
            ? std::unique_ptr<dxfReader>(
                  std::make_unique<dxfReaderBinary>(&input))
            : std::unique_ptr<dxfReader>(
                  std::make_unique<dxfReaderAscii>(&input));
        bool inAssociation = false;
        std::uint32_t dimension = 0;
        std::vector<std::uint32_t> osnaps;
        std::vector<std::uint32_t> intersections;
        std::string xref;
        std::string intersectionXref;
        int code = 0;
        while (reader->readRec(&code)) {
            if (code == 0) {
                inAssociation = reader->getString() == "DIMASSOC";
            } else if (inAssociation && code == 100) {
                inAssociation = reader->getString() == "AcDbDimAssoc";
            } else if (inAssociation && code == 330) {
                dimension = reader->getHandleString();
            } else if (inAssociation && code == 331) {
                osnaps.push_back(reader->getHandleString());
            } else if (inAssociation && code == 332) {
                intersections.push_back(reader->getHandleString());
            } else if (inAssociation && code == 301) {
                xref = reader->getString();
            } else if (inAssociation && code == 302) {
                intersectionXref = reader->getString();
            }
        }
        t.expect(dimension == expectedDimension
                     && osnaps == std::vector<std::uint32_t>{expectedOsnap}
                     && intersections
                         == (expectedIntersection == 0
                                 ? std::vector<std::uint32_t>{}
                                 : std::vector<std::uint32_t>{
                                       expectedIntersection})
                     && xref == expectedXref
                     && intersectionXref == expectedIntersectionXref,
                 "DIMASSOC writes final identities for every emitted graph edge");
    };

    for (const bool binary : {false, true}) {
        {
            std::ostringstream output;
            dxfRW owner("");
            owner.version = DRW::AC1032;
            owner.binFile = binary;
            owner.writer = binary
                ? std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterBinary>(&output))
                : std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterAscii>(&output));

            DRW_DimLinear dimension;
            dimension.handle = 0x220u;
            DRW_Line geometry;
            geometry.handle = 0x330u;
            DRW_Line intersectionGeometry;
            intersectionGeometry.handle = 0x340u;
            const bool entitiesWritten = owner.writeDimension(&dimension)
                && owner.writeLine(&geometry)
                && owner.writeLine(&intersectionGeometry);
            t.expect(entitiesWritten,
                     "DIMASSOC graph test emits dimension and both geometry entities first");
            if (!entitiesWritten)
                continue;

            const std::uint32_t emittedDimension =
                owner.m_writingContext.sourceHandleToMintedMap.at(0x220u);
            const std::uint32_t emittedGeometry =
                owner.m_writingContext.sourceHandleToMintedMap.at(0x330u);
            const std::uint32_t emittedIntersection =
                owner.m_writingContext.sourceHandleToMintedMap.at(0x340u);
            DRW_DimensionAssociation association;
            association.handle = 0x203u;
            association.m_dimensionHandle = 0x220u;
            association.m_associativityFlags = 1;
            DRW_DimensionAssociationOsnapRef ref;
            ref.m_className = "AcDbOsnapPointRef";
            ref.m_hasObjectOsnapType = true;
            ref.m_objectOsnapType = 6u;
            ref.m_objectHandle = 0x330u;
            ref.m_hasIntersectionObjectHandle = true;
            ref.m_intersectionObjectHandle = 0x340u;
            ref.m_hasXrefObjectHandleString = true;
            ref.m_xrefObjectHandleString = "XREF_MAIN_A1";
            ref.m_hasIntersectionXrefObjectHandleString = true;
            ref.m_intersectionXrefObjectHandleString = "XREF_INTERSECTION_B2";
            association.m_osnapRefs.push_back(ref);
            t.expect(owner.writeDimensionAssociation(&association),
                     "DIMASSOC accepts main and intersection references to emitted entities");
            readAssociationTargets(output.str(), binary,
                                   emittedDimension, emittedGeometry,
                                   emittedIntersection, "XREF_MAIN_A1",
                                   "XREF_INTERSECTION_B2");
        }

        for (const bool missingDimension : {false, true}) {
            std::ostringstream output;
            dxfRW owner("");
            owner.version = DRW::AC1032;
            owner.binFile = binary;
            owner.writer = binary
                ? std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterBinary>(&output))
                : std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterAscii>(&output));
            DRW_Line emittedGeometry;
            emittedGeometry.handle = 0x330u;
            const bool entityWritten = owner.writeLine(&emittedGeometry);
            t.expect(entityWritten,
                     "DIMASSOC negative test starts with a valid emitted entity");
            if (!entityWritten)
                continue;
            const std::string prefix = output.str();
            DRW_DimensionAssociation association;
            association.handle = 0x203u;
            association.m_dimensionHandle = missingDimension ? 0x440u : 0u;
            association.m_associativityFlags = 1;
            DRW_DimensionAssociationOsnapRef ref;
            ref.m_className = "AcDbOsnapPointRef";
            ref.m_objectHandle = missingDimension ? 0x330u : 0x440u;
            association.m_osnapRefs.push_back(ref);
            t.expect(!owner.writeDimensionAssociation(&association)
                         && output.str() == prefix,
                     missingDimension
                         ? "DIMASSOC rejects an unemitted dimension target atomically"
                         : "DIMASSOC rejects an unemitted osnap target atomically");
        }

        {
            std::ostringstream output;
            dxfRW owner("");
            owner.version = DRW::AC1032;
            owner.binFile = binary;
            owner.writer = binary
                ? std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterBinary>(&output))
                : std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterAscii>(&output));
            DRW_DimLinear dimension;
            dimension.handle = 0x220u;
            DRW_Line geometry;
            geometry.handle = 0x330u;
            const bool entitiesWritten = owner.writeDimension(&dimension)
                && owner.writeLine(&geometry);
            t.expect(entitiesWritten,
                     "DIMASSOC missing-intersection test emits its other graph edges");
            if (!entitiesWritten)
                continue;
            const std::string prefix = output.str();
            DRW_DimensionAssociation association;
            association.handle = 0x203u;
            association.m_dimensionHandle = 0x220u;
            association.m_associativityFlags = 1u;
            DRW_DimensionAssociationOsnapRef ref;
            ref.m_className = "AcDbOsnapPointRef";
            ref.m_objectOsnapType = 6u;
            ref.m_objectHandle = 0x330u;
            ref.m_hasIntersectionObjectHandle = true;
            ref.m_intersectionObjectHandle = 0x440u;
            association.m_osnapRefs.push_back(ref);
            t.expect(!owner.writeDimensionAssociation(&association)
                         && output.str() == prefix,
                     "DIMASSOC rejects an unemitted intersection target atomically");
        }

        {
            std::ostringstream output;
            dxfRW owner("");
            owner.version = DRW::AC1032;
            owner.binFile = binary;
            owner.writer = binary
                ? std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterBinary>(&output))
                : std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterAscii>(&output));
            const std::uint32_t planned = owner.preallocateEntityHandle(0x220u);
            DRW_Line emittedGeometry;
            emittedGeometry.handle = 0x330u;
            const bool entityWritten = owner.writeLine(&emittedGeometry);
            t.expect(planned != 0 && entityWritten,
                     "DIMASSOC planned-target test sets up a nonempty graph");
            if (!entityWritten)
                continue;
            const std::string prefix = output.str();
            DRW_DimensionAssociation association;
            association.handle = 0x203u;
            association.m_dimensionHandle = 0x220u;
            association.m_associativityFlags = 1;
            DRW_DimensionAssociationOsnapRef ref;
            ref.m_className = "AcDbOsnapPointRef";
            ref.m_objectHandle = 0x330u;
            association.m_osnapRefs.push_back(ref);
            t.expect(!owner.writeDimensionAssociation(&association)
                         && output.str() == prefix,
                     "DIMASSOC does not treat a preallocated handle as emitted");
        }

        {
            std::ostringstream output;
            std::uint32_t redirectedOutput = 0;
            std::uint32_t redirectedDimension = 0;
            dxfRW owner("");
            owner.version = DRW::AC1032;
            owner.binFile = binary;
            owner.setReferenceResolver(
                [&redirectedOutput, &redirectedDimension](
                    std::uint32_t source) {
                if (source == 0x440u)
                    return redirectedOutput;
                if (source == 0x220u)
                    return redirectedDimension;
                return source;
            });
            owner.writer = binary
                ? std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterBinary>(&output))
                : std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterAscii>(&output));
            DRW_DimLinear dimension;
            dimension.handle = 0x220u;
            DRW_Line geometry;
            geometry.handle = 0x330u;
            const bool entitiesWritten = owner.writeDimension(&dimension)
                && owner.writeLine(&geometry);
            t.expect(entitiesWritten,
                     "DIMASSOC resolver test emits its redirected target");
            if (!entitiesWritten)
                continue;
            redirectedDimension =
                owner.m_writingContext.sourceHandleToMintedMap.at(0x220u);
            redirectedOutput =
                owner.m_writingContext.sourceHandleToMintedMap.at(0x330u);
            DRW_DimensionAssociation association;
            association.handle = 0x203u;
            association.m_dimensionHandle = 0x220u;
            association.m_associativityFlags = 1;
            DRW_DimensionAssociationOsnapRef ref;
            ref.m_className = "AcDbOsnapPointRef";
            ref.m_objectHandle = 0x440u;
            association.m_osnapRefs.push_back(ref);
            t.expect(owner.writeDimensionAssociation(&association),
                     "DIMASSOC accepts resolver redirects to emitted entities");
            readAssociationTargets(
                output.str(), binary, redirectedDimension, redirectedOutput,
                0u, "", "");
        }

        {
            std::ostringstream output;
            std::uint32_t emittedDimension = 0;
            dxfRW owner("");
            owner.version = DRW::AC1032;
            owner.binFile = binary;
            owner.setReferenceResolver([&emittedDimension](std::uint32_t source) {
                if (source == 0x220u)
                    return emittedDimension;
                return source == 0x440u ? std::uint32_t{0x555u} : source;
            });
            owner.writer = binary
                ? std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterBinary>(&output))
                : std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterAscii>(&output));
            DRW_DimLinear dimension;
            dimension.handle = 0x220u;
            DRW_Line geometry;
            geometry.handle = 0x330u;
            const bool entitiesWritten = owner.writeDimension(&dimension)
                && owner.writeLine(&geometry);
            t.expect(entitiesWritten,
                     "DIMASSOC unresolved-resolver test emits graph entities");
            if (!entitiesWritten)
                continue;
            emittedDimension =
                owner.m_writingContext.sourceHandleToMintedMap.at(0x220u);
            const std::string prefix = output.str();
            DRW_DimensionAssociation association;
            association.handle = 0x203u;
            association.m_dimensionHandle = 0x220u;
            association.m_associativityFlags = 1;
            DRW_DimensionAssociationOsnapRef ref;
            ref.m_className = "AcDbOsnapPointRef";
            ref.m_objectHandle = 0x440u;
            association.m_osnapRefs.push_back(ref);
            t.expect(!owner.writeDimensionAssociation(&association)
                         && output.str() == prefix,
                     "DIMASSOC rejects a resolver identity outside the emitted graph");
        }
    }
}

std::vector<std::string> numericWireProjection(const std::string& bytes,
                                                bool binary, bool& ended,
                                                DxfClassifierProfile profile =
                                                    DxfClassifierProfile::StandaloneSafe) {
    std::istringstream input(bytes);
    std::unique_ptr<dxfReader> reader;
    if (binary)
        reader = std::make_unique<dxfReaderBinary>(&input);
    else
        reader = std::make_unique<dxfReaderAscii>(&input);
    reader->setClassifierProfile(profile);
    std::vector<std::string> result;
    int code = 0;
    ended = false;
    while (reader->readRec(&code)) {
        std::ostringstream value;
        value << code << ':';
        switch (reader->type) {
        case dxfReader::INT32:
        case dxfReader::BOOL: value << reader->getInt32(); break;
        case dxfReader::INT64: value << reader->getInt64(); break;
        case dxfReader::DOUBLE: value << std::hexfloat << reader->getDouble(); break;
        case dxfReader::STRING:
        case dxfReader::BINARY: value << reader->getString(); break;
        default: return {};
        }
        result.push_back(value.str());
        if (code == 0 && reader->getString() == "WIRE_END") {
            ended = true;
            break;
        }
    }
    return result;
}

void testAssociativeDependencyWriter(TestContext& t) {
    const auto handleText = [](std::uint32_t handle) {
        std::ostringstream value;
        value << std::uppercase << std::hex << handle;
        return value.str();
    };
    const auto remap = std::map<std::uint32_t, std::uint32_t>{
        {0x100u, 0x3E8u}, {0x101u, 0x3E9u}, {0x102u, 0x3EAu},
        {0x103u, 0x3EBu}, {0x104u, 0x3ECu}, {0x105u, 0x3EDu}};
    DRW_Class baseDependencyClass;
    DRW_Class valueDependencyClass;
    dxfRW classProbe("");
    t.expect(classProbe.dxfClassForRecordName(
                 "ACDBASSOCDEPENDENCY", baseDependencyClass)
                 && baseDependencyClass.className == "AcDbAssocDependency"
                 && baseDependencyClass.proxyFlag == 1025
                 && classProbe.dxfClassForRecordName(
                     "ACDBASSOCVALUEDEPENDENCY", valueDependencyClass)
                 && valueDependencyClass.className
                     == "AcDbAssocValueDependency"
                 && valueDependencyClass.proxyFlag == 1025,
             "associative dependency classes match independent source metadata");

    for (const bool binary : {false, true}) {
        for (const char* name : {"ACDBASSOCDEPENDENCY",
                                 "ACDBASSOCVALUEDEPENDENCY"}) {
            for (unsigned nameState = 0; nameState != 3; ++nameState) {
                for (unsigned flags = 0; flags != 16; ++flags) {
                    std::ostringstream output(
                        std::ios::out | std::ios::binary);
                    dxfRW owner("");
                    owner.version = DRW::AC1024;
                    owner.binFile = binary;
                    owner.setHandleRemap(remap);
                    if (binary)
                        owner.writer = std::make_unique<dxfWriterBinary>(
                            &output);
                    else
                        owner.writer = std::make_unique<dxfWriterAscii>(
                            &output);

                    DRW_AssociativeObject dependency(name);
                    dependency.handle = 0x100u;
                    dependency.parentHandle = 0x101u;
                    dependency.m_classVersion = 3u;
                    dependency.m_status = -7;
                    dependency.m_dependencyFieldsParsed = true;
                    dependency.m_dependencyIsRead = (flags & 1u) != 0u;
                    dependency.m_dependencyIsWrite = (flags & 2u) != 0u;
                    dependency.m_dependencyIsAttachedToObject =
                        (flags & 4u) != 0u;
                    dependency.m_dependencyIsDelegatingToOwningAction =
                        (flags & 8u) != 0u;
                    dependency.m_dependencyOrder = -2;
                    dependency.m_dependencyNamePresent = nameState != 0u;
                    if (nameState == 2u)
                        dependency.m_dependencyName = "dependency-name";
                    dependency.m_dependencyHandle = 0x102u;
                    dependency.m_readDependencyHandle = 0x103u;
                    dependency.m_dependencyNodeHandle = 0x104u;
                    dependency.m_writeDependencyHandle = 0x104u;
                    dependency.m_dependencyBodyHandle = 0x105u;
                    dependency.m_dependencyBodyId = -9;

                    const bool wrote =
                        owner.writeAssociativeDependency(&dependency)
                        && owner.writer->writeString(0, "WIRE_END");
                    bool ended = false;
                    const auto tags = numericWireProjection(
                        output.str(), binary, ended);
                    std::vector<std::string> expected{
                        std::string("0:") + name,
                        "5:" + handleText(0x3E8u),
                        "330:" + handleText(0x3E9u),
                        "100:AcDbAssocDependency", "90:3", "90:-7",
                        std::string("290:") + ((flags & 1u) ? "1" : "0"),
                        std::string("290:") + ((flags & 2u) ? "1" : "0"),
                        std::string("290:") + ((flags & 4u) ? "1" : "0"),
                        std::string("290:") + ((flags & 8u) ? "1" : "0"),
                        "90:-2", "330:" + handleText(0x3EAu),
                        std::string("290:") + (nameState != 0u ? "1" : "0")};
                    if (nameState != 0u)
                        expected.push_back(nameState == 1u
                            ? "1:" : "1:dependency-name");
                    expected.insert(expected.end(), {
                        "330:" + handleText(0x3EBu),
                        "330:" + handleText(0x3ECu),
                        "360:" + handleText(0x3EDu), "90:-9", "0:WIRE_END"});
                    t.expect(wrote && ended && tags == expected,
                             "AC1024 associative dependency fields survive ASCII/binary wire readback");
                }
            }
        }
    }

    const auto rejectedWithoutOutput = [](DRW::Version version,
                                          DRW_AssociativeObject dependency) {
        std::ostringstream output;
        dxfRW owner("");
        owner.version = version;
        owner.writer = std::make_unique<dxfWriterAscii>(&output);
        return !owner.writeAssociativeDependency(&dependency)
            && output.str().empty();
    };
    DRW_AssociativeObject incomplete("ACDBASSOCDEPENDENCY");
    incomplete.handle = 0x200u;
    t.expect(rejectedWithoutOutput(DRW::AC1024, incomplete),
             "partial associative dependency prefix refuses atomically");

    DRW_AssociativeObject unsupportedVersion("ACDBASSOCDEPENDENCY");
    unsupportedVersion.handle = 0x200u;
    unsupportedVersion.m_dependencyFieldsParsed = true;
    t.expect(rejectedWithoutOutput(DRW::AC1027, unsupportedVersion),
             "unqualified associative dependency target version refuses atomically");

    DRW_AssociativeObject unsupportedClass("ACDBASSOCACTION");
    unsupportedClass.handle = 0x200u;
    unsupportedClass.m_dependencyFieldsParsed = true;
    t.expect(rejectedWithoutOutput(DRW::AC1024, unsupportedClass),
             "unqualified associative class refuses atomically");

    DRW_AssociativeObject inconsistentAlias("ACDBASSOCDEPENDENCY");
    inconsistentAlias.handle = 0x200u;
    inconsistentAlias.m_dependencyFieldsParsed = true;
    inconsistentAlias.m_dependencyNodeHandle = 0x202u;
    inconsistentAlias.m_writeDependencyHandle = 0x203u;
    t.expect(rejectedWithoutOutput(DRW::AC1024, inconsistentAlias),
             "conflicting historical node-handle alias refuses atomically");

    DRW_AssociativeObject unresolvedReference("ACDBASSOCDEPENDENCY");
    unresolvedReference.handle = 0x200u;
    unresolvedReference.m_dependencyFieldsParsed = true;
    unresolvedReference.m_dependencyHandle = 0x201u;
    std::ostringstream unresolvedOutput;
    dxfRW unresolvedWriter("");
    unresolvedWriter.version = DRW::AC1024;
    unresolvedWriter.setReferenceResolver([](std::uint32_t) {
        return std::uint32_t{0};
    });
    unresolvedWriter.writer = std::make_unique<dxfWriterAscii>(
        &unresolvedOutput);
    t.expect(!unresolvedWriter.writeAssociativeDependency(
                 &unresolvedReference)
                 && unresolvedOutput.str().empty()
                 && unresolvedReference.handle == 0x200u,
             "unresolved dependency reference rolls back record and source identity");

    DRW_AssociativeObject rollbackRetry("ACDBASSOCDEPENDENCY");
    rollbackRetry.handle = 0x210u;
    rollbackRetry.parentHandle = 0u;
    rollbackRetry.m_dependencyFieldsParsed = true;
    rollbackRetry.m_dependencyNamePresent = true;
    rollbackRetry.m_dependencyName = "retry-name";
    const bool badExtDataAdded = rollbackRetry.addExtData(
        std::make_unique<DRW_Variant>(1004, UTF8STRING("not-binary")));
    std::ostringstream failedOutput;
    dxfRW failedWriter("");
    failedWriter.version = DRW::AC1024;
    failedWriter.writer = std::make_unique<dxfWriterAscii>(&failedOutput);
    const bool lateFailureRolledBack = badExtDataAdded
        && !failedWriter.writeAssociativeDependency(&rollbackRetry)
        && failedOutput.str().empty()
        && rollbackRetry.handle == 0x210u
        && rollbackRetry.m_dependencyName == "retry-name";
    rollbackRetry.clearExtData();
    std::ostringstream correctedOutput;
    dxfRW correctedWriter("");
    correctedWriter.version = DRW::AC1024;
    correctedWriter.writer = std::make_unique<dxfWriterAscii>(
        &correctedOutput);
    const bool sameObjectRetry =
        correctedWriter.writeAssociativeDependency(&rollbackRetry)
        && correctedWriter.writer->writeString(0, "WIRE_END");
    t.expect(lateFailureRolledBack && sameObjectRetry
                 && correctedOutput.str().find("retry-name")
                     != std::string::npos,
             "late EED failure rolls back bytes and permits corrected same-object retry");

    // Correcting a rejected model on a fresh writer succeeds; no failure state
    // or bytes from the refused attempt can leak into the retry output.
    DRW_AssociativeObject corrected("ACDBASSOCDEPENDENCY");
    corrected.handle = 0x200u;
    corrected.parentHandle = 0u;
    corrected.m_dependencyFieldsParsed = true;
    corrected.m_dependencyNamePresent = false;
    std::ostringstream retryOutput;
    dxfRW retryWriter("");
    retryWriter.version = DRW::AC1024;
    retryWriter.writer = std::make_unique<dxfWriterAscii>(&retryOutput);
    bool wroteRetry = retryWriter.writeAssociativeDependency(&corrected)
        && retryWriter.writer->writeString(0, "WIRE_END");
    bool retryEnded = false;
    const auto retryTags = numericWireProjection(
        retryOutput.str(), false, retryEnded);
    t.expect(wroteRetry && retryEnded
                 && std::find(retryTags.begin(), retryTags.end(), "290:0")
                     != retryTags.end()
                 && std::find_if(retryTags.begin(), retryTags.end(),
                        [](const std::string& tag) {
                            return tag.rfind("1:", 0) == 0;
                        }) == retryTags.end(),
             "corrected associative dependency retry emits absent name without stale data");
}

void expectTypedNumericWire(TestContext& t,
                           const std::function<bool(dxfRW&)>& emit,
                           const std::vector<std::string>& expected,
                           const char* family) {
    std::vector<std::string> asciiProjection;
    for (const bool binary : {false, true}) {
        std::ostringstream output;
        dxfRW owner("");
        owner.version = DRW::AC1032;
        owner.binFile = binary;
        if (binary)
            owner.writer = std::make_unique<dxfWriterBinary>(&output);
        else
            owner.writer = std::make_unique<dxfWriterAscii>(&output);
        const bool written = emit(owner);
        t.expect(written && owner.writer->writeString(0, "WIRE_END"), family);
        bool ended = false;
        const auto projection = numericWireProjection(output.str(), binary,
                                                       ended);
        t.expect(ended, "typed numeric widths preserve the following record");
        for (const auto& tag : expected)
            t.expect(std::find(projection.begin(), projection.end(), tag)
                         != projection.end(),
                     "typed numeric output retains the literal field value");
        if (!binary)
            asciiProjection = projection;
        else
            t.expect(projection == asciiProjection,
                     "typed ASCII/binary tag order and values agree exactly");
    }
}

void expectTypedNumericRangeRejected(
    TestContext& t, const std::function<bool(dxfRW&)>& emit) {
    for (const bool binary : {false, true}) {
        std::ostringstream output;
        dxfRW owner("");
        owner.version = DRW::AC1032;
        owner.binFile = binary;
        if (binary)
            owner.writer = std::make_unique<dxfWriterBinary>(&output);
        else
            owner.writer = std::make_unique<dxfWriterAscii>(&output);
        t.expect(!emit(owner) && output.str().empty(),
                 "unrepresentable typed I16 fails without publishing a record");
    }
}

void testCommonAppDataBaseScope(TestContext& t) {
    const auto configure = [](auto& entry, bool retainedCommonGroups) {
        entry.appData.push_back({{102, std::string("{SCOPE_TEST")},
                                 {1, std::string("alpha")},
                                 {330, std::string("0")},
                                 {102, std::string("}")}});
        entry.reactorHandles = {0x1234u};
        entry.xDictHandle = 0x2345u;
        if (retainedCommonGroups) {
            entry.appData.push_back({{102, std::string("{ACAD_REACTORS")},
                                     {330, std::string("1234")},
                                     {102, std::string("}")}});
            entry.appData.push_back({{102, std::string("{ACAD_XDICTIONARY")},
                                     {360, std::string("2345")},
                                     {102, std::string("}")}});
        }
    };
    const auto emitTable = [&configure](dxfRW& owner, auto& entry,
                                        bool retainedCommonGroups,
                                        auto write) {
        entry.name = "SCOPE_SENTINEL";
        configure(entry, retainedCommonGroups);
        if (!entry.addExtData(std::make_unique<DRW_Variant>(
                1001, std::string("SCOPE_APP")))
            || !entry.addExtData(std::make_unique<DRW_Variant>(
                1000, std::string("trailing"))))
            return false;
        return (owner.*write)(&entry);
    };
    for (const DRW::Version version : {DRW::AC1014, DRW::AC1015,
                                       DRW::AC1021, DRW::AC1032}) {
        for (const bool binary : {false, true}) {
            for (const bool retained : {false, true}) {
                for (int kind = 0; kind < 10; ++kind) {
                    std::ostringstream output;
                    dxfRW owner("");
                    owner.version = version;
                    owner.binFile = binary;
                    if (binary)
                        owner.writer = std::make_unique<dxfWriterBinary>(&output);
                    else
                        owner.writer = std::make_unique<dxfWriterAscii>(&output);
                    bool written = false;
                    switch (kind) {
                    case 0: { DRW_LType e; written = emitTable(owner, e, retained, &dxfRW::writeLineType); break; }
                    case 1: { DRW_Layer e; written = emitTable(owner, e, retained, &dxfRW::writeLayer); break; }
                    case 2: { DRW_Textstyle e; written = emitTable(owner, e, retained, &dxfRW::writeTextstyle); break; }
                    case 3: { DRW_Vport e; written = emitTable(owner, e, retained, &dxfRW::writeVport); break; }
                    case 4: { DRW_Dimstyle e; written = emitTable(owner, e, retained, &dxfRW::writeDimstyle); break; }
                    case 5: { DRW_View e; written = emitTable(owner, e, retained, &dxfRW::writeView); break; }
                    case 6: { DRW_UCS e; written = emitTable(owner, e, retained, &dxfRW::writeUCS); break; }
                    case 7: { DRW_AppId e; written = emitTable(owner, e, retained, &dxfRW::writeAppId); break; }
                    case 8: {
                        DRW_Line e;
                        configure(e, retained);
                        e.secPoint = DRW_Coord(4, 6, 8);
                        e.extData.push_back(std::make_shared<DRW_Variant>(
                            1001, std::string("SCOPE_APP")));
                        e.extData.push_back(std::make_shared<DRW_Variant>(
                            1000, std::string("trailing")));
                        written = owner.writeLine(&e);
                        break;
                    }
                    case 9: { DRW_PlotSettings e; written = emitTable(owner, e, retained, &dxfRW::writePlotSettings); break; }
                    }
                    t.expect(written && owner.writer->writeString(0, "WIRE_END"),
                             "common-scope typed sentinel writes");
                    bool ended = false;
                    const auto tags = numericWireProjection(output.str(), binary,
                                                             ended);
                    t.expect(ended, "common-scope framing reaches next record");
                    const auto firstSubclass = std::find_if(tags.begin(), tags.end(),
                        [](const auto& tag) { return tag.rfind("100:", 0) == 0; });
                    const auto xdata = std::find(tags.begin(), tags.end(),
                                                 "1001:SCOPE_APP");
                    t.expect(firstSubclass != tags.end() && xdata != tags.end(),
                             "common-scope record keeps subclasses and XDATA");
                    for (const auto& marker : {"102:{SCOPE_TEST", "102:{ACAD_REACTORS",
                                               "102:{ACAD_XDICTIONARY"}) {
                        const auto group = std::find(tags.begin(), tags.end(), marker);
                        t.expect(group < firstSubclass && group != tags.end(),
                                 "common application group precedes first subclass");
                        t.expect(std::count(tags.begin(), tags.end(), marker) == 1,
                                 "retained and typed common groups emit exactly once");
                    }
                    t.expect(std::find(tags.begin(), tags.end(), "1:alpha") < firstSubclass
                                 && std::find(tags.begin(), tags.end(), "360:2345") < firstSubclass,
                             "common group payload remains in base scope");
                    t.expect(xdata != tags.end() && xdata > firstSubclass
                                 && tags.end() - xdata == 3
                                 && *(xdata + 1) == "1000:trailing",
                             "XDATA remains trailing after subclass body");
                }
            }
        }
    }
}

void testTypedObjectCommonMetadata(TestContext& t) {
    const auto exercise = [&t](const char* family, auto entry, auto emit,
                               bool includeReferenceData = true) {
        for (const bool binary : {false, true}) {
            for (const bool retainedGroups : {false, true}) {
                auto object = entry;
                object.handle = 0x500u;
                object.parentHandle = 0x600u;
                if (includeReferenceData) {
                    object.reactorHandles = {0x1110u, 0x1111u};
                    object.xDictHandle = 0x2110u;
                }
                std::list<DRW_Variant> metadataGroup = {
                    {102, std::string("{OBJECT_META")},
                    {1, std::string("alpha")}, {70, 7}};
                if (includeReferenceData) {
                    metadataGroup.emplace_back(330, std::string("0"));
                    metadataGroup.emplace_back(320, std::string("1110"));
                    metadataGroup.emplace_back(340, std::string("2110"));
                }
                metadataGroup.emplace_back(102, std::string("}"));
                object.appData.push_back(std::move(metadataGroup));
                if (retainedGroups && includeReferenceData) {
                    // Typed DXF parsing stores an opener without its brace.
                    // The retained groups must suppress the typed fallbacks.
                    object.appData.push_back({{102, std::string("ACAD_REACTORS")},
                        {330, std::string("1110")}, {330, std::string("1111")},
                        {102, std::string("}")}});
                    object.appData.push_back({{102, std::string("ACAD_XDICTIONARY")},
                        {360, std::string("2110")}, {102, std::string("}")}});
                }
                t.expect(object.addExtData(std::make_unique<DRW_Variant>(
                             1001, std::string("OBJECT_META_APP")))
                             && object.addExtData(std::make_unique<DRW_Variant>(
                                 1000, std::string("trailing"))),
                         "object carrier accepts trailing XDATA");
                std::ostringstream output;
                dxfRW owner("");
                owner.version = DRW::AC1032;
                owner.binFile = binary;
                owner.setHandleRemap({{0x1110u, 0xA110u}, {0x1111u, 0xA111u},
                                       {0x2110u, 0xB110u}});
                if (binary)
                    owner.writer = std::make_unique<dxfWriterBinary>(&output);
                else
                    owner.writer = std::make_unique<dxfWriterAscii>(&output);
                t.expect(emit(owner, object)
                             && owner.writer->writeString(0, "WIRE_END"), family);
                if (retainedGroups && includeReferenceData) {
                    const auto hasMarker = [](const auto& group,
                                              const char* expected) {
                        return !group.empty()
                            && group.front().content.s != nullptr
                            && *group.front().content.s == expected;
                    };
                    auto reactorGroup = object.appData.begin();
                    if (reactorGroup != object.appData.end())
                        ++reactorGroup;
                    auto xDictionaryGroup = reactorGroup;
                    if (xDictionaryGroup != object.appData.end())
                        ++xDictionaryGroup;
                    t.expect(object.appData.size() == 3u
                                 && reactorGroup != object.appData.end()
                                 && xDictionaryGroup != object.appData.end()
                                 && hasMarker(*reactorGroup, "ACAD_REACTORS")
                                 && hasMarker(*xDictionaryGroup, "ACAD_XDICTIONARY"),
                             "borrowed app-data marker scans leave unbraced caller markers unchanged");
                }
                bool ended = false;
                const auto tags = numericWireProjection(output.str(), binary, ended);
                t.expect(ended, "common object metadata preserves next-record framing");
                const auto subclass = std::find_if(tags.begin(), tags.end(),
                    [](const auto& tag) { return tag.rfind("100:", 0) == 0; });
                t.expect(subclass != tags.end(), "common object keeps first subclass");
                std::vector<const char*> markers = {"102:{OBJECT_META"};
                if (includeReferenceData) {
                    markers.push_back("102:{ACAD_REACTORS");
                    markers.push_back("102:{ACAD_XDICTIONARY");
                }
                for (const char* marker : markers) {
                    const auto found = std::find(tags.begin(), tags.end(), marker);
                    t.expect(found != tags.end() && found < subclass
                                 && std::count(tags.begin(), tags.end(), marker) == 1,
                             "common object group emits once in base scope");
                }
                std::vector<const char*> values = {"1:alpha", "70:7"};
                if (includeReferenceData) {
                    values.insert(values.end(), {"330:0", "320:1110", "340:B110",
                        "330:A110", "330:A111", "360:B110"});
                }
                for (const char* value : values) {
                    const auto found = std::find(tags.begin(), tags.end(), value);
                    t.expect(found != tags.end() && found < subclass,
                             "object common references remap while arbitrary handles do not");
                }
                if (includeReferenceData) {
                    for (const char* stale : {"330:1110", "330:1111", "360:2110", "340:2110"})
                        t.expect(std::find(tags.begin(), tags.end(), stale) == tags.end(),
                                 "object common metadata contains no stale reference identity");
                }
                const auto xdata = std::find(tags.begin(), tags.end(),
                                             "1001:OBJECT_META_APP");
                t.expect(xdata != tags.end() && xdata > subclass
                             && tags.end() - xdata == 3
                             && *(xdata + 1) == "1000:trailing",
                         "object XDATA remains trailing after body");
                if (std::string(family) == "XRECORD") {
                    const auto body = std::find(tags.begin(), tags.end(), "102:EXTNAMES");
                    t.expect(body != tags.end() && body > subclass && body < xdata
                                 && *(body + 1) == "1:ordered-body",
                             "XRECORD body 102 remains application payload in order");
                }
            }
        }
    };
    exercise("UNDERLAYDEFINITION", DRW_UnderlayDefinition{}, [](auto& w, auto& e) { return w.writeUnderlayDefinition(&e); });
    exercise("POINTCLOUDDEF", DRW_PointCloudDef{}, [](auto& w, auto& e) { return w.writePointCloudDef(&e); });
    exercise("NAVISWORKSMODELDEF", DRW_NavisworksModelDef{}, [](auto& w, auto& e) { return w.writeNavisworksModelDef(&e); });
    exercise("POINTCLOUDCOLORMAP", DRW_PointCloudColorMap{}, [](auto& w, auto& e) { return w.writePointCloudColorMap(&e); });
    exercise("PLOTSETTINGS", DRW_PlotSettings{}, [](auto& w, auto& e) { return w.writePlotSettings(&e); });
    exercise("LAYOUT", DRW_Layout{}, [](auto& w, auto& e) { return w.writeLayout(&e); });
    exercise("DICTIONARY", DRW_Dictionary{}, [](auto& w, auto& e) { return w.writeDictionary(&e); });
    DRW_XRecord record;
    record.m_dataEntries = {{102, std::string("EXTNAMES")}, {1, std::string("ordered-body")}};
    exercise("XRECORD", record, [](auto& w, auto& e) { return w.writeXRecord(&e); });
    exercise("SUN", DRW_Sun{}, [](auto& w, auto& e) { return w.writeSun(&e); });
    exercise("SCALE", DRW_Scale{}, [](auto& w, auto& e) { return w.writeScale(&e); });
    exercise("DICTIONARYVAR", DRW_DictionaryVar{}, [](auto& w, auto& e) { return w.writeDictionaryVar(&e); });
    exercise("RASTERVARIABLES", DRW_RasterVariables{}, [](auto& w, auto& e) { return w.writeRasterVariables(&e); });
    exercise("GEODATA", DRW_GeoData{}, [](auto& w, auto& e) { return w.writeGeoData(&e); });
    DRW_SpatialFilter spatialFilter;
    spatialFilter.m_boundaryPoints = {
        {1.25, -2.5, 0.0}, {4.75, 6.5, 0.0}};
    for (std::size_t i = 0; i < 12u; ++i) {
        spatialFilter.m_inverseInsertTransform.push_back(100.0 + i);
        spatialFilter.m_insertTransform.push_back(200.0 + i);
    }
    exercise("SPATIAL_FILTER", spatialFilter,
             [](auto& w, auto& e) { return w.writeSpatialFilter(&e); });
    DRW_SortEntsTable sort;
    sort.m_blockOwnerHandle = 0x600u;
    exercise("SORTENTSTABLE", sort, [](auto& w, auto& e) { return w.writeSortEntsTable(&e); });
    exercise("FIELD", DRW_Field{}, [](auto& w, auto& e) { return w.writeField(&e); });
    exercise("FIELDLIST", DRW_FieldList{}, [](auto& w, auto& e) { return w.writeFieldList(&e); });
    exercise("MLEADERSTYLE", DRW_MLeaderStyle{}, [](auto& w, auto& e) { return w.writeMLeaderStyle(&e); });
    exercise("MLINESTYLE", DRW_MLineStyle{}, [](auto& w, auto& e) { return w.writeMLineStyle(&e); });
    exercise("WIPEOUTVARIABLES", DRW_WipeoutVariables{}, [](auto& w, auto& e) { return w.writeWipeoutVariables(&e); });
    exercise("MATERIAL", DRW_Material{}, [](auto& w, auto& e) { return w.writeMaterial(&e); });
    // Background's writer independently refuses unrooted reactors,
    // extension dictionaries, and handle-bearing app data. Exercise its
    // common textual metadata here; rooted reference graphs stay in the
    // dedicated Background tests.
    exercise("BACKGROUND", DRW_Background{}, [](auto& w, auto& e) { return w.writeBackground(&e, "SOLIDBACKGROUND"); }, false);
    exercise("SUNSTUDY", DRW_SunStudy{}, [](auto& w, auto& e) { return w.writeSunStudy(&e); });
    exercise("MOTIONPATH", DRW_MotionPath{}, [](auto& w, auto& e) { return w.writeMotionPath(&e); });
    exercise("CURVEPATH", DRW_CurvePath{}, [](auto& w, auto& e) { return w.writeCurvePath(&e); });
    exercise("POINTPATH", DRW_PointPath{}, [](auto& w, auto& e) { return w.writePointPath(&e); });
    exercise("OBJECTPTR", DRW_ObjectPtr{}, [](auto& w, auto& e) { return w.writeObjectPtr(&e); });
    exercise("PARTIALVIEWINGINDEX", DRW_PartialViewingIndex{}, [](auto& w, auto& e) { return w.writePartialViewingIndex(&e); });
    exercise("RENDERSETTINGS", DRW_RenderSettings{}, [](auto& w, auto& e) { return w.writeRenderSettings(&e, "RENDERSETTINGS"); });
    exercise("SECTION", DRW_Section{}, [](auto& w, auto& e) { return w.writeSection(&e, "SECTIONMANAGER"); });
    exercise("DBCOLOR", DRW_DbColor{}, [](auto& w, auto& e) { return w.writeDbColor(&e, "DBCOLOR"); });
    exercise("DIMASSOC", DRW_DimensionAssociation{}, [](auto& w, auto& e) { return w.writeDimensionAssociation(&e); });
    exercise("EVALUATIONGRAPH", DRW_EvaluationGraph{}, [](auto& w, auto& e) { return w.writeEvaluationGraph(&e, "ACAD_EVALUATION_GRAPH"); });
}

void testMlineStyleColorAdmission(TestContext& t) {
    for (const bool binary : {false, true}) {
        const auto rejectsWithoutPublication = [&](const char* label,
                                                   DRW_MLineStyle style) {
            std::ostringstream output;
            dxfRW writer("");
            writer.version = DRW::AC1032;
            writer.binFile = binary;
            writer.writer = binary
                ? std::unique_ptr<dxfWriter>(
                    std::make_unique<dxfWriterBinary>(&output))
                : std::unique_ptr<dxfWriter>(
                    std::make_unique<dxfWriterAscii>(&output));
            style.handle = 0xA500u;
            style.parentHandle = 0xA600u;
            style.name = "CMC_STYLE";
            const std::string message = std::string(label)
                + (binary ? " binary" : " ASCII")
                + " refuses before record publication";
            t.expect(!writer.writeMLineStyle(&style) && output.str().empty(),
                     message.c_str());
        };

        DRW_MLineStyle rgbFill;
        rgbFill.fillColor = 256;
        rgbFill.fillColor24 = 0x00A600;
        rgbFill.fillColorMethod = 0xC2;
        rejectsWithoutPublication("RGB fill", rgbFill);

        DRW_MLineStyle namedBookFill;
        namedBookFill.fillColor = 104;
        namedBookFill.fillColorMethod = 0xC3;
        namedBookFill.fillColorName = "named color";
        namedBookFill.fillColorBookName = "book palette";
        rejectsWithoutPublication("named/book fill", namedBookFill);

        DRW_MLineStyle flaggedColor;
        flaggedColor.fillColor = 104;
        flaggedColor.fillColorMethod = 0xC3;
        flaggedColor.fillColorFlags = 0x80;
        rejectsWithoutPublication("unknown CMC flags", flaggedColor);

        DRW_MLineStyle rgbElement;
        DRW_MLineElement element;
        element.offset = 0.5;
        element.color = 256;
        element.color24 = 0x00A600;
        element.colorMethod = 0xC2;
        element.linetype = "CONTINUOUS";
        rgbElement.elements.push_back(element);
        rejectsWithoutPublication("RGB element", rgbElement);

        for (const double invalidAngle : {
                 std::numeric_limits<double>::quiet_NaN(),
                 std::numeric_limits<double>::infinity(),
                 -std::numeric_limits<double>::infinity()}) {
            for (const bool startAngle : {true, false}) {
                DRW_MLineStyle invalid;
                invalid.name = "NONFINITE_ANGLE";
                if (startAngle)
                    invalid.startAngle = invalidAngle;
                else
                    invalid.endAngle = invalidAngle;
                const std::string label = std::string(startAngle
                    ? "non-finite start angle " : "non-finite end angle ")
                    + (std::isnan(invalidAngle) ? "NaN"
                       : invalidAngle > 0.0 ? "+Inf" : "-Inf");
                rejectsWithoutPublication(label.c_str(), invalid);
            }
        }

        std::ostringstream finiteOutput;
        dxfRW finiteWriter("");
        finiteWriter.version = DRW::AC1032;
        finiteWriter.binFile = binary;
        finiteWriter.writer = binary
            ? std::unique_ptr<dxfWriter>(
                std::make_unique<dxfWriterBinary>(&finiteOutput))
            : std::unique_ptr<dxfWriter>(
                std::make_unique<dxfWriterAscii>(&finiteOutput));
        DRW_MLineStyle finiteAngles;
        finiteAngles.handle = 0xA500u;
        finiteAngles.parentHandle = 0xA600u;
        finiteAngles.name = "FINITE_ANGLE";
        finiteAngles.startAngle = 1.25;
        finiteAngles.endAngle = 2.5;
        bool ended = false;
        const bool finiteAndSentinel =
            finiteWriter.writeMLineStyle(&finiteAngles)
            && finiteWriter.writer->writeString(0, "WIRE_END");
        const auto finiteTags = numericWireProjection(
            finiteOutput.str(), binary, ended);
        const auto finiteRecord = std::find(
            finiteTags.begin(), finiteTags.end(), "0:MLINESTYLE");
        const auto sentinelRecord = std::find(
            finiteTags.begin(), finiteTags.end(), "0:WIRE_END");
        t.expect(finiteAndSentinel && ended
                     && finiteRecord != finiteTags.end()
                     && sentinelRecord != finiteTags.end()
                     && finiteRecord < sentinelRecord,
                 binary
                     ? "finite MLINESTYLE and following sentinel write in binary after fresh-writer refusal controls"
                     : "finite MLINESTYLE and following sentinel write in ASCII after fresh-writer refusal controls");
    }

    std::ostringstream output;
    dxfRW writer("");
    writer.version = DRW::AC1032;
    writer.binFile = false;
    writer.writer = std::make_unique<dxfWriterAscii>(&output);
    writer.setHandleRemap({{0x500u, 0xA500u}, {0x600u, 0xA600u}});
    writer.getWritingContext()->lineTypesMap.emplace_back("CONTINUOUS", 0x800u);
    DRW_MLineStyle representable;
    representable.handle = 0x500u;
    representable.parentHandle = 0x600u;
    representable.name = "ACI_STYLE";
    DRW_MLineElement line;
    line.offset = 0.5;
    line.color = 104;
    line.linetype = "CONTINUOUS";
    representable.elements.push_back(line);
    bool ended = false;
    const bool written = writer.writeMLineStyle(&representable)
        && writer.writer->writeString(0, "WIRE_END");
    const auto tags = numericWireProjection(output.str(), false, ended);
    t.expect(written && ended
                 && std::find(tags.begin(), tags.end(), "5:A500") != tags.end()
                 && std::find(tags.begin(), tags.end(), "330:A600") != tags.end()
                 && std::find(tags.begin(), tags.end(), "62:104") != tags.end()
                 && std::find(tags.begin(), tags.end(), "6:CONTINUOUS") != tags.end(),
             "ACI MLINESTYLE output preserves remapped identity and resolved LTYPE name");
}

class MLineStyleParseProbe final : public DRW_MLineStyle {
public:
    bool parsePayload(const std::string& payload) {
        resetDxf();
        std::istringstream input(payload);
        std::unique_ptr<dxfReader> reader =
            std::make_unique<dxfReaderAscii>(&input);
        int code = 0;
        while (reader->readRec(&code)) {
            if (!parseCode(code, reader))
                return false;
        }
        return validateDxfRecord();
    }
};

void testMLineStyleDxfDefaults(TestContext& t) {
    DRW_MLineStyle api;
    t.expect(api.startAngle==0.0&&api.endAngle==0.0,
             "MLINESTYLE API construction preserves zero-angle defaults");
    api.startAngle=25.0;api.endAngle=155.0;api.reset();
    t.expect(api.startAngle==0.0&&api.endAngle==0.0,
             "MLINESTYLE API reset keeps its zero-angle contract");
    const std::string tuple="71\n1\n49\n0.5\n62\n104\n6\nCONTINUOUS\n";
    struct Row {std::string payload;double start,end;};
    const std::vector<Row> cases{
        {tuple,90.0,90.0},{"51\n0\n"+tuple,0.0,90.0},{"52\n0\n"+tuple,90.0,0.0},
        {"51\n0\n52\n0\n"+tuple,0.0,0.0},{"52\n157.5\n"+tuple+"51\n22.5\n",22.5,157.5},
        {tuple+"51\n-0\n52\n-0\n",-0.0,-0.0}};
    MLineStyleParseProbe reused;std::size_t parsed=0u;
    for(const auto& row:cases) {
        t.expect(reused.parsePayload(row.payload)&&reused.startAngle==row.start&&reused.endAngle==row.end
                     &&std::signbit(reused.startAngle)==std::signbit(row.start)
                     &&std::signbit(reused.endAngle)==std::signbit(row.end),
                 "MLINESTYLE independent omitted/default/explicit/reordered angles survive probe reuse");++parsed;
    }
    t.expect(!reused.parsePayload("51\n12\n71\n2\n49\n0.5\n62\n104\n6\nCONTINUOUS\n")
                 &&reused.parsePayload(tuple)&&reused.startAngle==90.0&&reused.endAngle==90.0,
             "MLINESTYLE rejected partial record cannot contaminate following DXF defaults");
    std::cout<<"MLINESTYLE DXF defaults profiles="<<parsed<<"; reused omission=90/90; API constructor/reset=0/0\n";
}

void testMlineStyleDxfSchemaAdmission(TestContext& t) {
    const std::string validReordered =
        "51\n0.0\n71\n1\n49\n0.5\n6\nCONTINUOUS\n"
        "62\n104\n52\n1.5\n";
    MLineStyleParseProbe valid;
    t.expect(valid.parsePayload(validReordered)
                 && valid.elements.size() == 1u
                 && valid.elements.front().linetype == "CONTINUOUS"
                 && valid.elements.front().color == 104,
             "MLINESTYLE parser accepts legal field reorder with complete tuple");

    const std::vector<std::string> invalidPayloads {
        "49\n0.5\n62\n104\n6\nCONTINUOUS\n", // missing count
        "71\n1\n71\n1\n49\n0.5\n62\n104\n6\nCONTINUOUS\n",
        "71\n2\n49\n0.5\n62\n104\n6\nCONTINUOUS\n", // count mismatch
        "71\n1\n49\n0.5\n62\n104\n", // missing linetype
        "71\n1\n49\n0.5\n6\nCONTINUOUS\n", // missing color
        "71\n1\n49\n0.5\n62\n104\n62\n105\n6\nCONTINUOUS\n",
        "71\n1\n49\n0.5\n62\n104\n6\nCONTINUOUS\n6\nBYLAYER\n"
    };
    for (std::size_t index = 0; index < invalidPayloads.size(); ++index) {
        MLineStyleParseProbe invalid;
        t.expect(!invalid.parsePayload(invalidPayloads[index]),
                 "MLINESTYLE parser rejects missing, duplicate, or incomplete schema fields");
    }
}

void testMLineStyleReferenceHandleRemapping(TestContext& t) {
    std::vector<std::string> asciiProjection;
    for (const bool binary : {false, true}) {
        std::ostringstream output;
        dxfRW writer("");
        writer.version = DRW::AC1032;
        writer.binFile = binary;
        writer.writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&output))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&output));
        writer.setHandleRemap({{0x500u, 0xA500u}, {0x600u, 0xA600u}});
        writer.getWritingContext()->lineTypesMap.emplace_back("CONTINUOUS", 0x800u);

        DRW_MLineStyle style;
        style.handle = 0x500u;
        style.parentHandle = 0x600u;
        style.name = "CUSTOM_MLINE";
        DRW_MLineElement namedElement;
        namedElement.linetype = "CONTINUOUS";
        style.elements.push_back(namedElement);
        DRW_MLineElement handleResolvedElement;
        handleResolvedElement.linetypeHandle = 0x800u;
        style.elements.push_back(handleResolvedElement);
        style.elements.emplace_back();  // Defaults to BYLAYER.
        DRW_MLine mline;
        mline.handle = 0x700u;
        mline.parentHandle = 0x800u;
        mline.styleName = style.name;
        mline.styleHandle = style.handle;

        const bool written = writer.writeMLineStyle(&style)
            && writer.writeMLine(&mline)
            && writer.writer->writeString(0, "WIRE_END");
        bool ended = false;
        const auto tags = numericWireProjection(output.str(), binary, ended);
        t.expect(written && ended,
                 "MLINESTYLE reference output preserves following record framing");
        t.expect(std::count(tags.begin(), tags.end(), "0:MLINESTYLE") == 1
                     && std::count(tags.begin(), tags.end(), "0:MLINE") == 1
                     && std::count(tags.begin(), tags.end(), "5:A500") == 1
                     && std::count(tags.begin(), tags.end(), "2:CUSTOM_MLINE") == 2
                     && std::count(tags.begin(), tags.end(), "340:A500") == 1,
                 "MLINE code 340 remaps to the emitted MLINESTYLE identity");
        std::vector<std::string> emittedLineTypes;
        const auto mlineRecord = std::find(tags.begin(), tags.end(), "0:MLINE");
        std::copy_if(tags.begin(), mlineRecord,
                     std::back_inserter(emittedLineTypes),
                     [](const auto& tag) { return tag.rfind("6:", 0) == 0; });
        t.expect(emittedLineTypes
                     == std::vector<std::string>{"6:CONTINUOUS",
                                                 "6:CONTINUOUS",
                                                 "6:BYLAYER"},
                 "MLINESTYLE emits direct, handle-resolved, and default linetypes in order");
        t.expect(style.elements.size() == 3
                     && style.elements[0].linetype == "CONTINUOUS"
                     && style.elements[1].linetype.empty()
                     && style.elements[2].linetype.empty(),
                 "MLINESTYLE linetype emission leaves caller-owned names unchanged");
        if (!binary)
            asciiProjection = tags;
        else
            t.expect(tags == asciiProjection,
                     "MLINESTYLE and MLINE ASCII/binary links agree exactly");

        std::ostringstream rejectedOutput;
        dxfRW rejectingWriter("");
        rejectingWriter.version = DRW::AC1032;
        rejectingWriter.binFile = binary;
        rejectingWriter.writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&rejectedOutput))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&rejectedOutput));
        DRW_MLine invalid = mline;
        invalid.styleHandle = DRW::NoHandle;
        t.expect(!rejectingWriter.writeMLine(&invalid)
                     && rejectedOutput.str().empty(),
                 "MLINE sentinel style handle refuses before record publication");
    }
}

void testMLeaderStyleVersionAndHandleRemapping(TestContext& t) {
    const std::array<DRW::Version, 3> versions{
        DRW::AC1021, DRW::AC1024, DRW::AC1027};
    const std::map<std::uint32_t, std::uint32_t> remap{
        {0xA000u, 0xB000u}, {0xA001u, 0xB001u},
        {0xA002u, 0xB002u}, {0xA003u, 0xB003u},
        {0xA004u, 0xB004u}, {0xA005u, 0xB005u}};
    const auto hasCode = [](const std::vector<std::string>& tags, int code) {
        const std::string prefix = std::to_string(code) + ":";
        return std::any_of(tags.begin(), tags.end(),
            [&prefix](const std::string& tag) {
                return tag.compare(0, prefix.size(), prefix) == 0;
            });
    };

    for (const DRW::Version version : versions) {
        std::vector<std::string> asciiProjection;
        for (const bool binary : {false, true}) {
            std::ostringstream output;
            dxfRW writer("");
            writer.version = version;
            writer.binFile = binary;
            writer.setHandleRemap(remap);
            writer.writer = binary
                ? std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterBinary>(&output))
                : std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterAscii>(&output));

            DRW_MLeaderStyle style;
            style.handle = 0xA000u;
            style.parentHandle = 0xA001u;
            style.name = "VERSIONED_MLEADERSTYLE";
            style.styleVersion = 2;
            style.textExtended = true;
            style.leaderLineTypeHandle.ref = 0xA002u;
            style.arrowHeadBlockHandle.ref = 0xA003u;
            style.textStyleHandle.ref = 0xA004u;
            style.blockHandle.ref = 0xA005u;

            const bool written = writer.writeMLeaderStyle(&style)
                && writer.writer->writeString(0, "WIRE_END");
            bool ended = false;
            const auto tags = numericWireProjection(output.str(), binary, ended);
            t.expect(written && ended,
                     "versioned MLEADERSTYLE preserves following-record framing");
            for (const char* expected : {"5:B000", "330:B001", "340:B002",
                                         "341:B003", "342:B004", "343:B005"})
                t.expect(std::find(tags.begin(), tags.end(), expected) != tags.end(),
                         "MLEADERSTYLE own, owner, and typed references use remapped handles");
            t.expect(hasCode(tags, 179) == (version >= DRW::AC1024),
                     "MLEADERSTYLE group 179 follows the R2010 target floor");
            t.expect(hasCode(tags, 298) == (version >= DRW::AC1027),
                     "MLEADERSTYLE group 298 follows the R2013 target floor");
            t.expect(hasCode(tags, 271) == (version >= DRW::AC1024),
                     "MLEADERSTYLE groups 271-273 follow the R2010 target floor");
            t.expect(!hasCode(tags, 272) && !hasCode(tags, 273),
                     "MLEADERSTYLE unset vertical attachments are omitted");
            if (!binary)
                asciiProjection = tags;
            else
                t.expect(tags == asciiProjection,
                         "versioned MLEADERSTYLE ASCII/binary fields agree exactly");
        }
    }

    for (const bool binary : {false, true}) {
        for (const DRW::Version version :
             {DRW::AC1021, DRW::AC1024, DRW::AC1027}) {
            std::ostringstream output;
            dxfRW writer("");
            writer.version = version;
            writer.binFile = binary;
            writer.writer = binary
                ? std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterBinary>(&output))
                : std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterAscii>(&output));
            DRW_MLeaderStyle style;
            style.handle = 0xA000u;
            style.parentHandle = 0xA001u;
            style.name = "VERSIONED_VERTICAL_ATTACHMENT_MLEADERSTYLE";
            style.attachmentDirection = 1;
            style.bottomAttachment = 9;
            style.topAttachment = 10;
            const bool written = writer.writeMLeaderStyle(&style);
            if (version < DRW::AC1024) {
                t.expect(!written && output.str().empty(),
                         "AC1021 refuses nondefault R2010 MLEADERSTYLE attachment data before output");
                continue;
            }
            bool ended = false;
            const auto tags = numericWireProjection(output.str(), binary, ended);
            t.expect(written && hasCode(tags, 271)
                         && std::find(tags.begin(), tags.end(), "271:1")
                             != tags.end()
                         && std::find(tags.begin(), tags.end(), "272:9")
                             != tags.end()
                         && std::find(tags.begin(), tags.end(), "273:10")
                             != tags.end(),
                     "R2010+ MLEADERSTYLE emits exact attachment groups 271-273");
        }
    }

    for (const bool binary : {false, true}) {
        std::ostringstream output;
        dxfRW writer("");
        writer.version = DRW::AC1027;
        writer.binFile = binary;
        writer.setHandleRemap(remap);
        writer.writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&output))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&output));
        DRW_MLeaderStyle style;
        style.handle = 0xA000u;
        style.parentHandle = 0xA001u;
        style.name = "VERSIONED_MLEADERSTYLE";
        style.styleVersion = 2;
        style.textExtended = true;
        style.leaderLineTypeHandle.ref = 0xA002u;
        style.textStyleHandle.ref = 0xA004u;
        const bool written = writer.writeMLeaderStyle(&style)
            && writer.writer->writeString(0, "WIRE_END");
        bool ended = false;
        const auto tags = numericWireProjection(output.str(), binary, ended);
        t.expect(written && ended
                     && hasCode(tags, 340) && !hasCode(tags, 341)
                     && hasCode(tags, 342) && !hasCode(tags, 343),
                 "MLEADERSTYLE omits absent arrowhead and block references");

        std::ostringstream oldVersionOutput;
        dxfRW oldVersionWriter("");
        oldVersionWriter.version = DRW::AC1018;
        oldVersionWriter.binFile = binary;
        oldVersionWriter.writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&oldVersionOutput))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&oldVersionOutput));
        t.expect(!oldVersionWriter.writeMLeaderStyle(&style)
                     && oldVersionOutput.str().empty(),
                 "MLEADERSTYLE refuses targets below its R2007 floor");
    }

    dwgHandle DRW_MLeaderStyle::* references[] = {
        &DRW_MLeaderStyle::leaderLineTypeHandle,
        &DRW_MLeaderStyle::arrowHeadBlockHandle,
        &DRW_MLeaderStyle::textStyleHandle,
        &DRW_MLeaderStyle::blockHandle};
    for (const auto reference : references) {
        for (const bool binary : {false, true}) {
            std::ostringstream output;
            dxfRW writer("");
            writer.version = DRW::AC1027;
            writer.binFile = binary;
            writer.setHandleRemap({{0xA002u, 0u}});
            writer.writer = binary
                ? std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterBinary>(&output))
                : std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterAscii>(&output));
            DRW_MLeaderStyle invalid;
            invalid.handle = 0xA000u;
            invalid.parentHandle = 0xA001u;
            invalid.name = "INVALID_MLEADERSTYLE_REFERENCE";
            (invalid.*reference).ref = 0xA002u;
            t.expect(!writer.writeMLeaderStyle(&invalid) && output.str().empty(),
                     "MLEADERSTYLE reference remapped to null refuses before publication");
        }
    }
}

void testMLeaderStyleEnumDomains(TestContext& t) {
    struct EnumField {
        std::uint16_t DRW_MLeaderStyle::*member;
        int groupCode;
        std::uint16_t minimum;
        std::uint16_t maximum;
        bool omitZero;
        const char* label;
    };
    const std::array<EnumField, 12> fields{{
        {&DRW_MLeaderStyle::contentType, 170, 0, 3, false, "content type"},
        {&DRW_MLeaderStyle::drawMLeaderOrder, 171, 0, 1, false,
            "draw multi-leader order"},
        {&DRW_MLeaderStyle::drawLeaderOrder, 172, 0, 1, false,
            "draw leader order"},
        {&DRW_MLeaderStyle::leaderType, 173, 0, 2, false, "leader type"},
        {&DRW_MLeaderStyle::leftAttachment, 174, 0, 8, false,
            "left attachment"},
        {&DRW_MLeaderStyle::textAngleType, 175, 0, 2, false,
            "text angle type"},
        {&DRW_MLeaderStyle::textAlignmentType, 176, 0, 2, false,
            "text alignment type"},
        {&DRW_MLeaderStyle::blockConnectionType, 177, 0, 1, false,
            "block content connection type"},
        {&DRW_MLeaderStyle::rightAttachment, 178, 0, 8, false,
            "right attachment"},
        {&DRW_MLeaderStyle::attachmentDirection, 271, 0, 1, false,
            "attachment direction"},
        {&DRW_MLeaderStyle::bottomAttachment, 272, 9, 10, true,
            "bottom attachment"},
        {&DRW_MLeaderStyle::topAttachment, 273, 9, 10, true,
            "top attachment"}
    }};
    const auto fileTags = [](const std::filesystem::path& path, bool binary) {
        std::ifstream input(path, std::ios::binary);
        std::string bytes((std::istreambuf_iterator<char>(input)), {});
        if (binary && bytes.size() >= 22u)
            bytes.erase(0u, 22u);
        bool ended = false;
        return numericWireProjection(bytes, binary, ended);
    };
    const auto fieldValueIs = [](const std::vector<std::string>& tags,
                                 int groupCode, int value) {
        const auto begin = std::find(tags.begin(), tags.end(), "0:MLEADERSTYLE");
        if (begin == tags.end())
            return false;
        const auto end = std::find_if(std::next(begin), tags.end(),
            [](const std::string& tag) {
                return tag.compare(0u, 2u, "0:") == 0;
            });
        const std::string expected = std::to_string(groupCode) + ":"
            + std::to_string(value);
        return std::find(std::next(begin), end, expected) != end;
    };
    const auto fieldCodeIsAbsent = [](const std::vector<std::string>& tags,
                                      int groupCode) {
        const auto begin = std::find(tags.begin(), tags.end(), "0:MLEADERSTYLE");
        if (begin == tags.end())
            return false;
        const auto end = std::find_if(std::next(begin), tags.end(),
            [](const std::string& tag) {
                return tag.compare(0u, 2u, "0:") == 0;
            });
        const std::string prefix = std::to_string(groupCode) + ":";
        return std::find_if(std::next(begin), end,
            [&prefix](const std::string& tag) {
                return tag.compare(0u, prefix.size(), prefix) == 0;
            }) == end;
    };
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-mleader-style-enum-"
            + std::to_string(std::chrono::steady_clock::now()
                .time_since_epoch().count()) + ".dxf");

    class RetryInterface final : public dx_iface {
    public:
        RetryInterface() {
            cData = &data;
            currentBlock = data.mBlock;
            style.handle = 0xA600u;
            style.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
            style.name = "ENUM_DOMAIN_MLEADERSTYLE";
            style.styleVersion = 2;
        }
        void writeObjects() override {
            (void)dxfW->writeMLeaderStyle(&style);
        }
        dx_data data;
        DRW_MLeaderStyle style;
    } iface;

    unsigned refusals = 0u;
    unsigned retries = 0u;
    for (const bool binary : {false, true}) {
        for (const EnumField& field : fields) {
            const std::vector<std::uint16_t> invalidValues = field.minimum == 9
                ? std::vector<std::uint16_t>{8, 11, 65535}
                : std::vector<std::uint16_t>{
                      static_cast<std::uint16_t>(field.maximum + 1u), 65535};
            for (const std::uint16_t invalid : invalidValues) {
                iface.style.contentType = 0;
                iface.style.drawMLeaderOrder = 0;
                iface.style.drawLeaderOrder = 0;
                iface.style.leaderType = 0;
                iface.style.leftAttachment = 0;
                iface.style.rightAttachment = 0;
                iface.style.textAngleType = 0;
                iface.style.textAlignmentType = 0;
                iface.style.blockConnectionType = 0;
                iface.style.attachmentDirection = 0;
                iface.style.topAttachment = 0;
                iface.style.bottomAttachment = 0;
                iface.style.*(field.member) = invalid;
                const std::string sentinel = "KEEP_MLEADERSTYLE_DESTINATION";
                { std::ofstream out(path, std::ios::binary); out << sentinel; }
                const bool written = iface.fileExport(
                    path.string(), DRW::AC1027, binary, &iface.data, false);
                std::ifstream saved(path, std::ios::binary);
                const bool destinationPreserved =
                    std::string(std::istreambuf_iterator<char>(saved), {})
                        == sentinel;
                const std::string refusalLabel =
                    "MLEADERSTYLE rejects invalid "
                    + std::string(field.label)
                    + " without publishing or editing the model";
                t.expect(!written && destinationPreserved
                             && iface.style.*(field.member) == invalid,
                         refusalLabel.c_str());
                if (written || !destinationPreserved)
                    continue;
                ++refusals;

                int corrected = invalid == field.maximum + 1u
                    ? field.maximum : 0;
                if (field.minimum == 9)
                    corrected = invalid == 8 ? 9 : invalid == 11 ? 10 : 0;
                iface.style.*(field.member) = corrected;
                const bool retried = iface.fileExport(
                    path.string(), DRW::AC1027, binary, &iface.data, false);
                const auto tags = retried ? fileTags(path, binary)
                                          : std::vector<std::string>{};
                const bool physicalValue = retried && (corrected == 0
                    && field.omitZero
                    ? fieldCodeIsAbsent(tags, field.groupCode)
                    : fieldValueIs(tags, field.groupCode, corrected));
                const std::string retryLabel =
                    "same-adapter MLEADERSTYLE corrected enum retry emits group "
                    + std::to_string(field.groupCode);
                t.expect(retried && physicalValue,
                         retryLabel.c_str());
                if (retried && physicalValue)
                    ++retries;
            }
        }
    }
    t.expect(refusals == 52u && retries == 52u,
             "MLEADERSTYLE enum boundaries cover ASCII/binary refusal and retry");

    for (const bool binary : {false, true}) {
        iface.style.attachmentDirection = 1;
        iface.style.topAttachment = 10;
        iface.style.bottomAttachment = 9;
        const std::string sentinel = "KEEP_PRE_R2010_MLEADERSTYLE_DESTINATION";
        { std::ofstream out(path, std::ios::binary); out << sentinel; }
        const bool written = iface.fileExport(
            path.string(), DRW::AC1021, binary, &iface.data, false);
        std::ifstream saved(path, std::ios::binary);
        const bool destinationPreserved =
            std::string(std::istreambuf_iterator<char>(saved), {}) == sentinel;
        t.expect(!written && destinationPreserved
                     && iface.style.attachmentDirection == 1
                     && iface.style.topAttachment == 10
                     && iface.style.bottomAttachment == 9,
                 "AC1021 MLEADERSTYLE R2010 attachment refusal preserves destination and model");

        iface.style.attachmentDirection = 0;
        iface.style.topAttachment = 0;
        iface.style.bottomAttachment = 0;
        const bool retried = iface.fileExport(
            path.string(), DRW::AC1021, binary, &iface.data, false);
        const auto tags = retried ? fileTags(path, binary)
                                  : std::vector<std::string>{};
        t.expect(retried && fieldCodeIsAbsent(tags, 271)
                     && fieldCodeIsAbsent(tags, 272)
                     && fieldCodeIsAbsent(tags, 273),
                 "same-adapter AC1021 retry omits unset R2010 attachment groups");
    }

    for (const DRW::Version version : {DRW::AC1024, DRW::AC1027}) {
        for (const bool binary : {false, true}) {
            for (const std::uint16_t invalid : {std::uint16_t{1},
                                                std::uint16_t{3},
                                                std::uint16_t{65535}}) {
                iface.style.styleVersion = invalid;
                const std::string sentinel = "KEEP_MLEADERSTYLE_VERSION_DESTINATION";
                { std::ofstream out(path, std::ios::binary); out << sentinel; }
                const bool written = iface.fileExport(
                    path.string(), version, binary, &iface.data, false);
                std::ifstream saved(path, std::ios::binary);
                const bool destinationPreserved =
                    std::string(std::istreambuf_iterator<char>(saved), {})
                        == sentinel;
                t.expect(!written && destinationPreserved
                             && iface.style.styleVersion == invalid,
                         "MLEADERSTYLE group-179 version refusal preserves destination and model");

                iface.style.styleVersion = 2;
                const bool retried = iface.fileExport(
                    path.string(), version, binary, &iface.data, false);
                const auto tags = retried ? fileTags(path, binary)
                                          : std::vector<std::string>{};
                t.expect(retried && fieldValueIs(tags, 179, 2),
                         "same-adapter MLEADERSTYLE group-179 version retry emits 2");
            }
        }
    }

    for (const bool binary : {false, true}) {
        for (const std::uint16_t invalid : {std::uint16_t{1},
                                            std::uint16_t{3},
                                            std::uint16_t{65535}}) {
            iface.style.styleVersion = invalid;
            const std::string sentinel =
                "KEEP_PRE_R2010_MLEADERSTYLE_VERSION_DESTINATION";
            { std::ofstream out(path, std::ios::binary); out << sentinel; }
            const bool written = iface.fileExport(
                path.string(), DRW::AC1021, binary, &iface.data, false);
            std::ifstream saved(path, std::ios::binary);
            const bool destinationPreserved =
                std::string(std::istreambuf_iterator<char>(saved), {})
                    == sentinel;
            t.expect(!written && destinationPreserved
                         && iface.style.styleVersion == invalid,
                     "AC1021 refuses an unrepresentable nondefault MLEADERSTYLE version before publication");

            iface.style.styleVersion = 2;
            const bool retried = iface.fileExport(
                path.string(), DRW::AC1021, binary, &iface.data, false);
            const auto tags = retried ? fileTags(path, binary)
                                      : std::vector<std::string>{};
            t.expect(retried && fieldCodeIsAbsent(tags, 179),
                     "same-adapter AC1021 MLEADERSTYLE version retry omits group 179");
        }
    }

    constexpr std::array<std::int32_t, 4> validMaxLeaderPoints{{
        -1, 0, 2, std::numeric_limits<std::int32_t>::max()
    }};
    constexpr std::array<std::int32_t, 3> invalidMaxLeaderPoints{{
        std::numeric_limits<std::int32_t>::min(), -2, 1
    }};
    for (const DRW::Version version : {
             DRW::AC1021, DRW::AC1024, DRW::AC1027}) {
        for (const bool binary : {false, true}) {
            for (const std::int32_t maxPoints : validMaxLeaderPoints) {
                iface.style.maxLeaderPoints = maxPoints;
                const bool written = iface.fileExport(
                    path.string(), version, binary, &iface.data, false);
                const auto tags = written ? fileTags(path, binary)
                                          : std::vector<std::string>{};
                t.expect(written && fieldValueIs(tags, 90, maxPoints),
                         "MLEADERSTYLE emits each valid maximum leader-points boundary exactly");
            }

            for (const std::int32_t invalid : invalidMaxLeaderPoints) {
                iface.style.maxLeaderPoints = invalid;
                const std::string sentinel =
                    "KEEP_MLEADERSTYLE_MAX_LEADER_POINTS_DESTINATION";
                { std::ofstream out(path, std::ios::binary); out << sentinel; }
                const bool written = iface.fileExport(
                    path.string(), version, binary, &iface.data, false);
                std::ifstream saved(path, std::ios::binary);
                const bool destinationPreserved =
                    std::string(std::istreambuf_iterator<char>(saved), {})
                        == sentinel;
                t.expect(!written && destinationPreserved
                             && iface.style.maxLeaderPoints == invalid,
                         "MLEADERSTYLE refuses invalid maximum leader points without mutation");

                iface.style.maxLeaderPoints = 2;
                const bool retried = iface.fileExport(
                    path.string(), version, binary, &iface.data, false);
                const auto tags = retried ? fileTags(path, binary)
                                          : std::vector<std::string>{};
                t.expect(retried && fieldValueIs(tags, 90, 2),
                         "same-adapter MLEADERSTYLE maximum leader-points retry emits group 90");
            }
        }
    }
    iface.style.maxLeaderPoints = 0;

    constexpr std::array<std::int32_t, 27> validLineWeights{{
        -3, -2, -1, 0, 5, 9, 13, 15, 18, 20, 25, 30, 35, 40,
        50, 53, 60, 70, 80, 90, 100, 106, 120, 140, 158, 200, 211
    }};
    for (const bool binary : {false, true}) {
        for (const std::int32_t lineWeight : validLineWeights) {
            std::ostringstream output;
            dxfRW writer("");
            writer.version = DRW::AC1027;
            writer.binFile = binary;
            writer.writer = binary
                ? std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterBinary>(&output))
                : std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterAscii>(&output));
            DRW_MLeaderStyle style;
            style.handle = 0xA700u;
            style.parentHandle = 0xA701u;
            style.name = "LINEWEIGHT_DOMAIN_MLEADERSTYLE";
            style.leaderLineWeight = lineWeight;
            const bool written = writer.writeMLeaderStyle(&style);
            bool ended = false;
            const auto tags = numericWireProjection(output.str(), binary, ended);
            t.expect(written
                         && std::find(tags.begin(), tags.end(),
                             "92:" + std::to_string(lineWeight)) != tags.end(),
                     "MLEADERSTYLE writes each documented DXF lineweight value");
        }
    }

    for (const bool binary : {false, true}) {
        for (const std::int32_t invalid : {1, 4, 10, 212, -4, 65535}) {
            iface.style.leaderLineWeight = invalid;
            const std::string sentinel = "KEEP_MLEADERSTYLE_LINEWEIGHT_DESTINATION";
            { std::ofstream out(path, std::ios::binary); out << sentinel; }
            const bool written = iface.fileExport(
                path.string(), DRW::AC1027, binary, &iface.data, false);
            std::ifstream saved(path, std::ios::binary);
            const bool destinationPreserved =
                std::string(std::istreambuf_iterator<char>(saved), {})
                    == sentinel;
            t.expect(!written && destinationPreserved
                         && iface.style.leaderLineWeight == invalid,
                     "MLEADERSTYLE refuses invalid lineweight before publication");

            const std::int32_t corrected = invalid == -4 ? -3 : 50;
            iface.style.leaderLineWeight = corrected;
            const bool retried = iface.fileExport(
                path.string(), DRW::AC1027, binary, &iface.data, false);
            const auto tags = retried ? fileTags(path, binary)
                                      : std::vector<std::string>{};
            t.expect(retried && fieldValueIs(tags, 92, corrected),
                     "same-adapter MLEADERSTYLE lineweight retry emits exact group 92");
        }
    }
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
}

void testMLeaderWriterMinimumVersion(TestContext& t) {
    std::vector<std::string> asciiProjection;
    std::vector<std::string> emptyContentAsciiProjection;
    std::vector<std::string> emptyBranchesAsciiProjection;
    for (const bool binary : {false, true}) {
        std::ostringstream rejectedOutput;
        dxfRW rejectedWriter("");
        rejectedWriter.version = DRW::AC1018;
        rejectedWriter.binFile = binary;
        rejectedWriter.writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&rejectedOutput))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&rejectedOutput));
        DRW_MLeader leader;
        leader.handle = 0xA100u;
        t.expect(!rejectedWriter.writeMultiLeader(&leader)
                     && rejectedOutput.str().empty(),
                 "MULTILEADER refuses pre-AC1021 output before record publication");

        std::ostringstream output;
        dxfRW writer("");
        writer.version = DRW::AC1021;
        writer.binFile = binary;
        writer.writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&output))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&output));
        const bool written = writer.writeMultiLeader(&leader)
            && writer.writer->writeString(0, "WIRE_END");
        bool ended = false;
        const auto tags = numericWireProjection(output.str(), binary, ended);
        const auto hasTag = [&tags](const std::string& expected) {
            return std::find(tags.begin(), tags.end(), expected) != tags.end();
        };
        t.expect(written && ended && hasTag("0:MULTILEADER")
                     && hasTag("100:AcDbMLeader") && hasTag("0:WIRE_END"),
                 "AC1021 MULTILEADER keeps record identity and following-record framing");
        if (!binary)
            asciiProjection = tags;
        else
            t.expect(tags == asciiProjection,
                     "AC1021 MULTILEADER ASCII/binary fields agree");

        std::ostringstream emptyContentOutput;
        dxfRW emptyContentWriter("");
        emptyContentWriter.version = DRW::AC1021;
        emptyContentWriter.binFile = binary;
        emptyContentWriter.writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&emptyContentOutput))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&emptyContentOutput));
        DRW_MLeader emptyContent;
        emptyContent.styleContentType = 0;
        const bool emptyContentWritten =
            emptyContentWriter.writeMultiLeader(&emptyContent)
            && emptyContentWriter.writer->writeString(0, "WIRE_END");
        bool emptyContentEnded = false;
        const auto emptyContentTags = numericWireProjection(
            emptyContentOutput.str(), binary, emptyContentEnded);
        const auto hasEmptyContentTag = [&emptyContentTags](
                const std::string& expected) {
            return std::find(emptyContentTags.begin(), emptyContentTags.end(),
                             expected) != emptyContentTags.end();
        };
        t.expect(emptyContentWritten && emptyContentEnded
                     && hasEmptyContentTag("172:0")
                     && !hasEmptyContentTag("300:CONTEXT_DATA{")
                     && hasEmptyContentTag("0:WIRE_END"),
                 "AC1021 MULTILEADER no-content state keeps framing without a context");
        if (!binary)
            emptyContentAsciiProjection = emptyContentTags;
        else
            t.expect(emptyContentTags == emptyContentAsciiProjection,
                     "AC1021 MULTILEADER no-content ASCII/binary fields agree");

        std::ostringstream emptyBranchesOutput;
        dxfRW emptyBranchesWriter("");
        emptyBranchesWriter.version = DRW::AC1021;
        emptyBranchesWriter.binFile = binary;
        emptyBranchesWriter.writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&emptyBranchesOutput))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&emptyBranchesOutput));
        DRW_MLeader emptyBranches;
        emptyBranches.styleContentType = 0;
        emptyBranches.context.roots.emplace_back();
        const bool emptyBranchesWritten =
            emptyBranchesWriter.writeMultiLeader(&emptyBranches)
            && emptyBranchesWriter.writer->writeString(0, "WIRE_END");
        bool emptyBranchesEnded = false;
        const auto emptyBranchesTags = numericWireProjection(
            emptyBranchesOutput.str(), binary, emptyBranchesEnded);
        t.expect(emptyBranchesWritten && emptyBranchesEnded
                     && std::find(emptyBranchesTags.begin(),
                                  emptyBranchesTags.end(),
                                  "290:0") != emptyBranchesTags.end()
                     && std::find(emptyBranchesTags.begin(),
                                  emptyBranchesTags.end(),
                                  "296:0") != emptyBranchesTags.end()
                     && std::find(emptyBranchesTags.begin(),
                                  emptyBranchesTags.end(),
                                  "302:LEADER{") != emptyBranchesTags.end()
                     && std::find(emptyBranchesTags.begin(),
                                  emptyBranchesTags.end(),
                                  "0:WIRE_END") != emptyBranchesTags.end(),
                 "MLEADER emits an empty 00 content-flag profile inside a context");
        if (!binary)
            emptyBranchesAsciiProjection = emptyBranchesTags;
        else
            t.expect(emptyBranchesTags == emptyBranchesAsciiProjection,
                     "MLEADER empty 00 context flags agree in ASCII and binary");

        for (const std::uint16_t unsupportedContentType : {
                 std::uint16_t{3}, std::uint16_t{4}}) {
            std::ostringstream unsupportedOutput;
            dxfRW unsupportedWriter("");
            unsupportedWriter.version = DRW::AC1021;
            unsupportedWriter.binFile = binary;
            unsupportedWriter.writer = binary
                ? std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterBinary>(&unsupportedOutput))
                : std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterAscii>(&unsupportedOutput));
            DRW_MLeader unsupported;
            unsupported.styleContentType = unsupportedContentType;
            const bool unsupportedWritten =
                unsupportedWriter.writeMultiLeader(&unsupported);
            const std::string message = unsupportedContentType == 3u
                ? "MULTILEADER TOLERANCE content refuses before publication"
                : "unknown MULTILEADER content type refuses before publication";
            t.expect(!unsupportedWritten && unsupportedOutput.str().empty(),
                     message.c_str());
        }
    }
}

void testSunDxfCmcProjectionGuard(TestContext& t) {
    struct ValidColor {
        std::uint32_t scalar;
        bool hasSourceMethod;
        std::uint8_t method;
    };
    const std::array<ValidColor, 5> validColors{{
        {7u, false, 0u},
        {256u, true, dwgColor::BYLAYER},
        {0u, true, dwgColor::BYBLOCK},
        {42u, true, dwgColor::ACI},
        {7u, true, dwgColor::FOREGROUND}
    }};
    const auto configureWriter = [](dxfRW& writer, bool binary,
                                    std::ostringstream& output) {
        writer.version = DRW::AC1032;
        writer.binFile = binary;
        writer.writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&output))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&output));
    };
    for (const bool binary : {false, true}) {
        for (const ValidColor& item : validColors) {
            std::ostringstream output;
            dxfRW writer("");
            configureWriter(writer, binary, output);
            DRW_Sun sun;
            sun.m_color = item.scalar;
            if (item.hasSourceMethod) {
                sun.m_colorCmc.present = true;
                sun.m_colorCmc.method = item.method;
            }
            const bool wrote = writer.writeSun(&sun)
                && writer.writer->writeString(0, "WIRE_END");
            bool ended = false;
            const auto tags = numericWireProjection(output.str(), binary,
                                                      ended);
            const std::string expected = "63:" + std::to_string(item.scalar);
            t.expect(wrote && ended
                         && std::find(tags.begin(), tags.end(), expected)
                             != tags.end()
                         && std::none_of(tags.begin(), tags.end(),
                             [](const std::string& tag) {
                                 return tag.rfind("421:", 0) == 0
                                     || tag.rfind("420:", 0) == 0
                                     || tag.rfind("430:", 0) == 0;
                             }),
                     "SUN emits faithful documented group 63 only");
        }

        const auto refuses = [&](const char* label, DRW_Sun sun) {
            std::ostringstream output;
            dxfRW writer("");
            configureWriter(writer, binary, output);
            const std::string initial = output.str();
            t.expect(!writer.writeSun(&sun) && output.str() == initial,
                     label);
        };
        DRW_Sun rgb;
        rgb.m_color = 256u;
        rgb.m_color24 = 0x123456;
        rgb.m_colorCmc.present = true;
        rgb.m_colorCmc.method = dwgColor::RGB;
        refuses("CMC RGB without a documented SUN DXF group", rgb);

        DRW_Sun named;
        named.m_color = 7u;
        named.m_colorCmc.present = true;
        named.m_colorCmc.method = dwgColor::ACI;
        named.m_colorCmc.flags = 1u;
        named.m_colorCmc.colorName = "source-color-name";
        named.m_colorName = "source-color-name";
        refuses("CMC optional color name", named);

        DRW_Sun flaggedEmpty;
        flaggedEmpty.m_color = 7u;
        flaggedEmpty.m_colorCmc.present = true;
        flaggedEmpty.m_colorCmc.method = dwgColor::ACI;
        flaggedEmpty.m_colorCmc.flags = 1u;
        refuses("flagged-empty CMC name", flaggedEmpty);

        DRW_Sun invalidIndex;
        invalidIndex.m_color = 7u;
        invalidIndex.m_colorCmc.present = true;
        invalidIndex.m_colorCmc.method = dwgColor::ACI;
        invalidIndex.m_colorCmc.rawIndex = 1u;
        refuses("nonzero reserved CMC index", invalidIndex);

        DRW_Sun unknownFlags;
        unknownFlags.m_color = 7u;
        unknownFlags.m_colorCmc.present = true;
        unknownFlags.m_colorCmc.method = dwgColor::ACI;
        unknownFlags.m_colorCmc.flags = 0x80u;
        refuses("unknown CMC flags", unknownFlags);

        DRW_Sun unsupportedMethod;
        unsupportedMethod.m_color = 7u;
        unsupportedMethod.m_colorCmc.present = true;
        unsupportedMethod.m_colorCmc.method = 0xC4u;
        refuses("unsupported CMC method", unsupportedMethod);

        DRW_Sun nonstandardTruecolor;
        nonstandardTruecolor.m_color = 256u;
        nonstandardTruecolor.m_color24 = 0x123456;
        refuses("nonstandard group 421 truecolor extension",
                nonstandardTruecolor);

        DRW_Sun unownedName;
        unownedName.m_color = 7u;
        unownedName.m_colorName = "unmapped-name";
        refuses("unmapped name projection", unownedName);
    }
}

void testSectionSettingsDxfCmcProjectionGuard(TestContext& t) {
    const auto configureWriter = [](dxfRW& writer, bool binary,
                                    std::ostringstream& output) {
        writer.version = DRW::AC1032;
        writer.binFile = binary;
        writer.writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&output))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&output));
    };
    const auto makeSection = [] {
        DRW_Section section;
        section.handle = 0x204u;
        section.m_kind = DRW_Section::Settings;
        DRW_SectionTypeSettings type;
        DRW_SectionGeometrySettings geometry;
        geometry.m_color = 42;
        geometry.m_colorMethod = dwgColor::ACI;
        type.m_geometry.push_back(geometry);
        section.m_types.push_back(type);
        return section;
    };
    for (const bool binary : {false, true}) {
        {
            std::ostringstream output;
            dxfRW writer("");
            configureWriter(writer, binary, output);
            DRW_Section section = makeSection();
            const bool wrote = writer.writeSection(&section, "SECTIONSETTINGS")
                && writer.writer->writeString(0, "WIRE_END");
            bool ended = false;
            const auto tags = numericWireProjection(output.str(), binary,
                                                      ended);
            t.expect(wrote && ended
                         && std::find(tags.begin(), tags.end(), "63:42")
                             != tags.end()
                         && std::none_of(tags.begin(), tags.end(),
                             [](const std::string& tag) {
                                 return tag.rfind("62:", 0) == 0
                                     || tag.rfind("420:", 0) == 0;
                             }),
                     "SECTION_SETTINGS emits documented group-63 ACI color");
        }

        const auto refuses = [&](const char* label,
                                 const std::function<void(
                                     DRW_SectionGeometrySettings&)>& mutate) {
            std::ostringstream output;
            dxfRW writer("");
            configureWriter(writer, binary, output);
            DRW_Section section = makeSection();
            DRW_SectionGeometrySettings& geometry =
                section.m_types.front().m_geometry.front();
            mutate(geometry);
            t.expect(!writer.writeSection(&section, "SECTIONSETTINGS")
                         && output.str().empty(),
                     label);
        };
        refuses("SECTION_SETTINGS refuses CMC names without documented DXF mapping",
                [](DRW_SectionGeometrySettings& geometry) {
                    geometry.m_colorName = "source-color";
                    geometry.m_colorBookName = "palette";
                    geometry.m_colorFlags = 3u;
                });
        refuses("SECTION_SETTINGS refuses RGB without documented class-specific DXF mapping",
                [](DRW_SectionGeometrySettings& geometry) {
                    geometry.m_color = 256;
                    geometry.m_rgbColor = 0x123456;
                    geometry.m_hasRgbColor = true;
                    geometry.m_colorMethod = dwgColor::RGB;
                });
        refuses("SECTION_SETTINGS refuses flagged-empty CMC names",
                [](DRW_SectionGeometrySettings& geometry) {
                    geometry.m_colorFlags = 1u;
                });
        refuses("SECTION_SETTINGS refuses nonzero reserved CMC index",
                [](DRW_SectionGeometrySettings& geometry) {
                    geometry.m_colorRawIndex = 1u;
                });
        refuses("SECTION_SETTINGS refuses unsupported CMC methods",
                [](DRW_SectionGeometrySettings& geometry) {
                    geometry.m_colorMethod = 0xC5u;
                });
        refuses("SECTION_SETTINGS refuses method/scalar disagreement",
                [](DRW_SectionGeometrySettings& geometry) {
                    geometry.m_colorMethod = dwgColor::BYBLOCK;
                });
    }
}

void testMLeaderDxfCmcColorLossGuard(TestContext& t) {
    using ColorMutation = std::function<void(DRW_MLeader&, int, bool)>;
    const auto setMetadata = [](std::int32_t& rgb, UTF8STRING& name,
                                UTF8STRING& bookName, int kind,
                                bool enabled) {
        rgb = -1;
        name.clear();
        bookName.clear();
        if (!enabled)
            return;
        if (kind == 0)
            rgb = 0;  // Black is a valid RGB payload, not an absent sentinel.
        else if (kind == 1)
            name = "DWG_COLOR_NAME";
        else
            bookName = "DWG_COLOR_BOOK";
    };
    const std::vector<std::pair<const char*, ColorMutation>> colors = {
        {"entity leader", [setMetadata](DRW_MLeader& entity, int kind,
                                         bool enabled) {
             setMetadata(entity.leaderColorRgb, entity.leaderColorName,
                         entity.leaderColorBookName, kind, enabled);
         }},
        {"entity style text", [setMetadata](DRW_MLeader& entity, int kind,
                                             bool enabled) {
             setMetadata(entity.styleTextColorRgb, entity.styleTextColorName,
                         entity.styleTextColorBookName, kind, enabled);
         }},
        {"entity style block", [setMetadata](DRW_MLeader& entity, int kind,
                                              bool enabled) {
             setMetadata(entity.styleBlockColorRgb,
                         entity.styleBlockColorName,
                         entity.styleBlockColorBookName, kind, enabled);
         }},
        {"context text", [setMetadata](DRW_MLeader& entity, int kind,
                                       bool enabled) {
             entity.context.hasTextContents = true;
             entity.context.hasContentsBlock = false;
             setMetadata(entity.context.textColorRgb,
                         entity.context.textColorName,
                         entity.context.textColorBookName, kind, enabled);
         }},
        {"context background", [setMetadata](DRW_MLeader& entity, int kind,
                                             bool enabled) {
             entity.context.hasTextContents = true;
             entity.context.hasContentsBlock = false;
             setMetadata(entity.context.bgFillColorRgb,
                         entity.context.bgFillColorName,
                         entity.context.bgFillColorBookName, kind, enabled);
         }},
        {"context block", [setMetadata](DRW_MLeader& entity, int kind,
                                         bool enabled) {
             entity.context.hasTextContents = false;
             entity.context.hasContentsBlock = true;
             setMetadata(entity.context.blockColorRgb,
                         entity.context.blockColorName,
                         entity.context.blockColorBookName, kind, enabled);
         }},
        {"leader line", [setMetadata](DRW_MLeader& entity, int kind,
                                       bool enabled) {
             if (entity.context.roots.empty())
                 entity.context.roots.emplace_back();
             auto& lines = entity.context.roots.front().leaderLines;
             if (lines.empty())
                 lines.emplace_back();
             setMetadata(lines.front().colorRgb, lines.front().colorName,
                         lines.front().colorBookName, kind, enabled);
         }}
    };
    const std::vector<std::pair<const char*, std::function<void(DRW_MLeader&)>>>
        unrepresentableMethods = {
            {"entity leader", [](DRW_MLeader& entity) {
                 entity.leaderColor = 7;
                 entity.leaderColorDwgMethod = 0xC5u;
                 entity.leaderColorDwgMetadataPresent = true;
             }},
            {"entity style text", [](DRW_MLeader& entity) {
                 entity.styleTextColor = 7;
                 entity.styleTextColorDwgMethod = 0xC5u;
                 entity.styleTextColorDwgMetadataPresent = true;
             }},
            {"entity style block", [](DRW_MLeader& entity) {
                 entity.styleBlockColor = 7;
                 entity.styleBlockColorDwgMethod = 0xC5u;
                 entity.styleBlockColorDwgMetadataPresent = true;
             }},
            {"context text", [](DRW_MLeader& entity) {
                 entity.context.hasTextContents = true;
                 entity.context.textColor = 7;
                 entity.context.textColorDwgMethod = 0xC5u;
                 entity.context.textColorDwgMetadataPresent = true;
             }},
            {"context background", [](DRW_MLeader& entity) {
                 entity.context.hasTextContents = true;
                 entity.context.bgFillColor = 7;
                 entity.context.bgFillColorDwgMethod = 0xC5u;
                 entity.context.bgFillColorDwgMetadataPresent = true;
             }},
            {"context block", [](DRW_MLeader& entity) {
                 entity.context.hasTextContents = false;
                 entity.context.hasContentsBlock = true;
                 entity.context.blockColor = 7;
                 entity.context.blockColorDwgMethod = 0xC5u;
                 entity.context.blockColorDwgMetadataPresent = true;
             }},
            {"leader line", [](DRW_MLeader& entity) {
                 entity.context.roots.emplace_back();
                 entity.context.roots.back().leaderLines.emplace_back();
                 DRW_MLeaderLeaderLine& line =
                     entity.context.roots.back().leaderLines.back();
                 line.color = 7;
                 line.colorDwgMethod = 0xC5u;
                 line.colorDwgMetadataPresent = true;
             }}
        };
    for (const bool binary : {false, true}) {
        for (const auto& color : colors) {
            for (int metadataKind = 0; metadataKind < 3; ++metadataKind) {
                std::ostringstream output;
                dxfRW writer("");
                writer.version = DRW::AC1032;
                writer.binFile = binary;
                writer.writer = binary
                    ? std::unique_ptr<dxfWriter>(
                          std::make_unique<dxfWriterBinary>(&output))
                    : std::unique_ptr<dxfWriter>(
                          std::make_unique<dxfWriterAscii>(&output));
                const std::string initialOutput = output.str();
                DRW_MLeader entity;
                entity.handle = 0xA100u;
                color.second(entity, metadataKind, true);
                t.expect(!writer.writeMultiLeader(&entity)
                             && output.str() == initialOutput,
                         "MLEADER refuses unrepresented DWG CMC metadata before output");
            }
        }

        std::ostringstream scalarOutput;
        dxfRW scalarWriter("");
        scalarWriter.version = DRW::AC1032;
        scalarWriter.binFile = binary;
        scalarWriter.writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&scalarOutput))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&scalarOutput));
        DRW_MLeader scalar;
        scalar.leaderColor = 7;
        scalar.styleTextColor = 256;
        scalar.styleBlockColor = 3;
        scalar.context.hasTextContents = true;
        scalar.context.textColor = 5;
        scalar.context.bgFillColor = 0;
        DRW_MLeaderRoot scalarRoot;
        DRW_MLeaderLeaderLine scalarLine;
        scalarLine.color = 2;
        scalarRoot.leaderLines.push_back(scalarLine);
        scalar.context.roots.push_back(scalarRoot);
        const bool scalarWritten = scalarWriter.writeMultiLeader(&scalar)
            && scalarWriter.writer->writeString(0, "WIRE_END");
        bool scalarEnded = false;
        const auto scalarTags = numericWireProjection(scalarOutput.str(),
                                                       binary, scalarEnded);
        const auto hasScalarTag = [&scalarTags](const char* expected) {
            return std::find(scalarTags.begin(), scalarTags.end(), expected)
                != scalarTags.end();
        };
        t.expect(scalarWritten && scalarEnded
                     && hasScalarTag("91:7") && hasScalarTag("92:256")
                     && hasScalarTag("93:3") && hasScalarTag("90:5")
                     && hasScalarTag("91:0") && hasScalarTag("92:2"),
                 "MLEADER scalar ACI/ByLayer/ByBlock colors retain documented groups");
        t.expect(std::none_of(scalarTags.begin(), scalarTags.end(),
                              [](const std::string& tag) {
                                  return tag.rfind("420:", 0) == 0
                                      || tag.rfind("421:", 0) == 0
                                      || tag.rfind("430:", 0) == 0
                                      || tag.rfind("431:", 0) == 0;
                              }),
                 "MLEADER does not invent generic truecolor/name extensions for class fields");

        for (const auto& method : unrepresentableMethods) {
            std::ostringstream methodOutput;
            dxfRW methodWriter("");
            methodWriter.version = DRW::AC1032;
            methodWriter.binFile = binary;
            methodWriter.writer = binary
                ? std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterBinary>(&methodOutput))
                : std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterAscii>(&methodOutput));
            DRW_MLeader methodEntity = scalar;
            methodEntity.handle = 0xA101u;
            method.second(methodEntity);
            const std::string initialMethodOutput = methodOutput.str();
            t.expect(!methodWriter.writeMultiLeader(&methodEntity)
                         && methodOutput.str() == initialMethodOutput,
                     "MLEADER refuses unsupported source CMC method before DXF output");
        }

        std::ostringstream flaggedEmptyOutput;
        dxfRW flaggedEmptyWriter("");
        flaggedEmptyWriter.version = DRW::AC1032;
        flaggedEmptyWriter.binFile = binary;
        flaggedEmptyWriter.writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&flaggedEmptyOutput))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&flaggedEmptyOutput));
        DRW_MLeader flaggedEmpty = scalar;
        flaggedEmpty.handle = 0xA102u;
        flaggedEmpty.context.roots.front().leaderLines.front()
            .colorDwgMethod = 0xC3u;
        flaggedEmpty.context.roots.front().leaderLines.front()
            .colorDwgFlags = 0x01u;
        flaggedEmpty.context.roots.front().leaderLines.front()
            .colorDwgMetadataPresent = true;
        const std::string initialFlaggedEmptyOutput = flaggedEmptyOutput.str();
        t.expect(!flaggedEmptyWriter.writeMultiLeader(&flaggedEmpty)
                     && flaggedEmptyOutput.str() == initialFlaggedEmptyOutput,
                 "MLEADER refuses source-flagged empty CMC name before DXF output");
    }
}

void testMLeaderStyleDxfCmcColorLossGuard(TestContext& t) {
    using ColorMutation = std::function<void(DRW_MLeaderStyle&, int, bool)>;
    const auto setMetadata = [](std::int32_t& rgb, UTF8STRING& name,
                                UTF8STRING& bookName, int kind,
                                bool enabled) {
        rgb = -1;
        name.clear();
        bookName.clear();
        if (!enabled)
            return;
        if (kind == 0)
            rgb = 0;
        else if (kind == 1)
            name = "DWG_COLOR_NAME";
        else
            bookName = "DWG_COLOR_BOOK";
    };
    const std::vector<ColorMutation> colors = {
        [setMetadata](DRW_MLeaderStyle& style, int kind, bool enabled) {
            setMetadata(style.leaderColorRgb, style.leaderColorName,
                        style.leaderColorBookName, kind, enabled);
        },
        [setMetadata](DRW_MLeaderStyle& style, int kind, bool enabled) {
            setMetadata(style.textColorRgb, style.textColorName,
                        style.textColorBookName, kind, enabled);
        },
        [setMetadata](DRW_MLeaderStyle& style, int kind, bool enabled) {
            setMetadata(style.blockColorRgb, style.blockColorName,
                        style.blockColorBookName, kind, enabled);
        }
    };
    for (const bool binary : {false, true}) {
        for (const ColorMutation& color : colors) {
            for (int metadataKind = 0; metadataKind < 3; ++metadataKind) {
                std::ostringstream output;
                dxfRW writer("");
                writer.version = DRW::AC1032;
                writer.binFile = binary;
                writer.writer = binary
                    ? std::unique_ptr<dxfWriter>(
                          std::make_unique<dxfWriterBinary>(&output))
                    : std::unique_ptr<dxfWriter>(
                          std::make_unique<dxfWriterAscii>(&output));
                const std::string initialOutput = output.str();
                DRW_MLeaderStyle style;
                style.handle = 0xA200u;
                style.name = "CMC_GUARD";
                color(style, metadataKind, true);
                t.expect(!writer.writeMLeaderStyle(&style)
                             && output.str() == initialOutput,
                         "MLEADERSTYLE refuses unrepresented DWG CMC metadata before output");
            }
        }

        struct CmcRefs {
            int* scalar;
            std::uint8_t* method;
            std::uint8_t* flags;
            std::uint16_t* reservedIndex;
            bool* metadataPresent;
        };
        const auto cmcRefs = [](DRW_MLeaderStyle& style) {
            return std::array<CmcRefs, 3>{{
                {&style.leaderColor, &style.leaderColorDwgMethod,
                 &style.leaderColorDwgFlags, &style.leaderColorDwgIndex,
                 &style.leaderColorDwgMetadataPresent},
                {&style.textColor, &style.textColorDwgMethod,
                 &style.textColorDwgFlags, &style.textColorDwgIndex,
                 &style.textColorDwgMetadataPresent},
                {&style.blockColor, &style.blockColorDwgMethod,
                 &style.blockColorDwgFlags, &style.blockColorDwgIndex,
                 &style.blockColorDwgMetadataPresent}
            }};
        };
        for (unsigned field = 0; field < 3u; ++field) {
            for (unsigned invalidKind = 0; invalidKind < 6u; ++invalidKind) {
                std::ostringstream output;
                dxfRW writer("");
                writer.version = DRW::AC1032;
                writer.binFile = binary;
                writer.writer = binary
                    ? std::unique_ptr<dxfWriter>(
                          std::make_unique<dxfWriterBinary>(&output))
                    : std::unique_ptr<dxfWriter>(
                          std::make_unique<dxfWriterAscii>(&output));
                const std::string initialOutput = output.str();
                DRW_MLeaderStyle style;
                style.handle = 0xA210u + field * 8u + invalidKind;
                style.name = "CMC_METHOD_GUARD";
                CmcRefs color = cmcRefs(style)[field];
                *color.metadataPresent = true;
                switch (invalidKind) {
                case 0: // Unsupported source method, projected ACI fallback.
                    *color.scalar = 7;
                    *color.method = 0xC5u;
                    break;
                case 1: // Flagged empty name has no DXF scalar projection.
                    *color.scalar = 7;
                    *color.method = 0xC3u;
                    *color.flags = 0x01u;
                    break;
                case 2:
                    *color.scalar = 7;
                    *color.method = 0xC3u;
                    *color.reservedIndex = 1u;
                    break;
                case 3:
                    *color.scalar = 256;
                    *color.method = 0xC3u;
                    break;
                case 4:
                    *color.scalar = 7;
                    *color.method = 0u;
                    break;
                case 5:
                    *color.scalar = 7;
                    *color.method = 0xC3u;
                    *color.flags = 0x80u;
                    break;
                }
                t.expect(!writer.writeMLeaderStyle(&style)
                             && output.str() == initialOutput,
                         "MLEADERSTYLE DXF refuses unsupported or malformed source CMC before output");
            }
        }

        std::ostringstream scalarOutput;
        dxfRW scalarWriter("");
        scalarWriter.version = DRW::AC1032;
        scalarWriter.binFile = binary;
        scalarWriter.writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&scalarOutput))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&scalarOutput));
        DRW_MLeaderStyle scalar;
        scalar.handle = 0xA201u;
        scalar.name = "SCALAR_ONLY";
        scalar.leaderColor = 7;
        scalar.textColor = 256;
        scalar.blockColor = 0;
        scalar.leaderColorDwgMethod = 0xC3u;
        scalar.textColorDwgMethod = 0xC0u;
        scalar.blockColorDwgMethod = 0xC1u;
        scalar.leaderColorDwgMetadataPresent = true;
        scalar.textColorDwgMetadataPresent = true;
        scalar.blockColorDwgMetadataPresent = true;
        const bool scalarWritten = scalarWriter.writeMLeaderStyle(&scalar)
            && scalarWriter.writer->writeString(0, "WIRE_END");
        bool scalarEnded = false;
        const auto scalarTags = numericWireProjection(scalarOutput.str(),
                                                       binary, scalarEnded);
        const auto hasScalarTag = [&scalarTags](const char* expected) {
            return std::find(scalarTags.begin(), scalarTags.end(), expected)
                != scalarTags.end();
        };
        t.expect(scalarWritten && scalarEnded && hasScalarTag("91:7")
                     && hasScalarTag("93:256") && hasScalarTag("94:0"),
                 "MLEADERSTYLE scalar ACI/ByLayer/ByBlock colors retain documented groups");
    }
}

void testMLeaderWriterReferenceHandleRemapping(TestContext& t) {
    const auto writeControl = [](const DRW_MLeader& source, bool binary,
                                 const std::map<std::uint32_t, std::uint32_t>& remap,
                                 std::vector<std::string>& tags,
                                 bool& ended, std::string& wire) {
        std::ostringstream output;
        dxfRW writer("");
        writer.version = DRW::AC1021;
        writer.binFile = binary;
        writer.setHandleRemap(remap);
        writer.writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&output))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&output));
        DRW_MLeader entity(source);
        const bool written = writer.writeMultiLeader(&entity)
            && writer.writer->writeString(0, "WIRE_END");
        wire = output.str();
        if (written)
            tags = numericWireProjection(wire, binary, ended);
        else {
            tags.clear();
            ended = false;
        }
        return written;
    };

    const std::map<std::uint32_t, std::uint32_t> textRemap{
        {0xA100u, 0xB100u}, {0xA101u, 0xB101u}, {0xA102u, 0xB102u},
        {0xA103u, 0xB103u}, {0xA104u, 0xB104u}, {0xA105u, 0xB105u},
        {0xA106u, 0xB106u}, {0xA107u, 0xB107u}, {0xA108u, 0xB108u}};
    DRW_MLeader text;
    text.handle = 0xA100u;
    text.parentHandle = 0xA101u;
    text.styleHandle.ref = 0xA102u;
    text.leaderLineTypeHandle.ref = 0xA103u;
    text.arrowHeadHandle.ref = 0xA104u;
    text.styleTextStyleHandle.ref = 0xA105u;
    text.styleBlockHandle.ref = 0xA106u;
    text.context.hasTextContents = true;
    text.context.contentBasePoint = DRW_Coord{7.25, -8.5, 9.75};
    text.context.textStyleHandle.ref = 0xA107u;
    DRW_MLeaderRoot textRoot;
    textRoot.connectionPoint = DRW_Coord{-11.25, 12.5, -13.75};
    textRoot.direction = DRW_Coord{0.25, -0.5, 0.75};
    textRoot.breaks.emplace_back(DRW_Coord{14.25, -15.5, 16.75},
                                 DRW_Coord{-17.25, 18.5, -19.75});
    DRW_MLeaderLeaderLine textLine;
    textLine.points.emplace_back(DRW_Coord{101.25, -202.5, 303.75});
    textLine.breaks.emplace_back(DRW_Coord{-404.25, 505.5, -606.75},
                                 DRW_Coord{707.25, -808.5, 909.75});
    textLine.lineTypeHandle.ref = 0xA108u;
    textRoot.leaderLines.push_back(textLine);
    text.context.roots.push_back(textRoot);

    const auto hasCoord = [](const std::vector<std::string>& tags, int xCode,
                             const DRW_Coord& coord) {
        std::vector<std::string> expected;
        const auto add = [&expected](int code, double value) {
            std::ostringstream tag;
            tag << code << ':' << std::hexfloat << value;
            expected.push_back(tag.str());
        };
        add(xCode, coord.x);
        add(xCode + 10, coord.y);
        add(xCode + 20, coord.z);
        return std::search(tags.begin(), tags.end(), expected.begin(),
                           expected.end()) != tags.end();
    };
    const auto hasDouble = [](const std::vector<std::string>& tags, int code,
                              double value) {
        std::ostringstream expected;
        expected << code << ':' << std::hexfloat << value;
        return std::find(tags.begin(), tags.end(), expected.str())
            != tags.end();
    };

    const std::vector<std::string> expectedTextReferences{
        "340:B102", "341:B103", "342:B104",
        "343:B105", "344:B106", "340:B107", "340:B108"};
    std::vector<std::string> asciiProjection;
    for (const bool binary : {false, true}) {
        std::vector<std::string> tags;
        bool ended = false;
        std::string wire;
        const bool written = writeControl(text, binary, textRemap,
                                          tags, ended, wire);
        t.expect(written && ended,
                 "MLEADER text-reference remap keeps following-record framing");
        for (const std::string& expected : expectedTextReferences)
        {
            const std::string message =
                "MLEADER remapped text reference missing " + expected;
            t.expect(std::find(tags.begin(), tags.end(), expected) != tags.end(),
                     message.c_str());
        }
        if (!binary)
            asciiProjection = tags;
        else
            t.expect(tags == asciiProjection,
                     "MLEADER text-reference ASCII/binary output agrees");
        t.expect(std::count(tags.begin(), tags.end(), "172:2") == 1
                     && std::count(tags.begin(), tags.end(), "172:0") == 1
                     && std::find(tags.begin(), tags.end(), "290:1")
                            != tags.end()
                     && std::find(tags.begin(), tags.end(), "296:0")
                            != tags.end(),
                 "MLEADER MTEXT content type and context flags remain distinct");
        t.expect(hasCoord(tags, 10, text.context.contentBasePoint)
                     && hasCoord(tags, 10,
                                 text.context.roots.front().connectionPoint)
                     && hasCoord(tags, 11,
                                 text.context.roots.front().direction)
                     && hasCoord(tags, 12,
                         text.context.roots.front().breaks.front().first)
                     && hasCoord(tags, 13,
                         text.context.roots.front().breaks.front().second)
                     && hasCoord(tags, 10,
                         text.context.roots.front().leaderLines.front()
                             .points.front())
                     && hasCoord(tags, 11,
                         text.context.roots.front().leaderLines.front()
                             .breaks.front().first)
                     && hasCoord(tags, 12,
                         text.context.roots.front().leaderLines.front()
                             .breaks.front().second),
                 "MLEADER WCS leader and break coordinates are serialized unchanged");
    }

    const std::map<std::uint32_t, std::uint32_t> blockRemap{
        {0xA200u, 0xB200u}, {0xA201u, 0xB201u}, {0xA202u, 0xB202u},
        {0xA203u, 0xB203u}, {0xA204u, 0xB204u}, {0xA205u, 0xB205u}};
    DRW_MLeader block;
    block.handle = 0xA200u;
    block.parentHandle = 0xA201u;
    block.styleContentType = 1;
    block.styleBlockHandle.ref = 0xA202u;
    block.context.hasContentsBlock = true;
    block.context.blockTableRecordHandle.ref = 0xA203u;
    block.context.blockNormal = DRW_Coord{0.0, 0.0, 1.0};
    block.context.blockLocation = DRW_Coord{21.25, -22.5, 23.75};
    block.context.blockScale = DRW_Coord{2.0, 3.0, 4.0};
    block.context.blockRotation = 0.5;
    block.context.basePoint = DRW_Coord{-31.25, 32.5, -33.75};
    block.context.baseDirection = DRW_Coord{0.25, 0.5, -0.75};
    block.context.baseVertical = DRW_Coord{-0.125, 0.25, 1.0};
    for (std::size_t i = 0; i < block.context.blockTransform.size(); ++i)
        block.context.blockTransform[i] = static_cast<double>(i + 1u);
    DRW_MLeaderRoot blockRoot;
    DRW_MLeaderLeaderLine blockLine;
    blockLine.arrowHandle.ref = 0xA204u;
    blockRoot.leaderLines.push_back(blockLine);
    block.context.roots.push_back(blockRoot);
    DRW_MLeader::ArrowHeadEntry arrowEntry;
    arrowEntry.handle.ref = 0xA205u;
    arrowEntry.dxfIndex = 0;
    arrowEntry.hasDxfIndex = true;
    block.arrowHeads.push_back(arrowEntry);
    DRW_MLeader::BlockLabelEntry labelEntry;
    labelEntry.attDefHandle.ref = 0xA204u;
    labelEntry.labelText = "ATTR";
    block.blockLabels.push_back(labelEntry);

    for (const bool binary : {false, true}) {
        std::vector<std::string> tags;
        bool ended = false;
        std::string wire;
        const bool written = writeControl(block, binary, blockRemap,
                                          tags, ended, wire);
        t.expect(written && ended,
                 "MLEADER block-reference remap keeps following-record framing");
        t.expect(std::count(tags.begin(), tags.end(), "172:1") == 1
                     && std::find(tags.begin(), tags.end(), "290:0")
                            != tags.end()
                     && std::find(tags.begin(), tags.end(), "296:1")
                            != tags.end(),
                 "MLEADER BLOCK content type and context flags remain distinct");
        std::vector<std::string> expectedBlockTransform;
        for (const double value : block.context.blockTransform) {
            std::ostringstream tag;
            tag << "47:" << std::hexfloat << value;
            expectedBlockTransform.push_back(tag.str());
        }
        t.expect(hasCoord(tags, 14, block.context.blockNormal)
                     && hasCoord(tags, 15, block.context.blockLocation)
                     && hasCoord(tags, 16, block.context.blockScale)
                     && hasDouble(tags, 46, block.context.blockRotation)
                     && hasCoord(tags, 110, block.context.basePoint)
                     && hasCoord(tags, 111, block.context.baseDirection)
                     && hasCoord(tags, 112, block.context.baseVertical)
                     && std::search(tags.begin(), tags.end(),
                                    expectedBlockTransform.begin(),
                                    expectedBlockTransform.end())
                            != tags.end(),
                 "MLEADER block transform, content frame, and placement stay distinct");
        for (const std::string& expected : {"344:B202", "341:B203",
                                            "341:B204", "345:B205",
                                            "330:B204"}) {
            const std::string message =
                std::string("MLEADER remapped block reference missing ") + expected;
            t.expect(std::find(tags.begin(), tags.end(), expected) != tags.end(),
                     message.c_str());
        }
    }

    using ReferenceMutator = std::pair<const char*,
        std::function<void(DRW_MLeader&)>>;
    const std::vector<ReferenceMutator> invalidReferences{
        {"entity style", [](DRW_MLeader& entity) {
             entity.styleHandle.ref = 0xAF01u;
         }},
        {"context text style", [](DRW_MLeader& entity) {
             entity.context.hasTextContents = true;
             entity.context.textStyleHandle.ref = 0xAF01u;
         }},
        {"context block record", [](DRW_MLeader& entity) {
             entity.context.hasContentsBlock = true;
             entity.context.blockTableRecordHandle.ref = 0xAF01u;
         }},
        {"leader-line linetype", [](DRW_MLeader& entity) {
             DRW_MLeaderRoot root;
             DRW_MLeaderLeaderLine line;
             line.lineTypeHandle.ref = 0xAF01u;
             root.leaderLines.push_back(line);
             entity.context.roots.push_back(root);
         }},
        {"leader-line arrowhead", [](DRW_MLeader& entity) {
             DRW_MLeaderRoot root;
             DRW_MLeaderLeaderLine line;
             line.arrowHandle.ref = 0xAF01u;
             root.leaderLines.push_back(line);
             entity.context.roots.push_back(root);
         }},
        {"legacy arrowhead", [](DRW_MLeader& entity) {
             DRW_MLeader::ArrowHeadEntry entry;
             entry.handle.ref = 0xAF01u;
             entry.hasDxfIndex = true;
             entry.dxfIndex = 2;
             entity.arrowHeads.push_back(entry);
         }},
        {"legacy block-label ATTDEF", [](DRW_MLeader& entity) {
             DRW_MLeader::BlockLabelEntry entry;
             entry.attDefHandle.ref = 0xAF01u;
             entity.blockLabels.push_back(entry);
         }}};
    for (const bool binary : {false, true}) {
        for (const ReferenceMutator& invalid : invalidReferences) {
            DRW_MLeader entity;
            entity.handle = 0xA300u;
            invalid.second(entity);
            std::vector<std::string> tags;
            bool ended = false;
            std::string wire;
            const bool written = writeControl(
                entity, binary, {{0xAF01u, 0u}}, tags, ended, wire);
            const std::string message = std::string("MLEADER ") + invalid.first
                + " remapped to null refuses before record publication";
            t.expect(!written && wire.empty(),
                     message.c_str());
        }
    }
}

void testFieldAndFieldListHandleRemapping(TestContext& t) {
    std::map<std::uint32_t, std::uint32_t> remap;
    const std::vector<std::uint32_t> sourceHandles = {
        0x501u, 0x502u, 0x503u, 0x504u, 0x505u, 0x506u,
        0x507u, 0x508u, 0x509u, 0x520u, 0x521u};
    const auto onceMapped = [](std::uint32_t source) {
        return source + 0xA000u;
    };
    const auto twiceMapped = [](std::uint32_t source) {
        return source + 0xE000u;
    };
    for (std::uint32_t source : sourceHandles) {
        remap.emplace(source, onceMapped(source));
        remap.emplace(onceMapped(source), twiceMapped(source));
    }
    const auto handleText = [](std::uint32_t handle) {
        std::ostringstream text;
        text << std::uppercase << std::hex << handle;
        return text.str();
    };
    const auto occurrenceCount = [](const std::vector<std::string>& tags,
                                    const std::string& expected) {
        return std::count(tags.begin(), tags.end(), expected);
    };

    std::vector<std::string> asciiProjection;
    for (const bool binary : {false, true}) {
        DRW_Field field;
        field.handle = 0x501u;
        field.parentHandle = 0x502u;
        field.reactorHandles = {0x508u};
        field.xDictHandle = 0x509u;
        field.m_childHandles = {0x503u, 0x50Bu};
        field.m_objectHandles = {0x504u, 0x505u};
        field.m_value.m_dataType = 64;
        field.m_value.m_handle = 0x506u;
        field.m_value.m_value.addInt(330, 0x506);
        DRW_Field::ChildValue childValue;
        childValue.m_key = "OBJECT_ID";
        childValue.m_value.m_dataType = 64;
        childValue.m_value.m_value.addInt(330, 0x507);
        field.m_childValues.push_back(childValue);

        std::ostringstream output;
        dxfRW writer("");
        writer.version = DRW::AC1032;
        writer.binFile = binary;
        writer.setHandleRemap(remap);
        if (binary)
            writer.writer = std::make_unique<dxfWriterBinary>(&output);
        else
            writer.writer = std::make_unique<dxfWriterAscii>(&output);
        const bool wrote = writer.writeField(&field)
            && writer.writer->writeString(0, "WIRE_END");
        bool ended = false;
        const auto tags = numericWireProjection(output.str(), binary, ended);
        t.expect(wrote && ended,
                 "FIELD handle remapping preserves following-record framing");
        for (const auto& expected : {
                 std::make_pair(5, onceMapped(0x501u)),
                 std::make_pair(330, onceMapped(0x502u)),
                 std::make_pair(360, onceMapped(0x503u)),
                 std::make_pair(360, 0x50Bu),
                 std::make_pair(331, onceMapped(0x504u)),
                 std::make_pair(331, onceMapped(0x505u)),
                 std::make_pair(330, onceMapped(0x506u)),
                 std::make_pair(330, onceMapped(0x507u)),
                 std::make_pair(330, onceMapped(0x508u)),
                 std::make_pair(360, onceMapped(0x509u))}) {
            const std::string tag = std::to_string(expected.first) + ":"
                + handleText(expected.second);
            t.expect(occurrenceCount(tags, tag) == 1,
                     "FIELD self, owner, graph, CadValue, and common handles remap once");
        }
        for (const auto& stale : {
                 std::make_pair(5, 0x501u), std::make_pair(330, 0x502u),
                 std::make_pair(360, 0x503u), std::make_pair(331, 0x504u),
                 std::make_pair(331, 0x505u), std::make_pair(330, 0x506u),
                 std::make_pair(330, 0x507u), std::make_pair(330, 0x508u),
                 std::make_pair(360, 0x509u)}) {
            const std::string tag = std::to_string(stale.first) + ":"
                + handleText(stale.second);
            t.expect(occurrenceCount(tags, tag) == 0,
                     "FIELD output contains no stale source handle at a remapped edge");
        }
        for (std::uint32_t source : sourceHandles) {
            const std::string secondStep = handleText(twiceMapped(source));
            t.expect(std::none_of(tags.begin(), tags.end(),
                         [&secondStep](const std::string& tag) {
                             return tag.size() > secondStep.size()
                                 && tag.compare(tag.size() - secondStep.size(),
                                                secondStep.size(), secondStep) == 0;
                         }),
                     "FIELD output does not apply a chained remap twice");
        }
        t.expect(field.handle == 0x501u && field.parentHandle == 0x502u
                     && field.m_childHandles == std::vector<std::uint32_t>{0x503u, 0x50Bu}
                     && field.m_objectHandles == std::vector<std::uint32_t>{0x504u, 0x505u}
                     && field.m_value.m_handle == 0x506u
                     && field.m_value.m_value.i_val() == 0x506
                     && field.m_childValues.front().m_value.m_value.i_val() == 0x507,
                 "FIELD remapping leaves the source model unchanged");
        if (!binary)
            asciiProjection = tags;
        else
            t.expect(tags == asciiProjection,
                     "FIELD ASCII and binary remapped values agree");
    }

    asciiProjection.clear();
    for (const bool binary : {false, true}) {
        DRW_FieldList fieldList;
        fieldList.handle = 0x520u;
        fieldList.parentHandle = 0x521u;
        fieldList.m_fieldHandles = {0x503u, 0x504u, 0x50Bu};

        std::ostringstream output;
        dxfRW writer("");
        writer.version = DRW::AC1032;
        writer.binFile = binary;
        writer.setHandleRemap(remap);
        if (binary)
            writer.writer = std::make_unique<dxfWriterBinary>(&output);
        else
            writer.writer = std::make_unique<dxfWriterAscii>(&output);
        const bool wrote = writer.writeFieldList(&fieldList)
            && writer.writer->writeString(0, "WIRE_END");
        bool ended = false;
        const auto tags = numericWireProjection(output.str(), binary, ended);
        t.expect(wrote && ended,
                 "FIELDLIST handle remapping preserves following-record framing");
        for (const auto& expected : {
                 std::make_pair(5, onceMapped(0x520u)),
                 std::make_pair(330, onceMapped(0x521u)),
                 std::make_pair(330, onceMapped(0x503u)),
                 std::make_pair(330, onceMapped(0x504u)),
                 std::make_pair(330, 0x50Bu)}) {
            const std::string tag = std::to_string(expected.first) + ":"
                + handleText(expected.second);
            t.expect(occurrenceCount(tags, tag) == 1,
                     "FIELDLIST self, owner, and each member remap once");
        }
        for (const auto& stale : {
                 std::make_pair(5, 0x520u), std::make_pair(330, 0x521u),
                 std::make_pair(330, 0x503u), std::make_pair(330, 0x504u)}) {
            const std::string tag = std::to_string(stale.first) + ":"
                + handleText(stale.second);
            t.expect(occurrenceCount(tags, tag) == 0,
                     "FIELDLIST output contains no stale source handle at a remapped edge");
        }
        for (std::uint32_t source : {0x520u, 0x521u, 0x503u, 0x504u}) {
            const std::string secondStep = handleText(twiceMapped(source));
            t.expect(std::none_of(tags.begin(), tags.end(),
                         [&secondStep](const std::string& tag) {
                             return tag.size() > secondStep.size()
                                 && tag.compare(tag.size() - secondStep.size(),
                                                secondStep.size(), secondStep) == 0;
                         }),
                     "FIELDLIST output does not apply a chained remap twice");
        }
        t.expect(fieldList.handle == 0x520u && fieldList.parentHandle == 0x521u
                     && fieldList.m_fieldHandles
                         == std::vector<std::uint32_t>{0x503u, 0x504u, 0x50Bu},
                 "FIELDLIST remapping leaves the source model unchanged");
        if (!binary)
            asciiProjection = tags;
        else
            t.expect(tags == asciiProjection,
                     "FIELDLIST ASCII and binary remapped values agree");
    }
}

void testAcadFieldExtensionDictionaryWireCandidate(TestContext& t) {
    const auto hasSequence = [](const std::vector<std::string>& tags,
                                const std::vector<std::string>& expected) {
        return std::search(tags.begin(), tags.end(), expected.begin(),
                           expected.end()) != tags.end();
    };
    std::vector<std::string> asciiProjection;
    std::vector<std::string> trueFlagAsciiProjection;
    for (const bool binary : {false, true}) {
        std::ostringstream output;
        dxfRW writer("");
        writer.version = DRW::AC1027;
        writer.binFile = binary;
        writer.setRootDictEntries({{"ACAD_FIELDLIST", "A740"}});
        ProfileProbeInterface outputInterface;
        writer.iface = &outputInterface;
        outputInterface.dxfW = &writer;
        if (binary)
            writer.writer = std::make_unique<dxfWriterBinary>(&output);
        else
            writer.writer = std::make_unique<dxfWriterAscii>(&output);

        DRW_Line ownerEntity;
        ownerEntity.handle = 0xA700u;
        ownerEntity.xDictHandle = 0xA710u;
        ownerEntity.basePoint = DRW_Coord(1.0, 2.0, 0.0);
        ownerEntity.secPoint = DRW_Coord(3.0, 4.0, 0.0);

        DRW_Dictionary extensionDictionary;
        extensionDictionary.handle = 0xA710u;
        extensionDictionary.parentHandle = 0xA700u;
        extensionDictionary.hardOwner = 0;
        extensionDictionary.m_entries = {
            {"ACAD_FIELD", 0xA720u,
             DRW_Dictionary::Entry::DxfHandleCode::HardOwner},
            {"SIBLING", 0xA730u,
             DRW_Dictionary::Entry::DxfHandleCode::SoftOwner}};

        DRW_Field field;
        field.handle = 0xA720u;
        field.parentHandle = 0xA710u;
        field.m_evaluatorId = "AcObjProp";
        field.m_fieldCode = "FIELD_CODE";
        field.m_childHandles = {0xA721u};
        field.m_objectHandles = {0xA730u};
        field.m_value.m_dataType = 64;
        field.m_value.m_handle = 0xA730u;
        field.m_value.m_value.addInt(330, 0xA730);

        DRW_Field childField;
        childField.handle = 0xA721u;
        childField.parentHandle = 0xA720u;
        childField.m_evaluatorId = "AcObjProp";
        childField.m_fieldCode = "CHILD_FIELD";

        DRW_FieldList fieldList;
        fieldList.handle = 0xA740u;
        fieldList.m_fieldHandles = {0xA720u, 0xA721u};

        DRW_XRecord sibling;
        sibling.handle = 0xA730u;
        sibling.parentHandle = 0xA710u;

        const bool wrote = writer.writeLine(&ownerEntity)
            && writer.writeObjects()
            && writer.writeDictionary(&extensionDictionary)
            && writer.writeField(&field)
            && writer.writeField(&childField)
            && writer.writeXRecord(&sibling)
            && writer.writeFieldList(&fieldList)
            && writer.writer->writeString(0, "WIRE_END");
        const std::string wire = output.str();
        bool ended = false;
        const std::vector<std::string> tags = numericWireProjection(
            wire, binary, ended);
        t.expect(wrote && ended,
                 "ACAD_FIELD candidate direct writer emits a bounded ASCII/Binary sequence");

        t.expect(hasSequence(tags, {"102:{ACAD_XDICTIONARY", "360:A710",
                                    "102:}"}),
                 "entity extension-dictionary attachment is a distinct group-360 edge");
        t.expect(hasSequence(tags, {"0:DICTIONARY", "5:C", "330:0",
                                    "100:AcDbDictionary", "281:1",
                                    "3:ACAD_GROUP", "350:D",
                                    "3:ACAD_FIELDLIST", "350:A740"}),
                 "regenerated NOD ACAD_FIELDLIST remains a separate group-350 route");
        const auto lineRecord = std::find(tags.begin(), tags.end(), "0:LINE");
        const std::string emittedLineHandle = lineRecord != tags.end()
                && std::next(lineRecord) != tags.end()
            ? *std::next(lineRecord) : std::string{};
        const std::string dictionaryOwner = emittedLineHandle.size() > 2
                && emittedLineHandle.compare(0, 2, "5:") == 0
            ? "330:" + emittedLineHandle.substr(2) : "330:UNRESOLVED";
        const std::vector<std::string> dictionaryCandidate{
            "0:DICTIONARY", "5:A710", dictionaryOwner,
            "100:AcDbDictionary", "280:0", "281:0", "3:ACAD_FIELD",
            "360:A720", "3:SIBLING", "350:A730"};
        const bool dictionaryCandidateMatches =
            hasSequence(tags, dictionaryCandidate);
        t.expect(dictionaryCandidateMatches,
                 "flag-280-clear ACAD_FIELD candidate is 360 while sibling stays 350");
        t.expect(hasSequence(tags, {"0:FIELD", "5:A720", "330:A710",
                                    "100:AcDbField", "1:AcObjProp",
                                    "2:FIELD_CODE", "90:1", "360:A721",
                                    "97:1", "331:A730"}),
                 "FIELD child 360 and expression object 331 remain distinct");
        t.expect(hasSequence(tags, {"7:ACFD_FIELD_VALUE", "93:0", "90:64",
                                    "330:A730", "94:0"}),
                 "FIELD ID-valued cache group 330 follows its typed-value marker and remains distinct from owner/member edges");
        t.expect(hasSequence(tags, {"100:AcDbIdSet", "90:2", "330:A720",
                                    "330:A721", "100:AcDbFieldList"}),
                 "default FIELDLIST emits its declared members without a flag tag");
        t.expect(std::find(tags.begin(), tags.end(), "290:0") == tags.end(),
                 "default FIELDLIST omits false id-set flag 290");
        t.expect(std::count(tags.begin(), tags.end(), "0:FIELD") == 2
                     && std::count(tags.begin(), tags.end(), "0:XRECORD") == 1,
                 "candidate graph emits one root FIELD, one child FIELD, and sibling target");

        if (!binary)
            asciiProjection = tags;
        else
            t.expect(tags == asciiProjection,
                     "ACAD_FIELD candidate ASCII and binary projections agree");

        std::ostringstream trueFlagOutput;
        dxfRW trueFlagWriter("");
        trueFlagWriter.version = DRW::AC1027;
        trueFlagWriter.binFile = binary;
        if (binary)
            trueFlagWriter.writer = std::make_unique<dxfWriterBinary>(
                &trueFlagOutput);
        else
            trueFlagWriter.writer = std::make_unique<dxfWriterAscii>(
                &trueFlagOutput);
        DRW_FieldList trueFlagFieldList;
        trueFlagFieldList.handle = 0xA741u;
        trueFlagFieldList.m_fieldHandles = {0xA720u, 0xA721u};
        trueFlagFieldList.m_unknown = 1;
        const bool wroteTrueFlag = trueFlagWriter.writeFieldList(
                &trueFlagFieldList)
            && trueFlagWriter.writer->writeString(0, "WIRE_END");
        bool trueFlagEnded = false;
        const std::vector<std::string> trueFlagTags = numericWireProjection(
            trueFlagOutput.str(), binary, trueFlagEnded);
        t.expect(wroteTrueFlag && trueFlagEnded
                     && hasSequence(trueFlagTags,
                         {"100:AcDbIdSet", "90:2", "290:1", "330:A720",
                          "330:A721", "100:AcDbFieldList"}),
                 "true FIELDLIST id-set flag emits 290:1 before member handles");
        t.expect(std::find(trueFlagTags.begin(), trueFlagTags.end(), "290:0")
                     == trueFlagTags.end(),
                 "true FIELDLIST candidate never emits a false flag value");
        if (!binary)
            trueFlagAsciiProjection = trueFlagTags;
        else
            t.expect(trueFlagTags == trueFlagAsciiProjection,
                     "FIELDLIST true-flag ASCII and binary projections agree");

        if (!binary) {
            const std::size_t dictionaryStart = wire.find("0\nDICTIONARY\n");
            const std::size_t sentinelStart = wire.find("0\nWIRE_END\n");
            const bool splitFound = dictionaryStart != std::string::npos
                && sentinelStart != std::string::npos
                && dictionaryStart < sentinelStart;
            t.expect(splitFound,
                     "generated ASCII carrier can be placed into strict DXF sections");
            if (splitFound) {
                const std::string entityRecords = wire.substr(0, dictionaryStart);
                const std::string objectRecords = wire.substr(
                    dictionaryStart, sentinelStart - dictionaryStart);
                std::string content =
                    "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1027\n"
                    "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
                    + entityRecords + "0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n"
                    + objectRecords + "0\nENDSEC\n0\nEOF\n";
                ProfileProbeInterface readback;
                dxfRW reader("");
                const bool parsed = reader.readAscii(&readback, false, content);
                const auto parsedDictionary = std::find_if(
                    readback.storage.dictionaries.begin(),
                    readback.storage.dictionaries.end(),
                    [](const DRW_Dictionary& value) {
                        return value.handle == 0xA710u;
                    });
                const auto parsedField = std::find_if(
                    readback.storage.fields.begin(), readback.storage.fields.end(),
                    [](const DRW_Field& value) {
                        return value.handle == 0xA720u;
                    });
                const auto parsedChild = std::find_if(
                    readback.storage.fields.begin(), readback.storage.fields.end(),
                    [](const DRW_Field& value) {
                        return value.handle == 0xA721u;
                    });
                const DRW_Line* parsedLine = nullptr;
                if (readback.storage.mBlock != nullptr) {
                    for (const DRW_Entity* entity : readback.storage.mBlock->ent) {
                        parsedLine = dynamic_cast<const DRW_Line*>(entity);
                        if (parsedLine != nullptr)
                            break;
                    }
                }
                const auto rootFieldListEntry = std::find_if(
                    readback.storage.namedObjectDictionary.m_entries.begin(),
                    readback.storage.namedObjectDictionary.m_entries.end(),
                    [](const DRW_Dictionary::Entry& value) {
                        return value.m_name == "ACAD_FIELDLIST";
                    });
                const auto parsedFieldList = std::find_if(
                    readback.storage.fieldLists.begin(),
                    readback.storage.fieldLists.end(),
                    [](const DRW_FieldList& value) {
                        return value.handle == 0xA740u;
                    });
                t.expect(parsed && parsedDictionary != readback.storage.dictionaries.end()
                             && parsedDictionary->handle == 0xA710u
                             && parsedDictionary->parentHandle == 0x30u
                             && parsedDictionary->m_entries.size() == 2u
                             && parsedDictionary->m_entries[0].m_name == "ACAD_FIELD"
                             && parsedDictionary->m_entries[0].m_handle == 0xA720u
                             && parsedDictionary->m_entries[0].m_dxfHandleCode
                                 == DRW_Dictionary::Entry::DxfHandleCode::HardOwner
                             && parsedDictionary->m_entries[1].m_name == "SIBLING"
                             && parsedDictionary->m_entries[1].m_handle == 0xA730u
                             && parsedDictionary->m_entries[1].m_dxfHandleCode
                                 == DRW_Dictionary::Entry::DxfHandleCode::SoftOwner,
                         "strict ASCII reread retains dictionary target, owner, and per-key edge kinds");
                t.expect(parsed && parsedField != readback.storage.fields.end()
                             && parsedField->parentHandle == 0xA710u
                             && parsedField->m_childHandles
                                 == std::vector<std::uint32_t>{0xA721u}
                             && parsedField->m_objectHandles
                                 == std::vector<std::uint32_t>{0xA730u}
                             && parsedField->m_value.m_dataType == 64
                             && parsedField->m_value.m_handle == 0xA730u
                             && parsedField->m_value.m_value.i_val() == 0xA730
                             && parsedChild != readback.storage.fields.end()
                             && parsedChild->parentHandle == 0xA720u
                             && parsedLine != nullptr
                             && parsedLine->xDictHandle == 0xA710u
                             && readback.storage.xRecords.size() == 1u
                             && readback.storage.xRecords.front().handle == 0xA730u
                             && readback.storage.hasNamedObjectDictionary
                             && rootFieldListEntry
                                 != readback.storage.namedObjectDictionary.m_entries.end()
                             && rootFieldListEntry->m_handle == 0xA740u
                             && rootFieldListEntry->m_dxfHandleCode
                                 == DRW_Dictionary::Entry::DxfHandleCode::SoftOwner
                             && parsedFieldList != readback.storage.fieldLists.end()
                             && parsedFieldList->m_fieldHandles
                                 == std::vector<std::uint32_t>{0xA720u, 0xA721u},
                         "strict ASCII reread closes extension dictionary, FIELD child/reference, and sibling target");
            }
        }
    }
}

void testSolidNonDefaultOcsTransform(TestContext& t) {
    const std::string record =
        "0\nSECTION\n2\nENTITIES\n0\nSOLID\n5\n711\n8\n0\n"
        "10\n1\n20\n2\n30\n3\n"
        "11\n4\n21\n5\n31\n6\n"
        "12\n7\n22\n8\n32\n9\n"
        "13\n10\n23\n11\n33\n12\n"
        "39\n2.5\n210\n0\n220\n1\n230\n0\n"
        "0\nENDSEC\n0\nEOF\n";
    const auto readSolid = [&record](bool applyExtrusion,
                                     DRW_Solid& solid) {
        ProfileProbeInterface capture;
        dxfRW reader("");
        std::string input = record;
        if (!reader.readAscii(&capture, applyExtrusion, input)
            || capture.storage.mBlock == nullptr)
            return false;
        const auto found = std::find_if(capture.storage.mBlock->ent.begin(),
            capture.storage.mBlock->ent.end(), [](const DRW_Entity* entity) {
                return entity != nullptr && entity->eType == DRW::SOLID;
            });
        if (found == capture.storage.mBlock->ent.end())
            return false;
        const auto* parsed = dynamic_cast<const DRW_Solid*>(*found);
        if (parsed == nullptr)
            return false;
        solid = *parsed;
        return true;
    };

    DRW_Solid ocs;
    t.expect(readSolid(false, ocs)
                 && ocs.basePoint.x == 1.0 && ocs.basePoint.y == 2.0
                 && ocs.basePoint.z == 3.0
                 && ocs.secPoint.x == 4.0 && ocs.secPoint.y == 5.0
                 && ocs.secPoint.z == 6.0
                 && ocs.thirdPoint.x == 7.0 && ocs.thirdPoint.y == 8.0
                 && ocs.thirdPoint.z == 9.0
                 && ocs.fourPoint.x == 10.0 && ocs.fourPoint.y == 11.0
                 && ocs.fourPoint.z == 12.0
                 && ocs.thickness == 2.5
                 && ocs.extPoint.x == 0.0 && ocs.extPoint.y == 1.0
                 && ocs.extPoint.z == 0.0,
             "SOLID ext=false preserves raw OCS corners, thickness, and extrusion");

    DRW_Solid wcs;
    t.expect(readSolid(true, wcs)
                 && wcs.basePoint.x == -1.0 && wcs.basePoint.y == 3.0
                 && wcs.basePoint.z == 2.0
                 && wcs.secPoint.x == -4.0 && wcs.secPoint.y == 6.0
                 && wcs.secPoint.z == 5.0
                 && wcs.thirdPoint.x == -7.0 && wcs.thirdPoint.y == 9.0
                 && wcs.thirdPoint.z == 8.0
                 && wcs.fourPoint.x == -10.0 && wcs.fourPoint.y == 12.0
                 && wcs.fourPoint.z == 11.0
                 && wcs.thickness == 2.5
                 && wcs.extPoint.x == 0.0 && wcs.extPoint.y == 1.0
                 && wcs.extPoint.z == 0.0,
             "SOLID ext=true applies the non-default OCS basis once to all corners");
}

void testFieldDxfVersionedWireProfiles(TestContext& t) {
    const auto runProfile = [&t](DRW::Version version, DRW_Field field,
                                 const std::vector<std::string>& required,
                                 const std::vector<std::string>& forbidden,
                                 const char* label) {
        std::vector<std::string> asciiProjection;
        for (const bool binary : {false, true}) {
            std::ostringstream output;
            dxfRW writer("");
            writer.version = version;
            writer.binFile = binary;
            if (binary)
                writer.writer = std::make_unique<dxfWriterBinary>(&output);
            else
                writer.writer = std::make_unique<dxfWriterAscii>(&output);
            const bool wrote = writer.writeField(&field)
                && writer.writer->writeString(0, "WIRE_END");
            bool ended = false;
            const std::vector<std::string> tags = numericWireProjection(
                output.str(), binary, ended);
            t.expect(wrote && ended, label);
            for (const std::string& tag : required)
                t.expect(std::count(tags.begin(), tags.end(), tag) == 1,
                         "FIELD version profile emits each required literal once");
            for (const std::string& tag : forbidden)
                t.expect(std::find(tags.begin(), tags.end(), tag) == tags.end(),
                         "FIELD version profile omits incompatible literal");
            if (!binary)
                asciiProjection = tags;
            else
                t.expect(tags == asciiProjection,
                         "FIELD version profile ASCII/binary tags agree");
        }
    };

    DRW_Field legacy;
    legacy.handle = 0xA500u;
    legacy.parentHandle = 0x20u;
    legacy.m_evaluatorId = "AcObjProp";
    legacy.m_fieldCode = "LEGACY_FIELD_CODE";
    legacy.m_formatString = "%lu2";
    legacy.m_value.m_dataType = 2;
    legacy.m_value.m_value.addDouble(140, 1303.11);
    legacy.m_valueString = "1303.11";
    legacy.m_valueStringLength = 7;
    runProfile(DRW::AC1018, legacy,
        {"4:%lu2", "7:ACFD_FIELD_VALUE", "90:2", "301:1303.11", "98:7"},
        {"304:ACVALUE_END", "300:%lu2", "302:1303.11"},
        "legacy FIELD group-4/cache output succeeds");

    DRW_Field modern;
    modern.handle = 0xA510u;
    modern.parentHandle = 0x20u;
    modern.m_evaluatorId = "AcObjProp";
    modern.m_fieldCode = "MODERN_FIELD_CODE";
    // A legacy root format is promoted into the modern CadValue when the
    // caller supplies a typed evaluated value.
    modern.m_formatString = "%lu2";
    modern.m_value.m_dataType = 2;
    modern.m_value.m_value.addDouble(140, 598880.0);
    modern.m_value.m_valueString = "598880.00";
    modern.m_valueString = "598880.00";
    modern.m_valueStringLength = 9;
    runProfile(DRW::AC1027, modern,
        {"7:ACFD_FIELD_VALUE", "300:%lu2", "302:598880.00",
         "301:598880.00", "98:9", "304:ACVALUE_END"},
        {"4:%lu2"},
        "modern FIELD nested-value output succeeds without legacy code 4");

    modern.m_value.m_formatString = "conflicting-format";
    std::ostringstream rejected;
    dxfRW rejectingWriter("");
    rejectingWriter.version = DRW::AC1027;
    rejectingWriter.writer = std::make_unique<dxfWriterAscii>(&rejected);
    t.expect(!rejectingWriter.writeField(&modern) && rejected.str().empty(),
             "modern FIELD conflicting root/CadValue formats reject transactionally");

    DRW_Field legacyWithModernPayload = legacy;
    legacyWithModernPayload.m_value.m_formatString = "%lu2";
    std::ostringstream rejectedLegacy;
    dxfRW rejectingLegacyWriter("");
    rejectingLegacyWriter.version = DRW::AC1018;
    rejectingLegacyWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedLegacy);
    t.expect(!rejectingLegacyWriter.writeField(&legacyWithModernPayload)
                 && rejectedLegacy.str().empty(),
             "legacy FIELD refuses unrepresentable nested CadValue strings");
}

void testFieldDxfVersionFloor(TestContext& t) {
    for (const bool binary : {false, true}) {
        for (const DRW::Version version : {DRW::AC1015, DRW::AC1018}) {
            {
                std::ostringstream output;
                dxfRW writer("");
                writer.version = version;
                writer.binFile = binary;
                if (binary)
                    writer.writer = std::make_unique<dxfWriterBinary>(&output);
                else
                    writer.writer = std::make_unique<dxfWriterAscii>(&output);

                DRW_Field field;
                field.handle = 0xB500u;
                field.parentHandle = 0x20u;
                field.m_evaluatorId = "AcObjProp";
                field.m_fieldCode = "VERSION_FLOOR_FIELD";
                const bool wroteField = writer.writeField(&field);
                const bool accepted = version >= DRW::AC1018;
                const bool wroteSentinel = accepted
                    && writer.writer->writeString(0, "WIRE_END");
                bool ended = false;
                const std::vector<std::string> tags = numericWireProjection(
                    output.str(), binary, ended);
                if (!accepted) {
                    t.expect(!wroteField && output.str().empty(),
                             "pre-R18 FIELD is rejected before emitting bytes");
                } else {
                    t.expect(wroteField && wroteSentinel && ended
                                 && std::find(tags.begin(), tags.end(),
                                              "0:FIELD") != tags.end()
                                 && std::find(tags.begin(), tags.end(),
                                              "0:WIRE_END") != tags.end(),
                             "R18 FIELD emits a bounded ASCII/Binary record");
                }
            }

            {
                std::ostringstream output;
                dxfRW writer("");
                writer.version = version;
                writer.binFile = binary;
                if (binary)
                    writer.writer = std::make_unique<dxfWriterBinary>(&output);
                else
                    writer.writer = std::make_unique<dxfWriterAscii>(&output);

                DRW_FieldList fieldList;
                fieldList.handle = 0xB510u;
                fieldList.parentHandle = 0xCu;
                const bool wroteFieldList = writer.writeFieldList(&fieldList);
                const bool accepted = version >= DRW::AC1018;
                const bool wroteSentinel = accepted
                    && writer.writer->writeString(0, "WIRE_END");
                bool ended = false;
                const std::vector<std::string> tags = numericWireProjection(
                    output.str(), binary, ended);
                if (!accepted) {
                    t.expect(!wroteFieldList && output.str().empty(),
                             "pre-R18 FIELDLIST is rejected before emitting bytes");
                } else {
                    t.expect(wroteFieldList && wroteSentinel && ended
                                 && std::find(tags.begin(), tags.end(),
                                              "0:FIELDLIST") != tags.end()
                                 && std::find(tags.begin(), tags.end(),
                                              "100:AcDbIdSet") != tags.end()
                                 && std::find(tags.begin(), tags.end(),
                                              "100:AcDbFieldList") != tags.end()
                                 && std::find(tags.begin(), tags.end(),
                                              "0:WIRE_END") != tags.end(),
                             "R18 FIELDLIST emits a bounded ASCII/Binary record");
                }
            }
        }
    }
}

void testFieldCadValueEmissionPreflight(TestContext& t) {
    struct InvalidPayload {
        const char* name;
        void (*apply)(DRW_Field&);
    };
    const InvalidPayload invalid[] = {
        {"empty flag with populated scalar", [](DRW_Field& field) {
             field.m_value.m_formatFlags = 1;
             field.m_value.m_dataType = 2;
             field.m_value.m_value.addDouble(140, 42.5);
         }},
        {"empty flag with object-ID target", [](DRW_Field& field) {
             field.m_value.m_formatFlags = 1;
             field.m_value.m_dataType = 64;
             field.m_value.m_handle = 0xA721u;
         }},
        {"opaque native string carrier", [](DRW_Field& field) {
             field.m_value.m_formatFlags = 4;
             field.m_value.m_dataType = 4;
             field.m_value.m_dataSize = 2u;
             field.m_value.m_rawData = {0u, 0u};
             field.m_value.m_value.addString(1, "");
         }},
        {"kGeneral without a DXF-defined type mapping", [](DRW_Field& field) {
             DRW_CadValue& value = field.m_value;
             value.m_formatFlags = 0x02;
             value.m_dataType = 512;
             value.m_dataSize = 4;
             value.m_rawData = {0xA1u, 0xB2u, 0xC3u, 0xD4u};
             value.m_value.addBinary(310, value.m_rawData);
             value.m_unitType = 1;
             value.m_formatString = "0.0";
             value.m_valueString = "42.5";
         }},
        {"non-finite scalar", [](DRW_Field& field) {
             field.m_value.m_dataType = 2;
             field.m_value.m_value.addDouble(
                 140, std::numeric_limits<double>::quiet_NaN());
         }},
        {"non-finite 3D coordinate", [](DRW_Field& field) {
             field.m_value.m_dataType = 32;
             field.m_value.m_dataSize = 24;
             field.m_value.m_value.addCoord(10, DRW_Coord(
                 1.0, 2.0, std::numeric_limits<double>::infinity()));
         }},
        {"null child string carrier", [](DRW_Field& field) {
             field.m_childValues.emplace_back();
             DRW_CadValue& value = field.m_childValues.back().m_value;
             value.m_dataType = 4;
             value.m_value.addString(1, "child-value");
             value.m_value.content.s = nullptr;
         }},
        {"non-finite child scalar", [](DRW_Field& field) {
             field.m_childValues.emplace_back();
             DRW_CadValue& value = field.m_childValues.back().m_value;
             value.m_dataType = 2;
             value.m_value.addDouble(
                 140, -std::numeric_limits<double>::infinity());
         }},
    };

    for (const bool binary : {false, true}) {
        for (const InvalidPayload& control : invalid) {
            DRW_Field field;
            field.handle = 0xA600u;
            control.apply(field);
            std::ostringstream output;
            dxfRW writer("");
            writer.version = DRW::AC1027;
            writer.binFile = binary;
            writer.writer = binary
                ? std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterBinary>(&output))
                : std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterAscii>(&output));
            const bool written = writer.writeField(&field);
            const std::string label = std::string("FIELD ") + control.name
                + (binary ? " is rejected before Binary output" : " is rejected before ASCII output");
            t.expect(!written && output.str().empty(), label.c_str());
        }
        for (int flags = 0; flags <= 3; ++flags) {
            DRW_Field field;
            field.handle = 0xA610u;
            field.m_evaluatorId = "ACAD";
            field.m_fieldCode = "CADVALUE_FLAG_TRANSPORT";
            field.m_value.m_formatFlags = flags;
            field.m_value.m_dataType = 2;
            if ((flags & 1) == 0)
                field.m_value.m_value.addDouble(140, 42.5);
            field.m_value.m_unitType = 1;
            field.m_value.m_formatString = "0.0";
            field.m_value.m_valueString = "42.5";
            std::ostringstream output;
            dxfRW writer("");
            writer.version = DRW::AC1021;
            writer.binFile = binary;
            writer.writer = binary
                ? std::unique_ptr<dxfWriter>(std::make_unique<dxfWriterBinary>(&output))
                : std::unique_ptr<dxfWriter>(std::make_unique<dxfWriterAscii>(&output));
            const bool written = writer.writeField(&field)
                && writer.writer->writeString(0, "WIRE_END");
            bool ended = false;
            const auto tags = numericWireProjection(output.str(), binary, ended);
            const auto doubles = std::count(tags.begin(), tags.end(), "140:0x1.54p+5");
            t.expect(written && ended
                         && std::find(tags.begin(), tags.end(), "93:" + std::to_string(flags)) != tags.end()
                         && doubles == ((flags & 1) == 0 ? 1 : 0),
                     "ASCII/Binary FIELD payload uses bit 0; flag 2 retains its scalar");
        }
    }
}

struct TestDxfRecordIdentity {
    std::uint32_t handle {0};
    std::string recordName;
    std::string sectionName;
    std::uint32_t ownerHandle {0};
    bool hasOwnerHandle {false};
};

std::string normalizeTestDxfName(std::string name) {
    for (char& value : name) {
        const unsigned char byte = static_cast<unsigned char>(value);
        if (byte >= 'a' && byte <= 'z')
            value = static_cast<char>(byte - ('a' - 'A'));
    }
    return name;
}

bool readPublishedDxfRecordInventory(
        const std::filesystem::path& path, bool binary,
        std::vector<TestDxfRecordIdentity>& identities) {
    identities.clear();
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    if (binary) {
        std::array<char, 22> header{};
        input.read(header.data(), static_cast<std::streamsize>(header.size()));
        std::string expected = "AutoCAD Binary DXF\r\n";
        expected.push_back(static_cast<char>(0x1A));
        expected.push_back('\0');
        if (input.gcount() != static_cast<std::streamsize>(header.size())
            || std::string(header.data(), header.size()) != expected)
            return false;
    }
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(std::make_unique<dxfReaderBinary>(&input))
        : std::unique_ptr<dxfReader>(std::make_unique<dxfReaderAscii>(&input));
    std::string currentRecord;
    std::string currentSection;
    bool sectionNamePending = false;
    bool recordHandlePending = false;
    bool recordOwnerPending = false;
    std::size_t ownerApplicationDepth = 0;
    std::size_t currentRecordIndex = 0;
    bool sawEof = false;
    int code = 0;
    while (reader->readRec(&code)) {
        if (code == 0) {
            const std::string value = normalizeTestDxfName(reader->getString());
            sectionNamePending = false;
            recordOwnerPending = false;
            ownerApplicationDepth = 0;
            if (value == "SECTION") {
                recordHandlePending = false;
                currentRecord.clear();
                sectionNamePending = true;
            } else if (value == "ENDSEC") {
                recordHandlePending = false;
                currentRecord.clear();
                currentSection.clear();
            } else if (value == "EOF" || value == "ENDTAB") {
                recordHandlePending = false;
                currentRecord.clear();
                sawEof = sawEof || value == "EOF";
            } else {
                currentRecord = value;
                recordHandlePending = true;
            }
        } else if (sectionNamePending && code == 2) {
            currentSection = normalizeTestDxfName(reader->getString());
            sectionNamePending = false;
        } else if (recordHandlePending && (code == 5 || code == 105)) {
            const std::uint32_t handle = reader->getHandleString();
            if (handle == 0 || currentRecord.empty())
                return false;
            identities.push_back(
                {handle, currentRecord, currentSection});
            currentRecordIndex = identities.size() - 1u;
            recordHandlePending = false;
            recordOwnerPending = true;
        } else if (recordOwnerPending && code == 102) {
            const auto value = reader->getString();
            if (!value.empty() && value.front() == '{')
                ++ownerApplicationDepth;
            else if (value == "}" && ownerApplicationDepth != 0)
                --ownerApplicationDepth;
        } else if (recordOwnerPending && code == 100 && ownerApplicationDepth == 0) {
            recordOwnerPending = false;
        } else if (recordOwnerPending && code == 330 && ownerApplicationDepth == 0) {
            if (currentRecordIndex >= identities.size())
                return false;
            identities[currentRecordIndex].ownerHandle =
                reader->getHandleString();
            identities[currentRecordIndex].hasOwnerHandle = true;
            recordOwnerPending = false;
        }
    }
    if (!sawEof)
        return false;
    std::sort(identities.begin(), identities.end(),
        [](const TestDxfRecordIdentity& left,
           const TestDxfRecordIdentity& right) {
            if (left.handle != right.handle)
                return left.handle < right.handle;
            if (left.recordName != right.recordName)
                return left.recordName < right.recordName;
            return left.sectionName < right.sectionName;
        });
    for (std::size_t index = 1; index < identities.size(); ++index) {
        if (identities[index - 1u].handle == identities[index].handle)
            return false;
    }
    return true;
}

bool sameTestDxfRecordInventory(
        const std::vector<TestDxfRecordIdentity>& left,
        const std::vector<TestDxfRecordIdentity>& right) {
    if (left.size() != right.size())
        return false;
    for (std::size_t index = 0; index < left.size(); ++index) {
        if (left[index].handle != right[index].handle
            || left[index].recordName != right[index].recordName
            || left[index].sectionName != right[index].sectionName
            || left[index].hasOwnerHandle != right[index].hasOwnerHandle
            || left[index].ownerHandle != right[index].ownerHandle)
            return false;
    }
    return true;
}

void testStandardObjectWireComparison(TestContext& t) {
    using namespace dx_standard_output;
    DRW_Dictionary dictionary;
    dictionary.handle = 0xA500u; dictionary.parentHandle = 0xCu;
    dictionary.hardOwner = 0; dictionary.cloning = 0;
    dictionary.m_entries = {{"FIRST", 0xA510u, DRW_Dictionary::Entry::DxfHandleCode::SoftOwner},
        {"SECOND", 0xA511u, DRW_Dictionary::Entry::DxfHandleCode::HardOwner}};
    DRW_Group group;
    group.handle = 0xA501u; group.parentHandle = 0xA500u;
    group.m_description = "caf\xC3\xA9 group";
    group.m_entityHandles = {0xA510u, 0xA511u};
    DRW_XRecord xrecord;
    xrecord.handle = 0xA502u; xrecord.parentHandle = 0xA500u;
    xrecord.m_dataEntries = {DRW_Variant(1, "seed"), DRW_Variant(90, 77),
        DRW_Variant(330, std::uint64_t(0xA510u)), DRW_Variant(320, std::uint64_t(0xBEEFu)),
        DRW_Variant(310, std::vector<std::uint8_t>{0x13u, 0xA5u}),
        DRW_Variant(10, DRW_Coord(1.25, 2.5, 3.75))};
    for (DRW_TableEntry* source : {static_cast<DRW_TableEntry*>(&dictionary),
            static_cast<DRW_TableEntry*>(&group), static_cast<DRW_TableEntry*>(&xrecord)}) {
        source->reactorHandles = {0xA520u}; source->xDictHandle = 0xA521u;
        source->setDwgCommonObjectState(1, 0, false);
        source->setDwgCommonLinkTailValidated(true);
        source->appData = {{DRW_Variant(102, "APP_META"), DRW_Variant(1070, 7), DRW_Variant(102, "}")}};
        (void)source->addExtData(std::make_unique<DRW_Variant>(1001, "APP"));
        (void)source->addExtData(std::make_unique<DRW_Variant>(1000, "caf\xC3\xA9 value"));
        (void)source->addExtData(std::make_unique<DRW_Variant>(1005, "A510"));
        (void)source->addExtData(std::make_unique<DRW_Variant>(1004, std::vector<std::uint8_t>{0x13u, 0xA5u}));
        (void)source->addExtData(std::make_unique<DRW_Variant>(1011, DRW_Coord(7.0, 8.0, 9.0)));
    }
    auto mapper = [](std::uint32_t source, std::uint32_t& output) {
        output = source == 0u ? 0u : source + 0x10000u; return true;
    };
    const auto mapped = [](std::uint32_t source) { return source == 0u ? 0u : source + 0x10000u; };
    const auto hex = [](std::uint32_t value) { std::ostringstream result; result << std::hex << value; return result.str(); };
    unsigned cases = 0;
    for (const auto version : {DRW::AC1018, DRW::AC1032}) for (const bool binary : {false, true}) {
      for (const auto kind : {Family::Dictionary, Family::Group, Family::Xrecord}) {
       for (unsigned fault = 0; fault < 16u; ++fault) {
        const DRW_TableEntry& source = kind == Family::Dictionary ? static_cast<const DRW_TableEntry&>(dictionary)
            : kind == Family::Group ? static_cast<const DRW_TableEntry&>(group) : static_cast<const DRW_TableEntry&>(xrecord);
        std::ostringstream stream(std::ios::out | std::ios::binary);
        std::unique_ptr<dxfWriter> writer;
        if (binary) writer = std::make_unique<dxfWriterBinary>(&stream);
        else writer = std::make_unique<dxfWriterAscii>(&stream);
        writer->setVersion(version == DRW::AC1018 ? "AC1018" : "AC1032", true);
        writer->setCodePage("ANSI_1252");
        writer->writeString(0, kind == Family::Dictionary ? "DICTIONARY" : kind == Family::Group ? "GROUP" : "XRECORD");
        writer->writeString(5, hex(mapped(source.handle) + (fault == 2u ? 1u : 0u)));
        writer->writeString(102, "{ACAD_REACTORS");
        writer->writeString(330, hex(mapped(source.reactorHandles.front()) + (fault == 11u ? 1u : 0u)));
        if (fault == 12u) writer->writeInt16(1070, 9);
        writer->writeString(102, "}");
        writer->writeString(102, "{ACAD_XDICTIONARY"); writer->writeString(360, hex(mapped(source.xDictHandle))); writer->writeString(102, "}");
        writer->writeString(102, "{APP_META"); writer->writeInt16(1070, fault == 13u ? 8 : 7); writer->writeString(102, "}");
        writer->writeString(330, hex(mapped(source.parentHandle) + (fault == 1u ? 1u : 0u)));
        writer->writeString(100, fault == 4u ? "WrongSubclass" : kind == Family::Dictionary ? "AcDbDictionary" : kind == Family::Group ? "AcDbGroup" : "AcDbXrecord");
        if (fault == 3u) writer->writeString(6, "unmodeled-extra");
        if (kind == Family::Dictionary) {
            if (fault != 5u) writer->writeInt16(280, 0);
            writer->writeInt16(281, 0);
            if (fault == 6u) writer->writeInt16(281, 0);
            for (unsigned index = 0; index < (fault == 9u ? 1u : 2u); ++index) {
                const auto& entry = dictionary.m_entries[fault == 10u ? 1u - index : index];
                writer->writeUtf8String(3, fault == 7u && index == 0u ? "RENAMED" : entry.m_name);
                writer->writeString(static_cast<int>(entry.m_dxfHandleCode), hex(mapped(entry.m_handle) + (fault == 8u && index == 0u ? 1u : 0u)));
            }
        } else if (kind == Family::Group) {
            writer->writeUtf8String(300, fault == 7u ? "changed" : group.m_description);
            writer->writeInt16(70, 0);
            if (fault != 5u) writer->writeInt16(71, 1);
            if (fault == 6u) writer->writeInt16(71, 1);
            for (unsigned index = 0; index < (fault == 9u ? 1u : 2u); ++index)
                writer->writeString(340, hex(mapped(group.m_entityHandles[fault == 10u ? 1u - index : index]) + (fault == 8u && index == 0u ? 1u : 0u)));
        } else {
            if (fault != 5u) writer->writeInt16(280, 0);
            if (fault == 6u) writer->writeInt16(280, 0);
            if (fault == 10u) writer->writeInt32(90, 77);
            writer->writeUtf8String(1, fault == 7u ? "changed" : "seed");
            if (fault != 10u) writer->writeInt32(90, 77);
            writer->writeString(330, hex(mapped(0xA510u) + (fault == 8u ? 1u : 0u)));
            writer->writeString(320, "BEEF");
            writer->writeString(310, fault == 14u ? "13A6" : "13A5");
            writer->writeDouble(10, 1.25); writer->writeDouble(20, 2.5);
            if (fault != 9u) writer->writeDouble(30, 3.75);
        }
        writer->writeUtf8String(1001, "APP");
        writer->writeUtf8String(1000, fault == 14u ? "changed" : "caf\xC3\xA9 value");
        writer->writeString(1005, hex(mapped(0xA510u)));
        writer->writeString(1004, "13A5");
        writer->writeDouble(1011, 7.0); writer->writeDouble(1021, 8.0); writer->writeDouble(1031, 9.0);
        auto bytes = stream.str();
        if (fault == 15u) bytes.resize(bytes.size() / 2u);
        const std::string page = writer->getCodePage();
        const dxfRW::DxfObjectsWriteView view{bytes, version, binary, page, dxfRW::DxfCompatibilityProfile::StandaloneSafe};
        Decoder decoder(view);
        bool matched = false;
        const bool scanned = inspect(view, [&](const Shape& shape, std::string_view body) {
            if (!shape.complete()) return true;
            std::vector<std::uint32_t> references;
            const auto compare = [&](const auto& actual) {
                using Record = std::decay_t<decltype(actual)>;
                const auto& original = static_cast<const Record&>(source);
                matched = equal(original, actual, mapper, references); return true;
            };
            if (kind == Family::Dictionary) return decoder.decodeRecord<DRW_Dictionary>(body, compare);
            if (kind == Family::Group) return decoder.decodeRecord<DRW_Group>(body, compare);
            return decoder.decodeRecord<DRW_XRecord>(body, compare);
        });
        if ((scanned && matched) != (fault == 0u)) {
            std::cerr << "Wire comparison kind=" << static_cast<int>(kind) << " fault=" << fault
                      << " binary=" << binary << " version=" << static_cast<int>(version) << '\n';
            t.expect(false, "Actual-byte comparison rejects changed/missing/defaulted/unmodeled fields and edges");
        }
        ++cases;
       }
      }
    }
    {
        std::vector<std::uint32_t> references;
        DRW_Variant plain(90, 7), color(plain);
        color.setDwgCmColor({});
        DRW_Variant layer(1003, "LAYER"), decodedLayer(1003, "LAYER");
        layer.setDwgRawLayerReference(0xA510u, DRW::AC1021);
        t.expect(!valueEqual(color, plain, mapper, references)
                     && !valueEqual(layer, decodedLayer, mapper, references),
                 "Unmodeled CMC and source layer-ID metadata cannot become full preservation from matching scalar/text");
        const DRW_Variant binary(1004, std::vector<std::uint8_t>{0x13u, 0xA5u});
        for (const auto text : {"13A5", "13a5", "13A6", "13A", "13AZ", "13A500", ""}) {
            const DRW_Variant physical(1004, text);
            t.expect(valueEqual(binary, physical, mapper, references)
                         == (std::string_view(text) == "13A5" || std::string_view(text) == "13a5"),
                     "EED binary comparison permits only exact byte-equivalent complete hex chunks");
        }
    }
    std::cout << "Standard OBJECTS generated wire-comparison cells=" << cases << '\n';
}

void testDxfObjectsWriteInspection(TestContext& t) {
    class Interface final : public dx_iface {
    public:
        Interface() { cData = &data; currentBlock = data.mBlock; }
        void writeHeader(DRW_Header& header) override {
            header.vars.clear();
            header.vars["$DWGCODEPAGE"] = new DRW_Variant(3, std::string("ANSI_1252"));
        }
        void writeObjects() override {
            DRW_XRecord record;
            record.handle = 0xA770u; record.parentHandle = 0xCu;
            record.m_values.emplace_back(1, "OBJECTS_INSPECTION_SENTINEL");
            (void)dxfW->writeXRecord(&record);
        }
        bool finalizeDxfWrite() override {
            if (!dx_iface::finalizeDxfWrite() || rejectFinalization) return false;
            if (writeAtFinalization) {
                DRW_Line line;
                (void)dxfW->writeLine(&line);
            }
            return true;
        }
        dx_data data;
        bool rejectFinalization{false}, writeAtFinalization{false};
    } iface;
    const auto path = std::filesystem::temp_directory_path() / ("libdxfrw-objects-inspection-"
        + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code ignored; std::filesystem::remove(path, ignored); }
    } cleanup{path};
    dxfRW codec(path.string().c_str()); iface.dxfW = &codec;
    codec.setDxfWriteRecordInventoryEnabled(true);
    for (const auto version : {DRW::AC1014, DRW::AC1015, DRW::AC1018,
            DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
      for (const bool binary : {false, true}) {
        unsigned calls = 0;
        std::string staged;
        t.expect(codec.setDxfObjectsWriteInspector([&](const auto& view) {
            ++calls;
            staged.assign(view.bytes);
            unsigned dictionaries = 0, xrecords = 0;
            const bool scanned = dx_standard_output::inspect(view, [&](const auto& shape, std::string_view) {
                dictionaries += shape.kind == dx_standard_output::Family::Dictionary;
                xrecords += shape.kind == dx_standard_output::Family::Xrecord;
                return true;
            });
            return scanned && view.version == version && view.binary == binary && !view.codePage.empty()
                && view.compatibilityProfile == dxfRW::DxfCompatibilityProfile::StandaloneSafe
                && dictionaries == 2u && xrecords == 1u;
        }), "OBJECTS inspection installs before write");
        const bool wrote = codec.write(&iface, version, binary);
        std::ifstream input(path, std::ios::binary);
        const std::string published((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        input.close();
        t.expect(wrote && calls == 1u && !staged.empty() && published.find(staged) != std::string::npos,
                 "Inspector sees the exact published OBJECTS bytes and actual version/encoding profile");
      }
    }
    for (const bool binary : {false, true}) {
      for (unsigned fault = 0; fault < 6u; ++fault) {
        const std::string sentinel = "OBJECTS_INSPECTOR_DESTINATION_UNCHANGED";
        { std::ofstream previous(path, std::ios::binary); previous << sentinel; }
        unsigned calls = 0;
        iface.rejectFinalization = fault == 4u;
        iface.writeAtFinalization = fault == 5u;
        codec.setDxfObjectsWriteInspector([&](const auto&) {
            ++calls;
            if (fault == 0u) return false;
            if (fault == 1u) throw std::runtime_error("inspection failure control");
            if (fault == 2u) (void)codec.setDxfObjectsWriteInspector({});
            if (fault == 3u) { DRW_Line line; (void)codec.writeLine(&line); }
            return true;
        });
        const bool wrote = codec.write(&iface, DRW::AC1032, binary);
        std::ifstream input(path, std::ios::binary);
        const std::string retained((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        input.close();
        t.expect(!wrote && calls == 1u && retained == sentinel
                     && codec.getWritingContext()->sourceHandleToWrittenObjectMap.empty(),
                 "Inspection veto/throw/mutation or later finalization failure retracts bytes and identities");
        iface.rejectFinalization = iface.writeAtFinalization = false;
        calls = 0;
        t.expect(codec.setDxfObjectsWriteInspector([&](const auto&) { ++calls; return true; })
                     && codec.write(&iface, DRW::AC1032, binary) && calls == 1u,
                 "Same-codec retry recovers after failed inspection/publication phase");
      }
    }
    unsigned r12Calls = 0;
    dxfRW r12(path.string().c_str());
    ProfileProbeInterface r12Iface;
    r12Iface.dxfW = &r12;
    r12.setDxfObjectsWriteInspector([&](const auto&) { ++r12Calls; return true; });
    t.expect(r12.write(&r12Iface, DRW::AC1009, false) && r12Calls == 0u,
             "Targets without OBJECTS do not fabricate an inspection callback");
    codec.setDxfObjectsWriteInspector({});
    t.expect(codec.write(&iface, DRW::AC1032, false), "Empty inspector preserves ordinary authored writing");
    std::cout << "OBJECTS inspector profile cells=14 atomic refusal/retry cells=12\n";
}

void testDxfStandardObjectIdentities(TestContext& t) {
    HandleAllocator planned;
    planned.reserve(0x40u);
    planned.reserveForWrite(0xA512u);
    planned.reserveForWrite(0xA512u);
    t.expect(planned.next() == 0x41u && planned.current() == 0xA513u
                 && !planned.isExplicitlyReserved(0xA512u),
             "per-write destination reservation is idempotent and disjoint from allocation");
    planned.resetGenerated();
    t.expect(planned.next() == 0x41u && planned.isExplicitlyReserved(0x40u),
             "per-write destinations expire without dropping caller-owned reservations");
    HandleAllocator exhausted;
    bool overflow = false;
    try { exhausted.reserveForWrite((std::numeric_limits<std::uint32_t>::max)()); }
    catch (const std::overflow_error&) { overflow = true; }
    t.expect(overflow && exhausted.next() == 0x30u,
             "unrepresentable destination reservation fails before changing allocator state");
    enum class Fault { None, LateFailure, DuplicateDictionary, DuplicateXRecord,
        DuplicateGroup, EntityCollision, PlannedEntity, PlannedTable,
        MissingMember, DictionaryAppend, XRecordAppend, Throw, FinalizeReject,
        GroupEntityCollision, GroupPlannedEntity, GroupPlannedTable,
        GroupRedirect, LateGroupChange, SharedGroupDestination, FixedGroupDestination,
        LateNamedDictionaryChange };
    class Interface final : public dx_iface {
    public:
        Interface() {
            cData = &storage;
            currentBlock = storage.mBlock;
            dictionary.handle = 0xA500u;
            dictionary.parentHandle = 0xCu;
            dictionary.m_entries = {{"VALUE", 0xA502u}};
            xrecord.handle = 0xA502u;
            xrecord.parentHandle = 0xA500u;
            xrecord.m_values.emplace_back(1, "source-object-identity");
        }
        void writeEntities() override {
            plannedObjectsUnpublished = dxfW->getWritingContext() != nullptr
                && dxfW->getWritingContext()->sourceHandleToWrittenObjectMap.empty()
                && !dxfW->dxfWriteSourceObjectHasIdentity(
                    0xA503u, dxfW->remapObjectHandle(0xA503u), "GROUP", "OBJECTS");
            if (fault == Fault::GroupPlannedEntity)
                (void)dxfW->preallocateEntityHandle(0xA503u);
            if (fault == Fault::GroupPlannedTable)
                (void)dxfW->preallocateTableRecordHandle(0xA503u);
            if (fault == Fault::GroupRedirect)
                dxfW->setHandleRemap({{0xA500u, 0xA510u}, {0xA501u, 0xA511u},
                                     {0xA502u, 0xA512u}, {0xA503u, 0xB503u}});
            if (fault == Fault::LateGroupChange) {
                DRW_Group replacement;
                replacement.name = "LATE_REPLACEMENT";
                dxfW->setGroups({replacement});
            }
            if (fault == Fault::LateNamedDictionaryChange) {
                DRW_Dictionary replacement;
                replacement.handle = dxfW->remapObjectHandle(0xA503u);
                dxfW->setNamedDictObjects({replacement});
            }
            if (fault == Fault::PlannedEntity)
                (void)dxfW->preallocateEntityHandle(dictionary.handle);
            if (fault == Fault::PlannedTable)
                (void)dxfW->preallocateTableRecordHandle(xrecord.handle);
            DRW_Line line;
            line.handle = fault == Fault::GroupEntityCollision ? 0xA503u : 0xA100u;
            line.secPoint = DRW_Coord(1, 2, 3);
            if (referenceGroups)
                line.reactorHandles = {0xA503u};
            if (secondaryGroup)
                line.reactorHandles.push_back(0xA504u);
            (void)dxfW->writeLine(&line);
        }
        void writeObjects() override {
            callbacksEntered = true;
            auto* context = dxfW->getWritingContext();
            // Generated named dictionaries and GROUP records have already
            // serialized into the enclosing, still-uncommitted OBJECTS scope.
            stagedObjectsObserved = context != nullptr
                && context->sourceHandleToWrittenObjectMap.count(0xA501u) == 1u
                && (anonymousGroup
                    || context->sourceHandleToWrittenObjectMap.count(0xA503u) == 1u);
            closedBeforeFinalize = !dxfW->dxfWriteSourceObjectHasIdentity(
                0xA501u, 0xA511u, "DICTIONARY", "OBJECTS");
            const auto appendFails = [this](bool dict) {
                RejectingStreambuf rejected;
                std::ostream sink(&rejected);
                std::ostream* parent = dxfW->writer->stream();
                dxfW->writer->setStream(&sink);
                const bool wrote = dict ? dxfW->writeDictionary(&dictionary)
                                        : dxfW->writeXRecord(&xrecord);
                dxfW->writer->setStream(parent);
                const auto* result = dxfW->getWritingContext();
                innerAppendRolledBack = !wrote && result != nullptr
                    && result->sourceHandleToWrittenObjectMap.count(
                        dict ? dictionary.handle : xrecord.handle) == 0u
                    && result->sourceHandleToWrittenObjectMap.count(0xA501u) == 1u;
            };
            if (fault == Fault::DictionaryAppend) {
                appendFails(true);
                return;
            }
            if (!dxfW->writeDictionary(&dictionary))
                return;
            if (fault == Fault::DuplicateDictionary) {
                (void)dxfW->writeDictionary(&dictionary);
                return;
            }
            if (fault == Fault::XRecordAppend) {
                appendFails(false);
                return;
            }
            if (!dxfW->writeXRecord(&xrecord))
                return;
            if (fault == Fault::DuplicateXRecord) {
                (void)dxfW->writeXRecord(&xrecord);
                return;
            }
            if (fault == Fault::Throw)
                throw 17;
            if (fault == Fault::LateFailure) {
                DRW_XRecord invalid;
                invalid.handle = 0xA599u;
                invalid.m_cloning = 6;
                (void)dxfW->writeXRecord(&invalid);
            }
        }
        bool finalizeDxfWrite() override {
            const auto* context = dxfW->getWritingContext();
            if (context == nullptr || dxfW->writer == nullptr)
                return false;
            inventory.clear();
            for (const auto& row : dxfW->writer->emittedRecords()) {
                const auto* record = dxfW->writer->recordInventoryName(row.recordName);
                const auto* section = dxfW->writer->recordInventoryName(row.sectionName);
                if (record == nullptr || section == nullptr)
                    return false;
                inventory.push_back({row.handle, *record, *section,
                                     row.ownerHandle, row.hasOwnerHandle});
            }
            const auto primary = context->sourceHandleToWrittenObjectMap.find(0xA503u);
            const auto group = std::find_if(inventory.begin(), inventory.end(),
                [this, context, primary](const auto& row) {
                    return row.recordName == "GROUP" && (anonymousGroup
                        || (primary != context->sourceHandleToWrittenObjectMap.end()
                            && row.handle == primary->second));
                });
            if (group == inventory.end())
                return false;
            const auto valid = [this](std::uint32_t source, std::uint32_t emitted,
                                      const char* type, std::uint32_t owner) {
                return dxfW->dxfWriteSourceObjectHasIdentity(source, emitted, type, "OBJECTS")
                    && (targetVersion <= DRW::AC1014
                        || dxfW->dxfWriteSourceObjectHasIdentity(source, emitted, type, "OBJECTS", owner))
                    && !dxfW->dxfWriteSourceObjectHasIdentity(source, emitted, type, "ENTITIES")
                    && !dxfW->dxfWriteSourceObjectHasIdentity(source, emitted + 1u, type, "OBJECTS")
                    && !dxfW->dxfWriteSourceObjectHasIdentity(source, emitted, "LINE", "OBJECTS")
                    && (targetVersion <= DRW::AC1014
                        || !dxfW->dxfWriteSourceObjectHasIdentity(source, emitted, type, "OBJECTS", 0xFFFFu));
            };
            identitiesObserved = valid(0xA500u, 0xA510u, "DICTIONARY", 0xCu)
                && valid(0xA501u, 0xA511u, "DICTIONARY", 0xCu)
                && valid(0xA502u, expectedXRecordHandle, "XRECORD", 0xA510u)
                && (anonymousGroup
                    ? !dxfW->dxfWriteSourceObjectHasIdentity(0u, group->handle, "GROUP", "OBJECTS")
                    : valid(0xA503u, group->handle, "GROUP", 0xDu))
                && !dxfW->dxfWriteSourceObjectHasIdentity(0xA5FFu, 0xA510u, "DICTIONARY", "OBJECTS")
                && !dxfW->dxfWriteSourceObjectHasIdentity(0xA100u, group->handle, "GROUP", "OBJECTS")
                && (nodSource == 0u
                    ? !dxfW->dxfWriteSourceObjectHasIdentity(0xCu, 0xCu, "DICTIONARY", "OBJECTS")
                    : valid(nodSource, 0xCu, "DICTIONARY", 0u))
                && (groupDictionarySource == 0u
                    || valid(groupDictionarySource, 0xDu, "DICTIONARY", 0xCu))
                && (!secondaryGroup
                    || (context->sourceHandleToWrittenObjectMap.count(0xA504u) == 1u
                        && valid(0xA504u, context->sourceHandleToWrittenObjectMap.at(0xA504u),
                                 "GROUP", 0xDu)))
                && context->sourceHandleToWrittenObjectMap.size()
                    == (anonymousGroup ? 3u : 4u) + (nodSource != 0u ? 1u : 0u)
                        + (groupDictionarySource != 0u ? 1u : 0u) + (secondaryGroup ? 1u : 0u);
            return plannedObjectsUnpublished && stagedObjectsObserved && closedBeforeFinalize && identitiesObserved
                && fault != Fault::FinalizeReject;
        }
        dx_data storage;
        DRW_Dictionary dictionary;
        DRW_XRecord xrecord;
        Fault fault{Fault::None};
        DRW::Version targetVersion{DRW::AC1032};
        bool anonymousGroup{false};
        bool referenceGroups{false};
        bool secondaryGroup{false};
        std::uint32_t nodSource{0u};
        std::uint32_t groupDictionarySource{0u};
        std::uint32_t expectedXRecordHandle{0xA512u};
        bool callbacksEntered{false};
        bool plannedObjectsUnpublished{false};
        bool stagedObjectsObserved{false};
        bool closedBeforeFinalize{false};
        bool innerAppendRolledBack{false};
        bool identitiesObserved{false};
        std::vector<TestDxfRecordIdentity> inventory;
    };
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-standard-object-identities-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    if (std::filesystem::exists(path)) {
        t.expect(false, "source-object identity scratch path unexpectedly exists");
        return;
    }
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code ignored; std::filesystem::remove(path, ignored); }
    } cleanup{path};
    for (const bool binary : {false, true}) {
      for (const auto version : {DRW::AC1014, DRW::AC1015, DRW::AC1018,
              DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
       for (const auto fault : {Fault::None, Fault::LateFailure, Fault::DuplicateDictionary,
               Fault::DuplicateXRecord, Fault::DuplicateGroup, Fault::EntityCollision,
               Fault::PlannedEntity, Fault::PlannedTable, Fault::MissingMember,
               Fault::DictionaryAppend, Fault::XRecordAppend, Fault::Throw, Fault::FinalizeReject,
               Fault::GroupEntityCollision, Fault::GroupPlannedEntity, Fault::GroupPlannedTable,
               Fault::GroupRedirect, Fault::LateGroupChange, Fault::SharedGroupDestination,
               Fault::FixedGroupDestination, Fault::LateNamedDictionaryChange}) {
        if (version != DRW::AC1021 && fault != Fault::None)
            continue; // run the full fault/retry matrix once per encoding
        Interface source;
        source.fault = fault;
        source.targetVersion = version;
        if (fault == Fault::EntityCollision) source.dictionary.handle = 0xA100u;
        dxfRW codec(path.string().c_str());
        source.dxfW = &codec;
        codec.setDxfWriteRecordInventoryEnabled(true, 4096u);
        codec.setHandleRemap({{0xA500u, 0xA510u}, {0xA501u, 0xA511u}, {0xA502u, 0xA512u}});
        if (fault == Fault::SharedGroupDestination || fault == Fault::FixedGroupDestination)
            codec.setHandleRemap({{0xA500u, 0xA510u}, {0xA501u, 0xA511u},
                                 {0xA502u, 0xA512u},
                                 {0xA503u, fault == Fault::FixedGroupDestination ? 0xCu : 0xA512u}});
        DRW_Dictionary named;
        named.handle = 0xA501u;
        named.parentHandle = 0xCu;
        codec.setNamedDictObjects({named});
        codec.setRootDictEntries({{"DIRECT", "A510"}, {"NAMED", "A511"}});
        DRW_Group group;
        group.handle = 0xA503u;
        group.name = "IDENTITY_GROUP";
        group.m_entityHandles = {0xA100u};
        if (fault == Fault::MissingMember) group.m_entityHandles.push_back(0xA199u);
        auto groups = std::vector<DRW_Group>{group};
        if (fault == Fault::DuplicateGroup) {
            group.name = "SECOND_IDENTITY_GROUP";
            groups.push_back(group);
        }
        codec.setGroups(groups);
        const std::string sentinel = "STANDARD_OBJECT_IDENTITY_SENTINEL\n";
        { std::ofstream prior(path, std::ios::binary); prior << sentinel; }
        const bool written = codec.write(&source, version, binary);
        if (fault != Fault::None) {
            std::ifstream prior(path, std::ios::binary);
            const std::string bytes(std::istreambuf_iterator<char>(prior), {});
            const auto* context = codec.getWritingContext();
            t.expect(!written && bytes == sentinel && context != nullptr
                         && context->sourceHandleToWrittenObjectMap.empty()
                         && codec.m_plannedDxfGroupSourceHandles.empty()
                         && codec.m_plannedDxfGroupHandles.empty()
                         && !codec.m_dxfGroupPlanFrozen,
                     "source-object failure rolls back OBJECTS bindings and preserves destination");
            if (fault == Fault::DictionaryAppend || fault == Fault::XRecordAppend)
                t.expect(source.innerAppendRolledBack,
                         "inner append failure retracts its object binding before outer rollback");
            source.fault = Fault::None;
            source.dictionary.handle = 0xA500u;
            codec.setHandleRemap({{0xA500u, 0xA510u}, {0xA501u, 0xA511u},
                                  {0xA502u, 0xA512u}});
            groups.resize(1u);
            groups.front().m_entityHandles = {0xA100u};
            codec.setGroups(groups);
            source.callbacksEntered = source.identitiesObserved = false;
            source.stagedObjectsObserved = source.closedBeforeFinalize = false;
        }
        const bool retried = fault == Fault::None ? written : codec.write(&source, version, binary);
        std::vector<TestDxfRecordIdentity> actual;
        const bool readBack = retried && readPublishedDxfRecordInventory(path, binary, actual);
        const bool sameInventory = readBack && sameTestDxfRecordInventory(source.inventory, actual);
        if (!retried || !source.identitiesObserved || !sameInventory) {
            const auto diagnostic = codec.getLastDiagnostic();
            std::cerr << "standard identity version=" << static_cast<int>(version)
                      << " binary=" << binary << " fault=" << static_cast<int>(fault)
                      << " retry=" << retried << " staged=" << source.stagedObjectsObserved
                      << " identities=" << source.identitiesObserved << " readBack=" << readBack
                      << " inventory=" << sameInventory << " diagnostic=" << diagnostic.code << '\n';
        }
        t.expect(retried && source.callbacksEntered && source.identitiesObserved
                     && sameInventory
                     && !codec.dxfWriteSourceObjectHasIdentity(0xA500u, 0xA510u, "DICTIONARY", "OBJECTS"),
                 "source-object identities match actual ASCII/Binary records and close after finalization");
       }
      }
      // Legacy callers may author a GROUP without a source handle. It still
      // serializes, but no source identity is invented for that record.
      Interface source;
      source.anonymousGroup = true;
      source.targetVersion = DRW::AC1032;
      dxfRW codec(path.string().c_str()); source.dxfW = &codec;
      codec.setDxfWriteRecordInventoryEnabled(true);
      codec.setHandleRemap({{0xA500u, 0xA510u}, {0xA501u, 0xA511u}, {0xA502u, 0xA512u}});
      DRW_Dictionary named; named.handle = 0xA501u; named.parentHandle = 0xCu;
      codec.setNamedDictObjects({named});
      DRW_Group group; group.name = "AUTHORED_GROUP"; group.m_entityHandles = {0xA100u};
      codec.setGroups({group});
      t.expect(codec.write(&source, DRW::AC1032, binary) && source.identitiesObserved,
               "authored zero-source GROUP preserves its existing route without an invented binding");
      for (int aliasMode = 0; aliasMode < 3; ++aliasMode) {
        Interface aliased;
        aliased.nodSource = aliasMode == 1 ? 0xCu : 0xA5F0u;
        aliased.groupDictionarySource = aliasMode == 1 ? 0xDu
            : aliasMode == 2 ? 0u : 0xA5F1u;
        dxfRW aliasedCodec(path.string().c_str()); aliased.dxfW = &aliasedCodec;
        aliasedCodec.setDxfWriteRecordInventoryEnabled(true);
        t.expect(!aliasedCodec.setGeneratedDictionarySourceHandles(0u, 0xA5F1u)
                     && !aliasedCodec.setGeneratedDictionarySourceHandles(0xA5F0u, 0xA5F0u)
                     && aliasedCodec.setGeneratedDictionarySourceHandles(
                         aliased.nodSource, aliased.groupDictionarySource),
                 "generated dictionary source declarations reject shared/unrooted identities");
        aliasedCodec.setNamedDictObjects({named});
        group.handle = 0xA503u;
        aliasedCodec.setGroups({group});
        std::map<std::uint32_t, std::uint32_t> remap{{0xA500u, 0xA510u},
            {0xA501u, 0xA511u}, {0xA502u, 0xA512u}, {0xA5F0u, 0xCu}, {0xA5F1u, 0xDu}};
        if (aliasMode == 0) {
            auto wrong = remap; wrong[0xA5F0u] = 0xA5F2u;
            aliasedCodec.setHandleRemap(std::move(wrong));
            const std::string sentinel = "GENERATED_DICTIONARY_ALIAS_SENTINEL";
            { std::ofstream prior(path, std::ios::binary); prior << sentinel; }
            const bool refused = !aliasedCodec.write(&aliased, DRW::AC1032, binary);
            std::ifstream prior(path, std::ios::binary);
            const std::string retained(std::istreambuf_iterator<char>(prior), {});
            t.expect(refused && retained == sentinel
                         && aliasedCodec.getWritingContext()->sourceHandleToWrittenObjectMap.empty(),
                     "a declaration with the wrong final root mapping refuses atomically");
        }
        aliasedCodec.setHandleRemap(remap);
        const bool wrote = aliasedCodec.write(&aliased, DRW::AC1032, binary);
        std::vector<TestDxfRecordIdentity> actual;
        t.expect(wrote && aliased.identitiesObserved
                     && readPublishedDxfRecordInventory(path, binary, actual)
                     && sameTestDxfRecordInventory(aliased.inventory, actual),
                 "generic/canonical/root-only aliases bind only actual C/D records with fresh retry");
        if (aliasMode == 0 && wrote) {
            const auto emitted = aliasedCodec.getWritingContext()->sourceHandleToWrittenObjectMap.find(0xA503u);
            t.expect(emitted != aliasedCodec.getWritingContext()->sourceHandleToWrittenObjectMap.end()
                         && emitted->second < 0xA5F2u,
                     "a failed temporary remap destination does not leak into the next write allocator");
        }
      }
      Interface high;
      high.expectedXRecordHandle = (std::numeric_limits<std::uint32_t>::max)() - 1u;
      dxfRW highCodec(path.string().c_str()); high.dxfW = &highCodec;
      highCodec.setDxfWriteRecordInventoryEnabled(true);
      highCodec.setHandleRemap({{0xA500u, 0xA510u}, {0xA501u, 0xA511u},
                               {0xA502u, high.expectedXRecordHandle}});
      highCodec.setNamedDictObjects({named});
      highCodec.setGroups({group});
      std::vector<TestDxfRecordIdentity> highActual;
      t.expect(highCodec.write(&high, DRW::AC1032, binary) && high.identitiesObserved
                   && highCodec.highWaterHandle() == (std::numeric_limits<std::uint32_t>::max)()
                   && readPublishedDxfRecordInventory(path, binary, highActual)
                   && sameTestDxfRecordInventory(high.inventory, highActual),
               "high remap destination preserves low allocation space and exact final identities/HANDSEED");
      for (const auto referenceVersion : {DRW::AC1014, DRW::AC1015, DRW::AC1018,
              DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
       for (const bool explicitGroupTargets : {false, true}) {
      Interface referenced;
      referenced.targetVersion = referenceVersion;
      referenced.referenceGroups = true;
      referenced.secondaryGroup = true;
      referenced.dictionary.m_entries.push_back({"BACKWARD_GROUP", 0xA503u});
      referenced.dictionary.m_entries.push_back({"BACKWARD_GROUP_2", 0xA504u});
      referenced.xrecord.m_values.emplace_back(340, std::string("A503"));
      referenced.xrecord.m_values.emplace_back(341, std::string("A504"));
      dxfRW references(path.string().c_str()); referenced.dxfW = &references;
      references.setDxfWriteRecordInventoryEnabled(true);
      std::map<std::uint32_t, std::uint32_t> referenceRemap{
          {0xA500u, 0xA510u}, {0xA501u, 0xA511u}, {0xA502u, 0xA512u}};
      if (explicitGroupTargets) {
          referenceRemap.emplace(0xA503u, 0xA513u);
          referenceRemap.emplace(0xA504u, 0xA514u);
      }
      references.setHandleRemap(referenceRemap);
      named.m_entries = {{"FORWARD_GROUP", 0xA503u}, {"FORWARD_GROUP_2", 0xA504u}};
      references.setNamedDictObjects({named});
      references.setRootDictEntries({{"DIRECT", "A510"}, {"NAMED", "A511"}});
      DRW_Group first = group;
      first.reactorHandles = {0xA504u};
      first.appData.push_back({{102, std::string("{ACAD_REACTORS")},
                               {330, std::string("A504")}, {102, std::string("}")}});
      DRW_Group second = group;
      second.handle = 0xA504u;
      second.name = "SECOND_REFERENCED_GROUP";
      second.reactorHandles = {0xA503u};
      references.setGroups(explicitGroupTargets ? std::vector<DRW_Group>{second, first}
                                                : std::vector<DRW_Group>{first, second});
      const bool referenceOutput = references.write(&referenced, referenceVersion, binary);
      dx_iface reader;
      dx_data imported;
      const bool referenceReadback = referenceOutput
          && reader.fileImport(path.string(), &imported, false);
      std::vector<TestDxfRecordIdentity> referenceInventory;
      const bool referenceInventoryMatches = referenceOutput
          && readPublishedDxfRecordInventory(path, binary, referenceInventory)
          && sameTestDxfRecordInventory(referenced.inventory, referenceInventory);
      const auto primaryBinding = references.getWritingContext()->sourceHandleToWrittenObjectMap.find(0xA503u);
      const auto secondaryBinding = references.getWritingContext()->sourceHandleToWrittenObjectMap.find(0xA504u);
      const auto& bindings = references.getWritingContext()->sourceHandleToWrittenObjectMap;
      const std::uint32_t primaryHandle = primaryBinding == bindings.end() ? 0u : primaryBinding->second;
      const std::uint32_t secondaryHandle = secondaryBinding == bindings.end() ? 0u : secondaryBinding->second;
      const auto primaryReadback = std::find_if(imported.groups.begin(), imported.groups.end(),
          [primaryHandle](const auto& value) { return value.handle == primaryHandle; });
      const auto secondaryReadback = std::find_if(imported.groups.begin(), imported.groups.end(),
          [secondaryHandle](const auto& value) { return value.handle == secondaryHandle; });
      const auto namedReadback = std::find_if(imported.dictionaries.begin(),
          imported.dictionaries.end(), [](const auto& value) { return value.handle == 0xA511u; });
      const auto directReadback = std::find_if(imported.dictionaries.begin(),
          imported.dictionaries.end(), [](const auto& value) { return value.handle == 0xA510u; });
      const auto xrecordReadback = std::find_if(imported.xRecords.begin(),
          imported.xRecords.end(), [](const auto& value) { return value.handle == 0xA512u; });
      const bool referencesMatch = referenceReadback && referenceInventoryMatches
          && primaryHandle != 0 && secondaryHandle != 0
          && primaryHandle != secondaryHandle
          && imported.mBlock->ent.size() == 1u
          && imported.mBlock->ent.front()->reactorHandles
              == std::vector<std::uint32_t>{primaryHandle, secondaryHandle}
          && namedReadback != imported.dictionaries.end() && namedReadback->m_entries.size() == 2u
          && namedReadback->m_entries.front().m_handle == primaryHandle
          && namedReadback->m_entries.back().m_handle == secondaryHandle
          && directReadback != imported.dictionaries.end() && directReadback->m_entries.size() == 3u
          && directReadback->m_entries[1].m_handle == primaryHandle
          && directReadback->m_entries.back().m_handle == secondaryHandle
          && primaryReadback != imported.groups.end() && secondaryReadback != imported.groups.end()
          && primaryReadback->reactorHandles == std::vector<std::uint32_t>{secondaryHandle}
          && secondaryReadback->reactorHandles == std::vector<std::uint32_t>{primaryHandle}
          && primaryReadback->m_entityHandles
              == std::vector<std::uint32_t>{imported.mBlock->ent.front()->handle}
          && secondaryReadback->m_entityHandles == primaryReadback->m_entityHandles
          && xrecordReadback != imported.xRecords.end() && xrecordReadback->m_dataEntries.size() == 3u
          && xrecordReadback->m_dataEntries[1].code() == 340
          && xrecordReadback->m_dataEntries.back().code() == 341
          && xrecordReadback->m_dataEntries.front().type() == DRW_Variant::STRING
          && std::string(xrecordReadback->m_dataEntries.front().c_str()) == "source-object-identity"
          && xrecordReadback->m_handleValues
              == std::vector<std::pair<int, std::uint32_t>>{{340, primaryHandle}, {341, secondaryHandle}};
      if (!referencesMatch) {
          std::cerr << "GROUP refs version=" << static_cast<int>(referenceVersion)
                    << " explicit=" << explicitGroupTargets << " binary=" << binary << " wrote=" << referenceOutput
                    << " read=" << referenceReadback << " entities=" << imported.mBlock->ent.size()
                    << " group=" << primaryHandle << ',' << secondaryHandle
                    << " staged=" << referenced.stagedObjectsObserved
                    << " identity=" << referenced.identitiesObserved
                    << " diagnostic=" << references.getLastDiagnostic().message
                    << " named=" << (namedReadback == imported.dictionaries.end() ? 0u : namedReadback->m_entries.size())
                    << " direct=" << (directReadback == imported.dictionaries.end() ? 0u : directReadback->m_entries.size())
                    << " xrecord=" << (xrecordReadback == imported.xRecords.end() ? 0u : xrecordReadback->m_dataEntries.size()) << '\n';
          if (!imported.mBlock->ent.empty()) {
              for (const auto target : imported.mBlock->ent.front()->reactorHandles)
                  std::cerr << " reactor=" << target;
          }
          if (namedReadback != imported.dictionaries.end())
              for (const auto& entry : namedReadback->m_entries) std::cerr << " named=" << entry.m_handle;
          if (directReadback != imported.dictionaries.end())
              for (const auto& entry : directReadback->m_entries) std::cerr << " direct=" << entry.m_handle;
          if (xrecordReadback != imported.xRecords.end())
              for (const auto& value : xrecordReadback->m_dataEntries)
                  if (value.type() == DRW_Variant::STRING) std::cerr << " xvalue=" << value.code() << ':' << value.c_str();
          std::cerr << '\n';
      }
      t.expect(referencesMatch,
               "entity reactors and forward/backward dictionary/XRECORD GROUP references use final identities");
       }
      }
    }
}

void testGeneratedDictionarySourceMetadata(TestContext& t) {
    using Edge = DRW_Dictionary::Entry::DxfHandleCode;
    class Source final : public dx_iface {
    public:
        Source() {
            cData = &storage;
            currentBlock = storage.mBlock;
            DRW_AppId app; app.name = "DICT_META";
            storage.appIds.push_back(app);
        }
        void writeEntities() override {
            DRW_Line line; line.handle = 0xA100u; line.secPoint.x = 1.0;
            (void)dxfW->writeLine(&line);
            if (lateMetadata)
                (void)dxfW->setGeneratedDictionarySourceRecords(nullptr, nullptr);
            if (lateRootEntries)
                dxfW->setRootDictEntries({{"REPLACED", "A510"}});
        }
        void writeObjects() override {
            if (rootOnly)
                return;
            DRW_XRecord value;
            value.handle = 0xA502u; value.parentHandle = 0xA5F0u;
            value.m_values.emplace_back(1, std::string("root-value"));
            value.m_values.emplace_back(90, 314);
            (void)dxfW->writeXRecord(&value);
            for (const auto pair : {std::pair<std::uint32_t, std::uint32_t>{0xA506u, 0xA5F0u},
                                     {0xA508u, 0xA5F1u}}) {
                DRW_Dictionary extension;
                extension.handle = pair.first; extension.parentHandle = pair.second;
                (void)dxfW->writeDictionary(&extension);
            }
            if (lateFailure) {
                DRW_XRecord invalid; invalid.handle = 0xA5FFu; invalid.m_cloning = 6;
                (void)dxfW->writeXRecord(&invalid);
            }
        }
        bool finalizeDxfWrite() override {
            identities = dxfW->dxfWriteSourceObjectHasIdentity(
                    0xA5F0u, 0xCu, "DICTIONARY", "OBJECTS", 0u)
                && dxfW->dxfWriteSourceObjectHasIdentity(
                    0xA5F1u, 0xDu, "DICTIONARY", "OBJECTS", 0xCu);
            if (rootOnly)
                identities = dxfW->dxfWriteSourceObjectHasIdentity(
                    0xA5F0u, 0xCu, "DICTIONARY", "OBJECTS", 0u);
            return identities;
        }
        dx_data storage;
        bool lateMetadata{false}, lateRootEntries{false}, lateFailure{false}, identities{false}, rootOnly{false};
    };
    const auto rootSource = [] {
        auto root = std::make_unique<DRW_Dictionary>();
        root->handle = 0xA5F0u; root->hardOwner = 1; root->cloning = 2;
        root->xDictHandle = 0xA506u;
        root->reactorHandles = {0xA5F1u};
        root->appData.push_back({{102, std::string("{ROOT_META")}, {1, std::string("source-root")},
                                {320, std::string("DEADBEEF")}, {102, std::string("}")}});
        (void)root->addExtData(std::make_unique<DRW_Variant>(1001, std::string("DICT_META")));
        (void)root->addExtData(std::make_unique<DRW_Variant>(1000, std::string("root-eed-caf\xC3\xA9")));
        root->m_entries = {{"DIRECT_XREC", 0xA502u, Edge::SoftOwner},
                          {"acad_group", 0xA5F1u, Edge::HardOwner},
                          {"CUSTOM_\xC3\x89", 0xA500u, Edge::SoftOwner},
                          {"UNSELECTED", 0u, Edge::SoftOwner}};
        return root;
    };
    const auto groupSource = [] {
        auto groups = std::make_unique<DRW_Dictionary>();
        groups->handle = 0xA5F1u; groups->parentHandle = 0xA5F0u;
        groups->hardOwner = 0; groups->cloning = 3;
        groups->xDictHandle = 0xA508u;
        groups->reactorHandles = {0xA5F0u};
        groups->appData.push_back({{102, std::string("{ACAD_REACTORS")},
                                  {330, std::string("A5F0")}, {102, std::string("}")}});
        (void)groups->addExtData(std::make_unique<DRW_Variant>(1001, std::string("DICT_META")));
        (void)groups->addExtData(std::make_unique<DRW_Variant>(1000, std::string("group-eed")));
        groups->m_entries = {{"SECOND", 0xA504u, Edge::HardOwner},
                            {"FIRST", 0xA503u, Edge::SoftOwner}};
        return groups;
    };
    const auto path = std::filesystem::temp_directory_path() / ("libdxfrw-source-dictionaries-"
        + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    if (std::filesystem::exists(path)) { t.expect(false, "dictionary scratch path exists"); return; }
    struct Cleanup { std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
    } cleanup{path};
    for (const bool binary : {false, true}) {
      for (const auto version : {DRW::AC1014, DRW::AC1015, DRW::AC1018, DRW::AC1021,
                                 DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
       for (int fault = 0; fault < 9; ++fault) {
        if (fault != 0 && version != DRW::AC1021) continue;
        Source source;
        dxfRW codec(path.string().c_str()); source.dxfW = &codec;
        codec.setDxfWriteRecordInventoryEnabled(true);
        codec.setHandleRemap({{0xA5F0u, 0xCu}, {0xA5F1u, 0xDu}, {0xA500u, 0xA510u},
                             {0xA502u, 0xA512u}, {0xA506u, 0xA516u}, {0xA508u, 0xA518u}});
        DRW_Dictionary named; named.handle = 0xA500u; named.parentHandle = 0xA5F0u;
        codec.setNamedDictObjects({named});
        // Reverse requested root order deliberately; retained source keys have
        // their own order, spelling and hard/soft ownership roles.
        codec.setRootDictEntries({{"CUSTOM_\xC3\x89", "A510"}, {"DIRECT_XREC", "A512"}});
        DRW_Group first; first.handle = 0xA503u; first.name = "FIRST";
        first.m_entityHandles = {0xA100u};
        DRW_Group second = first; second.handle = 0xA504u; second.name = "SECOND";
        codec.setGroups({first, second});
        auto root = rootSource(); auto groups = groupSource();
        if (fault == 1) groups->m_entries.pop_back();
        if (fault == 2) groups->m_entries.front().m_name = "RENAMED";
        if (fault == 3) groups->m_entries.front().m_dxfHandleCode = static_cast<Edge>(351);
        if (fault == 4) source.lateMetadata = true;
        if (fault == 5) source.lateFailure = true;
        if (fault == 6) codec.setRootDictEntries({{"CUSTOM_\xC3\x89", "A511"}, {"DIRECT_XREC", "A512"}});
        if (fault == 7) (void)root->addExtData(std::make_unique<DRW_Variant>(1070, 70001));
        if (fault == 8) source.lateRootEntries = true;
        const bool admitted = codec.setGeneratedDictionarySourceRecords(std::move(root), std::move(groups));
        t.expect(admitted == (fault != 3),
                 "source dictionary setup owns a bounded snapshot and rejects invalid edge roles");
        const std::string sentinel = "GENERATED_DICTIONARY_METADATA_SENTINEL";
        { std::ofstream previous(path, std::ios::binary); previous << sentinel; }
        bool wrote = admitted && codec.write(&source, version, binary);
        if (fault != 0) {
            std::ifstream previous(path, std::ios::binary);
            t.expect(!wrote && std::string(std::istreambuf_iterator<char>(previous), {}) == sentinel
                         && codec.getWritingContext()->sourceHandleToWrittenObjectMap.empty(),
                     "metadata/membership/callback failure retracts generated identities and preserves output");
            previous.close();
            source.lateMetadata = source.lateRootEntries = source.lateFailure = false;
            codec.setRootDictEntries({{"CUSTOM_\xC3\x89", "A510"}, {"DIRECT_XREC", "A512"}});
            t.expect(codec.setGeneratedDictionarySourceRecords(rootSource(), groupSource()),
                     "failed write permits corrected owned source metadata");
            wrote = codec.write(&source, version, binary);
        }
        dx_iface reader; dx_data decoded;
        const bool read = wrote && source.identities && reader.fileImport(path.string(), &decoded, false);
        const auto groupDictionary = std::find_if(decoded.dictionaries.begin(), decoded.dictionaries.end(),
            [](const auto& value) { return value.handle == 0xDu; });
        const auto rootValue = std::find_if(decoded.xRecords.begin(), decoded.xRecords.end(),
            [](const auto& value) { return value.handle == 0xA512u; });
        const auto common = [](const DRW_Dictionary& value, std::uint32_t reactor,
                               std::uint32_t extension, const char* text) {
            return value.reactorHandles == std::vector<std::uint32_t>{reactor}
                && value.xDictHandle == extension && value.extData.size() == 2u
                && value.extData.back()->type() == DRW_Variant::STRING
                && std::string(value.extData.back()->c_str()) == text;
        };
        const auto& bindings = codec.getWritingContext()->sourceHandleToWrittenObjectMap;
        const auto rawMetadata = std::find_if(decoded.namedObjectDictionary.appData.begin(),
            decoded.namedObjectDictionary.appData.end(), [](const auto& group) {
                return !group.empty() && group.front().type() == DRW_Variant::STRING
                    && std::string(group.front().c_str()) == "ROOT_META";
            });
        bool rawMetadataMatches = rawMetadata != decoded.namedObjectDictionary.appData.end()
            && rawMetadata->size() == 4u;
        if (rawMetadataMatches) {
            auto value = std::next(rawMetadata->begin());
            rawMetadataMatches = value->code() == 1 && value->type() == DRW_Variant::STRING
                && std::string(value->c_str()) == "source-root";
            ++value;
            rawMetadataMatches = rawMetadataMatches && value->code() == 320
                && value->type() == DRW_Variant::STRING && std::string(value->c_str()) == "DEADBEEF";
        }
        const bool retained = read && decoded.hasNamedObjectDictionary
            && rootValue != decoded.xRecords.end() && rootValue->parentHandle == 0xCu
            && rootValue->m_dataEntries.size() == 2u
            && rootValue->m_dataEntries[0].code() == 1
            && rootValue->m_dataEntries[0].type() == DRW_Variant::STRING
            && std::string(rootValue->m_dataEntries[0].c_str()) == "root-value"
            && rootValue->m_dataEntries[1].code() == 90
            && rootValue->m_dataEntries[1].type() == DRW_Variant::INTEGER
            && rootValue->m_dataEntries[1].i_val() == 314
            && decoded.namedObjectDictionary.parentHandle == 0
            && decoded.namedObjectDictionary.hardOwner == 1 && decoded.namedObjectDictionary.cloning == 2
            && common(decoded.namedObjectDictionary, 0xDu, 0xA516u, "root-eed-caf\xC3\xA9")
            && rawMetadataMatches
            && decoded.namedObjectDictionary.m_entries.size() == 3u
            && decoded.namedObjectDictionary.m_entries[0].m_name == "DIRECT_XREC"
            && decoded.namedObjectDictionary.m_entries[0].m_handle == 0xA512u
            && decoded.namedObjectDictionary.m_entries[0].m_dxfHandleCode == Edge::SoftOwner
            && decoded.namedObjectDictionary.m_entries[1].m_name == "acad_group"
            && decoded.namedObjectDictionary.m_entries[1].m_handle == 0xDu
            && decoded.namedObjectDictionary.m_entries[1].m_dxfHandleCode == Edge::HardOwner
            && decoded.namedObjectDictionary.m_entries[2].m_name == "CUSTOM_\xC3\x89"
            && groupDictionary != decoded.dictionaries.end() && groupDictionary->parentHandle == 0xCu
            && groupDictionary->hardOwner == 0 && groupDictionary->cloning == 3
            && common(*groupDictionary, 0xCu, 0xA518u, "group-eed")
            && groupDictionary->m_entries.size() == 2u
            && groupDictionary->m_entries[0].m_name == "SECOND"
            && bindings.count(0xA504u) == 1u
            && groupDictionary->m_entries[0].m_handle == bindings.at(0xA504u)
            && groupDictionary->m_entries[0].m_dxfHandleCode == Edge::HardOwner
            && groupDictionary->m_entries[1].m_name == "FIRST"
            && bindings.count(0xA503u) == 1u
            && groupDictionary->m_entries[1].m_handle == bindings.at(0xA503u)
            && groupDictionary->m_entries[1].m_dxfHandleCode == Edge::SoftOwner;
        if (!retained) std::cerr << "dictionary metadata version=" << static_cast<int>(version)
            << " binary=" << binary << " fault=" << fault << " wrote=" << wrote
            << " read=" << read << " diagnostic=" << codec.getLastDiagnostic().message << '\n';
        t.expect(retained, "C/D metadata, original key order/edge roles and EED survive actual readback");
       }
      }
    }
    dxfRW bounded("");
    auto empty = std::make_unique<DRW_Dictionary>(); empty->handle = 0xA5F0u;
    t.expect(bounded.setGeneratedDictionarySourceRecords(std::move(empty), nullptr, sizeof(DRW_Dictionary)),
             "exact visible source metadata byte boundary is admitted");
    const auto retainedAlias = bounded.m_generatedNodSourceHandle;
    empty = std::make_unique<DRW_Dictionary>(); empty->handle = 0xA5F2u;
    t.expect(!bounded.setGeneratedDictionarySourceRecords(std::move(empty), nullptr, sizeof(DRW_Dictionary) - 1u)
                 && bounded.m_generatedNodSourceHandle == retainedAlias
                 && !bounded.setGeneratedDictionarySourceHandles(0xA5F2u, 0u),
             "one-byte overflow and alias relabel refuse without replacing accepted metadata");
    for (int invalid = 0; invalid < 7; ++invalid) {
        auto root = rootSource(); auto groups = groupSource();
        if (invalid == 0) root->parentHandle = 1u;
        if (invalid == 1) groups->parentHandle = 1u;
        if (invalid == 2) groups->handle = root->handle;
        if (invalid == 3) root->m_entries[1].m_handle = 1u;
        if (invalid == 4) root->m_entries.push_back(root->m_entries.front());
        if (invalid == 5) root->countCap = DRW_DictionaryCountCap{};
        if (invalid == 6) root->setDwgCommonObjectState(0, 0, true);
        t.expect(!bounded.setGeneratedDictionarySourceRecords(std::move(root), std::move(groups))
                     && bounded.m_generatedNodSourceHandle == retainedAlias,
                 "invalid/ambiguous/incomplete/unmapped common source metadata preserves prior setup");
    }
    empty = std::make_unique<DRW_Dictionary>(); empty->handle = 0xA5F2u;
    empty->reactorHandles.resize(DRW_TableEntry::kMaxExtendedDataItems + 1u, 0xA500u);
    t.expect(!bounded.setGeneratedDictionarySourceRecords(std::move(empty), nullptr)
                 && bounded.m_generatedNodSourceHandle == retainedAlias
                 && !bounded.setGeneratedDictionarySourceRecords(nullptr, nullptr, 64u * 1024u * 1024u + 1u)
                 && bounded.m_generatedNodSourceHandle == retainedAlias,
             "aggregate common-item overflow and an enlarged hard-byte cap cannot replace accepted metadata");
    t.expect(bounded.setGeneratedDictionarySourceRecords(nullptr, nullptr)
                 && bounded.m_generatedNodSourceHandle == 0u,
             "explicit reset clears owned generated metadata and aliases");
    // New source-preservation setup cannot silently disappear in a target
    // whose writer never visits the OBJECTS section.
    Source unsupported;
    unsupported.rootOnly = true;
    dxfRW legacy(path.string().c_str()); unsupported.dxfW = &legacy;
    legacy.setDxfWriteRecordInventoryEnabled(true);
    legacy.setHandleRemap({{0xA5F0u, 0xCu}});
    auto rootOnly = std::make_unique<DRW_Dictionary>(); rootOnly->handle = 0xA5F0u;
    t.expect(legacy.setGeneratedDictionarySourceRecords(std::move(rootOnly), nullptr),
             "root-only source setup is independent of eventual target selection");
    { std::ofstream previous(path); previous << "LEGACY_DICTIONARY_SENTINEL"; }
    t.expect(!legacy.write(&unsupported, DRW::AC1009, false)
                 && legacy.getLastDiagnostic().code == "generated-dictionary-target-profile",
             "non-OBJECTS target refuses source metadata instead of silently discarding it");
    std::ifstream previous(path);
    t.expect(std::string(std::istreambuf_iterator<char>(previous), {}) == "LEGACY_DICTIONARY_SENTINEL",
             "unsupported dictionary target leaves destination intact");
    previous.close();
    t.expect(legacy.write(&unsupported, DRW::AC1032, false) && unsupported.identities,
             "same codec recovers from unsupported target with root-only metadata and generated empty D");
}

void testDxfWriterEmittedHandleInventory(TestContext& t) {
    std::vector<std::uint32_t> asciiHandles;
    for (const bool binary : {false, true}) {
        std::ostringstream output;
        std::unique_ptr<dxfWriter> writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&output))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&output));
        writer->setRecordInventoryEnabled(true, 8u);

        bool wrote = writer->writeString(0, "SECTION")
            && writer->writeString(2, "HEADER")
            && writer->writeString(9, "$HANDSEED")
            && writer->writeString(5, "F000")
            && writer->writeString(0, "ENDSEC")
            && writer->writeString(0, "SECTION")
            && writer->writeString(2, "ENTITIES");
        {
            DxfWriterRecordScope line(*writer);
            wrote = wrote && writer->writeString(0, "LINE")
                && writer->writeString(5, "A");
            bool rolledBackOwner = false;
            {
                DxfWriterRecordScope owner(*writer);
                rolledBackOwner = writer->writeString(102, "{ACAD_REACTORS")
                    && writer->writeString(330, "2F");
            }
            wrote = wrote && rolledBackOwner
                && writer->writeString(102, "{ACAD_REACTORS")
                && writer->writeString(330, "3F")
                && writer->writeString(102, "}")
                && writer->writeString(330, "1F")
                && line.commit();
        }
        {
            DxfWriterRecordScope rolledBack(*writer);
            wrote = wrote && writer->writeString(0, "CIRCLE")
                && writer->writeString(5, "B");
            {
                DxfWriterRecordScope nested(*writer);
                wrote = wrote && writer->writeString(0, "POINT")
                    && writer->writeString(5, "C")
                    && nested.commit();
            }
            // Deliberately leave the enclosing scope uncommitted. The nested
            // commit must roll back with its parent, both in bytes and ledger.
        }
        wrote = wrote && writer->writeString(0, "ENDSEC")
            && writer->writeString(0, "SECTION")
            && writer->writeString(2, "TABLES");
        {
            DxfWriterRecordScope dimstyle(*writer);
            wrote = wrote && writer->writeString(0, "DIMSTYLE")
                && writer->writeString(105, "2B")
                && dimstyle.commit();
        }
        wrote = wrote && writer->writeString(0, "ENDSEC")
            && writer->writeString(0, "SECTION")
            && writer->writeString(2, "OBJECTS")
            && writer->writeString(0, "XRECORD")
            && writer->writeString(5, "3B")
            && writer->writeString(100, "AcDbXrecord")
            && writer->writeString(330, "4B")
            && writer->writeString(0, "ENDSEC")
            && writer->writeString(0, "EOF")
            && writer->writeString(0, "WIRE_END");

        const bool inventoryFinalized = writer->finalizeRecordInventory();
        const std::vector<std::uint32_t> handles =
            writer->emittedRecordHandles();
        t.expect(wrote && inventoryFinalized
                     && handles == std::vector<std::uint32_t>{0xAu, 0x2Bu, 0x3Bu}
                     && writer->recordHandleHasIdentity(
                         0xAu, "line", "entities")
                     && writer->recordHandleHasIdentity(
                         0x2Bu, "DIMSTYLE", "TABLES"),
                 "DXF record inventory tracks normalized type/section and rolled-back handles");
        t.expect(writer->recordHandleHasOwner(0xAu, 0x1Fu)
                     && !writer->recordHandleHasOwner(0xAu, 0x2Fu)
                     && !writer->recordHandleHasOwner(0xAu, 0x3Fu)
                     && !writer->recordHandleHasOwner(0x3Bu, 0x4Bu),
                 "DXF owner inventory skips reactors/body handles and restores application depth on rollback");
        if (binary)
            t.expect(handles == asciiHandles,
                     "ASCII and binary writer handle inventories agree");
        else
            asciiHandles = handles;

        bool ended = false;
        const auto tags = numericWireProjection(output.str(), binary, ended);
        t.expect(ended, "DXF handle inventory test output remains framed");
        t.expect(std::find(tags.begin(), tags.end(), "5:A") != tags.end()
                     && std::find(tags.begin(), tags.end(), "105:2B") != tags.end()
                     && std::find(tags.begin(), tags.end(), "0:CIRCLE") == tags.end()
                     && std::find(tags.begin(), tags.end(), "0:POINT") == tags.end(),
                 "DXF handle inventory matches committed output bytes");
    }

    std::ostringstream limitedOutput;
    dxfWriterAscii limitedWriter(&limitedOutput);
    limitedWriter.setRecordInventoryEnabled(true, 1u);
    bool withinLimit = false;
    {
        DxfWriterRecordScope line(limitedWriter);
        withinLimit = limitedWriter.writeString(0, "LINE")
            && limitedWriter.writeString(5, "1") && line.commit();
    }
    {
        DxfWriterRecordScope circle(limitedWriter);
        (void)limitedWriter.writeString(0, "CIRCLE");
        (void)limitedWriter.writeString(5, "2");
    }
    t.expect(withinLimit && limitedWriter.hasWriteError()
                 && limitedWriter.emittedRecordHandles()
                        == std::vector<std::uint32_t>{1u},
             "DXF handle inventory limit stays sticky across scope rollback");

    std::ostringstream duplicateOutput;
    dxfWriterAscii duplicateWriter(&duplicateOutput);
    duplicateWriter.setRecordInventoryEnabled(true, 4u);
    const bool wroteDuplicates = duplicateWriter.writeString(0, "SECTION")
        && duplicateWriter.writeString(2, "ENTITIES")
        && duplicateWriter.writeString(0, "LINE")
        && duplicateWriter.writeString(5, "1")
        && duplicateWriter.writeString(0, "CIRCLE")
        && duplicateWriter.writeString(5, "1");
    t.expect(wroteDuplicates && !duplicateWriter.finalizeRecordInventory()
                 && duplicateWriter.hasWriteError(),
             "DXF record inventory rejects duplicate emitted handles");

    class FinalizeInventoryInterface final : public dx_iface {
    public:
        FinalizeInventoryInterface() {
            cData = &storage;
            currentBlock = storage.mBlock;
            auto line = std::make_unique<DRW_Line>();
            line->handle = 0xA100u;
            line->basePoint = DRW_Coord(1, 2, 3);
            line->secPoint = DRW_Coord(4, 5, 6);
            storage.mBlock->ent.push_back(line.release());

            auto block = std::make_unique<dx_ifaceBlock>();
            block->name = "InventoryBlock";
            block->handle = 0xA200u;
            block->parentHandle = 0xA210u;
            auto blockLine = std::make_unique<DRW_Line>();
            blockLine->handle = 0xA101u;
            blockLine->parentHandle = DRW::NoHandle;
            blockLine->basePoint = DRW_Coord(10, 20, 30);
            blockLine->secPoint = DRW_Coord(40, 50, 60);
            block->ent.push_back(blockLine.release());
            storage.blocks.push_back(block.release());
            DRW_Block_Record blockRecord;
            blockRecord.handle = 0xA210u;
            blockRecord.name = "InventoryBlock";
            storage.blockRecords.push_back(blockRecord);

            field.handle = 0xA400u;
            field.parentHandle = 0xCu;
            field.m_childHandles = {0xA402u};
            field.m_objectHandles = {0xA300u};
            fieldList.handle = 0xA401u;
            fieldList.parentHandle = 0xCu;
            fieldList.m_fieldHandles = {0xA400u, 0xA402u};
            fieldChild.handle = 0xA402u;
            fieldChild.parentHandle = 0xA400u;
        }
        void writeObjects() override {
            dx_iface::writeObjects();
            DRW_RawDxfObject raw;
            raw.name = "XRECORD";
            raw.handle = 0xA300u;
            raw.parentHandle = 0xCu;
            raw.m_version = DRW::AC1027;
            raw.groups = {
                DRW_Variant(5, std::string("A300")),
                DRW_Variant(330, std::string("C")),
                DRW_Variant(100, std::string("AcDbXrecord")),
                DRW_Variant(1, std::string("inventory-raw-object"))};
            rawObjectEmitted = dxfW != nullptr
                && dxfW->writeRawDxfObject(&raw);
            fieldListEmitted = rawObjectEmitted && dxfW != nullptr
                && dxfW->writeFieldList(&fieldList);
            fieldEmitted = fieldListEmitted && dxfW != nullptr
                && dxfW->writeField(&field);
            if (failAfterFirstField) {
                DRW_Field invalidChild = fieldChild;
                invalidChild.m_childHandles.resize(
                    DRW_Field::kMaxItems + 1u, fieldChild.handle);
                if (dxfW != nullptr)
                    (void)dxfW->writeField(&invalidChild);
                return;
            }
            fieldChildEmitted = fieldEmitted && dxfW != nullptr
                && dxfW->writeField(&fieldChild);
        }
        bool finalizeDxfWrite() override {
            const auto* context = dxfW != nullptr
                ? dxfW->getWritingContext() : nullptr;
            bool lineWasEmitted = false;
            bool blockLineOwnerMatches = false;
            bool inventoryCaptured = false;
            const bool rawRoutesObserved = dxfW != nullptr
                && rawObjectEmitted
                && dxfW->dxfWriteRecordHasIdentity(
                    0xA300u, "XRECORD", "OBJECTS")
                && dxfW->dxfWriteRecordHasOwner(0xA300u, 0xCu)
                && dxfW->dxfWriteRecordHasIdentity(
                    0xA301u, "RAW_INVENTORY_RECORD",
                    "LOCAL_RAW_INVENTORY")
                && dxfW->dxfWriteRecordHasOwner(0xA301u, 0xCu);
            const bool fieldObjectIdentitiesObserved = dxfW != nullptr
                && fieldListEmitted && fieldEmitted && fieldChildEmitted
                && dxfW->dxfWriteSourceObjectHasIdentity(
                    0xA400u, 0xA410u, "FIELD", "OBJECTS", 0xCu)
                && dxfW->dxfWriteSourceObjectHasIdentity(
                    0xA402u, 0xA412u, "FIELD", "OBJECTS", 0xA410u)
                && dxfW->dxfWriteSourceObjectHasIdentity(
                    0xA401u, 0xA411u, "FIELDLIST", "OBJECTS", 0xCu);
            const bool missingSourceRejected = dxfW != nullptr
                && !dxfW->dxfWriteSourceObjectHasIdentity(
                    0xA499u, 0xA410u, "FIELD", "OBJECTS");
            const bool wrongHandleRejected = dxfW != nullptr
                && !dxfW->dxfWriteSourceObjectHasIdentity(
                    0xA400u, 0xA411u, "FIELD", "OBJECTS");
            const bool wrongTypeRejected = dxfW != nullptr
                && !dxfW->dxfWriteSourceObjectHasIdentity(
                    0xA400u, 0xA410u, "XRECORD", "OBJECTS");
            const bool wrongSectionRejected = dxfW != nullptr
                && !dxfW->dxfWriteSourceObjectHasIdentity(
                    0xA400u, 0xA410u, "FIELD", "ENTITIES");
            const bool wrongOwnerRejected = dxfW != nullptr
                && !dxfW->dxfWriteSourceObjectHasIdentity(
                    0xA402u, 0xA412u, "FIELD", "OBJECTS", 0xCu);
            const bool fieldObjectIdentityNegativesRejected =
                missingSourceRejected && wrongHandleRejected
                && wrongTypeRejected && wrongSectionRejected
                && wrongOwnerRejected;
            if (dxfW != nullptr && dxfW->writer != nullptr) {
                const auto& emitted = dxfW->writer->emittedRecords();
                inventory.reserve(emitted.size());
                inventoryCaptured = true;
                for (const auto& identity : emitted) {
                    const std::string* recordName =
                        dxfW->writer->recordInventoryName(identity.recordName);
                    const std::string* sectionName =
                        dxfW->writer->recordInventoryName(identity.sectionName);
                    if (recordName == nullptr || sectionName == nullptr) {
                        inventoryCaptured = false;
                        inventory.clear();
                        break;
                    }
                    inventory.push_back(
                        {identity.handle, *recordName, *sectionName,
                         identity.ownerHandle, identity.hasOwnerHandle});
                }
            }
            const auto modelLine = std::find_if(
                inventory.begin(), inventory.end(), [](const auto& identity) {
                    return identity.handle != 0
                        && identity.recordName == "LINE"
                        && identity.sectionName == "ENTITIES"
                        && identity.hasOwnerHandle
                        && identity.ownerHandle
                            == DRW::DxfModelSpaceBlockRecordHandle;
                });
            const std::uint32_t blockOwner = dxfW != nullptr
                ? dxfW->getBlockRecordHandleToWrite("InventoryBlock")
                : DRW::NoHandle;
            const auto blockLine = std::find_if(
                inventory.begin(), inventory.end(), [&](const auto& identity) {
                    return identity.handle != 0
                        && identity.recordName == "LINE"
                        && identity.sectionName == "BLOCKS"
                        && identity.hasOwnerHandle
                        && identity.ownerHandle == blockOwner;
                });
            modelLineEmittedHandle = modelLine != inventory.end()
                ? modelLine->handle : 0u;
            lineWasEmitted = dxfW != nullptr
                && modelLine != inventory.end()
                && dxfW->dxfWriteRecordHasIdentity(
                    modelLine->handle, "LINE", "ENTITIES")
                && dxfW->dxfWriteRecordHasOwner(
                    modelLine->handle,
                    DRW::DxfModelSpaceBlockRecordHandle);
            blockLineOwnerMatches = dxfW != nullptr
                && blockOwner != DRW::NoHandle
                && blockLine != inventory.end()
                && dxfW->dxfWriteRecordHasIdentity(
                    blockLine->handle, "LINE", "BLOCKS")
                && dxfW->dxfWriteRecordHasOwner(blockLine->handle, blockOwner);
            const bool entitySourceRejected = dxfW != nullptr
                && modelLineEmittedHandle != 0
                && !dxfW->dxfWriteSourceObjectHasIdentity(
                    0xA100u, modelLineEmittedHandle, "LINE", "ENTITIES");
            identityNegativesRejected = fieldObjectIdentityNegativesRejected
                && entitySourceRejected;
            modelSpaceIdentityValid = lineWasEmitted;
            blockOwnedIdentityValid = blockLineOwnerMatches;
            inventorySnapshotCaptured = inventoryCaptured;
            sawRecords = dxfW != nullptr && context != nullptr
                && dxfW->dxfWriteRecordHandleCount(0x20u) == 1u
                && dxfW->dxfWriteRecordHasIdentity(
                    0x20u, "BLOCK", "BLOCKS")
                && lineWasEmitted && blockLineOwnerMatches
                && entitySourceRejected
                && inventoryCaptured && rawRoutesObserved
                && fieldObjectIdentitiesObserved
                && fieldObjectIdentityNegativesRejected;
            return sawRecords;
        }
        dx_data storage;
        DRW_Field field;
        DRW_Field fieldChild;
        DRW_FieldList fieldList;
        bool sawRecords {false};
        bool modelSpaceIdentityValid {false};
        bool blockOwnedIdentityValid {false};
        bool identityNegativesRejected {false};
        bool inventorySnapshotCaptured {false};
        bool rawObjectEmitted {false};
        bool fieldListEmitted {false};
        bool fieldEmitted {false};
        bool fieldChildEmitted {false};
        bool failAfterFirstField {true};
        std::uint32_t modelLineEmittedHandle {0};
        std::vector<TestDxfRecordIdentity> inventory;
    };
    const auto outputPath = std::filesystem::temp_directory_path()
        / ("libdxfrw-record-inventory-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count())
            + ".dxf");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }
    } cleanup{outputPath};
    for (const bool binary : {false, true}) {
        FinalizeInventoryInterface interface_;
        const std::string outputName = outputPath.string();
        dxfRW codec(outputName.c_str());
        interface_.dxfW = &codec;
        codec.setDxfWriteRecordInventoryEnabled(true, 4096u);
        codec.setHandleRemap({
            {0xA400u, 0xA410u}, {0xA401u, 0xA411u},
            {0xA402u, 0xA412u}});
        DRW_RawDxfSection rawSection;
        rawSection.m_name = "LOCAL_RAW_INVENTORY";
        rawSection.m_version = DRW::AC1027;
        rawSection.m_groups = {
            DRW_Variant(0, std::string("RAW_INVENTORY_RECORD")),
            DRW_Variant(5, std::string("A301")),
            DRW_Variant(330, std::string("C")),
            DRW_Variant(1, std::string("inventory-raw-section"))};
        codec.setRawDxfSections({rawSection});
        const bool reserved = codec.reserveHandle(0xA300u)
            && codec.reserveHandle(0xA301u);
        const std::string destinationSentinel = "field-object-rollback-sentinel\n";
        {
            std::ofstream existing(outputPath,
                                   std::ios::binary | std::ios::trunc);
            existing.write(destinationSentinel.data(),
                           static_cast<std::streamsize>(destinationSentinel.size()));
        }
        const bool failedAfterObjectRecords = reserved
            && !codec.write(&interface_, DRW::AC1027, binary);
        std::ifstream rollbackInput(outputPath, std::ios::binary);
        const std::string retainedDestination = std::string(
            std::istreambuf_iterator<char>(rollbackInput),
            std::istreambuf_iterator<char>());
        const DRW_WritingContext* failedContext = codec.getWritingContext();
        const bool objectBindingsRolledBack = failedContext != nullptr
            && failedContext->sourceHandleToWrittenObjectMap.empty();
        interface_.failAfterFirstField = false;
        const bool written = reserved
            && codec.write(&interface_, DRW::AC1027, binary);
        if (!written) {
            const DRW_OperationDiagnostic diagnostic = codec.getLastDiagnostic();
            std::cerr << "record inventory writer rejected: " << diagnostic.code
                      << ": " << diagnostic.message << '\n';
        }
        std::vector<TestDxfRecordIdentity> parsedInventory;
        const bool parsed = readPublishedDxfRecordInventory(
            outputPath, binary, parsedInventory);
        if (parsed && !sameTestDxfRecordInventory(
                          interface_.inventory, parsedInventory)) {
            std::cerr << "record inventory mismatch (" << (binary ? "binary" : "ascii")
                      << ") captured=" << interface_.inventory.size()
                      << " parsed=" << parsedInventory.size() << '\n';
            for (std::size_t i = 0;
                 i < std::min(interface_.inventory.size(), parsedInventory.size());
                 ++i) {
                const auto& expected = interface_.inventory[i];
                const auto& actual = parsedInventory[i];
                if (expected.hasOwnerHandle != actual.hasOwnerHandle
                    || expected.ownerHandle != actual.ownerHandle) {
                    std::cerr << "  handle=" << expected.handle
                              << " expected-owner=" << expected.ownerHandle
                              << '/' << expected.hasOwnerHandle
                              << " actual-owner=" << actual.ownerHandle
                              << '/' << actual.hasOwnerHandle << '\n';
                    break;
                }
            }
        }
        t.expect(written && interface_.sawRecords
                     && interface_.modelLineEmittedHandle != 0
                     && !codec.dxfWriteSourceObjectHasIdentity(
                         0xA400u, 0xA410u, "FIELD", "OBJECTS"),
                 "DXF object identity query closes after finalization");
        t.expect(failedAfterObjectRecords
                     && retainedDestination == destinationSentinel
                     && objectBindingsRolledBack,
                 "DXF FIELD object bindings roll back with failed OBJECTS output and preserve destination");
        t.expect(interface_.modelSpaceIdentityValid
                     && interface_.blockOwnedIdentityValid,
                 "DXF emitted LINE inventory checks model and named-block owners");
        t.expect(interface_.identityNegativesRejected
                     && interface_.inventorySnapshotCaptured,
                 "DXF object identity query rejects wrong source, handle, type, section, owner, and entity inputs");
        t.expect(parsed && sameTestDxfRecordInventory(
                              interface_.inventory, parsedInventory),
                 "DXF ASCII/binary emitted identity inventory matches exact published bytes");
        std::ifstream publishedInput(outputPath, std::ios::binary);
        const std::string publishedBytes = std::string(
            std::istreambuf_iterator<char>(publishedInput),
            std::istreambuf_iterator<char>());
        std::string wireBytes = publishedBytes;
        bool binaryHeaderStripped = !binary;
        if (binary) {
            std::string binaryHeader = "AutoCAD Binary DXF\r\n";
            binaryHeader.push_back(static_cast<char>(0x1A));
            binaryHeader.push_back('\0');
            binaryHeaderStripped = wireBytes.compare(
                0, binaryHeader.size(), binaryHeader) == 0;
            if (binaryHeaderStripped)
                wireBytes.erase(0, binaryHeader.size());
        }
        bool hasWireEnd = false;
        const std::vector<std::string> publishedTags = numericWireProjection(
            wireBytes, binary, hasWireEnd);
        const bool fieldEdgesClosed = binaryHeaderStripped
            && !publishedTags.empty()
            && std::count(publishedTags.begin(), publishedTags.end(),
                          "330:A410") == 2
            && std::count(publishedTags.begin(), publishedTags.end(),
                          "330:A412") == 1
            && std::count(publishedTags.begin(), publishedTags.end(),
                          "360:A412") == 1
            && std::count(publishedTags.begin(), publishedTags.end(),
                          "331:A300") == 1
            && std::none_of(publishedTags.begin(), publishedTags.end(),
                [](const std::string& tag) {
                    return tag == "330:A400" || tag == "330:A402"
                        || tag == "360:A402";
                });
        t.expect(fieldEdgesClosed,
                 "DXF FIELD/FIELDLIST references resolve once to emitted remapped objects");
    }
}

void testDxfWriterControlCharacterFraming(TestContext& t) {
    const std::string controlText = "before\r\nmiddle\nafter\r";

    std::ostringstream asciiBytes;
    dxfWriterAscii asciiWriter(&asciiBytes);
    asciiWriter.setRecordInventoryEnabled(true, 8u);
    bool asciiStarted = asciiWriter.writeString(0, "SECTION")
        && asciiWriter.writeString(2, "ENTITIES");
    bool asciiRejectedAndRolledBack = false;
    {
        DxfWriterRecordScope textRecord(asciiWriter);
        const bool textStarted = asciiWriter.writeString(0, "TEXT")
            && asciiWriter.writeString(5, "10");
        const bool rejectedValue = textStarted
            && !asciiWriter.writeString(1, controlText);
        asciiRejectedAndRolledBack = rejectedValue && !textRecord.commit();
    }
    bool asciiFollowingRecord = false;
    {
        DxfWriterRecordScope lineRecord(asciiWriter);
        asciiFollowingRecord = asciiWriter.writeString(0, "LINE")
            && asciiWriter.writeString(5, "11")
            && asciiWriter.writeString(1, "FOLLOWING_RECORD")
            && lineRecord.commit();
    }
    const bool asciiEnded = asciiWriter.writeString(0, "ENDSEC")
        && asciiWriter.writeString(0, "WIRE_END");
    const bool asciiInventoryFinalized =
        asciiWriter.finalizeRecordInventory();
    bool asciiFramed = false;
    const std::vector<std::string> asciiTags = numericWireProjection(
        asciiBytes.str(), false, asciiFramed);
    const std::string asciiWire = asciiBytes.str();
    t.expect(asciiStarted && asciiRejectedAndRolledBack,
             "ASCII CR/LF text refusal rolls back its staged record");
    t.expect(asciiFollowingRecord && asciiEnded && asciiFramed,
             "ASCII CR/LF refusal preserves following record framing");
    t.expect(asciiInventoryFinalized && !asciiWriter.hasWriteError()
                 && asciiWriter.recordHandleCount(0x10u) == 0u
                 && asciiWriter.recordHandleHasIdentity(
                     0x11u, "LINE", "ENTITIES"),
             "ASCII CR/LF refusal rolls back emitted-record identity");
    t.expect(std::find(asciiTags.begin(), asciiTags.end(), "0:TEXT")
                     == asciiTags.end()
                 && std::find(asciiTags.begin(), asciiTags.end(), "0:LINE")
                     != asciiTags.end()
                 && std::find(asciiTags.begin(), asciiTags.end(),
                              "1:FOLLOWING_RECORD") != asciiTags.end(),
             "ASCII CR/LF refusal removes the partial record and retains the sentinel");
    const std::size_t asciiLinePosition = asciiWire.find("LINE\n");
    t.expect(asciiWire.find("TEXT") == std::string::npos
                 && asciiLinePosition != std::string::npos
                 && asciiWire.find("FOLLOWING_RECORD\n", asciiLinePosition)
                     != std::string::npos,
             "ASCII physical bytes contain no rolled-back TEXT value and retain the next record");

    std::ostringstream binaryBytes;
    dxfWriterBinary binaryWriter(&binaryBytes);
    binaryWriter.setRecordInventoryEnabled(true, 8u);
    bool binaryWrote = binaryWriter.writeString(0, "SECTION")
        && binaryWriter.writeString(2, "ENTITIES")
        && binaryWriter.writeString(0, "TEXT")
        && binaryWriter.writeString(5, "10")
        && binaryWriter.writeString(1, controlText)
        && binaryWriter.writeString(0, "LINE")
        && binaryWriter.writeString(5, "11")
        && binaryWriter.writeString(1, "FOLLOWING_RECORD")
        && binaryWriter.writeString(0, "ENDSEC")
        && binaryWriter.writeString(0, "WIRE_END");
    const bool binaryInventoryFinalized =
        binaryWriter.finalizeRecordInventory();
    bool binaryFramed = false;
    const std::vector<std::string> binaryTags = numericWireProjection(
        binaryBytes.str(), true, binaryFramed);
    const std::string binaryWire = binaryBytes.str();
    t.expect(binaryWrote && binaryFramed,
             "binary DXF CR/LF values retain following-record framing");
    t.expect(binaryInventoryFinalized
                 && binaryWriter.recordHandleHasIdentity(
                     0x10u, "TEXT", "ENTITIES")
                 && binaryWriter.recordHandleHasIdentity(
                     0x11u, "LINE", "ENTITIES"),
             "binary DXF CR/LF values preserve emitted-record identity");
    t.expect(std::find(binaryTags.begin(), binaryTags.end(),
                       "1:" + controlText) != binaryTags.end()
                 && std::find(binaryTags.begin(), binaryTags.end(),
                              "1:FOLLOWING_RECORD") != binaryTags.end(),
             "binary DXF preserves CR/LF as NUL-delimited value data");
    std::string binaryPhysicalSequence(1, static_cast<char>(1));
    binaryPhysicalSequence.push_back('\0');
    binaryPhysicalSequence += controlText;
    binaryPhysicalSequence.push_back('\0');
    binaryPhysicalSequence.push_back('\0');
    binaryPhysicalSequence.push_back('\0');
    binaryPhysicalSequence += "LINE";
    binaryPhysicalSequence.push_back('\0');
    t.expect(binaryWire.find(binaryPhysicalSequence) != std::string::npos,
             "binary physical bytes terminate the CR/LF value before the next group-code byte");

    dx_data data;
    auto text = std::make_unique<DRW_Text>();
    text->text = controlText;
    text->height = 1.0;
    data.mBlock->ent.push_back(text.release());
    auto line = std::make_unique<DRW_Line>();
    line->basePoint = DRW_Coord(1.0, 2.0, 3.0);
    line->secPoint = DRW_Coord(4.0, 5.0, 6.0);
    data.mBlock->ent.push_back(line.release());

    const auto nonce = std::chrono::steady_clock::now()
        .time_since_epoch().count();
    for (const bool binary : {false, true}) {
        const auto path = std::filesystem::temp_directory_path()
            / ("libdxfrw-control-text-refusal-" + std::to_string(nonce)
               + (binary ? "-binary.dxf" : "-ascii.dxf"));
        const std::string sentinel = "pre-existing-destination";
        {
            std::ofstream existing(path, std::ios::binary | std::ios::trunc);
            existing.write(sentinel.data(),
                           static_cast<std::streamsize>(sentinel.size()));
        }
        dx_iface adapter;
        const bool refused = !adapter.fileExport(
            path.string(), DRW::AC1021, binary, &data, false);
        std::ifstream retained(path, std::ios::binary);
        const std::string bytes(
            (std::istreambuf_iterator<char>(retained)),
            std::istreambuf_iterator<char>());
        t.expect(refused && bytes == sentinel,
                 binary
                     ? "typed binary export refuses unsafe text without replacing the destination"
                     : "typed ASCII export refuses unsafe text without replacing the destination");
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
    }
}

void testGeoDataVersionedWriter(TestContext& t) {
    const auto makeLegacy = []() {
        DRW_GeoData geo;
        geo.handle = 0x700u;
        geo.parentHandle = 0x2u;
        geo.m_hostBlockHandle = 0x3u;
        geo.m_version = 1;
        geo.m_legacy.m_controlFlags = 2;
        geo.m_designPoint = DRW_Coord{10.125, 20.125, 30.125};
        geo.m_legacy.m_secondWcsPoint = DRW_Coord{11.125, 21.125, 31.125};
        geo.m_legacy.m_geolocation = {{40.125, 41.125, 42.125}};
        geo.m_legacy.m_drawingScales = {{43.125, 44.125, 45.125}};
        geo.m_legacy.m_mapUnits = 46.125;
        geo.m_legacy.m_northDirectionAngle = 52.125;
        geo.m_upDirection = DRW_Coord{210.125, 220.125, 230.125};
        geo.m_coordinateSystemDefinition = "legacy-coordinate-definition";
        geo.m_geoRssTag = "legacy-georss";
        geo.m_legacy.m_coordinateSystemDescription =
            "legacy-coordinate-description";
        geo.m_legacy.m_coordinateSystemDatum = "legacy-datum";
        geo.m_observationFromTag = "legacy-observation-tag";
        geo.m_observationToTag = "legacy-observation-to";
        geo.m_observationCoverageTag = "legacy-observation-coverage";
        geo.m_legacy.m_dwgUnits = 91;
        geo.m_points = {{{12.125, 22.125, 0.0}, {13.125, 23.125, 0.0}}};
        geo.m_faces = {{1, 2, 3}};
        DRW_GeoDataLegacyCivil& civil = geo.m_legacy.m_civil;
        civil.m_present = true;
        civil.m_transformLocalToGrid = true;
        civil.m_localReferencePoint = {{14.125, 24.125}};
        civil.m_gridReferencePoint = {{15.125, 25.125}};
        civil.m_localReferencePointNumber = 1;
        civil.m_gridReferencePointNumber = 94;
        civil.m_useRotationPoints = true;
        civil.m_localRotationPoint = {{16.125, 26.125}};
        civil.m_gridRotationPoint = {{17.125, 27.125}};
        civil.m_rotationAngle = 54.125;
        civil.m_gridAzimuth = 140.125;
        civil.m_scaleEstimationMethod = 2;
        civil.m_ambiguous141Value = 141.125;
        civil.m_enableSeaLevelCorrection = true;
        civil.m_seaLevelElevation = 142.125;
        civil.m_coordinateProjectionRadius = 143.125;
        return geo;
    };

    const auto codesOf = [](const std::vector<std::string>& projection) {
        std::vector<int> codes;
        for (const std::string& tag : projection) {
            const std::size_t colon = tag.find(':');
            if (colon == std::string::npos)
                return std::vector<int>{};
            codes.push_back(std::stoi(tag.substr(0, colon)));
        }
        return codes;
    };

    std::vector<std::string> asciiProjection;
    for (const bool binary : {false, true}) {
        std::ostringstream output;
        dxfRW owner("");
        owner.version = DRW::AC1021;
        owner.binFile = binary;
        if (binary)
            owner.writer = std::make_unique<dxfWriterBinary>(&output);
        else
            owner.writer = std::make_unique<dxfWriterAscii>(&output);
        // Group 999 is intentionally absent from binary DXF; use a normal
        // string group to assert that failed preflight preserves prior bytes.
        t.expect(owner.writer->writeString(1, "PRESERVED_PREFIX"),
                 "GEODATA preflight setup writes prefix");
        const std::string beforeReject = output.str();
        DRW_GeoData invalid = makeLegacy();
        invalid.m_legacy.m_controlFlags = 40000;
        const std::uint32_t invalidHandle = invalid.handle;
        const bool rejected = !owner.writeGeoData(&invalid)
            && output.str() == beforeReject
            && invalid.handle == invalidHandle
            && !owner.m_writeError
            && !owner.writer->hasWriteError();

        // Correct the same object and retry on the same writer.  This checks
        // both that preflight left the caller identity untouched and that no
        // record-scope/output state leaked from the rejected attempt.
        invalid.m_legacy.m_controlFlags = 2;
        const bool wrote = owner.writeGeoData(&invalid)
            && owner.writer->writeString(0, "WIRE_END");
        t.expect(rejected && wrote && invalid.handle == invalidHandle,
                 "unrepresentable v1 flags reject atomically and same-object retry succeeds");
        bool ended = false;
        const std::vector<std::string> projection = numericWireProjection(
            output.str(), binary, ended);
        const std::vector<int> expectedCodes = {
            1, 0, 5, 330, 100, 90, 330, 70, 10, 20, 30, 11, 21, 31,
            40, 41, 42, 43, 44, 45, 46, 52, 210, 220, 230, 301, 302,
            303, 304, 305, 306, 307, 91, 92, 96, 97, 98, 99, 12, 22,
            13, 23, 3, 292, 14, 24, 15, 25, 93, 94, 293, 16, 26, 17,
            27, 54, 140, 95, 141, 294, 142, 143, 4, 0};
        t.expect(wrote && ended && codesOf(projection) == expectedCodes,
                 "v1 ASCII/binary writer emits the complete archival group map");
        if (!binary)
            asciiProjection = projection;
        else
            t.expect(projection == asciiProjection,
                     "v1 ASCII/binary writer values and order are identical");

        std::istringstream input(output.str());
        dxfRW parser("");
        parser.binFile = binary;
        if (binary)
            parser.reader = std::make_unique<dxfReaderBinary>(&input);
        else
            parser.reader = std::make_unique<dxfReaderAscii>(&input);
        DRW_GeoData decoded;
        DRW_RawDxfObject raw;
        bool readingGeoData = false;
        bool parsed = true;
        int code = 0;
        while (parser.reader->readRec(&code)) {
            if (code == 0) {
                const std::string record = parser.reader->getString();
                if (record == "GEODATA") {
                    readingGeoData = true;
                    raw.name = record;
                } else if (readingGeoData || record == "WIRE_END") {
                    break;
                }
                continue;
            }
            if (readingGeoData)
                parsed = parser.captureAndParseRawDxfGroup(raw, code, decoded)
                    && parsed;
        }
        t.expect(parsed && decoded.m_version == 1
                     && decoded.handle == 0x700u
                     && decoded.parentHandle == 0x2u
                     && decoded.m_hostBlockHandle == 0x3u
                     && decoded.m_coordinatesType == 0
                     && decoded.m_legacy.m_controlFlags == 2
                     && decoded.m_designPoint.x == 10.125
                     && decoded.m_designPoint.y == 20.125
                     && decoded.m_designPoint.z == 30.125
                     && decoded.m_legacy.m_secondWcsPoint.x == 11.125
                     && decoded.m_legacy.m_secondWcsPoint.y == 21.125
                     && decoded.m_legacy.m_secondWcsPoint.z == 31.125
                     && decoded.m_legacy.m_geolocation
                         == std::array<double, 3>{{40.125, 41.125, 42.125}}
                     && decoded.m_legacy.m_drawingScales
                         == std::array<double, 3>{{43.125, 44.125, 45.125}}
                     && decoded.m_legacy.m_mapUnits == 46.125
                     && decoded.m_legacy.m_northDirectionAngle == 52.125
                     && decoded.m_upDirection.x == 210.125
                     && decoded.m_upDirection.y == 220.125
                     && decoded.m_upDirection.z == 230.125
                     && decoded.m_coordinateSystemDefinition
                         == "legacy-coordinate-definition"
                     && decoded.m_geoRssTag == "legacy-georss"
                     && decoded.m_legacy.m_coordinateSystemDescription
                         == "legacy-coordinate-description"
                     && decoded.m_observationFromTag == "legacy-observation-tag"
                     && decoded.m_observationToTag == "legacy-observation-to"
                     && decoded.m_observationCoverageTag
                         == "legacy-observation-coverage"
                     && decoded.m_legacy.m_dwgUnits == 91
                     && decoded.m_points.size() == 1u
                     && decoded.m_points.front().m_source.x == 12.125
                     && decoded.m_points.front().m_source.y == 22.125
                     && decoded.m_points.front().m_destination.x == 13.125
                     && decoded.m_points.front().m_destination.y == 23.125
                     && decoded.m_faces.size() == 1u
                     && decoded.m_faces.front().m_index1 == 1
                     && decoded.m_faces.front().m_index2 == 2
                     && decoded.m_faces.front().m_index3 == 3
                     && decoded.m_legacy.m_civil.m_present
                     && decoded.m_legacy.m_civil.m_transformLocalToGrid
                     && decoded.m_legacy.m_civil.m_localReferencePoint
                         == std::array<double, 2>{{14.125, 24.125}}
                     && decoded.m_legacy.m_civil.m_gridReferencePoint
                         == std::array<double, 2>{{15.125, 25.125}}
                     && decoded.m_legacy.m_civil.m_localReferencePointNumber == 1
                     && decoded.m_legacy.m_civil.m_gridReferencePointNumber == 94
                     && decoded.m_legacy.m_civil.m_useRotationPoints
                     && decoded.m_legacy.m_civil.m_localRotationPoint
                         == std::array<double, 2>{{16.125, 26.125}}
                     && decoded.m_legacy.m_civil.m_gridRotationPoint
                         == std::array<double, 2>{{17.125, 27.125}}
                     && decoded.m_legacy.m_civil.m_rotationAngle == 54.125
                     && decoded.m_legacy.m_civil.m_gridAzimuth == 140.125
                     && decoded.m_legacy.m_civil.m_scaleEstimationMethod == 2
                     && decoded.m_legacy.m_civil.m_ambiguous141Value == 141.125
                     && decoded.m_legacy.m_civil.m_enableSeaLevelCorrection
                     && decoded.m_legacy.m_civil.m_seaLevelElevation == 142.125
                     && decoded.m_legacy.m_civil.m_coordinateProjectionRadius == 143.125
                     && decoded.m_legacy.m_coordinateSystemDatum == "legacy-datum",
                 "v1 writer output returns through the typed v1 reader map");
    }

    const std::vector<std::pair<const char*, std::function<void(DRW_GeoData&)>>>
        legacyRejectCases = {
            {"non-finite legacy point", [](DRW_GeoData& geo) {
                geo.m_designPoint.x = std::numeric_limits<double>::quiet_NaN();
            }},
            {"overlong legacy text", [](DRW_GeoData& geo) {
                geo.m_legacy.m_coordinateSystemDatum.assign(256, 'x');
            }},
            {"Civil data without marker", [](DRW_GeoData& geo) {
                geo.m_legacy.m_civil.m_present = false;
                geo.m_legacy.m_civil.m_localReferencePoint[0] = 1.0;
            }},
            {"DWG-only Civil tail", [](DRW_GeoData& geo) {
                geo.m_civilTail.m_present = true;
            }},
            {"nonzero Geo-Mesh Z", [](DRW_GeoData& geo) {
                geo.m_points.front().m_source.z = 1.0;
            }},
            {"oversized Geo-Mesh point count", [](DRW_GeoData& geo) {
                geo.m_points.resize(100001u);
            }},
            {"oversized Geo-Mesh face count", [](DRW_GeoData& geo) {
                geo.m_faces.resize(100001u);
            }}};
    for (const bool binary : {false, true}) {
        for (const auto& rejectCase : legacyRejectCases) {
            std::ostringstream output;
            dxfRW owner("");
            owner.version = DRW::AC1021;
            owner.binFile = binary;
            if (binary)
                owner.writer = std::make_unique<dxfWriterBinary>(&output);
            else
                owner.writer = std::make_unique<dxfWriterAscii>(&output);
            t.expect(owner.writer->writeString(1, "PRESERVED_PREFIX"),
                     "GEODATA negative preflight setup writes prefix");
            const std::string beforeReject = output.str();
            DRW_GeoData invalid = makeLegacy();
            rejectCase.second(invalid);
            const bool rejected = !owner.writeGeoData(&invalid)
                && output.str() == beforeReject
                && invalid.handle == 0x700u
                && !owner.m_writeError
                && !owner.writer->hasWriteError();
            DRW_GeoData valid = makeLegacy();
            const bool retry = owner.writeGeoData(&valid);
            t.expect(rejected && retry && !owner.m_writeError
                         && !owner.writer->hasWriteError(),
                     rejectCase.first);
        }
    }

    for (const std::int32_t objectVersion : {2}) {
        std::vector<std::string> modernAsciiProjection;
        for (const bool binary : {false, true}) {
            std::ostringstream output;
            dxfRW owner("");
            owner.version = DRW::AC1027;
            owner.binFile = binary;
            if (binary)
                owner.writer = std::make_unique<dxfWriterBinary>(&output);
            else
                owner.writer = std::make_unique<dxfWriterAscii>(&output);
            DRW_GeoData modern;
            modern.handle = 0x710u;
            modern.parentHandle = 0x2u;
            modern.m_hostBlockHandle = 0x3u;
            modern.m_version = objectVersion;
            modern.m_coordinatesType = 2;
            modern.m_designPoint = DRW_Coord{10.125, 20.125, 30.125};
            modern.m_referencePoint = DRW_Coord{11.125, 21.125, 31.125};
            modern.m_horizontalUnitScale = 40.125;
            modern.m_verticalUnitScale = 41.125;
            modern.m_horizontalUnits = 91;
            modern.m_verticalUnits = 92;
            modern.m_upDirection = DRW_Coord{210.125, 220.125, 230.125};
            modern.m_northDirection = DRW_Coord{12.125, 22.125, 0.0};
            modern.m_coordinateSystemDefinition = "modern-coordinate-definition";
            modern.m_geoRssTag = "modern-georss";
            modern.m_observationFromTag = "modern-observation-tag";
            modern.m_observationToTag = "modern-observation-to";
            modern.m_observationCoverageTag = "modern-observation-coverage";
            modern.m_points = {{{13.125, 23.125, 0.0}, {14.125, 24.125, 0.0}}};
            modern.m_faces = {{1, 2, 3}};
            const bool wrote = owner.writeGeoData(&modern)
                && owner.writer->writeString(0, "WIRE_END");
            bool ended = false;
            const auto projection = numericWireProjection(output.str(), binary,
                                                          ended);
            const auto codes = codesOf(projection);
            const std::vector<int> expectedCodes = {
                0, 5, 330, 100, 90, 330, 70, 10, 20, 30, 11, 21, 31, 40,
                41, 91, 92, 210, 220, 230, 12, 22, 95, 141, 294, 142, 143,
                301, 302, 305, 306, 307, 93, 13, 23, 14, 24, 96, 97, 98,
                99, 0};
            t.expect(wrote && ended && codes == expectedCodes
                         && std::find(projection.begin(), projection.end(),
                                      "90:" + std::to_string(objectVersion))
                            != projection.end(),
                     "v2/v3 writer controls preserve the modern group order");
            if (!binary)
                modernAsciiProjection = projection;
            else
                t.expect(projection == modernAsciiProjection,
                         "v2/v3 ASCII and binary writer values are identical");
        }
    }

    {
        std::ostringstream output;
        dxfRW owner("");
        owner.version = DRW::AC1027;
        owner.writer = std::make_unique<dxfWriterAscii>(&output);
        t.expect(owner.writer->writeString(1, "PRESERVED_PREFIX"),
                 "unsupported GEODATA version preflight setup writes prefix");
        const std::string beforeReject = output.str();
        DRW_GeoData unsupported;
        unsupported.handle = 0x720u;
        unsupported.m_version = 3;
        t.expect(!owner.writeGeoData(&unsupported)
                     && output.str() == beforeReject
                     && !owner.m_writeError
                     && !owner.writer->hasWriteError(),
                 "unpublished GEODATA version 3 is rejected before output");

        DRW_GeoData defaultVersion;
        defaultVersion.handle = 0x721u;
        const bool wrote = owner.writeGeoData(&defaultVersion)
            && owner.writer->writeString(0, "WIRE_END");
        bool ended = false;
        const auto projection = numericWireProjection(output.str(), false,
                                                      ended);
        t.expect(wrote && ended
                     && std::find(projection.begin(), projection.end(),
                                  "90:2") != projection.end(),
                 "unspecified GEODATA version defaults to documented version 2");
    }

    {
        std::vector<std::string> asciiProjection;
        for (const bool binary : {false, true}) {
            std::ostringstream output;
            dxfRW owner("");
            owner.version = DRW::AC1027;
            owner.binFile = binary;
            owner.writer = binary
                ? std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterBinary>(&output))
                : std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterAscii>(&output));
            DRW_GeoData geo;
            geo.handle = 0x730u;
            geo.m_version = 2;
            std::string definition;
            for (std::size_t i = 0; i < 600; ++i)
                definition.push_back(static_cast<char>('A' + (i % 26u)));
            geo.m_coordinateSystemDefinition = definition;

            const bool wrote = owner.writeGeoData(&geo)
                && owner.writer->writeString(0, "WIRE_END");
            bool ended = false;
            const auto projection = numericWireProjection(output.str(), binary,
                                                          ended);
            std::vector<int> chunkCodes;
            std::vector<std::string> chunks;
            for (const auto& tag : projection) {
                if (tag.rfind("303:", 0) == 0) {
                    chunkCodes.push_back(303);
                    chunks.push_back(tag.substr(4));
                } else if (tag.rfind("301:", 0) == 0) {
                    chunkCodes.push_back(301);
                    chunks.push_back(tag.substr(4));
                }
            }
            const std::vector<std::string> expectedChunks{
                definition.substr(0, 255), definition.substr(255, 255),
                definition.substr(510)};
            std::string joinedDefinition;
            for (const auto& chunk : chunks)
                joinedDefinition += chunk;
            t.expect(wrote && ended
                         && chunkCodes == std::vector<int>{303, 303, 301}
                         && chunks == expectedChunks
                         && joinedDefinition == definition
                         && !projection.empty()
                         && projection.back() == "0:WIRE_END",
                     binary
                         ? "long GEODATA definition preserves exact Binary DXF chunks and sentinel"
                         : "long GEODATA definition preserves exact ASCII DXF chunks and sentinel");
            if (!binary)
                asciiProjection = projection;
            else
                t.expect(projection == asciiProjection,
                         "long GEODATA ASCII and Binary DXF projections are identical");
        }
    }

    {
        std::ostringstream output;
        dxfRW owner("");
        owner.version = DRW::AC1027;
        owner.writer = std::make_unique<dxfWriterAscii>(&output);
        t.expect(owner.writer->writeString(1, "PRESERVED_PREFIX"),
                 "modern GEODATA schema preflight setup writes prefix");
        const std::string beforeReject = output.str();
        DRW_GeoData mixed;
        mixed.handle = 0x722u;
        mixed.m_version = 2;
        mixed.m_legacy.m_controlFlags = 1;
        const bool rejected = !owner.writeGeoData(&mixed)
            && output.str() == beforeReject
            && !owner.m_writeError
            && !owner.writer->hasWriteError();
        mixed.m_legacy.m_controlFlags = 0;
        mixed.m_geoRssTag = "unsafe\ntext";
        const bool unsafeTextRejected = !owner.writeGeoData(&mixed)
            && output.str() == beforeReject
            && !owner.m_writeError
            && !owner.writer->hasWriteError();
        mixed.m_geoRssTag.clear();
        const bool retry = owner.writeGeoData(&mixed);
        t.expect(rejected && unsafeTextRejected && retry,
                 "modern schema rejects legacy-only values then retries");
    }
}

void testGeneratedDictionaryGroupCommonMetadata(TestContext& t) {
    for (const DRW::Version version : {DRW::AC1014, DRW::AC1015,
                                       DRW::AC1021, DRW::AC1032}) {
        for (const bool binary : {false, true}) {
            for (const bool metadata : {false, true}) {
                for (const bool retained : {false, true}) {
                    const auto configure = [metadata, retained](auto& entry) {
                        if (!metadata)
                            return true;
                        entry.reactorHandles = {0x1110u};
                        entry.xDictHandle = 0x2110u;
                        entry.appData.push_back({{102, std::string("{GENERATED_META")},
                            {1, std::string("alpha")}, {70, 7},
                            {320, std::string("1110")}, {102, std::string("}")}});
                        if (retained) {
                            entry.appData.push_back({{102, std::string("ACAD_REACTORS")},
                                {330, std::string("1110")}, {102, std::string("}")}});
                            entry.appData.push_back({{102, std::string("ACAD_XDICTIONARY")},
                                {360, std::string("2110")}, {102, std::string("}")}});
                        }
                        return entry.addExtData(std::make_unique<DRW_Variant>(
                                   1001, std::string("GENERATED_META_APP")))
                            && entry.addExtData(std::make_unique<DRW_Variant>(
                                   1000, std::string("trailing")));
                    };
                    DRW_Dictionary dictionary;
                    dictionary.handle = 0x500u;
                    dictionary.parentHandle = 0xCu;
                    dictionary.m_entries.push_back({"CHILD", 0x700u});
                    DRW_Group group;
                    group.handle = 0x600u;
                    group.name = "GENERATED_GROUP";
                    group.m_entityHandles = {0x900u};
                    t.expect(configure(dictionary) && configure(group),
                             "generated object carriers accept common metadata");
                    std::ostringstream output;
                    dxfRW owner("");
                    owner.version = version;
                    owner.binFile = binary;
                    owner.setNamedDictObjects({dictionary});
                    owner.setGroups({group});
                    owner.setHandleRemap({{0x1110u, 0xA110u}, {0x2110u, 0xB110u}});
                    owner.m_writingContext.sourceHandleToMintedMap.emplace(0x900u, 0x901u);
                    if (binary)
                        owner.writer = std::make_unique<dxfWriterBinary>(&output);
                    else
                        owner.writer = std::make_unique<dxfWriterAscii>(&output);
                    ProfileProbeInterface interface_;
                    owner.iface = &interface_;
                    interface_.dxfW = &owner;
                    t.expect(owner.writeObjects() && owner.writer->writeString(0, "WIRE_END"),
                             "actual generated dictionary/GROUP writer publishes");
                    bool ended = false;
                    const auto tags = numericWireProjection(output.str(), binary, ended);
                    t.expect(ended, "generated object metadata preserves next record");
                    std::vector<std::vector<std::string>> records;
                    for (const auto& tag : tags) {
                        if (tag.rfind("0:", 0) == 0)
                            records.emplace_back();
                        if (!records.empty())
                            records.back().push_back(tag);
                    }
                    int selected = 0;
                    for (const auto& record : records) {
                        const bool named = std::find(record.begin(), record.end(), "5:500") != record.end();
                        const bool grouped = !record.empty() && record.front() == "0:GROUP";
                        if (!named && !grouped)
                            continue;
                        ++selected;
                        const auto subclass = std::find_if(record.begin(), record.end(),
                            [](const auto& tag) { return tag.rfind("100:", 0) == 0; });
                        if (metadata) {
                            for (const char* marker : {"102:{GENERATED_META", "102:{ACAD_REACTORS",
                                                       "102:{ACAD_XDICTIONARY"}) {
                                const auto value = std::find(record.begin(), record.end(), marker);
                                t.expect(value != record.end() && value < subclass
                                             && std::count(record.begin(), record.end(), marker) == 1,
                                         "generated common groups emit once before subclasses");
                            }
                            for (const char* value : {"1:alpha", "70:7", "320:1110",
                                                      "330:A110", "360:B110"})
                                t.expect(std::find(record.begin(), record.end(), value) < subclass,
                                         "generated common fields preserve arbitrary/remapped values");
                            const auto xdata = std::find(record.begin(), record.end(),
                                                         "1001:GENERATED_META_APP");
                            t.expect(xdata != record.end() && record.end() - xdata == 2
                                         && *(xdata + 1) == "1000:trailing",
                                     "generated objects keep XDATA trailing after body");
                        } else {
                            t.expect(std::none_of(record.begin(), record.end(), [](const auto& tag) {
                                return tag.rfind("102:", 0) == 0 || tag.rfind("1001:", 0) == 0;
                            }), "default generated objects gain no metadata groups");
                        }
                        const auto cloningTag = std::find(
                            record.begin(), record.end(),
                            "281:" + std::to_string(dictionary.cloning));
                        const auto cloningTagCount = std::count_if(
                            record.begin(), record.end(), [](const auto& tag) {
                                return tag.rfind("281:", 0) == 0;
                            });
                        const bool dictionaryMatch = version > DRW::AC1014
                            ? cloningTag != record.end() && cloningTagCount == 1
                            : cloningTagCount == 0;
                        const bool defaultsMatch = named
                            ? (dictionaryMatch
                               && std::find(record.begin(), record.end(), "350:700") != record.end())
                            : (std::find(record.begin(), record.end(), "70:0") != record.end()
                               && std::find(record.begin(), record.end(), "71:1") != record.end()
                               && std::find(record.begin(), record.end(), "340:901") != record.end());
                        t.expect(defaultsMatch,
                                 "generated dictionary/GROUP defaults, version gates and member mapping stay intact");
                    }
                    t.expect(selected == 2, "generated path checks named dictionary and GROUP separately");
                }
            }
        }
    }
}

void testCustomTextNumericWireKinds(TestContext& t) {
    expectTypedNumericWire(t, [](dxfRW& writer) {
        DRW_RText text;
        text.text = "RTEXT_AFTER_FLAGS";
        text.m_rTextFlags = 3;
        return writer.writeRText(&text);
    }, {"70:3", "1:RTEXT_AFTER_FLAGS"}, "RTEXT typed numeric sentinel writes");
    expectTypedNumericWire(t, [](dxfRW& writer) {
        DRW_ArcAlignedText text;
        text.text = "ARC_FLAGS";
        text.m_rawColor = 70001;
        text.m_characterSet = 2;
        text.m_pitchAndFamily = 3;
        text.m_isShx = text.m_isBold = text.m_isItalic = text.m_isUnderlined = 1;
        text.m_alignment = 2;
        text.m_isReverse = 1;
        text.m_wizardFlag = 3;
        text.m_textPosition = 2;
        text.m_textDirection = 1;
        return writer.writeArcAlignedText(&text);
    }, {"90:70001", "77:2", "78:3", "79:1", "74:1", "75:1", "76:1",
        "72:2", "70:1", "280:3", "73:2", "71:1", "210:0x0p+0"},
       "ARCALIGNEDTEXT typed numeric sentinel writes");
    for (const int invalid : {-32769, 65536}) {
        expectTypedNumericRangeRejected(t, [invalid](dxfRW& writer) {
            DRW_RText text;
            text.m_rTextFlags = invalid;
            return writer.writeRText(&text);
        });
        const std::array<int DRW_ArcAlignedText::*, 11> fields{
            &DRW_ArcAlignedText::m_characterSet,
            &DRW_ArcAlignedText::m_pitchAndFamily,
            &DRW_ArcAlignedText::m_isShx, &DRW_ArcAlignedText::m_isBold,
            &DRW_ArcAlignedText::m_isItalic, &DRW_ArcAlignedText::m_isUnderlined,
            &DRW_ArcAlignedText::m_alignment, &DRW_ArcAlignedText::m_isReverse,
            &DRW_ArcAlignedText::m_wizardFlag,
            &DRW_ArcAlignedText::m_textPosition,
            &DRW_ArcAlignedText::m_textDirection};
        for (const auto field : fields)
            expectTypedNumericRangeRejected(t, [invalid, field](dxfRW& writer) {
                DRW_ArcAlignedText text;
                text.*field = invalid;
                return writer.writeArcAlignedText(&text);
            });
    }
}

void testLayoutStyleNumericWireKinds(TestContext& t) {
    expectTypedNumericWire(t, [](dxfRW& writer) {
        writer.setHandleRemap({{0xA700u, 0xB700u},
                               {0xA701u, 0xB701u},
                               {0xA702u, 0xB702u},
                               {0xA703u, 0xB703u},
                               {0xA704u, 0xB704u},
                               {0xA705u, 0xB705u}});
        DRW_Layout layout;
        layout.handle = 0xA706u;
        layout.parentHandle = 0xA700u;
        layout.name = "WIRE_LAYOUT_LINKS";
        layout.orthoViewType = 1;
        layout.shadePlotHandle.ref = 0xA702u;
        layout.paperSpaceBlockRecordHandle.ref = 0xA701u;
        layout.lastActiveViewportHandle.ref = 0xA703u;
        layout.namedUcsHandle.ref = 0xA704u;
        layout.baseUcsHandle.ref = 0xA705u;
        return writer.writeLayout(&layout);
    }, {"330:B700", "330:B701", "333:B702", "331:B703",
        "345:B704", "346:B705"},
       "LAYOUT typed structural references use the source handle remap");
    expectTypedNumericWire(t, [](dxfRW& writer) {
        DRW_Layout layout;
        layout.handle = 0x210u;
        layout.name = "WIRE_LAYOUT";
        layout.tabOrder = 3;
        layout.limMinX = 1.25;
        return writer.writeLayout(&layout);
    }, {"71:3", "10:0x1.4p+0"}, "LAYOUT typed numeric sentinel writes");
    expectTypedNumericWire(t, [](dxfRW& writer) {
        DRW_MLeaderStyle style;
        style.handle = 0x211u;
        style.name = "WIRE_MLEADERSTYLE";
        style.styleVersion = 2;
        style.contentType = 2;
        style.drawMLeaderOrder = 1;
        style.drawLeaderOrder = 1;
        style.maxLeaderPoints = 70001;
        style.leaderType = 1;
        style.leftAttachment = 3;
        style.rightAttachment = 4;
        style.textAngleType = 2;
        style.textAlignmentType = 1;
        style.blockConnectionType = 1;
        style.attachmentDirection = 1;
        style.topAttachment = 9;
        style.bottomAttachment = 10;
        style.textExtended = true;
        return writer.writeMLeaderStyle(&style);
    }, {"179:2", "170:2", "171:1", "172:1", "90:70001", "173:1",
        "174:3", "178:4", "175:2", "176:1", "177:1", "271:1",
        "273:9", "272:10", "298:1"},
       "MLEADERSTYLE typed numeric sentinel writes");
    for (const int invalid : {-32769, 65536})
        expectTypedNumericRangeRejected(t, [invalid](dxfRW& writer) {
            DRW_Layout layout;
            layout.handle = 0x212u;
            layout.tabOrder = invalid;
            return writer.writeLayout(&layout);
        });
}

void testEntityNumericWireKinds(TestContext& t) {
    expectTypedNumericWire(t, [](dxfRW& writer) {
        DRW_SectionObject section;
        section.m_state = 1;
        section.m_flags = 2;
        section.m_name = "WIRE_SECTION";
        section.m_indicatorAlpha = 64;
        section.m_indicatorColor = 231;
        section.m_verts.emplace_back(1.0, 2.0, 3.0);
        return writer.writeSectionObject(&section);
    }, {"63:231", "92:1", "11:0x1p+0", "93:0"},
       "SECTIONOBJECT typed numeric sentinel writes");
    expectTypedNumericWire(t, [](dxfRW& writer) {
        DRW_MLeader leader;
        DRW_MLeaderRoot root;
        DRW_MLeaderLeaderLine line;
        line.points.emplace_back(1.0, 2.0, 3.0);
        line.lineWeight = 35;
        line.arrowSize = 1.25;
        root.leaderLines.push_back(line);
        leader.context.roots.push_back(root);
        return writer.writeMultiLeader(&leader);
    }, {"171:35", "40:0x1.4p+0", "305:}"},
       "MLEADER context typed numeric sentinel writes");
    expectTypedNumericWire(t, [](dxfRW& writer) {
        DRW_NavisworksModel model;
        model.flags = 0x1234u;
        model.unitFactor = 0.5;
        return writer.writeNavisworksModel(&model);
    }, {"70:4660", "40:0x1p-1"},
       "coordination model typed numeric sentinel writes");
    for (const std::uint32_t invalid : {65536u, 0xffffffffu})
        expectTypedNumericRangeRejected(t, [invalid](dxfRW& writer) {
            DRW_SectionObject section;
            section.m_indicatorColor = invalid;
            return writer.writeSectionObject(&section);
        });
    for (const int invalid : {-32769, 65536})
        expectTypedNumericRangeRejected(t, [invalid](dxfRW& writer) {
            DRW_MLeader leader;
            DRW_MLeaderRoot root;
            DRW_MLeaderLeaderLine line;
            line.lineWeight = invalid;
            root.leaderLines.push_back(line);
            leader.context.roots.push_back(root);
            return writer.writeMultiLeader(&leader);
        });
}

void testImageObjectNumericWireKinds(TestContext& t) {
    // Version 0 is the ordinary IMAGEDEF value. Retaining a larger caller-
    // supplied integer tests the I32 carrier, not native support for that
    // hypothetical image-definition revision.
    for (const int imageVersion : {0, 70001, 2147483647}) {
        expectTypedNumericWire(t, [imageVersion](dxfRW& writer) {
            ProfileProbeInterface interface_;
            writer.iface = &interface_;
            interface_.dxfW = &writer;
            auto definition = std::make_unique<DRW_ImageDef>();
            definition->handle = 0x300u;
            definition->name = "runtime-only.png";
            definition->imgVersion = imageVersion;
            definition->u = 32.0;
            definition->v = 16.0;
            definition->up = 1.0;
            definition->vp = 1.0;
            definition->reactors.emplace("301", "302");
            writer.imageDef.push_back(definition.release());
            return writer.writeObjects();
        }, {"90:2", "330:302", "90:" + std::to_string(imageVersion),
            "1:runtime-only.png", "10:0x1p+5", "20:0x1p+4"},
           "generated image OBJECTS typed numeric sentinel writes");
    }
}

void testSectionObjectIndicatorColorWire(TestContext& t) {
    const auto expectCmcLossRejected = [&t](
            const std::function<bool(dxfRW&)>& emit) {
        for (const bool binary : {false, true}) {
            std::ostringstream output;
            dxfRW owner("");
            owner.version = DRW::AC1032;
            owner.binFile = binary;
            if (binary)
                owner.writer = std::make_unique<dxfWriterBinary>(&output);
            else
                owner.writer = std::make_unique<dxfWriterAscii>(&output);
            t.expect(!emit(owner) && output.str().empty(),
                     "SECTIONOBJECT rejects unmapped CMC color metadata before output");
        }
    };
    expectTypedNumericWire(t, [](dxfRW& writer) {
        DRW_SectionObject section;
        section.m_indicatorColor = 231u;
        return writer.writeSectionObject(&section);
    }, {"63:231"}, "SECTIONOBJECT indicator color uses group 63");
    expectCmcLossRejected([](dxfRW& writer) {
        DRW_SectionObject section;
        section.m_indicatorColor = 256u;
        section.m_indicatorColorRgb = 0u;
        return writer.writeSectionObject(&section);
    });
    expectCmcLossRejected([](dxfRW& writer) {
        DRW_SectionObject section;
        section.m_indicatorColorName = "unmapped-name";
        return writer.writeSectionObject(&section);
    });
    expectCmcLossRejected([](dxfRW& writer) {
        DRW_SectionObject section;
        section.m_indicatorColorBookName = "unmapped-book";
        return writer.writeSectionObject(&section);
    });
}

void testDxfBinaryRawBoundaryReplay(TestContext& t) {
    const DRW_RawDxfObject object = rawBinaryBoundaryObject();
    std::string output;
    t.expect(writeRawBinaryBoundaryObject(object, output),
             "binary DXF raw boundary object writes");
    t.expect(!output.empty(), "binary DXF raw boundary object commits bytes");

    std::stringstream records(output);
    dxfReaderBinary reader(&records);
    const std::vector<int> expectedCodes {0, 5, 90, 91, 1004};
    const std::vector<dxfReader::TYPE> expectedTypes {
        dxfReader::STRING, dxfReader::STRING, dxfReader::INT32,
        dxfReader::INT32, dxfReader::BINARY};
    int code = 0;
    std::size_t index = 0;
    while (reader.readRec(&code)) {
        t.expect(index < expectedCodes.size() && code == expectedCodes[index],
                 "binary DXF raw replay code order matches source");
        t.expect(index < expectedTypes.size() && reader.type == expectedTypes[index],
                 "binary DXF raw replay type matches classifier");
        if (index == 0)
            t.expect(reader.getString() == "RAW_BINARY_BOUNDARY",
                     "binary DXF raw replay preserves object name");
        if (index == 1)
            t.expect(reader.getString() == "2A",
                     "binary DXF raw replay preserves handle spelling");
        if (index == 2)
            t.expect(reader.getInt32() == 2147483647,
                     "binary DXF raw replay preserves code 90 value");
        if (index == 3)
            t.expect(reader.getInt32() == -7,
                     "binary DXF raw replay preserves code 91 value");
        if (index == 4)
            t.expect(reader.getString() == "ABCD",
                     "binary DXF raw replay preserves code 1004 bytes");
        ++index;
    }
    t.expect(index == expectedCodes.size(),
             "binary DXF raw replay parser consumes complete object");

    const DRW_RawDxfSection section = rawBinaryBoundarySection();
    output.clear();
    t.expect(writeRawBinaryBoundarySection(section, output),
             "binary DXF raw boundary section writes");
    std::stringstream sectionRecords(output);
    dxfReaderBinary sectionReader(&sectionRecords);
    const std::vector<int> expectedSectionCodes {0, 2, 90, 91, 1004, 0};
    index = 0;
    while (sectionReader.readRec(&code)) {
        t.expect(index < expectedSectionCodes.size()
                     && code == expectedSectionCodes[index],
                 "binary DXF raw section framing matches source");
        ++index;
    }
    t.expect(index == expectedSectionCodes.size(),
             "binary DXF raw section parser consumes SECTION and ENDSEC");

    DRW_RawDxfObject badNumeric = object;
    badNumeric.groups[1] = DRW_Variant(90, std::string("2147483647"));
    output.clear();
    t.expect(!writeRawBinaryBoundaryObject(badNumeric, output)
                 && output.empty(),
             "binary DXF raw replay rejects string for typed 32-bit code");
    output.clear();
    t.expect(!writeRawBinaryBoundaryObject(badNumeric, output)
                 && output.empty(),
             "binary DXF raw replay rejects string for typed numeric boundary");

    DRW_RawDxfObject badBinary = object;
    badBinary.groups[3] = DRW_Variant(1004, std::string("ABC"));
    output.clear();
    t.expect(!writeRawBinaryBoundaryObject(badBinary, output)
                 && output.empty(),
             "binary DXF raw replay rejects odd binary chunk");
}

void testDxfBinaryRawSectionCaptureReplay(TestContext& t) {
    const dxfRW::DxfCompatibilityProfile profiles[] = {
        dxfRW::DxfCompatibilityProfile::StandaloneSafe,
        dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy};
    for (const dxfRW::DxfCompatibilityProfile profile : profiles) {
        const bool legacy = profile
            == dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy;
        const int profileCode = legacy ? 482 : 470;
        const int firstNumericCode = legacy ? 260 : 90;
        const int secondNumericCode = legacy ? 269 : 91;
        std::ostringstream source;
        dxfWriterBinary sourceWriter(&source);
        if (legacy)
            sourceWriter.setClassifierProfile(
                DxfClassifierProfile::LibreCadMasterLegacy);
        sourceWriter.writeString(0, "SECTION");
        sourceWriter.writeString(2, "LOCAL_BINARY_CAPTURE");
        if (legacy)
            sourceWriter.writeBool(260, true);
        else
            sourceWriter.writeInt32(firstNumericCode, 2147483647);
        if (legacy)
            sourceWriter.writeBool(269, true);
        else
            sourceWriter.writeInt32(secondNumericCode, -7);
        if (legacy)
            sourceWriter.writeDouble(profileCode, 3.5);
        else
            sourceWriter.writeString(profileCode, "SAFE");
        sourceWriter.writeString(1004, "ABCD");
        sourceWriter.writeString(0, "ENDSEC");
        sourceWriter.writeString(0, "EOF");

        std::stringstream input(source.str());
        ProfileProbeInterface interface_;
        dxfRW reader("");
        reader.binFile = true;
        reader.reader = std::make_unique<dxfReaderBinary>(&input);
        reader.reader->setClassifierProfile(
            legacy ? DxfClassifierProfile::LibreCadMasterLegacy
                   : DxfClassifierProfile::StandaloneSafe);
        reader.iface = &interface_;
        t.expect(reader.processDxf() && interface_.sections.size() == 1,
                 "binary DXF raw section capture publishes one section");
        if (interface_.sections.size() != 1)
            continue;
        const DRW_RawDxfSection& captured = interface_.sections.front();
        t.expect(!captured.m_hasRawValues && captured.m_groups.size() == 4
                     && captured.m_groups[0].type() == DRW_Variant::INTEGER
                     && captured.m_groups[1].type() == DRW_Variant::INTEGER
                     && captured.m_groups[2].code() == profileCode
                     && captured.m_groups[2].type()
                         == (legacy ? DRW_Variant::DOUBLE
                                    : DRW_Variant::STRING)
                     && captured.m_groups[3].type() == DRW_Variant::STRING,
                 "binary DXF profile capture retains canonical carrier types");

        std::ostringstream replay;
        dxfRW writer("");
        writer.setDxfCompatibilityProfile(profile);
        writer.version = DRW::AC1027;
        writer.binFile = true;
        writer.writer = std::make_unique<dxfWriterBinary>(&replay);
        t.expect(writer.writeRawDxfSection(captured),
                 "binary DXF captured raw section replays through profile");
        std::stringstream replayStream(replay.str());
        dxfReaderBinary replayReader(&replayStream);
        replayReader.setClassifierProfile(
            legacy ? DxfClassifierProfile::LibreCadMasterLegacy
                   : DxfClassifierProfile::StandaloneSafe);
        const std::vector<int> expectedCodes {
            0, 2, firstNumericCode, secondNumericCode, profileCode, 1004, 0};
        std::size_t index = 0;
        int code = 0;
        while (replayReader.readRec(&code)) {
            t.expect(index < expectedCodes.size()
                         && code == expectedCodes[index],
                     "binary DXF captured section preserves framing");
            ++index;
        }
        t.expect(index == expectedCodes.size(),
                 "binary DXF captured section replay consumes complete frame");

        DRW_RawDxfSection malformed = captured;
        malformed.m_groups[2] = legacy
            ? DRW_Variant(profileCode, std::string("3.5"))
            : DRW_Variant(profileCode, static_cast<std::int32_t>(7));
        std::ostringstream rejected;
        dxfRW rejectingWriter("");
        rejectingWriter.setDxfCompatibilityProfile(profile);
        rejectingWriter.version = DRW::AC1027;
        rejectingWriter.binFile = true;
        rejectingWriter.writer = std::make_unique<dxfWriterBinary>(&rejected);
        t.expect(!rejectingWriter.writeRawDxfSection(malformed)
                     && rejected.str().empty(),
                 "binary DXF malformed profile section rolls back output");
    }
}

void testDxfBinaryRawObjectCaptureReplay(TestContext& t) {
    const dxfRW::DxfCompatibilityProfile profiles[] = {
        dxfRW::DxfCompatibilityProfile::StandaloneSafe,
        dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy};
    for (const dxfRW::DxfCompatibilityProfile profile : profiles) {
        const bool legacy = profile
            == dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy;
        const int profileCode = legacy ? 482 : 470;
        const int firstNumericCode = legacy ? 260 : 90;
        const int secondNumericCode = legacy ? 269 : 91;
        std::ostringstream source;
        dxfWriterBinary sourceWriter(&source);
        if (legacy)
            sourceWriter.setClassifierProfile(
                DxfClassifierProfile::LibreCadMasterLegacy);
        sourceWriter.writeString(0, "SECTION");
        sourceWriter.writeString(2, "OBJECTS");
        sourceWriter.writeString(0, "LOCAL_BINARY_OBJECT");
        sourceWriter.writeString(5, "2A");
        if (legacy) {
            sourceWriter.writeBool(260, true);
            sourceWriter.writeBool(269, true);
        } else {
            sourceWriter.writeInt32(firstNumericCode, 2147483647);
            sourceWriter.writeInt32(secondNumericCode, -7);
        }
        if (legacy)
            sourceWriter.writeDouble(profileCode, 3.5);
        else
            sourceWriter.writeString(profileCode, "SAFE");
        sourceWriter.writeString(1004, "ABCD");
        sourceWriter.writeString(0, "ENDSEC");
        sourceWriter.writeString(0, "EOF");

        std::stringstream input(source.str());
        ProfileProbeInterface interface_;
        dxfRW reader("");
        reader.binFile = true;
        reader.reader = std::make_unique<dxfReaderBinary>(&input);
        reader.reader->setClassifierProfile(
            legacy ? DxfClassifierProfile::LibreCadMasterLegacy
                   : DxfClassifierProfile::StandaloneSafe);
        reader.iface = &interface_;
        t.expect(reader.processDxf() && interface_.objects.size() == 1,
                 "binary DXF raw object capture publishes one object");
        if (interface_.objects.size() != 1)
            continue;
        const DRW_RawDxfObject& captured = interface_.objects.front();
        t.expect(captured.groups.size() == 5
                     && captured.groups[0].code() == 5
                     && captured.groups[1].code() == firstNumericCode
                     && captured.groups[2].code() == secondNumericCode
                     && captured.groups[3].code() == profileCode
                     && captured.groups[4].code() == 1004
                     && captured.groups[0].type() == DRW_Variant::STRING
                     && captured.groups[1].type() == DRW_Variant::INTEGER
                     && captured.groups[2].type() == DRW_Variant::INTEGER
                     && captured.groups[3].type()
                         == (legacy ? DRW_Variant::DOUBLE
                                    : DRW_Variant::STRING),
                 "binary DXF raw object capture retains profile carriers");

        std::ostringstream replay;
        dxfRW writer("");
        writer.setDxfCompatibilityProfile(profile);
        writer.version = DRW::AC1027;
        writer.binFile = true;
        writer.writer = std::make_unique<dxfWriterBinary>(&replay);
        DRW_RawDxfObject copy = captured;
        t.expect(writer.writeRawDxfObject(&copy),
                 "binary DXF raw object replays through profile");
        std::stringstream replayStream(replay.str());
        dxfReaderBinary replayReader(&replayStream);
        replayReader.setClassifierProfile(
            legacy ? DxfClassifierProfile::LibreCadMasterLegacy
                   : DxfClassifierProfile::StandaloneSafe);
        const std::vector<int> expectedCodes {
            0, 5, firstNumericCode, secondNumericCode, profileCode, 1004};
        std::size_t index = 0;
        int code = 0;
        while (replayReader.readRec(&code)) {
            t.expect(index < expectedCodes.size()
                         && code == expectedCodes[index],
                     "binary DXF raw object preserves code order");
            ++index;
        }
        t.expect(index == expectedCodes.size(),
                 "binary DXF raw object replay consumes complete record");

        DRW_RawDxfObject malformed = captured;
        malformed.groups[4] = DRW_Variant(1004, std::string("ABC"));
        std::ostringstream rejected;
        dxfRW rejectingWriter("");
        rejectingWriter.setDxfCompatibilityProfile(profile);
        rejectingWriter.version = DRW::AC1027;
        rejectingWriter.binFile = true;
        rejectingWriter.writer = std::make_unique<dxfWriterBinary>(&rejected);
        t.expect(!rejectingWriter.writeRawDxfObject(&malformed)
                     && rejected.str().empty(),
                 "binary DXF malformed raw object rolls back output");
    }
}

void testDxfRawObjectHandleScope(TestContext& t) {
    const std::string duplicateRecords =
        "0\nSECTION\n2\nOBJECTS\n0\nLOCAL_FIRST\n5\n1A\n"
        "0\nLOCAL_SECOND\n5\n1A\n0\nENDSEC\n0\nEOF\n";
    ProfileProbeInterface duplicateInterface;
    dxfRW duplicateReader("");
    std::string duplicateInput = duplicateRecords;
    t.expect(!duplicateReader.readAscii(
                  &duplicateInterface, false, duplicateInput)
                 && duplicateInterface.objects.size() == 1
                 && duplicateInterface.objects.front().name == "LOCAL_FIRST",
             "DXF duplicate handles reject only the later raw object");

    const std::string duplicateSections =
        "0\nSECTION\n2\nOBJECTS\n0\nLOCAL_SECTION_FIRST\n5\n2A\n"
        "0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n0\nLOCAL_SECTION_SECOND\n"
        "5\n2A\n0\nENDSEC\n0\nEOF\n";
    ProfileProbeInterface sectionInterface;
    dxfRW sectionReader("");
    std::string sectionInput = duplicateSections;
    t.expect(!sectionReader.readAscii(&sectionInterface, false, sectionInput)
                 && sectionInterface.objects.size() == 1
                 && sectionInterface.objects.front().name
                        == "LOCAL_SECTION_FIRST",
             "DXF duplicate handles remain unique across sections");

    const std::string singleObject =
        "0\nSECTION\n2\nOBJECTS\n0\nLOCAL_FRESH\n5\n1A\n"
        "0\nENDSEC\n0\nEOF\n";
    ProfileProbeInterface firstSession;
    ProfileProbeInterface secondSession;
    dxfRW freshReader("");
    std::string firstInput = singleObject;
    std::string secondInput = singleObject;
    t.expect(freshReader.readAscii(&firstSession, false, firstInput)
                 && firstSession.objects.size() == 1,
             "DXF first handle session accepts the object");
    t.expect(freshReader.readAscii(&secondSession, false, secondInput)
                 && secondSession.objects.size() == 1,
             "DXF fresh read session resets handle uniqueness");

    std::ostringstream binarySource;
    dxfWriterBinary binaryWriter(&binarySource);
    binaryWriter.writeString(0, "SECTION");
    binaryWriter.writeString(2, "OBJECTS");
    binaryWriter.writeString(0, "LOCAL_BINARY_FIRST");
    binaryWriter.writeString(5, "1A");
    binaryWriter.writeString(0, "LOCAL_BINARY_SECOND");
    binaryWriter.writeString(5, "1A");
    binaryWriter.writeString(0, "ENDSEC");
    binaryWriter.writeString(0, "EOF");
    std::stringstream binaryInput(binarySource.str());
    ProfileProbeInterface binaryInterface;
    dxfRW binaryReader("");
    binaryReader.binFile = true;
    binaryReader.reader = std::make_unique<dxfReaderBinary>(&binaryInput);
    binaryReader.reader->setClassifierProfile(
        DxfClassifierProfile::StandaloneSafe);
    binaryReader.iface = &binaryInterface;
    t.expect(!binaryReader.processDxf()
                 && binaryInterface.objects.size() == 1
                 && binaryInterface.objects.front().name
                        == "LOCAL_BINARY_FIRST",
             "binary DXF duplicate handles share record scope policy");
}

void testDxfRawObjectHandleDiagnostics(TestContext& t) {
    const std::string duplicateRecords =
        "0\nSECTION\n2\nOBJECTS\n0\nLOCAL_DIAG_FIRST\n5\n1A\n"
        "0\nLOCAL_DIAG_SECOND\n5\n1A\n0\nENDSEC\n0\nEOF\n";
    ProfileProbeInterface interface_;
    dxfRW reader("");
    std::string input = duplicateRecords;
    t.expect(!reader.readAscii(&interface_, false, input)
                 && reader.getError() == DRW::BAD_CODE_PARSED
                 && interface_.objects.size() == 1,
             "DXF duplicate handle keeps legacy parse error and prior callback");
    const DRW_OperationDiagnostic diagnostic = reader.getLastDiagnostic();
    t.expect(diagnostic.operation == DRW::OperationKind::Read
                 && diagnostic.phase == DRW::OperationPhase::Validation
                 && diagnostic.cause == DRW::OperationCause::ValidationFailure
                 && diagnostic.code == "duplicate-handle"
                 && diagnostic.hasHandle && diagnostic.handle == 0x1Au
                 && diagnostic.message.find("self handle") != std::string::npos,
             "DXF duplicate handle records structured validation context");

    std::ostringstream binarySource;
    dxfWriterBinary binaryWriter(&binarySource);
    binaryWriter.writeString(0, "SECTION");
    binaryWriter.writeString(2, "OBJECTS");
    binaryWriter.writeString(0, "LOCAL_BINARY_DIAG_FIRST");
    binaryWriter.writeString(5, "2A");
    binaryWriter.writeString(0, "LOCAL_BINARY_DIAG_SECOND");
    binaryWriter.writeString(5, "2A");
    binaryWriter.writeString(0, "ENDSEC");
    binaryWriter.writeString(0, "EOF");
    std::stringstream binaryInput(binarySource.str());
    ProfileProbeInterface binaryInterface;
    dxfRW binaryReader("");
    binaryReader.binFile = true;
    binaryReader.reader = std::make_unique<dxfReaderBinary>(&binaryInput);
    binaryReader.reader->setClassifierProfile(
        DxfClassifierProfile::StandaloneSafe);
    binaryReader.iface = &binaryInterface;
    binaryReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(!binaryReader.processDxf()
                 && binaryReader.getError() == DRW::BAD_CODE_PARSED
                 && binaryInterface.objects.size() == 1,
             "binary DXF duplicate handle keeps legacy error and callback");
    const DRW_OperationDiagnostic binaryDiagnostic =
        binaryReader.getLastDiagnostic();
    t.expect(binaryDiagnostic.operation == DRW::OperationKind::Read
                 && binaryDiagnostic.phase == DRW::OperationPhase::Validation
                 && binaryDiagnostic.cause
                        == DRW::OperationCause::ValidationFailure
                 && binaryDiagnostic.code == "duplicate-handle"
                 && binaryDiagnostic.hasHandle
                 && binaryDiagnostic.handle == 0x2Au,
             "binary DXF duplicate handle records offending handle");
}

void testDxfRawObjectMalformedHandleDiagnostics(TestContext& t) {
    const std::string malformedRecords =
        "0\nSECTION\n2\nOBJECTS\n0\nLOCAL_BAD_HANDLE\n"
        "5\nnot-hex\n0\nENDSEC\n0\nEOF\n";
    ProfileProbeInterface interface_;
    dxfRW reader("");
    std::string input = malformedRecords;
    t.expect(!reader.readAscii(&interface_, false, input)
                 && reader.getError() == DRW::BAD_CODE_PARSED
                 && interface_.objects.empty(),
             "DXF malformed handle keeps parse error and suppresses callback");
    const DRW_OperationDiagnostic diagnostic = reader.getLastDiagnostic();
    t.expect(diagnostic.operation == DRW::OperationKind::Read
                 && diagnostic.phase == DRW::OperationPhase::Validation
                 && diagnostic.cause == DRW::OperationCause::ValidationFailure
                 && diagnostic.code == "invalid-handle"
                 && !diagnostic.hasHandle
                 && diagnostic.message.find("invalid self handle")
                        != std::string::npos,
             "DXF malformed handle records bounded validation context");

    std::ostringstream binarySource;
    dxfWriterBinary binaryWriter(&binarySource);
    binaryWriter.writeString(0, "SECTION");
    binaryWriter.writeString(2, "OBJECTS");
    binaryWriter.writeString(0, "LOCAL_BINARY_BAD_HANDLE");
    binaryWriter.writeString(5, "123456789ABCDEF01");
    binaryWriter.writeString(0, "ENDSEC");
    binaryWriter.writeString(0, "EOF");
    std::stringstream binaryInput(binarySource.str());
    ProfileProbeInterface binaryInterface;
    dxfRW binaryReader("");
    binaryReader.binFile = true;
    binaryReader.reader = std::make_unique<dxfReaderBinary>(&binaryInput);
    binaryReader.reader->setClassifierProfile(
        DxfClassifierProfile::StandaloneSafe);
    binaryReader.iface = &binaryInterface;
    binaryReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(!binaryReader.processDxf()
                 && binaryReader.getError() == DRW::BAD_CODE_PARSED
                 && binaryInterface.objects.empty(),
             "binary overlength handle keeps parse error and callback policy");
    const DRW_OperationDiagnostic binaryDiagnostic =
        binaryReader.getLastDiagnostic();
    t.expect(binaryDiagnostic.operation == DRW::OperationKind::Read
                 && binaryDiagnostic.phase == DRW::OperationPhase::Validation
                 && binaryDiagnostic.cause
                        == DRW::OperationCause::ValidationFailure
                 && binaryDiagnostic.code == "invalid-handle"
                 && !binaryDiagnostic.hasHandle,
             "binary overlength handle records invalid-handle diagnostic");

    const std::string malformedOwnerRecords =
        "0\nSECTION\n2\nOBJECTS\n0\nLOCAL_BAD_OWNER\n5\n3A\n"
        "330\nnot-owner\n0\nENDSEC\n0\nEOF\n";
    ProfileProbeInterface ownerInterface;
    dxfRW ownerReader("");
    std::string ownerInput = malformedOwnerRecords;
    t.expect(!ownerReader.readAscii(&ownerInterface, false, ownerInput)
                 && ownerReader.getError() == DRW::BAD_READ_OBJECTS
                 && ownerInterface.objects.empty(),
             "DXF malformed owner handle suppresses the raw callback");
    const DRW_OperationDiagnostic ownerDiagnostic =
        ownerReader.getLastDiagnostic();
    t.expect(ownerDiagnostic.code == "invalid-handle"
                 && ownerDiagnostic.message.find("handle reference")
                        != std::string::npos,
             "DXF owner handle diagnostic identifies reference context");

    std::ostringstream binaryOwnerSource;
    dxfWriterBinary binaryOwnerWriter(&binaryOwnerSource);
    binaryOwnerWriter.writeString(0, "SECTION");
    binaryOwnerWriter.writeString(2, "OBJECTS");
    binaryOwnerWriter.writeString(0, "LOCAL_BINARY_BAD_OWNER");
    binaryOwnerWriter.writeString(5, "3A");
    binaryOwnerWriter.writeString(330, "not-owner");
    binaryOwnerWriter.writeString(0, "ENDSEC");
    binaryOwnerWriter.writeString(0, "EOF");
    std::stringstream binaryOwnerInput(binaryOwnerSource.str());
    ProfileProbeInterface binaryOwnerInterface;
    dxfRW binaryOwnerReader("");
    binaryOwnerReader.binFile = true;
    binaryOwnerReader.reader =
        std::make_unique<dxfReaderBinary>(&binaryOwnerInput);
    binaryOwnerReader.reader->setClassifierProfile(
        DxfClassifierProfile::StandaloneSafe);
    binaryOwnerReader.iface = &binaryOwnerInterface;
    binaryOwnerReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(!binaryOwnerReader.processDxf()
                 && binaryOwnerReader.getError() == DRW::BAD_READ_OBJECTS
                 && binaryOwnerInterface.objects.empty(),
             "binary malformed owner handle preserves callback policy");
    const DRW_OperationDiagnostic binaryOwnerDiagnostic =
        binaryOwnerReader.getLastDiagnostic();
    t.expect(binaryOwnerDiagnostic.code == "invalid-handle"
                 && binaryOwnerDiagnostic.message.find("handle reference")
                        != std::string::npos,
             "binary owner handle diagnostic identifies reference context");
}

void testDxfRawEntityHandleDiagnostics(TestContext& t) {
    const std::string malformedEntityRecords =
        "0\nSECTION\n2\nENTITIES\n0\nLOCAL_RAW_ENTITY\n5\n4A\n"
        "330\nnot-owner\n0\nENDSEC\n0\nEOF\n";
    ProfileProbeInterface interface_;
    dxfRW reader("");
    std::string input = malformedEntityRecords;
    t.expect(!reader.readAscii(&interface_, false, input)
                 && reader.getError() == DRW::BAD_READ_ENTITIES
                 && interface_.entities.empty(),
             "DXF malformed raw entity handle preserves stage and callback policy");
    const DRW_OperationDiagnostic diagnostic = reader.getLastDiagnostic();
    t.expect(diagnostic.operation == DRW::OperationKind::Read
                 && diagnostic.phase == DRW::OperationPhase::Validation
                 && diagnostic.cause == DRW::OperationCause::ValidationFailure
                 && diagnostic.code == "invalid-handle"
                 && diagnostic.message.find("handle reference")
                        != std::string::npos,
             "DXF raw entity carries field-context handle diagnostic");

    std::ostringstream binarySource;
    dxfWriterBinary binaryWriter(&binarySource);
    binaryWriter.writeString(0, "SECTION");
    binaryWriter.writeString(2, "ENTITIES");
    binaryWriter.writeString(0, "LOCAL_BINARY_RAW_ENTITY");
    binaryWriter.writeString(5, "4A");
    binaryWriter.writeString(330, "not-owner");
    binaryWriter.writeString(0, "ENDSEC");
    binaryWriter.writeString(0, "EOF");
    std::stringstream binaryInput(binarySource.str());
    ProfileProbeInterface binaryInterface;
    dxfRW binaryReader("");
    binaryReader.binFile = true;
    binaryReader.reader = std::make_unique<dxfReaderBinary>(&binaryInput);
    binaryReader.reader->setClassifierProfile(
        DxfClassifierProfile::StandaloneSafe);
    binaryReader.iface = &binaryInterface;
    binaryReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(!binaryReader.processDxf()
                 && binaryReader.getError() == DRW::BAD_READ_ENTITIES
                 && binaryInterface.entities.empty(),
             "binary malformed raw entity handle preserves stage and callback");
    const DRW_OperationDiagnostic binaryDiagnostic =
        binaryReader.getLastDiagnostic();
    t.expect(binaryDiagnostic.operation == DRW::OperationKind::Read
                 && binaryDiagnostic.phase == DRW::OperationPhase::Validation
                 && binaryDiagnostic.cause
                        == DRW::OperationCause::ValidationFailure
                 && binaryDiagnostic.code == "invalid-handle"
                 && binaryDiagnostic.message.find("handle reference")
                        != std::string::npos,
             "binary raw entity carries field-context handle diagnostic");

    const std::string duplicateEntityRecords =
        "0\nSECTION\n2\nENTITIES\n0\nLOCAL_ENTITY_FIRST\n5\n5A\n"
        "0\nLOCAL_ENTITY_SECOND\n5\n5A\n0\nENDSEC\n0\nEOF\n";
    ProfileProbeInterface duplicateInterface;
    dxfRW duplicateReader("");
    std::string duplicateInput = duplicateEntityRecords;
    t.expect(!duplicateReader.readAscii(&duplicateInterface, false,
                                        duplicateInput)
                 && duplicateReader.getError() == DRW::BAD_CODE_PARSED
                 && duplicateInterface.entities.size() == 1,
             "DXF raw entity duplicate preserves error and prior callback");
    const DRW_OperationDiagnostic duplicateDiagnostic =
        duplicateReader.getLastDiagnostic();
    t.expect(duplicateDiagnostic.code == "duplicate-handle"
                 && duplicateDiagnostic.hasHandle
                 && duplicateDiagnostic.handle == 0x5Au
                 && duplicateDiagnostic.message.find("self handle")
                        != std::string::npos,
             "DXF raw entity duplicate records structured handle context");

    const std::string duplicateEntitySections =
        "0\nSECTION\n2\nENTITIES\n0\nLOCAL_ENTITY_SECTION_FIRST\n5\n6A\n"
        "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
        "0\nLOCAL_ENTITY_SECTION_SECOND\n5\n6A\n0\nENDSEC\n0\nEOF\n";
    ProfileProbeInterface sectionInterface;
    dxfRW sectionReader("");
    std::string sectionInput = duplicateEntitySections;
    t.expect(!sectionReader.readAscii(&sectionInterface, false, sectionInput)
                 && sectionReader.getError() == DRW::BAD_CODE_PARSED
                 && sectionInterface.entities.size() == 1,
             "DXF raw entity duplicates remain scoped across sections");

    const std::string singleEntity =
        "0\nSECTION\n2\nENTITIES\n0\nLOCAL_ENTITY_FRESH\n5\n5A\n"
        "0\nENDSEC\n0\nEOF\n";
    ProfileProbeInterface firstSession;
    ProfileProbeInterface secondSession;
    dxfRW freshReader("");
    std::string firstInput = singleEntity;
    std::string secondInput = singleEntity;
    t.expect(freshReader.readAscii(&firstSession, false, firstInput)
                 && firstSession.entities.size() == 1,
             "DXF raw entity first session accepts the handle");
    t.expect(freshReader.readAscii(&secondSession, false, secondInput)
                 && secondSession.entities.size() == 1,
             "DXF raw entity fresh session resets handle scope");

    std::ostringstream binaryDuplicateSource;
    dxfWriterBinary binaryDuplicateWriter(&binaryDuplicateSource);
    binaryDuplicateWriter.writeString(0, "SECTION");
    binaryDuplicateWriter.writeString(2, "ENTITIES");
    binaryDuplicateWriter.writeString(0, "LOCAL_BINARY_ENTITY_FIRST");
    binaryDuplicateWriter.writeString(5, "5A");
    binaryDuplicateWriter.writeString(0, "LOCAL_BINARY_ENTITY_SECOND");
    binaryDuplicateWriter.writeString(5, "5A");
    binaryDuplicateWriter.writeString(0, "ENDSEC");
    binaryDuplicateWriter.writeString(0, "EOF");
    std::stringstream binaryDuplicateInput(binaryDuplicateSource.str());
    ProfileProbeInterface binaryDuplicateInterface;
    dxfRW binaryDuplicateReader("");
    binaryDuplicateReader.binFile = true;
    binaryDuplicateReader.reader = std::make_unique<dxfReaderBinary>(
        &binaryDuplicateInput);
    binaryDuplicateReader.reader->setClassifierProfile(
        DxfClassifierProfile::StandaloneSafe);
    binaryDuplicateReader.iface = &binaryDuplicateInterface;
    binaryDuplicateReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(!binaryDuplicateReader.processDxf()
                 && binaryDuplicateReader.getError() == DRW::BAD_CODE_PARSED
                 && binaryDuplicateInterface.entities.size() == 1,
             "binary raw entity duplicate preserves prior callback");
    const DRW_OperationDiagnostic binaryDuplicateDiagnostic =
        binaryDuplicateReader.getLastDiagnostic();
    t.expect(binaryDuplicateDiagnostic.code == "duplicate-handle"
                 && binaryDuplicateDiagnostic.hasHandle
                 && binaryDuplicateDiagnostic.handle == 0x5Au,
             "binary raw entity duplicate records handle context");
}

void testDxfRawEntityWideHandleReplay(TestContext& t) {
    const std::string wideHandle = "123456789ABCDEF0";
    const std::string content =
        "0\nSECTION\n2\nENTITIES\n0\nLOCAL_WIDE_ENTITY\n5\n"
        + wideHandle + "\n0\nENDSEC\n0\nEOF\n";
    ProfileProbeInterface interface_;
    dxfRW reader("");
    std::string input = content;
    t.expect(reader.readAscii(&interface_, false, input)
                 && interface_.entities.size() == 1,
             "DXF raw entity captures a wide self handle");
    if (interface_.entities.size() != 1)
        return;
    const DRW_RawDxfObject& captured = interface_.entities.front();
    t.expect(captured.groups.size() == 1
                 && captured.groups.front().code() == 5
                 && captured.groups.front().type() == DRW_Variant::STRING
                 && std::string(captured.groups.front().c_str()) == wideHandle
                 && captured.rawValues.size() == 1
                 && captured.rawValues.front() == wideHandle
                 && captured.handle == 0,
             "DXF raw entity preserves wide lexeme without narrowing");

    std::ostringstream asciiReplay;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiReplay);
    DRW_RawDxfObject asciiCopy = captured;
    asciiCopy.m_version = DRW::AC1027;
    t.expect(asciiWriter.writeRawDxfObject(&asciiCopy),
             "DXF raw entity wide handle replays through ASCII");
    std::stringstream asciiRecords(asciiReplay.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    int code = 0;
    t.expect(asciiReader.readRec(&code) && code == 0
                 && asciiReader.getString() == "LOCAL_WIDE_ENTITY"
                 && asciiReader.readRec(&code) && code == 5
                 && asciiReader.getString() == wideHandle,
             "DXF ASCII replay retains the wide self-handle spelling");

    std::ostringstream binarySource;
    dxfWriterBinary binarySourceWriter(&binarySource);
    binarySourceWriter.writeString(0, "SECTION");
    binarySourceWriter.writeString(2, "ENTITIES");
    binarySourceWriter.writeString(0, "LOCAL_WIDE_ENTITY");
    binarySourceWriter.writeString(5, wideHandle);
    binarySourceWriter.writeString(0, "ENDSEC");
    binarySourceWriter.writeString(0, "EOF");
    std::stringstream binaryInput(binarySource.str());
    ProfileProbeInterface binaryInterface;
    dxfRW binaryReader("");
    binaryReader.binFile = true;
    binaryReader.reader = std::make_unique<dxfReaderBinary>(&binaryInput);
    binaryReader.reader->setClassifierProfile(
        DxfClassifierProfile::StandaloneSafe);
    binaryReader.iface = &binaryInterface;
    binaryReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(binaryReader.processDxf()
                 && binaryInterface.entities.size() == 1,
             "binary DXF raw entity captures a wide self handle");
    if (binaryInterface.entities.size() == 1) {
        const DRW_RawDxfObject& binaryCaptured = binaryInterface.entities.front();
        t.expect(binaryCaptured.groups.size() == 1
                     && binaryCaptured.groups.front().code() == 5
                     && std::string(binaryCaptured.groups.front().c_str())
                            == wideHandle
                     && binaryCaptured.handle == 0,
                 "binary DXF raw entity keeps wide handle un-narrowed");

        std::ostringstream binaryReplay;
        dxfRW binaryWriter("");
        binaryWriter.version = DRW::AC1027;
        binaryWriter.binFile = true;
        binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryReplay);
        DRW_RawDxfObject binaryCopy = binaryCaptured;
        binaryCopy.m_version = DRW::AC1027;
        t.expect(binaryWriter.writeRawDxfObject(&binaryCopy),
                 "DXF raw entity wide handle replays through binary");
        std::stringstream binaryReplayInput(binaryReplay.str());
        dxfReaderBinary binaryReplayReader(&binaryReplayInput);
        binaryReplayReader.setClassifierProfile(
            DxfClassifierProfile::StandaloneSafe);
        t.expect(binaryReplayReader.readRec(&code) && code == 0
                     && binaryReplayReader.getString() == "LOCAL_WIDE_ENTITY"
                     && binaryReplayReader.readRec(&code) && code == 5
                     && binaryReplayReader.getString() == wideHandle,
                 "DXF binary replay retains the wide self-handle spelling");
    }
}

void testDxfRawEntityHandleRemap(TestContext& t) {
    const std::string wideHandle = "123456789ABCDEF0";
    const std::string wideReference = "FEDCBA9876543210";
    DRW_RawDxfObject object;
    object.name = "LOCAL_REMAP_ENTITY";
    object.handle = 0x1Au;
    object.parentHandle = 0x2Au;
    object.m_version = DRW::AC1027;
    object.hasRawValues = true;
    object.groups = {DRW_Variant(5, std::string("1A")),
                     DRW_Variant(330, std::string("2A")),
                     DRW_Variant(340, wideReference)};
    object.rawValues = {"1A", "2A", wideReference};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    asciiWriter.setHandleRemap({{0x1Au, 0x3Au}, {0x2Au, 0x4Au}});
    t.expect(asciiWriter.writeRawDxfObject(&object),
             "DXF raw entity remaps narrow handles through ASCII replay");
    int code = 0;
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    asciiReader.setAllowWideHandleLexemes(true);
    t.expect(asciiReader.readRec(&code) && code == 0
                 && asciiReader.readRec(&code) && code == 5
                 && asciiReader.getString() == "3A"
                 && asciiReader.readRec(&code) && code == 330
                 && asciiReader.getString() == "4A"
                 && asciiReader.readRec(&code) && code == 340
                 && asciiReader.getString() == wideReference,
             "DXF ASCII remap preserves wide reference identity verbatim");

    DRW_RawDxfObject wideObject;
    wideObject.name = "LOCAL_WIDE_REMAP_ENTITY";
    wideObject.m_version = DRW::AC1027;
    wideObject.groups = {DRW_Variant(5, wideHandle),
                         DRW_Variant(330, wideReference)};
    std::ostringstream wideOutput;
    dxfRW wideWriter("");
    wideWriter.version = DRW::AC1027;
    wideWriter.binFile = false;
    wideWriter.writer = std::make_unique<dxfWriterAscii>(&wideOutput);
    wideWriter.setHandleRemap({{0x1Au, 0x3Au}});
    t.expect(wideWriter.writeRawDxfObject(&wideObject),
             "DXF wide raw entity ignores non-representable remap keys");
    std::stringstream wideRecords(wideOutput.str());
    dxfReaderAscii wideReader(&wideRecords);
    wideReader.setAllowWideHandleLexemes(true);
    t.expect(wideReader.readRec(&code) && code == 0
                 && wideReader.readRec(&code) && code == 5
                 && wideReader.getString() == wideHandle
                 && wideReader.readRec(&code) && code == 330
                 && wideReader.getString() == wideReference,
             "DXF wide ASCII self/reference handles remain lossless");

    DRW_RawDxfObject binaryObject = object;
    binaryObject.hasRawValues = false;
    binaryObject.rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    binaryWriter.setHandleRemap({{0x1Au, 0x3Au}, {0x2Au, 0x4Au}});
    t.expect(binaryWriter.writeRawDxfObject(&binaryObject),
             "DXF raw entity remaps narrow handles through binary replay");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    binaryReader.setAllowWideHandleLexemes(true);
    t.expect(binaryReader.readRec(&code) && code == 0
                 && binaryReader.readRec(&code) && code == 5
                 && binaryReader.getString() == "3A"
                 && binaryReader.readRec(&code) && code == 330
                 && binaryReader.getString() == "4A"
                 && binaryReader.readRec(&code) && code == 340
                 && binaryReader.getString() == wideReference,
             "DXF binary remap preserves wide reference identity verbatim");

    DRW_RawDxfObject malformedAscii = object;
    malformedAscii.groups.push_back(
        DRW_Variant(90, std::string("not-an-int")));
    malformedAscii.rawValues.push_back("not-an-int");
    std::ostringstream rejectedAsciiOutput;
    dxfRW rejectingAsciiWriter("");
    rejectingAsciiWriter.version = DRW::AC1027;
    rejectingAsciiWriter.binFile = false;
    rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedAsciiOutput);
    rejectingAsciiWriter.setHandleRemap({{0x1Au, 0x3Au}, {0x2Au, 0x4Au}});
    t.expect(!rejectingAsciiWriter.writeRawDxfObject(&malformedAscii)
                 && rejectedAsciiOutput.str().empty(),
             "DXF ASCII remap rolls back when a trailing group is malformed");

    DRW_RawDxfObject malformedBinary = binaryObject;
    malformedBinary.groups.push_back(DRW_Variant(1004, std::string("ABC")));
    std::ostringstream rejectedBinaryOutput;
    dxfRW rejectingBinaryWriter("");
    rejectingBinaryWriter.version = DRW::AC1027;
    rejectingBinaryWriter.binFile = true;
    rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
        &rejectedBinaryOutput);
    rejectingBinaryWriter.setHandleRemap({{0x1Au, 0x3Au}, {0x2Au, 0x4Au}});
    t.expect(!rejectingBinaryWriter.writeRawDxfObject(&malformedBinary)
                 && rejectedBinaryOutput.str().empty(),
             "DXF binary remap rolls back malformed trailing chunks");
}

void testDxfRawEntityApplicationGroupRemap(TestContext& t) {
    DRW_RawDxfObject object;
    object.name = "LOCAL_REACTOR_REMAP_ENTITY";
    object.handle = 0x1Au;
    object.m_version = DRW::AC1027;
    object.hasRawValues = true;
    object.groups = {DRW_Variant(5, std::string("1A")),
                     DRW_Variant(102, std::string("{ACAD_REACTORS")),
                     DRW_Variant(330, std::string("2A")),
                     DRW_Variant(102, std::string("{NESTED_REFS")),
                     DRW_Variant(340, std::string("3A")),
                     DRW_Variant(102, std::string("}")),
                     DRW_Variant(102, std::string("}"))};
    object.rawValues = {"1A", "{ACAD_REACTORS", "2A", "{NESTED_REFS",
                        "3A", "}", "}"};

    const std::map<std::uint32_t, std::uint32_t> remap = {
        {0x1Au, 0x3Au}, {0x2Au, 0x4Au}, {0x3Au, 0x5Au}};
    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    asciiWriter.setHandleRemap(remap);
    t.expect(asciiWriter.writeRawDxfObject(&object),
             "DXF raw entity remaps nested application-group references through ASCII");

    int code = 0;
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    asciiReader.setAllowWideHandleLexemes(true);
    const std::vector<std::pair<int, std::string>> expected = {
        {0, "LOCAL_REACTOR_REMAP_ENTITY"},
        {5, "3A"},
        {102, "{ACAD_REACTORS"},
        {330, "4A"},
        {102, "{NESTED_REFS"},
        {340, "5A"},
        {102, "}"},
        {102, "}"}};
    bool asciiShape = true;
    for (const auto& item : expected) {
        if (!asciiReader.readRec(&code) || code != item.first
            || asciiReader.getString() != item.second) {
            asciiShape = false;
            break;
        }
    }
    t.expect(asciiShape,
             "DXF ASCII replay preserves balanced nested application groups");

    DRW_RawDxfObject binaryObject = object;
    binaryObject.hasRawValues = false;
    binaryObject.rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    binaryWriter.setHandleRemap(remap);
    t.expect(binaryWriter.writeRawDxfObject(&binaryObject),
             "DXF raw entity remaps nested application-group references through binary");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    binaryReader.setAllowWideHandleLexemes(true);
    bool binaryShape = true;
    for (const auto& item : expected) {
        if (!binaryReader.readRec(&code) || code != item.first
            || binaryReader.getString() != item.second) {
            binaryShape = false;
            break;
        }
    }
    t.expect(binaryShape,
             "DXF binary replay preserves balanced nested application groups");

    DRW_RawDxfObject malformedAscii = object;
    malformedAscii.groups.pop_back();
    malformedAscii.rawValues.pop_back();
    std::ostringstream rejectedAsciiOutput;
    dxfRW rejectingAsciiWriter("");
    rejectingAsciiWriter.version = DRW::AC1027;
    rejectingAsciiWriter.binFile = false;
    rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedAsciiOutput);
    rejectingAsciiWriter.setHandleRemap(remap);
    t.expect(!rejectingAsciiWriter.writeRawDxfObject(&malformedAscii)
                 && rejectedAsciiOutput.str().empty(),
             "DXF ASCII nested application-group depth rejects transactionally");

    DRW_RawDxfObject malformedBinary = binaryObject;
    malformedBinary.groups.pop_back();
    std::ostringstream rejectedBinaryOutput;
    dxfRW rejectingBinaryWriter("");
    rejectingBinaryWriter.version = DRW::AC1027;
    rejectingBinaryWriter.binFile = true;
    rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
        &rejectedBinaryOutput);
    rejectingBinaryWriter.setHandleRemap(remap);
    t.expect(!rejectingBinaryWriter.writeRawDxfObject(&malformedBinary)
                 && rejectedBinaryOutput.str().empty(),
             "DXF binary nested application-group depth rejects transactionally");
}

void testDxfRawEntityApplicationGroupDepth(TestContext& t) {
    const auto makeObject = [](std::size_t depth, bool hasRawValues) {
        DRW_RawDxfObject object;
        object.name = "LOCAL_DEPTH_ENTITY";
        object.handle = 0x1Au;
        object.m_version = DRW::AC1027;
        object.hasRawValues = hasRawValues;
        object.groups.emplace_back(5, std::string("1A"));
        if (hasRawValues)
            object.rawValues.emplace_back("1A");
        for (std::size_t i = 0; i < depth; ++i) {
            object.groups.emplace_back(102, std::string("{DEPTH"));
            if (hasRawValues)
                object.rawValues.emplace_back("{DEPTH");
        }
        for (std::size_t i = 0; i < depth; ++i) {
            object.groups.emplace_back(102, std::string("}"));
            if (hasRawValues)
                object.rawValues.emplace_back("}");
        }
        return object;
    };
    constexpr std::size_t maxDepth = DRW::kMaxDxfApplicationGroupNesting;

    DRW_RawDxfObject asciiBoundary = makeObject(maxDepth, true);
    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiWriter.writeRawDxfObject(&asciiBoundary)
                 && !asciiOutput.str().empty(),
             "DXF ASCII application-group maximum depth is accepted");

    DRW_RawDxfObject binaryBoundary = makeObject(maxDepth, false);
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryWriter.writeRawDxfObject(&binaryBoundary)
                 && !binaryOutput.str().empty(),
             "DXF binary application-group maximum depth is accepted");

    DRW_RawDxfObject asciiOver = makeObject(maxDepth + 1u, true);
    std::ostringstream rejectedAsciiOutput;
    dxfRW rejectingAsciiWriter("");
    rejectingAsciiWriter.version = DRW::AC1027;
    rejectingAsciiWriter.binFile = false;
    rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedAsciiOutput);
    t.expect(!rejectingAsciiWriter.writeRawDxfObject(&asciiOver)
                 && rejectedAsciiOutput.str().empty(),
             "DXF ASCII over-limit application-group depth rejects transactionally");

    DRW_RawDxfObject binaryOver = makeObject(maxDepth + 1u, false);
    std::ostringstream rejectedBinaryOutput;
    dxfRW rejectingBinaryWriter("");
    rejectingBinaryWriter.version = DRW::AC1027;
    rejectingBinaryWriter.binFile = true;
    rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
        &rejectedBinaryOutput);
    t.expect(!rejectingBinaryWriter.writeRawDxfObject(&binaryOver)
                 && rejectedBinaryOutput.str().empty(),
             "DXF binary over-limit application-group depth rejects transactionally");
}

void testDxfRawEntityApplicationGroupAggregate(TestContext& t) {
    const auto makeObject = [](std::size_t pairCount, bool hasRawValues) {
        DRW_RawDxfObject object;
        object.name = "LOCAL_AGGREGATE_ENTITY";
        object.handle = 0x1Au;
        object.m_version = DRW::AC1027;
        object.hasRawValues = hasRawValues;
        object.groups.emplace_back(5, std::string("1A"));
        if (hasRawValues)
            object.rawValues.emplace_back("1A");

        const std::size_t remaining = pairCount - 1u;
        const std::size_t markerPairs = remaining / 2u;
        for (std::size_t i = 0; i < markerPairs; ++i) {
            object.groups.emplace_back(102, std::string("{AGGREGATE"));
            object.groups.emplace_back(102, std::string("}"));
            if (hasRawValues) {
                object.rawValues.emplace_back("{AGGREGATE");
                object.rawValues.emplace_back("}");
            }
        }
        if (object.groups.size() < pairCount) {
            object.groups.emplace_back(1000, std::string("payload"));
            if (hasRawValues)
                object.rawValues.emplace_back("payload");
        }
        return object;
    };
    constexpr std::size_t maxPairs = DRW::kMaxDxfApplicationGroupPairs;

    DRW_RawDxfObject asciiBoundary = makeObject(maxPairs, true);
    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiBoundary.groups.size() == maxPairs
                 && asciiWriter.writeRawDxfObject(&asciiBoundary)
                 && !asciiOutput.str().empty(),
             "DXF ASCII application-group aggregate limit is accepted");

    DRW_RawDxfObject binaryBoundary = makeObject(maxPairs, false);
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryBoundary.groups.size() == maxPairs
                 && binaryWriter.writeRawDxfObject(&binaryBoundary)
                 && !binaryOutput.str().empty(),
             "DXF binary application-group aggregate limit is accepted");

    DRW_RawDxfObject asciiOver = makeObject(maxPairs + 1u, true);
    std::ostringstream rejectedAsciiOutput;
    dxfRW rejectingAsciiWriter("");
    rejectingAsciiWriter.version = DRW::AC1027;
    rejectingAsciiWriter.binFile = false;
    rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedAsciiOutput);
    t.expect(asciiOver.groups.size() == maxPairs + 1u
                 && !rejectingAsciiWriter.writeRawDxfObject(&asciiOver)
                 && rejectedAsciiOutput.str().empty(),
             "DXF ASCII over-limit application-group aggregate rejects transactionally");

    DRW_RawDxfObject binaryOver = makeObject(maxPairs + 1u, false);
    std::ostringstream rejectedBinaryOutput;
    dxfRW rejectingBinaryWriter("");
    rejectingBinaryWriter.version = DRW::AC1027;
    rejectingBinaryWriter.binFile = true;
    rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
        &rejectedBinaryOutput);
    t.expect(binaryOver.groups.size() == maxPairs + 1u
                 && !rejectingBinaryWriter.writeRawDxfObject(&binaryOver)
                 && rejectedBinaryOutput.str().empty(),
             "DXF binary over-limit application-group aggregate rejects transactionally");
}

void testDxfRawEntityApplicationGroupMarker(TestContext& t) {
    const std::vector<std::string> invalidMarkers = {"{", "NOT_A_MARKER"};
    for (const std::string& marker : invalidMarkers) {
        DRW_RawDxfObject asciiObject;
        asciiObject.name = "LOCAL_MARKER_ENTITY";
        asciiObject.handle = 0x1Au;
        asciiObject.m_version = DRW::AC1027;
        asciiObject.hasRawValues = true;
        asciiObject.groups = {DRW_Variant(5, std::string("1A")),
                              DRW_Variant(102, marker)};
        asciiObject.rawValues = {"1A", marker};
        std::ostringstream asciiOutput;
        dxfRW asciiWriter("");
        asciiWriter.version = DRW::AC1027;
        asciiWriter.binFile = false;
        asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
        t.expect(!asciiWriter.writeRawDxfObject(&asciiObject)
                     && asciiOutput.str().empty(),
                 "DXF ASCII invalid application-group marker rejects transactionally");

        DRW_RawDxfObject binaryObject = asciiObject;
        binaryObject.hasRawValues = false;
        binaryObject.rawValues.clear();
        std::ostringstream binaryOutput;
        dxfRW binaryWriter("");
        binaryWriter.version = DRW::AC1027;
        binaryWriter.binFile = true;
        binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
        t.expect(!binaryWriter.writeRawDxfObject(&binaryObject)
                     && binaryOutput.str().empty(),
                 "DXF binary invalid application-group marker rejects transactionally");
    }
}

void testDxfRawEntityApplicationGroupReferenceMatrix(TestContext& t) {
    DRW_RawDxfObject object;
    object.name = "LOCAL_REFERENCE_MATRIX_ENTITY";
    object.handle = 0x1Au;
    object.m_version = DRW::AC1027;
    object.hasRawValues = true;
    object.groups = {
        DRW_Variant(5, std::string("1A")),
        DRW_Variant(102, std::string("{REFERENCE_MATRIX")),
        DRW_Variant(320, std::string("2A")),
        DRW_Variant(330, std::string("3A")),
        DRW_Variant(350, std::string("4A")),
        DRW_Variant(390, std::string("5A")),
        DRW_Variant(399, std::string("6A")),
        DRW_Variant(480, std::string("1A")),
        DRW_Variant(481, std::string("2A")),
        DRW_Variant(102, std::string("}"))};
    object.rawValues = {"1A", "{REFERENCE_MATRIX", "2A", "3A", "4A",
                        "5A", "6A", "1A", "2A", "}"};
    const std::map<std::uint32_t, std::uint32_t> remap = {
        {0x1Au, 0x3Au}, {0x2Au, 0x4Au}, {0x3Au, 0x5Au},
        {0x4Au, 0x6Au}, {0x5Au, 0x7Au}, {0x6Au, 0x8Au}};
    const std::vector<std::pair<int, std::string>> expected = {
        {0, "LOCAL_REFERENCE_MATRIX_ENTITY"},
        {5, "3A"},
        {102, "{REFERENCE_MATRIX"},
        {320, "2A"},
        {330, "5A"},
        {350, "6A"},
        {390, "7A"},
        {399, "8A"},
        {480, "3A"},
        {481, "4A"},
        {102, "}"}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    asciiWriter.setHandleRemap(remap);
    t.expect(asciiWriter.writeRawDxfObject(&object),
             "DXF ASCII remaps every handle-reference code family in an application group");
    int code = 0;
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    bool asciiShape = true;
    for (const auto& item : expected) {
        if (!asciiReader.readRec(&code) || code != item.first
            || asciiReader.getString() != item.second) {
            asciiShape = false;
            break;
        }
    }
    t.expect(asciiShape,
             "DXF ASCII application-group reference-code matrix replays losslessly");

    DRW_RawDxfObject binaryObject = object;
    binaryObject.hasRawValues = false;
    binaryObject.rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    binaryWriter.setHandleRemap(remap);
    t.expect(binaryWriter.writeRawDxfObject(&binaryObject),
             "DXF binary remaps every handle-reference code family in an application group");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    bool binaryShape = true;
    for (const auto& item : expected) {
        if (!binaryReader.readRec(&code) || code != item.first
            || binaryReader.getString() != item.second) {
            binaryShape = false;
            break;
        }
    }
    t.expect(binaryShape,
             "DXF binary application-group reference-code matrix replays losslessly");
}

void testDxfRawEntityApplicationGroupBinaryChunks(TestContext& t) {
    DRW_RawDxfObject object;
    object.name = "LOCAL_BINARY_GROUP_ENTITY";
    object.handle = 0x1Au;
    object.m_version = DRW::AC1027;
    object.hasRawValues = true;
    object.groups = {DRW_Variant(5, std::string("1A")),
                     DRW_Variant(102, std::string("{BINARY_PAYLOAD")),
                     DRW_Variant(330, std::string("2A")),
                     DRW_Variant(310, std::string("ABCD")),
                     DRW_Variant(1004, std::string("0102")),
                     DRW_Variant(102, std::string("}"))};
    object.rawValues = {"1A", "{BINARY_PAYLOAD", "2A", "ABCD", "0102", "}"};
    const std::map<std::uint32_t, std::uint32_t> remap = {{0x1Au, 0x3Au},
                                                           {0x2Au, 0x4Au}};
    const std::vector<std::pair<int, std::string>> expected = {
        {0, "LOCAL_BINARY_GROUP_ENTITY"},
        {5, "3A"},
        {102, "{BINARY_PAYLOAD"},
        {330, "4A"},
        {310, "ABCD"},
        {1004, "0102"},
        {102, "}"}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    asciiWriter.setHandleRemap(remap);
    t.expect(asciiWriter.writeRawDxfObject(&object),
             "DXF ASCII application-group binary chunks replay beside remapped references");
    int code = 0;
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    bool asciiShape = true;
    for (const auto& item : expected) {
        if (!asciiReader.readRec(&code) || code != item.first
            || asciiReader.getString() != item.second) {
            asciiShape = false;
            break;
        }
    }
    t.expect(asciiShape,
             "DXF ASCII application-group binary chunks preserve group structure");

    DRW_RawDxfObject binaryObject = object;
    binaryObject.hasRawValues = false;
    binaryObject.rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    binaryWriter.setHandleRemap(remap);
    t.expect(binaryWriter.writeRawDxfObject(&binaryObject),
             "DXF binary application-group binary chunks replay beside remapped references");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    bool binaryShape = true;
    for (const auto& item : expected) {
        if (!binaryReader.readRec(&code) || code != item.first
            || binaryReader.getString() != item.second) {
            binaryShape = false;
            break;
        }
    }
    t.expect(binaryShape,
             "DXF binary application-group binary chunks preserve group structure");

    DRW_RawDxfObject malformedAscii = object;
    malformedAscii.groups[4] = DRW_Variant(1004, std::string("ABC"));
    malformedAscii.rawValues[4] = "ABC";
    std::ostringstream rejectedAsciiOutput;
    dxfRW rejectingAsciiWriter("");
    rejectingAsciiWriter.version = DRW::AC1027;
    rejectingAsciiWriter.binFile = false;
    rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedAsciiOutput);
    rejectingAsciiWriter.setHandleRemap(remap);
    t.expect(!rejectingAsciiWriter.writeRawDxfObject(&malformedAscii)
                 && rejectedAsciiOutput.str().empty(),
             "DXF ASCII malformed application-group chunk rejects transactionally");

    DRW_RawDxfObject malformedBinary = binaryObject;
    malformedBinary.groups[4] = DRW_Variant(1004, std::string("GG"));
    std::ostringstream rejectedBinaryOutput;
    dxfRW rejectingBinaryWriter("");
    rejectingBinaryWriter.version = DRW::AC1027;
    rejectingBinaryWriter.binFile = true;
    rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
        &rejectedBinaryOutput);
    rejectingBinaryWriter.setHandleRemap(remap);
    t.expect(!rejectingBinaryWriter.writeRawDxfObject(&malformedBinary)
                 && rejectedBinaryOutput.str().empty(),
             "DXF binary malformed application-group chunk rejects transactionally");
}

void testDxfRawEntityApplicationGroupChunkSize(TestContext& t) {
    const auto makeObject = [](std::size_t byteCount, bool hasRawValues) {
        DRW_RawDxfObject object;
        object.name = "LOCAL_CHUNK_SIZE_ENTITY";
        object.handle = 0x1Au;
        object.m_version = DRW::AC1027;
        object.hasRawValues = hasRawValues;
        const std::string chunk(byteCount * 2u, 'A');
        object.groups = {DRW_Variant(5, std::string("1A")),
                         DRW_Variant(102, std::string("{CHUNK_SIZE")),
                         DRW_Variant(310, chunk),
                         DRW_Variant(102, std::string("}"))};
        if (hasRawValues)
            object.rawValues = {"1A", "{CHUNK_SIZE", chunk, "}"};
        return object;
    };
    constexpr std::size_t maxChunkBytes = 127u;

    DRW_RawDxfObject asciiBoundary = makeObject(maxChunkBytes, true);
    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiWriter.writeRawDxfObject(&asciiBoundary)
                 && !asciiOutput.str().empty(),
             "DXF ASCII application-group chunk accepts the 127-byte boundary");

    DRW_RawDxfObject binaryBoundary = makeObject(maxChunkBytes, false);
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryWriter.writeRawDxfObject(&binaryBoundary)
                 && !binaryOutput.str().empty(),
             "DXF binary application-group chunk accepts the 127-byte boundary");

    DRW_RawDxfObject asciiOver = makeObject(maxChunkBytes + 1u, true);
    std::ostringstream rejectedAsciiOutput;
    dxfRW rejectingAsciiWriter("");
    rejectingAsciiWriter.version = DRW::AC1027;
    rejectingAsciiWriter.binFile = false;
    rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedAsciiOutput);
    t.expect(!rejectingAsciiWriter.writeRawDxfObject(&asciiOver)
                 && rejectedAsciiOutput.str().empty(),
             "DXF ASCII application-group chunk rejects at 128 bytes transactionally");

    DRW_RawDxfObject binaryOver = makeObject(maxChunkBytes + 1u, false);
    std::ostringstream rejectedBinaryOutput;
    dxfRW rejectingBinaryWriter("");
    rejectingBinaryWriter.version = DRW::AC1027;
    rejectingBinaryWriter.binFile = true;
    rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
        &rejectedBinaryOutput);
    t.expect(!rejectingBinaryWriter.writeRawDxfObject(&binaryOver)
                 && rejectedBinaryOutput.str().empty(),
             "DXF binary application-group chunk rejects at 128 bytes transactionally");
}

void testDxfRawEntityApplicationGroupChunkCodeMatrix(TestContext& t) {
    DRW_RawDxfObject object;
    object.name = "LOCAL_CHUNK_CODE_MATRIX_ENTITY";
    object.handle = 0x1Au;
    object.m_version = DRW::AC1027;
    object.hasRawValues = true;
    object.groups.emplace_back(5, std::string("1A"));
    object.groups.emplace_back(102, std::string("{CHUNK_CODES"));
    object.rawValues = {"1A", "{CHUNK_CODES"};
    for (int code = 310; code <= 319; ++code) {
        const std::string value = (code & 1) == 0 ? "A1B2" : "C3D4";
        object.groups.emplace_back(code, value);
        object.rawValues.emplace_back(value);
    }
    object.groups.emplace_back(1004, std::string("01020304"));
    object.rawValues.emplace_back("01020304");
    object.groups.emplace_back(102, std::string("}"));
    object.rawValues.emplace_back("}");
    const std::map<std::uint32_t, std::uint32_t> remap = {{0x1Au, 0x3Au}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    asciiWriter.setHandleRemap(remap);
    t.expect(asciiWriter.writeRawDxfObject(&object),
             "DXF ASCII replays every application-group binary chunk code");
    int code = 0;
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    bool asciiShape = asciiReader.readRec(&code) && code == 0
        && asciiReader.getString() == object.name
        && asciiReader.readRec(&code) && code == 5
        && asciiReader.getString() == "3A"
        && asciiReader.readRec(&code) && code == 102
        && asciiReader.getString() == "{CHUNK_CODES";
    for (int expectedCode = 310; expectedCode <= 319 && asciiShape;
         ++expectedCode) {
        const std::string expectedValue = (expectedCode & 1) == 0
            ? "A1B2" : "C3D4";
        asciiShape = asciiReader.readRec(&code) && code == expectedCode
            && asciiReader.getString() == expectedValue;
    }
    asciiShape = asciiShape && asciiReader.readRec(&code) && code == 1004
        && asciiReader.getString() == "01020304"
        && asciiReader.readRec(&code) && code == 102
        && asciiReader.getString() == "}";
    t.expect(asciiShape,
             "DXF ASCII application-group binary chunk-code matrix preserves order");

    DRW_RawDxfObject binaryObject = object;
    binaryObject.hasRawValues = false;
    binaryObject.rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    binaryWriter.setHandleRemap(remap);
    t.expect(binaryWriter.writeRawDxfObject(&binaryObject),
             "DXF binary replays every application-group binary chunk code");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    bool binaryShape = binaryReader.readRec(&code) && code == 0
        && binaryReader.getString() == object.name
        && binaryReader.readRec(&code) && code == 5
        && binaryReader.getString() == "3A"
        && binaryReader.readRec(&code) && code == 102
        && binaryReader.getString() == "{CHUNK_CODES";
    for (int expectedCode = 310; expectedCode <= 319 && binaryShape;
         ++expectedCode) {
        const std::string expectedValue = (expectedCode & 1) == 0
            ? "A1B2" : "C3D4";
        binaryShape = binaryReader.readRec(&code) && code == expectedCode
            && binaryReader.getString() == expectedValue;
    }
    binaryShape = binaryShape && binaryReader.readRec(&code) && code == 1004
        && binaryReader.getString() == "01020304"
        && binaryReader.readRec(&code) && code == 102
        && binaryReader.getString() == "}";
    t.expect(binaryShape,
             "DXF binary application-group binary chunk-code matrix preserves order");

    DRW_RawDxfObject malformedAscii = object;
    malformedAscii.groups[5] = DRW_Variant(313, std::string("ABC"));
    malformedAscii.rawValues[5] = "ABC";
    std::ostringstream rejectedAsciiOutput;
    dxfRW rejectingAsciiWriter("");
    rejectingAsciiWriter.version = DRW::AC1027;
    rejectingAsciiWriter.binFile = false;
    rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedAsciiOutput);
    rejectingAsciiWriter.setHandleRemap(remap);
    t.expect(!rejectingAsciiWriter.writeRawDxfObject(&malformedAscii)
                 && rejectedAsciiOutput.str().empty(),
             "DXF ASCII malformed chunk-code matrix rejects transactionally");

    DRW_RawDxfObject malformedBinary = binaryObject;
    malformedBinary.groups[5] = DRW_Variant(313, std::string("GG"));
    std::ostringstream rejectedBinaryOutput;
    dxfRW rejectingBinaryWriter("");
    rejectingBinaryWriter.version = DRW::AC1027;
    rejectingBinaryWriter.binFile = true;
    rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
        &rejectedBinaryOutput);
    rejectingBinaryWriter.setHandleRemap(remap);
    t.expect(!rejectingBinaryWriter.writeRawDxfObject(&malformedBinary)
                 && rejectedBinaryOutput.str().empty(),
             "DXF binary malformed chunk-code matrix rejects transactionally");
}

void testDxfRawEntityApplicationGroupRawValueCardinality(TestContext& t) {
    DRW_RawDxfObject object;
    object.name = "LOCAL_RAW_VALUE_ENTITY";
    object.handle = 0x1Au;
    object.m_version = DRW::AC1027;
    object.hasRawValues = true;
    object.groups = {DRW_Variant(5, std::string("1A")),
                     DRW_Variant(102, std::string("{RAW_VALUES")),
                     DRW_Variant(330, std::string("2A")),
                     DRW_Variant(102, std::string("}"))};
    object.rawValues = {"1A", "{RAW_VALUES", "2A", "}"};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiWriter.writeRawDxfObject(&object)
                 && !asciiOutput.str().empty(),
             "DXF ASCII raw-value cardinality accepts a matching carrier");

    DRW_RawDxfObject missing = object;
    missing.rawValues.pop_back();
    std::ostringstream missingOutput;
    dxfRW missingWriter("");
    missingWriter.version = DRW::AC1027;
    missingWriter.binFile = false;
    missingWriter.writer = std::make_unique<dxfWriterAscii>(&missingOutput);
    t.expect(!missingWriter.writeRawDxfObject(&missing)
                 && missingOutput.str().empty(),
             "DXF ASCII missing raw-value spelling rejects transactionally");

    DRW_RawDxfObject extra = object;
    extra.rawValues.emplace_back("EXTRA");
    std::ostringstream extraOutput;
    dxfRW extraWriter("");
    extraWriter.version = DRW::AC1027;
    extraWriter.binFile = false;
    extraWriter.writer = std::make_unique<dxfWriterAscii>(&extraOutput);
    t.expect(!extraWriter.writeRawDxfObject(&extra)
                 && extraOutput.str().empty(),
             "DXF ASCII extra raw-value spelling rejects transactionally");

    DRW_RawDxfObject binaryObject = object;
    binaryObject.hasRawValues = false;
    binaryObject.rawValues.assign(binaryObject.groups.size(), UTF8STRING());
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryWriter.writeRawDxfObject(&binaryObject)
                 && !binaryOutput.str().empty(),
             "DXF binary empty raw-value placeholders remain valid");
}

void testDxfRawEntityApplicationGroupSourceSpelling(TestContext& t) {
    DRW_RawDxfObject object;
    object.name = "LOCAL_SPELLING_ENTITY";
    object.handle = 0x1Au;
    object.m_version = DRW::AC1027;
    object.hasRawValues = true;
    object.groups = {DRW_Variant(5, std::string("1a")),
                     DRW_Variant(102, std::string("{Mixed_Group")),
                     DRW_Variant(330, std::string("2b")),
                     DRW_Variant(340, std::string("3c")),
                     DRW_Variant(102, std::string("}"))};
    object.rawValues = {"1a", "{Mixed_Group", "2b", "3c", "}"};
    const std::map<std::uint32_t, std::uint32_t> remap = {{0x1Au, 0x3Au},
                                                           {0x3Cu, 0x4Cu}};
    const std::vector<std::pair<int, std::string>> expected = {
        {0, "LOCAL_SPELLING_ENTITY"},
        {5, "3A"},
        {102, "{Mixed_Group"},
        {330, "2b"},
        {340, "4C"},
        {102, "}"}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    asciiWriter.setHandleRemap(remap);
    t.expect(asciiWriter.writeRawDxfObject(&object),
             "DXF ASCII remap canonicalizes only mapped handle spellings");
    int code = 0;
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    bool asciiShape = true;
    for (const auto& item : expected) {
        if (!asciiReader.readRec(&code) || code != item.first
            || asciiReader.getString() != item.second) {
            asciiShape = false;
            break;
        }
    }
    t.expect(asciiShape,
             "DXF ASCII replay preserves untouched application-group source spelling");

    DRW_RawDxfObject binaryObject = object;
    binaryObject.hasRawValues = false;
    binaryObject.rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    binaryWriter.setHandleRemap(remap);
    t.expect(binaryWriter.writeRawDxfObject(&binaryObject),
             "DXF binary remap canonicalizes only mapped handle spellings");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    bool binaryShape = true;
    for (const auto& item : expected) {
        if (!binaryReader.readRec(&code) || code != item.first
            || binaryReader.getString() != item.second) {
            binaryShape = false;
            break;
        }
    }
    t.expect(binaryShape,
             "DXF binary replay preserves untouched application-group source spelling");
}

void testDxfRawEntityApplicationGroupRemapChain(TestContext& t) {
    DRW_RawDxfObject object;
    object.name = "LOCAL_REMAP_CHAIN_ENTITY";
    object.handle = 0x1Au;
    object.m_version = DRW::AC1027;
    object.hasRawValues = true;
    object.groups = {DRW_Variant(5, std::string("1A")),
                     DRW_Variant(102, std::string("{REMAP_CHAIN")),
                     DRW_Variant(330, std::string("2A")),
                     DRW_Variant(340, std::string("1A")),
                     DRW_Variant(102, std::string("}"))};
    object.rawValues = {"1A", "{REMAP_CHAIN", "2A", "1A", "}"};
    const std::map<std::uint32_t, std::uint32_t> remap = {
        {0x1Au, 0x2Au}, {0x2Au, 0x3Au}};
    const std::vector<std::pair<int, std::string>> expected = {
        {0, "LOCAL_REMAP_CHAIN_ENTITY"},
        {5, "2A"},
        {102, "{REMAP_CHAIN"},
        {330, "3A"},
        {340, "2A"},
        {102, "}"}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    asciiWriter.setHandleRemap(remap);
    t.expect(asciiWriter.writeRawDxfObject(&object),
             "DXF ASCII application-group remap uses one-step lookup");
    int code = 0;
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    bool asciiShape = true;
    for (const auto& item : expected) {
        if (!asciiReader.readRec(&code) || code != item.first
            || asciiReader.getString() != item.second) {
            asciiShape = false;
            break;
        }
    }
    t.expect(asciiShape,
             "DXF ASCII application-group remap does not cascade destinations");

    DRW_RawDxfObject binaryObject = object;
    binaryObject.hasRawValues = false;
    binaryObject.rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    binaryWriter.setHandleRemap(remap);
    t.expect(binaryWriter.writeRawDxfObject(&binaryObject),
             "DXF binary application-group remap uses one-step lookup");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    bool binaryShape = true;
    for (const auto& item : expected) {
        if (!binaryReader.readRec(&code) || code != item.first
            || binaryReader.getString() != item.second) {
            binaryShape = false;
            break;
        }
    }
    t.expect(binaryShape,
             "DXF binary application-group remap does not cascade destinations");
}

void testDxfRawSectionApplicationGroupRemap(TestContext& t) {
    DRW_RawDxfSection section;
    section.m_name = "LOCAL_REMAP_SECTION";
    section.m_version = DRW::AC1027;
    section.m_hasRawValues = true;
    section.m_groups = {DRW_Variant(102, std::string("{SECTION_REFS")),
                        DRW_Variant(330, std::string("2A")),
                        DRW_Variant(340, std::string("3A")),
                        DRW_Variant(102, std::string("}"))};
    section.m_rawValues = {"{SECTION_REFS", "2A", "3A", "}"};
    const std::map<std::uint32_t, std::uint32_t> remap = {{0x2Au, 0x4Au},
                                                           {0x3Au, 0x5Au}};
    const std::vector<std::pair<int, std::string>> expected = {
        {0, "SECTION"},
        {2, "LOCAL_REMAP_SECTION"},
        {102, "{SECTION_REFS"},
        {330, "4A"},
        {340, "5A"},
        {102, "}"},
        {0, "ENDSEC"}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    asciiWriter.setHandleRemap(remap);
    t.expect(asciiWriter.writeRawDxfSection(section),
             "DXF ASCII raw section remaps nested application-group references");
    int code = 0;
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    bool asciiShape = true;
    for (const auto& item : expected) {
        if (!asciiReader.readRec(&code) || code != item.first
            || asciiReader.getString() != item.second) {
            asciiShape = false;
            break;
        }
    }
    t.expect(asciiShape,
             "DXF ASCII raw section preserves remapped application-group framing");

    DRW_RawDxfSection binarySection = section;
    binarySection.m_hasRawValues = false;
    binarySection.m_rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    binaryWriter.setHandleRemap(remap);
    t.expect(binaryWriter.writeRawDxfSection(binarySection),
             "DXF binary raw section remaps nested application-group references");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    bool binaryShape = true;
    for (const auto& item : expected) {
        if (!binaryReader.readRec(&code) || code != item.first
            || binaryReader.getString() != item.second) {
            binaryShape = false;
            break;
        }
    }
    t.expect(binaryShape,
             "DXF binary raw section preserves remapped application-group framing");

    DRW_RawDxfSection malformed = section;
    malformed.m_groups.pop_back();
    malformed.m_rawValues.pop_back();
    std::ostringstream rejectedAsciiOutput;
    dxfRW rejectingAsciiWriter("");
    rejectingAsciiWriter.version = DRW::AC1027;
    rejectingAsciiWriter.binFile = false;
    rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedAsciiOutput);
    rejectingAsciiWriter.setHandleRemap(remap);
    t.expect(!rejectingAsciiWriter.writeRawDxfSection(malformed)
                 && rejectedAsciiOutput.str().empty(),
             "DXF ASCII malformed section application group rejects transactionally");

    malformed.m_hasRawValues = false;
    malformed.m_rawValues.clear();
    std::ostringstream rejectedBinaryOutput;
    dxfRW rejectingBinaryWriter("");
    rejectingBinaryWriter.version = DRW::AC1027;
    rejectingBinaryWriter.binFile = true;
    rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
        &rejectedBinaryOutput);
    rejectingBinaryWriter.setHandleRemap(remap);
    t.expect(!rejectingBinaryWriter.writeRawDxfSection(malformed)
                 && rejectedBinaryOutput.str().empty(),
             "DXF binary malformed section application group rejects transactionally");
}

void testDxfRawSectionApplicationGroupSourceSpelling(TestContext& t) {
    DRW_RawDxfSection section;
    section.m_name = "LOCAL_SECTION_SPELLING";
    section.m_version = DRW::AC1027;
    section.m_hasRawValues = true;
    section.m_groups = {DRW_Variant(102, std::string("{Mixed_Section")),
                        DRW_Variant(330, std::string("2b")),
                        DRW_Variant(340, std::string("3c")),
                        DRW_Variant(102, std::string("}"))};
    section.m_rawValues = {"{Mixed_Section", "2b", "3c", "}"};
    const std::map<std::uint32_t, std::uint32_t> remap = {{0x2Bu, 0x4Bu}};
    const std::vector<std::pair<int, std::string>> expected = {
        {0, "SECTION"},
        {2, "LOCAL_SECTION_SPELLING"},
        {102, "{Mixed_Section"},
        {330, "4B"},
        {340, "3c"},
        {102, "}"},
        {0, "ENDSEC"}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    asciiWriter.setHandleRemap(remap);
    t.expect(asciiWriter.writeRawDxfSection(section),
             "DXF ASCII raw section canonicalizes only mapped handles");
    int code = 0;
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    bool asciiShape = true;
    for (const auto& item : expected) {
        if (!asciiReader.readRec(&code) || code != item.first
            || asciiReader.getString() != item.second) {
            asciiShape = false;
            break;
        }
    }
    t.expect(asciiShape,
             "DXF ASCII raw section preserves untouched source spelling");

    DRW_RawDxfSection binarySection = section;
    binarySection.m_hasRawValues = false;
    binarySection.m_rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    binaryWriter.setHandleRemap(remap);
    t.expect(binaryWriter.writeRawDxfSection(binarySection),
             "DXF binary raw section canonicalizes only mapped handles");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    bool binaryShape = true;
    for (const auto& item : expected) {
        if (!binaryReader.readRec(&code) || code != item.first
            || binaryReader.getString() != item.second) {
            binaryShape = false;
            break;
        }
    }
    t.expect(binaryShape,
             "DXF binary raw section preserves untouched source spelling");
}

void testDxfRawSectionApplicationGroupRawValueCardinality(TestContext& t) {
    DRW_RawDxfSection section;
    section.m_name = "LOCAL_SECTION_RAW_VALUES";
    section.m_version = DRW::AC1027;
    section.m_hasRawValues = true;
    section.m_groups = {DRW_Variant(102, std::string("{RAW_SECTION")),
                        DRW_Variant(330, std::string("2A")),
                        DRW_Variant(102, std::string("}"))};
    section.m_rawValues = {"{RAW_SECTION", "2A", "}"};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiWriter.writeRawDxfSection(section)
                 && !asciiOutput.str().empty(),
             "DXF ASCII raw section accepts matching raw-value cardinality");

    DRW_RawDxfSection missing = section;
    missing.m_rawValues.pop_back();
    std::ostringstream missingOutput;
    dxfRW missingWriter("");
    missingWriter.version = DRW::AC1027;
    missingWriter.binFile = false;
    missingWriter.writer = std::make_unique<dxfWriterAscii>(&missingOutput);
    t.expect(!missingWriter.writeRawDxfSection(missing)
                 && missingOutput.str().empty(),
             "DXF ASCII raw section rejects missing raw-value spelling");

    DRW_RawDxfSection extra = section;
    extra.m_rawValues.emplace_back("EXTRA");
    std::ostringstream extraOutput;
    dxfRW extraWriter("");
    extraWriter.version = DRW::AC1027;
    extraWriter.binFile = false;
    extraWriter.writer = std::make_unique<dxfWriterAscii>(&extraOutput);
    t.expect(!extraWriter.writeRawDxfSection(extra)
                 && extraOutput.str().empty(),
             "DXF ASCII raw section rejects extra raw-value spelling");

    DRW_RawDxfSection binarySection = section;
    binarySection.m_hasRawValues = false;
    binarySection.m_rawValues.assign(binarySection.m_groups.size(), UTF8STRING());
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryWriter.writeRawDxfSection(binarySection)
                 && !binaryOutput.str().empty(),
             "DXF binary raw section accepts empty raw-value placeholders");
}

void testDxfRawSectionApplicationGroupRemapChain(TestContext& t) {
    DRW_RawDxfSection section;
    section.m_name = "LOCAL_SECTION_REMAP_CHAIN";
    section.m_version = DRW::AC1027;
    section.m_hasRawValues = true;
    section.m_groups = {DRW_Variant(102, std::string("{SECTION_CHAIN")),
                        DRW_Variant(330, std::string("2A")),
                        DRW_Variant(340, std::string("3A")),
                        DRW_Variant(102, std::string("}"))};
    section.m_rawValues = {"{SECTION_CHAIN", "2A", "3A", "}"};
    const std::map<std::uint32_t, std::uint32_t> remap = {
        {0x2Au, 0x3Au}, {0x3Au, 0x4Au}};
    const std::vector<std::pair<int, std::string>> expected = {
        {0, "SECTION"},
        {2, "LOCAL_SECTION_REMAP_CHAIN"},
        {102, "{SECTION_CHAIN"},
        {330, "3A"},
        {340, "4A"},
        {102, "}"},
        {0, "ENDSEC"}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    asciiWriter.setHandleRemap(remap);
    t.expect(asciiWriter.writeRawDxfSection(section),
             "DXF ASCII raw section remap uses one-step lookup");
    int code = 0;
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    bool asciiShape = true;
    for (const auto& item : expected) {
        if (!asciiReader.readRec(&code) || code != item.first
            || asciiReader.getString() != item.second) {
            asciiShape = false;
            break;
        }
    }
    t.expect(asciiShape,
             "DXF ASCII raw section remap does not cascade destinations");

    DRW_RawDxfSection binarySection = section;
    binarySection.m_hasRawValues = false;
    binarySection.m_rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    binaryWriter.setHandleRemap(remap);
    t.expect(binaryWriter.writeRawDxfSection(binarySection),
             "DXF binary raw section remap uses one-step lookup");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    bool binaryShape = true;
    for (const auto& item : expected) {
        if (!binaryReader.readRec(&code) || code != item.first
            || binaryReader.getString() != item.second) {
            binaryShape = false;
            break;
        }
    }
    t.expect(binaryShape,
             "DXF binary raw section remap does not cascade destinations");
}

void testDxfRawSectionApplicationGroupBinaryChunks(TestContext& t) {
    DRW_RawDxfSection section;
    section.m_name = "LOCAL_SECTION_BINARY_GROUP";
    section.m_version = DRW::AC1027;
    section.m_hasRawValues = true;
    section.m_groups = {DRW_Variant(102, std::string("{SECTION_BINARY")),
                        DRW_Variant(330, std::string("2A")),
                        DRW_Variant(310, std::string("ABCD")),
                        DRW_Variant(1004, std::string("0102")),
                        DRW_Variant(102, std::string("}"))};
    section.m_rawValues = {"{SECTION_BINARY", "2A", "ABCD", "0102", "}"};
    const std::map<std::uint32_t, std::uint32_t> remap = {{0x2Au, 0x4Au}};
    const std::vector<std::pair<int, std::string>> expected = {
        {0, "SECTION"},
        {2, "LOCAL_SECTION_BINARY_GROUP"},
        {102, "{SECTION_BINARY"},
        {330, "4A"},
        {310, "ABCD"},
        {1004, "0102"},
        {102, "}"},
        {0, "ENDSEC"}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    asciiWriter.setHandleRemap(remap);
    t.expect(asciiWriter.writeRawDxfSection(section),
             "DXF ASCII raw section replays binary chunks beside remapped references");
    int code = 0;
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    bool asciiShape = true;
    for (const auto& item : expected) {
        if (!asciiReader.readRec(&code) || code != item.first
            || asciiReader.getString() != item.second) {
            asciiShape = false;
            break;
        }
    }
    t.expect(asciiShape,
             "DXF ASCII raw section preserves binary chunk framing");

    DRW_RawDxfSection binarySection = section;
    binarySection.m_hasRawValues = false;
    binarySection.m_rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    binaryWriter.setHandleRemap(remap);
    t.expect(binaryWriter.writeRawDxfSection(binarySection),
             "DXF binary raw section replays binary chunks beside remapped references");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    bool binaryShape = true;
    for (const auto& item : expected) {
        if (!binaryReader.readRec(&code) || code != item.first
            || binaryReader.getString() != item.second) {
            binaryShape = false;
            break;
        }
    }
    t.expect(binaryShape,
             "DXF binary raw section preserves binary chunk framing");

    DRW_RawDxfSection malformedAscii = section;
    malformedAscii.m_groups[3] = DRW_Variant(1004, std::string("ABC"));
    malformedAscii.m_rawValues[3] = "ABC";
    std::ostringstream rejectedAsciiOutput;
    dxfRW rejectingAsciiWriter("");
    rejectingAsciiWriter.version = DRW::AC1027;
    rejectingAsciiWriter.binFile = false;
    rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedAsciiOutput);
    rejectingAsciiWriter.setHandleRemap(remap);
    t.expect(!rejectingAsciiWriter.writeRawDxfSection(malformedAscii)
                 && rejectedAsciiOutput.str().empty(),
             "DXF ASCII malformed section binary chunk rejects transactionally");

    DRW_RawDxfSection malformedBinary = binarySection;
    malformedBinary.m_groups[3] = DRW_Variant(1004, std::string("GG"));
    std::ostringstream rejectedBinaryOutput;
    dxfRW rejectingBinaryWriter("");
    rejectingBinaryWriter.version = DRW::AC1027;
    rejectingBinaryWriter.binFile = true;
    rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
        &rejectedBinaryOutput);
    rejectingBinaryWriter.setHandleRemap(remap);
    t.expect(!rejectingBinaryWriter.writeRawDxfSection(malformedBinary)
                 && rejectedBinaryOutput.str().empty(),
             "DXF binary malformed section binary chunk rejects transactionally");
}

void testDxfRawSectionApplicationGroupChunkCodeMatrix(TestContext& t) {
    DRW_RawDxfSection section;
    section.m_name = "LOCAL_SECTION_CHUNK_CODES";
    section.m_version = DRW::AC1027;
    section.m_hasRawValues = true;
    section.m_groups.emplace_back(102, std::string("{SECTION_CHUNKS"));
    section.m_rawValues.emplace_back("{SECTION_CHUNKS");
    for (int code = 310; code <= 319; ++code) {
        const std::string value = (code & 1) == 0 ? "A1B2" : "C3D4";
        section.m_groups.emplace_back(code, value);
        section.m_rawValues.emplace_back(value);
    }
    section.m_groups.emplace_back(1004, std::string("01020304"));
    section.m_rawValues.emplace_back("01020304");
    section.m_groups.emplace_back(102, std::string("}"));
    section.m_rawValues.emplace_back("}");

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiWriter.writeRawDxfSection(section),
             "DXF ASCII raw section replays every binary chunk code");
    int code = 0;
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    bool asciiShape = asciiReader.readRec(&code) && code == 0
        && asciiReader.getString() == "SECTION"
        && asciiReader.readRec(&code) && code == 2
        && asciiReader.getString() == section.m_name
        && asciiReader.readRec(&code) && code == 102
        && asciiReader.getString() == "{SECTION_CHUNKS";
    for (int expectedCode = 310; expectedCode <= 319 && asciiShape;
         ++expectedCode) {
        const std::string expectedValue = (expectedCode & 1) == 0
            ? "A1B2" : "C3D4";
        asciiShape = asciiReader.readRec(&code) && code == expectedCode
            && asciiReader.getString() == expectedValue;
    }
    asciiShape = asciiShape && asciiReader.readRec(&code) && code == 1004
        && asciiReader.getString() == "01020304"
        && asciiReader.readRec(&code) && code == 102
        && asciiReader.getString() == "}"
        && asciiReader.readRec(&code) && code == 0
        && asciiReader.getString() == "ENDSEC";
    t.expect(asciiShape,
             "DXF ASCII raw section binary chunk-code matrix preserves order");

    DRW_RawDxfSection binarySection = section;
    binarySection.m_hasRawValues = false;
    binarySection.m_rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryWriter.writeRawDxfSection(binarySection),
             "DXF binary raw section replays every binary chunk code");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    bool binaryShape = binaryReader.readRec(&code) && code == 0
        && binaryReader.getString() == "SECTION"
        && binaryReader.readRec(&code) && code == 2
        && binaryReader.getString() == section.m_name
        && binaryReader.readRec(&code) && code == 102
        && binaryReader.getString() == "{SECTION_CHUNKS";
    for (int expectedCode = 310; expectedCode <= 319 && binaryShape;
         ++expectedCode) {
        const std::string expectedValue = (expectedCode & 1) == 0
            ? "A1B2" : "C3D4";
        binaryShape = binaryReader.readRec(&code) && code == expectedCode
            && binaryReader.getString() == expectedValue;
    }
    binaryShape = binaryShape && binaryReader.readRec(&code) && code == 1004
        && binaryReader.getString() == "01020304"
        && binaryReader.readRec(&code) && code == 102
        && binaryReader.getString() == "}"
        && binaryReader.readRec(&code) && code == 0
        && binaryReader.getString() == "ENDSEC";
    t.expect(binaryShape,
             "DXF binary raw section binary chunk-code matrix preserves order");

    DRW_RawDxfSection malformedAscii = section;
    malformedAscii.m_groups[5] = DRW_Variant(313, std::string("ABC"));
    malformedAscii.m_rawValues[5] = "ABC";
    std::ostringstream rejectedAsciiOutput;
    dxfRW rejectingAsciiWriter("");
    rejectingAsciiWriter.version = DRW::AC1027;
    rejectingAsciiWriter.binFile = false;
    rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedAsciiOutput);
    t.expect(!rejectingAsciiWriter.writeRawDxfSection(malformedAscii)
                 && rejectedAsciiOutput.str().empty(),
             "DXF ASCII malformed section chunk-code matrix rejects transactionally");

    DRW_RawDxfSection malformedBinary = binarySection;
    malformedBinary.m_groups[5] = DRW_Variant(313, std::string("GG"));
    std::ostringstream rejectedBinaryOutput;
    dxfRW rejectingBinaryWriter("");
    rejectingBinaryWriter.version = DRW::AC1027;
    rejectingBinaryWriter.binFile = true;
    rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
        &rejectedBinaryOutput);
    t.expect(!rejectingBinaryWriter.writeRawDxfSection(malformedBinary)
                 && rejectedBinaryOutput.str().empty(),
             "DXF binary malformed section chunk-code matrix rejects transactionally");
}

void testDxfRawSectionApplicationGroupChunkSize(TestContext& t) {
    const auto makeSection = [](std::size_t byteCount, bool hasRawValues) {
        DRW_RawDxfSection section;
        section.m_name = "LOCAL_SECTION_CHUNK_SIZE";
        section.m_version = DRW::AC1027;
        section.m_hasRawValues = hasRawValues;
        const std::string chunk(byteCount * 2u, 'A');
        section.m_groups = {DRW_Variant(102, std::string("{SECTION_SIZE")),
                            DRW_Variant(310, chunk),
                            DRW_Variant(102, std::string("}"))};
        if (hasRawValues)
            section.m_rawValues = {"{SECTION_SIZE", chunk, "}"};
        return section;
    };
    constexpr std::size_t maxChunkBytes = 127u;

    DRW_RawDxfSection asciiBoundary = makeSection(maxChunkBytes, true);
    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiWriter.writeRawDxfSection(asciiBoundary)
                 && !asciiOutput.str().empty(),
             "DXF ASCII raw section accepts the 127-byte chunk boundary");

    DRW_RawDxfSection binaryBoundary = makeSection(maxChunkBytes, false);
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryWriter.writeRawDxfSection(binaryBoundary)
                 && !binaryOutput.str().empty(),
             "DXF binary raw section accepts the 127-byte chunk boundary");

    DRW_RawDxfSection asciiOver = makeSection(maxChunkBytes + 1u, true);
    std::ostringstream rejectedAsciiOutput;
    dxfRW rejectingAsciiWriter("");
    rejectingAsciiWriter.version = DRW::AC1027;
    rejectingAsciiWriter.binFile = false;
    rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedAsciiOutput);
    t.expect(!rejectingAsciiWriter.writeRawDxfSection(asciiOver)
                 && rejectedAsciiOutput.str().empty(),
             "DXF ASCII raw section rejects a 128-byte chunk transactionally");

    DRW_RawDxfSection binaryOver = makeSection(maxChunkBytes + 1u, false);
    std::ostringstream rejectedBinaryOutput;
    dxfRW rejectingBinaryWriter("");
    rejectingBinaryWriter.version = DRW::AC1027;
    rejectingBinaryWriter.binFile = true;
    rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
        &rejectedBinaryOutput);
    t.expect(!rejectingBinaryWriter.writeRawDxfSection(binaryOver)
                 && rejectedBinaryOutput.str().empty(),
             "DXF binary raw section rejects a 128-byte chunk transactionally");
}

void testDxfRawSectionHandleDiagnostics(TestContext& t) {
    const std::string malformedSelf =
        "0\nSECTION\n2\nLOCAL_BAD_SECTION\n5\nnot-hex\n"
        "0\nENDSEC\n0\nEOF\n";
    ProfileProbeInterface selfInterface;
    dxfRW selfReader("");
    std::string selfInput = malformedSelf;
    const bool selfRead = selfReader.readAscii(&selfInterface, false, selfInput);
    t.expect(!selfRead
                 && selfReader.getError() == DRW::BAD_CODE_PARSED
                 && selfInterface.sections.empty(),
             "DXF malformed raw-section self handle preserves stage and callback policy");
    const DRW_OperationDiagnostic selfDiagnostic = selfReader.getLastDiagnostic();
    t.expect(selfDiagnostic.phase == DRW::OperationPhase::Validation
                 && selfDiagnostic.cause == DRW::OperationCause::ValidationFailure
                 && selfDiagnostic.code == "invalid-handle"
                 && selfDiagnostic.message.find("invalid self handle")
                        != std::string::npos,
             "DXF raw-section self handle records invalid-handle context");

    std::ostringstream binarySource;
    dxfWriterBinary binaryWriter(&binarySource);
    binaryWriter.writeString(0, "SECTION");
    binaryWriter.writeString(2, "LOCAL_BINARY_BAD_SECTION");
    binaryWriter.writeString(330, "not-owner");
    binaryWriter.writeString(0, "ENDSEC");
    binaryWriter.writeString(0, "EOF");
    std::stringstream binaryInput(binarySource.str());
    ProfileProbeInterface ownerInterface;
    dxfRW ownerReader("");
    ownerReader.binFile = true;
    ownerReader.reader = std::make_unique<dxfReaderBinary>(&binaryInput);
    ownerReader.reader->setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    ownerReader.iface = &ownerInterface;
    ownerReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    const bool ownerRead = ownerReader.processDxf();
    t.expect(!ownerRead
                 && ownerReader.getError() == DRW::BAD_READ_SECTION
                 && ownerInterface.sections.empty(),
             "binary malformed raw-section owner handle preserves stage and callback policy");
    const DRW_OperationDiagnostic ownerDiagnostic = ownerReader.getLastDiagnostic();
    t.expect(ownerDiagnostic.phase == DRW::OperationPhase::Validation
                 && ownerDiagnostic.cause == DRW::OperationCause::ValidationFailure
                 && ownerDiagnostic.code == "invalid-handle"
                 && ownerDiagnostic.message.find("handle reference")
                        != std::string::npos,
             "binary raw-section owner handle records reference context");
}

void testDxfRawSectionHandleScope(TestContext& t) {
    const std::string duplicateRecords =
        "0\nSECTION\n2\nLOCAL_DUP_ONE\n0\nREC_A\n5\n2A\n"
        "0\nENDSEC\n0\nSECTION\n2\nLOCAL_DUP_TWO\n0\nREC_B\n5\n2A\n"
        "0\nENDSEC\n0\nEOF\n";
    ProfileProbeInterface interface_;
    dxfRW reader("");
    std::string input = duplicateRecords;
    t.expect(!reader.readAscii(&interface_, false, input)
                 && reader.getError() == DRW::BAD_CODE_PARSED
                 && interface_.sections.size() == 1,
             "DXF duplicate raw-section handles preserve the first section callback");
    const DRW_OperationDiagnostic diagnostic = reader.getLastDiagnostic();
    t.expect(diagnostic.code == "duplicate-handle"
                 && diagnostic.hasHandle && diagnostic.handle == 0x2Au,
             "DXF duplicate raw-section handle records offending handle");

    const std::string freshRecords =
        "0\nSECTION\n2\nLOCAL_FRESH\n0\nREC_C\n5\n2A\n"
        "0\nENDSEC\n0\nEOF\n";
    ProfileProbeInterface freshInterface;
    dxfRW freshReader("");
    std::string freshInput = freshRecords;
    t.expect(freshReader.readAscii(&freshInterface, false, freshInput)
                 && freshInterface.sections.size() == 1,
             "DXF fresh raw-section read resets duplicate-handle scope");

    std::ostringstream binarySource;
    dxfWriterBinary binaryWriter(&binarySource);
    binaryWriter.writeString(0, "SECTION");
    binaryWriter.writeString(2, "LOCAL_BINARY_DUP_ONE");
    binaryWriter.writeString(0, "REC_A");
    binaryWriter.writeString(5, "2A");
    binaryWriter.writeString(0, "ENDSEC");
    binaryWriter.writeString(0, "SECTION");
    binaryWriter.writeString(2, "LOCAL_BINARY_DUP_TWO");
    binaryWriter.writeString(0, "REC_B");
    binaryWriter.writeString(5, "2A");
    binaryWriter.writeString(0, "ENDSEC");
    binaryWriter.writeString(0, "EOF");
    std::stringstream binaryInput(binarySource.str());
    ProfileProbeInterface binaryInterface;
    dxfRW binaryReader("");
    binaryReader.binFile = true;
    binaryReader.reader = std::make_unique<dxfReaderBinary>(&binaryInput);
    binaryReader.reader->setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    binaryReader.iface = &binaryInterface;
    binaryReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(!binaryReader.processDxf()
                 && binaryReader.getError() == DRW::BAD_CODE_PARSED
                 && binaryInterface.sections.size() == 1,
             "binary duplicate raw-section handles preserve the first callback");
    const DRW_OperationDiagnostic binaryDiagnostic = binaryReader.getLastDiagnostic();
    t.expect(binaryDiagnostic.code == "duplicate-handle"
                 && binaryDiagnostic.hasHandle && binaryDiagnostic.handle == 0x2Au,
             "binary duplicate raw-section handle records offending handle");
}

void testDxfRawSectionWideHandleReplay(TestContext& t) {
    const std::string wideSelf = "123456789ABCDEF0";
    const std::string wideReference = "FEDCBA9876543210";
    const std::string asciiSource =
        "0\nSECTION\n2\nLOCAL_WIDE_SECTION\n0\nRECORD\n5\n"
        + wideSelf + "\n330\n" + wideReference + "\n0\nENDSEC\n0\nEOF\n";
    ProfileProbeInterface interface_;
    dxfRW reader("");
    reader.binFile = false;
    std::stringstream asciiInput(asciiSource);
    reader.reader = std::make_unique<dxfReaderAscii>(&asciiInput);
    reader.reader->setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    reader.reader->setAllowWideHandleLexemes(true);
    reader.iface = &interface_;
    reader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(reader.processDxf() && interface_.sections.size() == 1,
             "DXF ASCII raw section captures wide handle lexemes");
    if (interface_.sections.size() == 1) {
        const DRW_RawDxfSection& captured = interface_.sections.front();
        t.expect(captured.m_groups.size() == 3
                     && captured.m_groups[1].code() == 5
                     && std::string(captured.m_groups[1].c_str()) == wideSelf
                     && captured.m_groups[2].code() == 330
                     && std::string(captured.m_groups[2].c_str()) == wideReference,
                 "DXF ASCII raw section retains wide handle strings");
        std::ostringstream replay;
        dxfRW writer("");
        writer.version = DRW::AC1027;
        writer.binFile = false;
        writer.writer = std::make_unique<dxfWriterAscii>(&replay);
        t.expect(writer.writeRawDxfSection(captured)
                     && replay.str().find(wideSelf) != std::string::npos
                     && replay.str().find(wideReference) != std::string::npos,
                 "DXF ASCII raw section wide handles replay losslessly");
    }

    std::ostringstream binarySource;
    dxfWriterBinary binarySourceWriter(&binarySource);
    binarySourceWriter.writeString(0, "SECTION");
    binarySourceWriter.writeString(2, "LOCAL_WIDE_SECTION");
    binarySourceWriter.writeString(0, "RECORD");
    binarySourceWriter.writeString(5, wideSelf);
    binarySourceWriter.writeString(330, wideReference);
    binarySourceWriter.writeString(0, "ENDSEC");
    binarySourceWriter.writeString(0, "EOF");
    std::stringstream binaryInput(binarySource.str());
    ProfileProbeInterface binaryInterface;
    dxfRW binaryReader("");
    binaryReader.binFile = true;
    binaryReader.reader = std::make_unique<dxfReaderBinary>(&binaryInput);
    binaryReader.reader->setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    binaryReader.reader->setAllowWideHandleLexemes(true);
    binaryReader.iface = &binaryInterface;
    binaryReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(binaryReader.processDxf() && binaryInterface.sections.size() == 1,
             "DXF binary raw section captures wide handle lexemes");
    if (binaryInterface.sections.size() == 1) {
        const DRW_RawDxfSection& captured = binaryInterface.sections.front();
        t.expect(captured.m_groups.size() == 3
                     && captured.m_groups[1].code() == 5
                     && std::string(captured.m_groups[1].c_str()) == wideSelf
                     && captured.m_groups[2].code() == 330
                     && std::string(captured.m_groups[2].c_str()) == wideReference,
                 "DXF binary raw section retains wide handle strings");
        std::ostringstream replay;
        dxfRW writer("");
        writer.version = DRW::AC1027;
        writer.binFile = true;
        writer.writer = std::make_unique<dxfWriterBinary>(&replay);
        DRW_RawDxfSection binaryCopy = captured;
        t.expect(writer.writeRawDxfSection(binaryCopy)
                     && !replay.str().empty(),
                 "DXF binary raw section wide handles replay losslessly");
        std::stringstream replayInput(replay.str());
        dxfReaderBinary replayReader(&replayInput);
        replayReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
        replayReader.setAllowWideHandleLexemes(true);
        int code = 0;
        bool retained = false;
        while (replayReader.readRec(&code)) {
            if ((code == 5 && replayReader.getString() == wideSelf)
                || (code == 330 && replayReader.getString() == wideReference))
                retained = true;
        }
        t.expect(retained,
                 "DXF binary raw section replay retains wide handle strings");
    }
}

void testDxfRawSectionWideHandleRemap(TestContext& t) {
    const std::string wideSelf = "123456789ABCDEF0";
    const std::string wideReference = "FEDCBA9876543210";
    DRW_RawDxfSection section;
    section.m_name = "LOCAL_WIDE_REMAP_SECTION";
    section.m_version = DRW::AC1027;
    section.m_hasRawValues = true;
    section.m_groups = {DRW_Variant(5, wideSelf),
                        DRW_Variant(102, std::string("{WIDE_REMAP")),
                        DRW_Variant(330, wideReference),
                        DRW_Variant(102, std::string("}"))};
    section.m_rawValues = {wideSelf, "{WIDE_REMAP", wideReference, "}"};
    const std::map<std::uint32_t, std::uint32_t> remap = {{0x1Au, 0x3Au},
                                                           {0x2Au, 0x4Au}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    asciiWriter.setHandleRemap(remap);
    t.expect(asciiWriter.writeRawDxfSection(section)
                 && asciiOutput.str().find(wideSelf) != std::string::npos
                 && asciiOutput.str().find(wideReference) != std::string::npos,
             "DXF ASCII raw section wide handles ignore narrow remap keys");

    DRW_RawDxfSection binarySection = section;
    binarySection.m_hasRawValues = false;
    binarySection.m_rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    binaryWriter.setHandleRemap(remap);
    t.expect(binaryWriter.writeRawDxfSection(binarySection)
                 && !binaryOutput.str().empty(),
             "DXF binary raw section wide handles ignore narrow remap keys");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    binaryReader.setAllowWideHandleLexemes(true);
    bool retainedSelf = false;
    bool retainedReference = false;
    int code = 0;
    while (binaryReader.readRec(&code)) {
        retainedSelf = retainedSelf
            || (code == 5 && binaryReader.getString() == wideSelf);
        retainedReference = retainedReference
            || (code == 330 && binaryReader.getString() == wideReference);
    }
    t.expect(retainedSelf && retainedReference,
             "DXF binary raw section preserves wide identities under remap");
}

void testDxfRawSectionRemapRollback(TestContext& t) {
    DRW_RawDxfSection malformed;
    malformed.m_name = "LOCAL_SECTION_ROLLBACK";
    malformed.m_version = DRW::AC1027;
    malformed.m_hasRawValues = true;
    malformed.m_groups = {DRW_Variant(102, std::string("{ROLLBACK")),
                          DRW_Variant(330, std::string("2A")),
                          DRW_Variant(90, std::string("not-an-int")),
                          DRW_Variant(102, std::string("}"))};
    malformed.m_rawValues = {"{ROLLBACK", "2A", "not-an-int", "}"};
    const std::map<std::uint32_t, std::uint32_t> remap = {{0x2Au, 0x4Au}};

    std::ostringstream rejectedAsciiOutput;
    dxfRW rejectingAsciiWriter("");
    rejectingAsciiWriter.version = DRW::AC1027;
    rejectingAsciiWriter.binFile = false;
    rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedAsciiOutput);
    rejectingAsciiWriter.setHandleRemap(remap);
    t.expect(!rejectingAsciiWriter.writeRawDxfSection(malformed)
                 && rejectedAsciiOutput.str().empty(),
             "DXF ASCII raw section remap rolls back malformed typed groups");

    DRW_RawDxfSection malformedBinary = malformed;
    malformedBinary.m_hasRawValues = false;
    malformedBinary.m_rawValues.clear();
    std::ostringstream rejectedBinaryOutput;
    dxfRW rejectingBinaryWriter("");
    rejectingBinaryWriter.version = DRW::AC1027;
    rejectingBinaryWriter.binFile = true;
    rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
        &rejectedBinaryOutput);
    rejectingBinaryWriter.setHandleRemap(remap);
    t.expect(!rejectingBinaryWriter.writeRawDxfSection(malformedBinary)
                 && rejectedBinaryOutput.str().empty(),
             "DXF binary raw section remap rolls back malformed typed groups");
}

void testDxfRawSectionReservedNames(TestContext& t) {
    const std::vector<std::string> reservedNames = {
        "", "HEADER", "header", "CLASSES", "classes", "TABLES",
        "tables", "BLOCKS", "blocks", "ENTITIES", "entities",
        "OBJECTS", "objects"};
    for (const std::string& name : reservedNames) {
        DRW_RawDxfSection section;
        section.m_name = name;
        section.m_version = DRW::AC1027;
        section.m_hasRawValues = true;
        section.m_groups = {DRW_Variant(1000, std::string("payload"))};
        section.m_rawValues = {"payload"};

        std::ostringstream asciiOutput;
        dxfRW asciiWriter("");
        asciiWriter.version = DRW::AC1027;
        asciiWriter.binFile = false;
        asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
        t.expect(!asciiWriter.writeRawDxfSection(section)
                     && asciiOutput.str().empty(),
                 "DXF ASCII raw section rejects reserved or empty names");

        DRW_RawDxfSection binarySection = section;
        binarySection.m_hasRawValues = false;
        binarySection.m_rawValues.clear();
        std::ostringstream binaryOutput;
        dxfRW binaryWriter("");
        binaryWriter.version = DRW::AC1027;
        binaryWriter.binFile = true;
        binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
        t.expect(!binaryWriter.writeRawDxfSection(binarySection)
                     && binaryOutput.str().empty(),
                 "DXF binary raw section rejects reserved or empty names");
    }

    DRW_RawDxfSection custom;
    custom.m_name = "LOCAL_RESERVED_NAME_CONTROL";
    custom.m_version = DRW::AC1027;
    custom.m_hasRawValues = true;
    custom.m_groups = {DRW_Variant(1000, std::string("payload"))};
    custom.m_rawValues = {"payload"};
    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiWriter.writeRawDxfSection(custom)
                 && !asciiOutput.str().empty(),
             "DXF ASCII raw section accepts a custom name");

    DRW_RawDxfSection binarySection = custom;
    binarySection.m_hasRawValues = false;
    binarySection.m_rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryWriter.writeRawDxfSection(binarySection)
                 && !binaryOutput.str().empty(),
             "DXF binary raw section accepts a custom name");
}

void testDxfRawSectionCustomFraming(TestContext& t) {
    DRW_RawDxfSection section;
    section.m_name = "LOCAL_CUSTOM_SECTION_FRAMING";
    section.m_version = DRW::AC1027;
    section.m_hasRawValues = true;
    section.m_groups = {DRW_Variant(1000, std::string("payload"))};
    section.m_rawValues = {"payload"};
    const std::vector<std::pair<int, std::string>> expected = {
        {0, "SECTION"}, {2, section.m_name}, {1000, "payload"},
        {0, "ENDSEC"}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiWriter.writeRawDxfSection(section),
             "DXF ASCII raw section custom framing writes");
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    std::vector<std::pair<int, std::string>> asciiActual;
    int asciiCode = 0;
    while (asciiReader.readRec(&asciiCode))
        asciiActual.emplace_back(asciiCode, asciiReader.getString());
    t.expect(asciiActual == expected,
             "DXF ASCII raw section emits exact SECTION/name/payload/ENDSEC framing");

    DRW_RawDxfSection binarySection = section;
    binarySection.m_hasRawValues = false;
    binarySection.m_rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryWriter.writeRawDxfSection(binarySection),
             "DXF binary raw section custom framing writes");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    std::vector<std::pair<int, std::string>> binaryActual;
    int binaryCode = 0;
    while (binaryReader.readRec(&binaryCode))
        binaryActual.emplace_back(binaryCode, binaryReader.getString());
    t.expect(binaryActual == expected,
             "DXF binary raw section emits exact SECTION/name/payload/ENDSEC framing");
}

void testDxfRawSectionVersionCompatibility(TestContext& t) {
    const std::vector<DRW::Version> accepted = {
        DRW::AC1027, DRW::UNKNOWNV};
    for (const DRW::Version taggedVersion : accepted) {
        DRW_RawDxfSection section;
        section.m_name = "LOCAL_SECTION_VERSION_ACCEPTED";
        section.m_version = taggedVersion;
        section.m_hasRawValues = true;
        section.m_groups = {DRW_Variant(1000, std::string("payload"))};
        section.m_rawValues = {"payload"};

        std::ostringstream asciiOutput;
        dxfRW asciiWriter("");
        asciiWriter.version = DRW::AC1027;
        asciiWriter.binFile = false;
        asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
        t.expect(asciiWriter.writeRawDxfSection(section)
                     && !asciiOutput.str().empty(),
                 "DXF ASCII raw section accepts matching and unknown versions");

        DRW_RawDxfSection binarySection = section;
        binarySection.m_hasRawValues = false;
        binarySection.m_rawValues.clear();
        std::ostringstream binaryOutput;
        dxfRW binaryWriter("");
        binaryWriter.version = DRW::AC1027;
        binaryWriter.binFile = true;
        binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
        t.expect(binaryWriter.writeRawDxfSection(binarySection)
                     && !binaryOutput.str().empty(),
                 "DXF binary raw section accepts matching and unknown versions");
    }

    DRW_RawDxfSection mismatched;
    mismatched.m_name = "LOCAL_SECTION_VERSION_MISMATCH";
    mismatched.m_version = DRW::AC1024;
    mismatched.m_hasRawValues = true;
    mismatched.m_groups = {DRW_Variant(1000, std::string("payload"))};
    mismatched.m_rawValues = {"payload"};
    std::ostringstream rejectedAsciiOutput;
    dxfRW rejectingAsciiWriter("");
    rejectingAsciiWriter.version = DRW::AC1027;
    rejectingAsciiWriter.binFile = false;
    rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedAsciiOutput);
    t.expect(!rejectingAsciiWriter.writeRawDxfSection(mismatched)
                 && rejectedAsciiOutput.str().empty(),
             "DXF ASCII raw section rejects mismatched versions transactionally");

    DRW_RawDxfSection mismatchedBinary = mismatched;
    mismatchedBinary.m_hasRawValues = false;
    mismatchedBinary.m_rawValues.clear();
    std::ostringstream rejectedBinaryOutput;
    dxfRW rejectingBinaryWriter("");
    rejectingBinaryWriter.version = DRW::AC1027;
    rejectingBinaryWriter.binFile = true;
    rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
        &rejectedBinaryOutput);
    t.expect(!rejectingBinaryWriter.writeRawDxfSection(mismatchedBinary)
                 && rejectedBinaryOutput.str().empty(),
             "DXF binary raw section rejects mismatched versions transactionally");
}

void testDxfRawSectionEmptyPayload(TestContext& t) {
    DRW_RawDxfSection section;
    section.m_name = "LOCAL_EMPTY_SECTION_PAYLOAD";
    section.m_version = DRW::AC1027;
    section.m_hasRawValues = true;
    const std::vector<std::pair<int, std::string>> expected = {
        {0, "SECTION"}, {2, section.m_name}, {0, "ENDSEC"}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiWriter.writeRawDxfSection(section),
             "DXF ASCII raw section accepts an empty payload");
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    std::vector<std::pair<int, std::string>> asciiActual;
    int asciiCode = 0;
    while (asciiReader.readRec(&asciiCode))
        asciiActual.emplace_back(asciiCode, asciiReader.getString());
    t.expect(asciiActual == expected,
             "DXF ASCII empty section emits only framing records");

    DRW_RawDxfSection binarySection = section;
    binarySection.m_hasRawValues = false;
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryWriter.writeRawDxfSection(binarySection),
             "DXF binary raw section accepts an empty payload");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    std::vector<std::pair<int, std::string>> binaryActual;
    int binaryCode = 0;
    while (binaryReader.readRec(&binaryCode))
        binaryActual.emplace_back(binaryCode, binaryReader.getString());
    t.expect(binaryActual == expected,
             "DXF binary empty section emits only framing records");
}

void testDxfRawSectionCommentPreservation(TestContext& t) {
    DRW_RawDxfSection section;
    section.m_name = "LOCAL_SECTION_COMMENTS";
    section.m_version = DRW::AC1027;
    section.m_hasRawValues = true;
    section.m_groups = {DRW_Variant(999, std::string("before comment")),
                        DRW_Variant(1000, std::string("payload")),
                        DRW_Variant(999, std::string("after COMMENT"))};
    section.m_rawValues = {"before comment", "payload", "after COMMENT"};
    const std::vector<std::pair<int, std::string>> expected = {
        {0, "SECTION"}, {2, section.m_name}, {999, "before comment"},
        {1000, "payload"}, {999, "after COMMENT"}, {0, "ENDSEC"}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiWriter.writeRawDxfSection(section),
             "DXF ASCII raw section writes comments around payload");
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    std::vector<std::pair<int, std::string>> asciiActual;
    int asciiCode = 0;
    while (asciiReader.readRec(&asciiCode))
        asciiActual.emplace_back(asciiCode, asciiReader.getString());
    t.expect(asciiActual == expected,
             "DXF ASCII raw section preserves code-999 comments and framing");

    DRW_RawDxfSection binarySection = section;
    binarySection.m_hasRawValues = false;
    binarySection.m_rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(!binaryWriter.writeRawDxfSection(binarySection)
                 && binaryOutput.str().empty(),
             "DXF binary raw section rejects code-999 comments transactionally");
}

void testDxfRawSectionCommentReadPolicy(TestContext& t) {
    const std::string content =
        "0\nSECTION\n2\nLOCAL_COMMENT_READ\n"
        "999\nbefore comment\n1000\npayload\n999\nafter COMMENT\n"
        "0\nENDSEC\n0\nEOF\n";
    ProfileProbeInterface interface_;
    dxfRW reader("");
    std::string input = content;
    t.expect(reader.readAscii(&interface_, false, input)
                 && interface_.sections.size() == 1,
             "DXF ASCII raw section reads through code-999 comments");
    if (interface_.sections.size() == 1) {
        const DRW_RawDxfSection& captured = interface_.sections.front();
        t.expect(captured.m_groups.size() == 1
                     && captured.m_groups.front().code() == 1000
                     && std::string(captured.m_groups.front().c_str()) == "payload"
                     && captured.m_rawValues.size() == 1
                     && captured.m_rawValues.front() == "payload",
                 "DXF ASCII raw section filters comments but retains payload");
    }
}

void testDxfRawSectionCaseInsensitiveEndsec(TestContext& t) {
    const std::string asciiSource =
        "0\nSECTION\n2\nLOCAL_ENDSEC_CASE\n1000\npayload\n"
        "0\neNdSeC\n0\nEOF\n";
    ProfileProbeInterface asciiInterface;
    dxfRW asciiReader("");
    asciiReader.binFile = false;
    std::stringstream asciiInput(asciiSource);
    asciiReader.reader = std::make_unique<dxfReaderAscii>(&asciiInput);
    asciiReader.reader->setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    asciiReader.iface = &asciiInterface;
    asciiReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(asciiReader.processDxf()
                 && asciiInterface.sections.size() == 1
                 && asciiInterface.sections.front().m_name == "LOCAL_ENDSEC_CASE"
                 && asciiInterface.sections.front().m_groups.size() == 1
                 && asciiInterface.sections.front().m_groups.front().code() == 1000,
             "DXF ASCII raw section accepts mixed-case ENDSEC");

    std::ostringstream binarySource;
    dxfWriterBinary binarySourceWriter(&binarySource);
    binarySourceWriter.writeString(0, "SECTION");
    binarySourceWriter.writeString(2, "LOCAL_ENDSEC_CASE");
    binarySourceWriter.writeString(1000, "payload");
    binarySourceWriter.writeString(0, "eNdSeC");
    binarySourceWriter.writeString(0, "EOF");
    std::stringstream binaryInput(binarySource.str());
    ProfileProbeInterface binaryInterface;
    dxfRW binaryReader("");
    binaryReader.binFile = true;
    binaryReader.reader = std::make_unique<dxfReaderBinary>(&binaryInput);
    binaryReader.reader->setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    binaryReader.iface = &binaryInterface;
    binaryReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(binaryReader.processDxf()
                 && binaryInterface.sections.size() == 1
                 && binaryInterface.sections.front().m_name == "LOCAL_ENDSEC_CASE"
                 && binaryInterface.sections.front().m_groups.size() == 1
                 && binaryInterface.sections.front().m_groups.front().code() == 1000,
             "DXF binary raw section accepts mixed-case ENDSEC");
}

void testDxfRawSectionCaseInsensitiveSection(TestContext& t) {
    const std::string asciiSource =
        "0\nsEcTiOn\n2\nLOCAL_SECTION_CASE\n1000\npayload\n"
        "0\nENDSEC\n0\nEOF\n";
    ProfileProbeInterface asciiInterface;
    dxfRW asciiReader("");
    asciiReader.binFile = false;
    std::stringstream asciiInput(asciiSource);
    asciiReader.reader = std::make_unique<dxfReaderAscii>(&asciiInput);
    asciiReader.reader->setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    asciiReader.iface = &asciiInterface;
    asciiReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(asciiReader.processDxf()
                 && asciiInterface.sections.size() == 1
                 && asciiInterface.sections.front().m_name == "LOCAL_SECTION_CASE"
                 && asciiInterface.sections.front().m_groups.size() == 1
                 && asciiInterface.sections.front().m_groups.front().code() == 1000,
             "DXF ASCII raw section accepts mixed-case SECTION");

    std::ostringstream binarySource;
    dxfWriterBinary binarySourceWriter(&binarySource);
    binarySourceWriter.writeString(0, "sEcTiOn");
    binarySourceWriter.writeString(2, "LOCAL_SECTION_CASE");
    binarySourceWriter.writeString(1000, "payload");
    binarySourceWriter.writeString(0, "ENDSEC");
    binarySourceWriter.writeString(0, "EOF");
    std::stringstream binaryInput(binarySource.str());
    ProfileProbeInterface binaryInterface;
    dxfRW binaryReader("");
    binaryReader.binFile = true;
    binaryReader.reader = std::make_unique<dxfReaderBinary>(&binaryInput);
    binaryReader.reader->setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    binaryReader.iface = &binaryInterface;
    binaryReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(binaryReader.processDxf()
                 && binaryInterface.sections.size() == 1
                 && binaryInterface.sections.front().m_name == "LOCAL_SECTION_CASE"
                 && binaryInterface.sections.front().m_groups.size() == 1
                 && binaryInterface.sections.front().m_groups.front().code() == 1000,
             "DXF binary raw section accepts mixed-case SECTION");
}

void testDxfRawSectionCaseInsensitiveEof(TestContext& t) {
    const std::string asciiSource =
        "0\nSECTION\n2\nLOCAL_EOF_CASE\n1000\npayload\n"
        "0\nENDSEC\n0\neOf\n";
    ProfileProbeInterface asciiInterface;
    dxfRW asciiReader("");
    asciiReader.binFile = false;
    std::stringstream asciiInput(asciiSource);
    asciiReader.reader = std::make_unique<dxfReaderAscii>(&asciiInput);
    asciiReader.reader->setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    asciiReader.iface = &asciiInterface;
    asciiReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(asciiReader.processDxf()
                 && asciiInterface.sections.size() == 1
                 && asciiInterface.sections.front().m_name == "LOCAL_EOF_CASE"
                 && asciiInterface.sections.front().m_groups.size() == 1,
             "DXF ASCII raw section accepts mixed-case EOF");

    std::ostringstream binarySource;
    dxfWriterBinary binarySourceWriter(&binarySource);
    binarySourceWriter.writeString(0, "SECTION");
    binarySourceWriter.writeString(2, "LOCAL_EOF_CASE");
    binarySourceWriter.writeString(1000, "payload");
    binarySourceWriter.writeString(0, "ENDSEC");
    binarySourceWriter.writeString(0, "eOf");
    std::stringstream binaryInput(binarySource.str());
    ProfileProbeInterface binaryInterface;
    dxfRW binaryReader("");
    binaryReader.binFile = true;
    binaryReader.reader = std::make_unique<dxfReaderBinary>(&binaryInput);
    binaryReader.reader->setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    binaryReader.iface = &binaryInterface;
    binaryReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(binaryReader.processDxf()
                 && binaryInterface.sections.size() == 1
                 && binaryInterface.sections.front().m_name == "LOCAL_EOF_CASE"
                 && binaryInterface.sections.front().m_groups.size() == 1,
             "DXF binary raw section accepts mixed-case EOF");
}

void testDxfRawSectionFinalEofWithoutNewline(TestContext& t) {
    const std::string content =
        "0\nSECTION\n2\nLOCAL_FINAL_EOF\n1000\npayload\n"
        "0\nENDSEC\n0\nEOF";
    ProfileProbeInterface interface_;
    dxfRW reader("");
    std::string input = content;
    t.expect(reader.readAscii(&interface_, false, input)
                 && interface_.sections.size() == 1
                 && interface_.sections.front().m_name == "LOCAL_FINAL_EOF"
                 && interface_.sections.front().m_groups.size() == 1
                 && interface_.sections.front().m_groups.front().code() == 1000,
             "DXF ASCII raw section accepts EOF without trailing newline");
}

void testDxfRawSectionMissingEof(TestContext& t) {
    const std::string asciiSource =
        "0\nSECTION\n2\nLOCAL_MISSING_EOF\n1000\npayload\n"
        "0\nENDSEC\n";
    ProfileProbeInterface asciiInterface;
    dxfRW asciiReader("");
    asciiReader.binFile = false;
    std::stringstream asciiInput(asciiSource);
    asciiReader.reader = std::make_unique<dxfReaderAscii>(&asciiInput);
    asciiReader.reader->setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    asciiReader.iface = &asciiInterface;
    asciiReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(!asciiReader.processDxf()
                 && asciiReader.getError() == DRW::BAD_UNKNOWN
                 && asciiInterface.sections.size() == 1,
             "DXF ASCII raw section rejects a missing EOF marker");

    std::ostringstream binarySource;
    dxfWriterBinary binarySourceWriter(&binarySource);
    binarySourceWriter.writeString(0, "SECTION");
    binarySourceWriter.writeString(2, "LOCAL_MISSING_EOF");
    binarySourceWriter.writeString(1000, "payload");
    binarySourceWriter.writeString(0, "ENDSEC");
    std::stringstream binaryInput(binarySource.str());
    ProfileProbeInterface binaryInterface;
    dxfRW binaryReader("");
    binaryReader.binFile = true;
    binaryReader.reader = std::make_unique<dxfReaderBinary>(&binaryInput);
    binaryReader.reader->setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    binaryReader.iface = &binaryInterface;
    binaryReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(!binaryReader.processDxf()
                 && binaryReader.getError() == DRW::BAD_UNKNOWN
                 && binaryInterface.sections.size() == 1,
             "DXF binary raw section rejects a missing EOF marker");
}

void testDxfRawSectionMissingEndsec(TestContext& t) {
    const std::string asciiSource =
        "0\nSECTION\n2\nLOCAL_MISSING_ENDSEC\n1000\npayload\n";
    ProfileProbeInterface asciiInterface;
    dxfRW asciiReader("");
    asciiReader.binFile = false;
    std::stringstream asciiInput(asciiSource);
    asciiReader.reader = std::make_unique<dxfReaderAscii>(&asciiInput);
    asciiReader.reader->setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    asciiReader.iface = &asciiInterface;
    asciiReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(!asciiReader.processDxf()
                 && asciiReader.getError() == DRW::BAD_READ_SECTION
                 && asciiInterface.sections.empty(),
             "DXF ASCII raw section rejects a missing ENDSEC marker");

    std::ostringstream binarySource;
    dxfWriterBinary binarySourceWriter(&binarySource);
    binarySourceWriter.writeString(0, "SECTION");
    binarySourceWriter.writeString(2, "LOCAL_MISSING_ENDSEC");
    binarySourceWriter.writeString(1000, "payload");
    std::stringstream binaryInput(binarySource.str());
    ProfileProbeInterface binaryInterface;
    dxfRW binaryReader("");
    binaryReader.binFile = true;
    binaryReader.reader = std::make_unique<dxfReaderBinary>(&binaryInput);
    binaryReader.reader->setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    binaryReader.iface = &binaryInterface;
    binaryReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(!binaryReader.processDxf()
                 && binaryReader.getError() == DRW::BAD_READ_SECTION
                 && binaryInterface.sections.empty(),
             "DXF binary raw section rejects a missing ENDSEC marker");
}

void testDxfRawSectionEmptyNameRead(TestContext& t) {
    const std::string asciiSource =
        "0\nSECTION\n2\n\n0\nENDSEC\n0\nEOF\n";
    ProfileProbeInterface asciiInterface;
    dxfRW asciiReader("");
    asciiReader.binFile = false;
    std::stringstream asciiInput(asciiSource);
    asciiReader.reader = std::make_unique<dxfReaderAscii>(&asciiInput);
    asciiReader.reader->setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    asciiReader.iface = &asciiInterface;
    asciiReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(!asciiReader.processDxf()
                 && asciiReader.getError() == DRW::BAD_READ_SECTION
                 && asciiInterface.sections.empty(),
             "DXF ASCII raw section rejects an empty section name");

    std::ostringstream binarySource;
    dxfWriterBinary binarySourceWriter(&binarySource);
    binarySourceWriter.writeString(0, "SECTION");
    binarySourceWriter.writeString(2, "");
    binarySourceWriter.writeString(0, "ENDSEC");
    binarySourceWriter.writeString(0, "EOF");
    std::stringstream binaryInput(binarySource.str());
    ProfileProbeInterface binaryInterface;
    dxfRW binaryReader("");
    binaryReader.binFile = true;
    binaryReader.reader = std::make_unique<dxfReaderBinary>(&binaryInput);
    binaryReader.reader->setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    binaryReader.iface = &binaryInterface;
    binaryReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(!binaryReader.processDxf()
                 && binaryReader.getError() == DRW::BAD_READ_SECTION
                 && binaryInterface.sections.empty(),
             "DXF binary raw section rejects an empty section name");
}

void testDxfRawSectionRecordBoundaries(TestContext& t) {
    const std::string asciiSource =
        "0\nSECTION\n2\nLOCAL_RECORD_BOUNDARIES\n"
        "0\nREC_A\n1000\npayload A\n0\nREC_B\n1000\npayload B\n"
        "0\nENDSEC\n0\nEOF\n";
    const std::vector<std::pair<int, std::string>> expected = {
        {0, "REC_A"}, {1000, "payload A"}, {0, "REC_B"},
        {1000, "payload B"}};
    ProfileProbeInterface asciiInterface;
    dxfRW asciiReader("");
    asciiReader.binFile = false;
    std::stringstream asciiInput(asciiSource);
    asciiReader.reader = std::make_unique<dxfReaderAscii>(&asciiInput);
    asciiReader.reader->setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    asciiReader.iface = &asciiInterface;
    asciiReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(asciiReader.processDxf()
                 && asciiInterface.sections.size() == 1,
             "DXF ASCII raw section reads code-0 record boundaries");
    if (asciiInterface.sections.size() == 1) {
        const DRW_RawDxfSection& section = asciiInterface.sections.front();
        std::vector<std::pair<int, std::string>> actual;
        for (std::size_t i = 0; i < section.m_groups.size(); ++i)
            actual.emplace_back(section.m_groups[i].code(),
                                section.m_groups[i].c_str());
        t.expect(actual == expected,
                 "DXF ASCII raw section preserves record-boundary order");
    }

    std::ostringstream binarySource;
    dxfWriterBinary binarySourceWriter(&binarySource);
    binarySourceWriter.writeString(0, "SECTION");
    binarySourceWriter.writeString(2, "LOCAL_RECORD_BOUNDARIES");
    binarySourceWriter.writeString(0, "REC_A");
    binarySourceWriter.writeString(1000, "payload A");
    binarySourceWriter.writeString(0, "REC_B");
    binarySourceWriter.writeString(1000, "payload B");
    binarySourceWriter.writeString(0, "ENDSEC");
    binarySourceWriter.writeString(0, "EOF");
    std::stringstream binaryInput(binarySource.str());
    ProfileProbeInterface binaryInterface;
    dxfRW binaryReader("");
    binaryReader.binFile = true;
    binaryReader.reader = std::make_unique<dxfReaderBinary>(&binaryInput);
    binaryReader.reader->setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    binaryReader.iface = &binaryInterface;
    binaryReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(binaryReader.processDxf()
                 && binaryInterface.sections.size() == 1,
             "DXF binary raw section reads code-0 record boundaries");
    if (binaryInterface.sections.size() == 1) {
        const DRW_RawDxfSection& section = binaryInterface.sections.front();
        std::vector<std::pair<int, std::string>> actual;
        for (const DRW_Variant& group : section.m_groups)
            actual.emplace_back(group.code(), group.c_str());
        t.expect(actual == expected,
                 "DXF binary raw section preserves record-boundary order");
    }
}

void testDxfRawSectionRecordBoundaryReplay(TestContext& t) {
    DRW_RawDxfSection section;
    section.m_name = "LOCAL_BOUNDARY_REPLAY";
    section.m_version = DRW::AC1027;
    section.m_hasRawValues = true;
    section.m_groups = {DRW_Variant(0, std::string("REC_A")),
                        DRW_Variant(1000, std::string("payload A")),
                        DRW_Variant(0, std::string("REC_B")),
                        DRW_Variant(1000, std::string("payload B"))};
    section.m_rawValues = {"REC_A", "payload A", "REC_B", "payload B"};
    const std::vector<std::pair<int, std::string>> expected = {
        {0, "SECTION"}, {2, section.m_name}, {0, "REC_A"},
        {1000, "payload A"}, {0, "REC_B"}, {1000, "payload B"},
        {0, "ENDSEC"}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiWriter.writeRawDxfSection(section),
             "DXF ASCII raw section replays record boundaries");
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    std::vector<std::pair<int, std::string>> asciiActual;
    int asciiCode = 0;
    while (asciiReader.readRec(&asciiCode))
        asciiActual.emplace_back(asciiCode, asciiReader.getString());
    t.expect(asciiActual == expected,
             "DXF ASCII raw section boundary replay preserves exact framing");

    DRW_RawDxfSection binarySection = section;
    binarySection.m_hasRawValues = false;
    binarySection.m_rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryWriter.writeRawDxfSection(binarySection),
             "DXF binary raw section replays record boundaries");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    std::vector<std::pair<int, std::string>> binaryActual;
    int binaryCode = 0;
    while (binaryReader.readRec(&binaryCode))
        binaryActual.emplace_back(binaryCode, binaryReader.getString());
    t.expect(binaryActual == expected,
             "DXF binary raw section boundary replay preserves exact framing");
}

void testDxfRawSectionEndsecTerminator(TestContext& t) {
    const std::string asciiSource =
        "0\nSECTION\n2\nLOCAL_ENDSEC_BOUNDARY\n"
        "0\nREC_A\n1000\npayload A\n0\nENDSEC\n0\nEOF\n";
    ProfileProbeInterface asciiInterface;
    dxfRW asciiReader("");
    asciiReader.binFile = false;
    std::stringstream asciiInput(asciiSource);
    asciiReader.reader = std::make_unique<dxfReaderAscii>(&asciiInput);
    asciiReader.reader->setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    asciiReader.iface = &asciiInterface;
    asciiReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(asciiReader.processDxf()
                 && asciiInterface.sections.size() == 1,
             "DXF ASCII raw section terminates on ENDSEC");
    if (asciiInterface.sections.size() == 1) {
        const DRW_RawDxfSection& section = asciiInterface.sections.front();
        t.expect(section.m_groups.size() == 2
                     && section.m_groups[0].code() == 0
                     && std::string(section.m_groups[0].c_str()) == "REC_A"
                     && section.m_groups[1].code() == 1000,
                 "DXF ASCII raw section excludes structural ENDSEC");
    }

    std::ostringstream binarySource;
    dxfWriterBinary binarySourceWriter(&binarySource);
    binarySourceWriter.writeString(0, "SECTION");
    binarySourceWriter.writeString(2, "LOCAL_ENDSEC_BOUNDARY");
    binarySourceWriter.writeString(0, "REC_A");
    binarySourceWriter.writeString(1000, "payload A");
    binarySourceWriter.writeString(0, "ENDSEC");
    binarySourceWriter.writeString(0, "EOF");
    std::stringstream binaryInput(binarySource.str());
    ProfileProbeInterface binaryInterface;
    dxfRW binaryReader("");
    binaryReader.binFile = true;
    binaryReader.reader = std::make_unique<dxfReaderBinary>(&binaryInput);
    binaryReader.reader->setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    binaryReader.iface = &binaryInterface;
    binaryReader.beginOperationDiagnostic(DRW::OperationKind::Read);
    t.expect(binaryReader.processDxf()
                 && binaryInterface.sections.size() == 1,
             "DXF binary raw section terminates on ENDSEC");
    if (binaryInterface.sections.size() == 1) {
        const DRW_RawDxfSection& section = binaryInterface.sections.front();
        t.expect(section.m_groups.size() == 2
                     && section.m_groups[0].code() == 0
                     && std::string(section.m_groups[0].c_str()) == "REC_A"
                     && section.m_groups[1].code() == 1000,
                 "DXF binary raw section excludes structural ENDSEC");
    }
}

void testDxfRawSectionGroupCodeBounds(TestContext& t) {
    const std::vector<int> invalidCodes = {-1, 1072};
    for (const int code : invalidCodes) {
        DRW_RawDxfSection section;
        section.m_name = "LOCAL_GROUP_CODE_BOUNDS";
        section.m_version = DRW::AC1027;
        section.m_hasRawValues = true;
        section.m_groups = {DRW_Variant(code, std::string("payload"))};
        section.m_rawValues = {"payload"};

        std::ostringstream asciiOutput;
        dxfRW asciiWriter("");
        asciiWriter.version = DRW::AC1027;
        asciiWriter.binFile = false;
        asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
        t.expect(!asciiWriter.writeRawDxfSection(section)
                     && asciiOutput.str().empty(),
                 "DXF ASCII raw section rejects out-of-range group codes");

        DRW_RawDxfSection binarySection = section;
        binarySection.m_hasRawValues = false;
        binarySection.m_rawValues.clear();
        std::ostringstream binaryOutput;
        dxfRW binaryWriter("");
        binaryWriter.version = DRW::AC1027;
        binaryWriter.binFile = true;
        binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
        t.expect(!binaryWriter.writeRawDxfSection(binarySection)
                     && binaryOutput.str().empty(),
             "DXF binary raw section rejects out-of-range group codes");
    }
}

void testDxfRawSectionAggregateLimit(TestContext& t) {
    const auto makeSection = [](std::size_t pairCount, bool hasRawValues) {
        DRW_RawDxfSection section;
        section.m_name = "LOCAL_SECTION_AGGREGATE";
        section.m_version = DRW::AC1027;
        section.m_hasRawValues = hasRawValues;
        section.m_groups.reserve(pairCount);
        if (hasRawValues)
            section.m_rawValues.reserve(pairCount);
        for (std::size_t i = 0; i < pairCount; ++i) {
            section.m_groups.emplace_back(1000, std::string("payload"));
            if (hasRawValues)
                section.m_rawValues.emplace_back("payload");
        }
        return section;
    };
    constexpr std::size_t maxPairs = DRW::kMaxDxfApplicationGroupPairs;

    DRW_RawDxfSection asciiBoundary = makeSection(maxPairs, true);
    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiBoundary.m_groups.size() == maxPairs
                 && asciiWriter.writeRawDxfSection(asciiBoundary)
                 && !asciiOutput.str().empty(),
             "DXF ASCII raw section aggregate limit is accepted");

    DRW_RawDxfSection binaryBoundary = makeSection(maxPairs, false);
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryBoundary.m_groups.size() == maxPairs
                 && binaryWriter.writeRawDxfSection(binaryBoundary)
                 && !binaryOutput.str().empty(),
             "DXF binary raw section aggregate limit is accepted");

    DRW_RawDxfSection asciiOver = makeSection(maxPairs + 1u, true);
    std::ostringstream rejectedAsciiOutput;
    dxfRW rejectingAsciiWriter("");
    rejectingAsciiWriter.version = DRW::AC1027;
    rejectingAsciiWriter.binFile = false;
    rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedAsciiOutput);
    t.expect(!rejectingAsciiWriter.writeRawDxfSection(asciiOver)
                 && rejectedAsciiOutput.str().empty(),
             "DXF ASCII raw section over-limit aggregate rejects transactionally");

    DRW_RawDxfSection binaryOver = makeSection(maxPairs + 1u, false);
    std::ostringstream rejectedBinaryOutput;
    dxfRW rejectingBinaryWriter("");
    rejectingBinaryWriter.version = DRW::AC1027;
    rejectingBinaryWriter.binFile = true;
    rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
        &rejectedBinaryOutput);
    t.expect(!rejectingBinaryWriter.writeRawDxfSection(binaryOver)
                 && rejectedBinaryOutput.str().empty(),
             "DXF binary raw section over-limit aggregate rejects transactionally");
}

void testDxfRawSectionDepthLimit(TestContext& t) {
    const auto makeSection = [](std::size_t depth, bool hasRawValues) {
        DRW_RawDxfSection section;
        section.m_name = "LOCAL_SECTION_DEPTH";
        section.m_version = DRW::AC1027;
        section.m_hasRawValues = hasRawValues;
        section.m_groups.reserve(depth * 2u);
        if (hasRawValues)
            section.m_rawValues.reserve(depth * 2u);
        for (std::size_t i = 0; i < depth; ++i) {
            section.m_groups.emplace_back(102, std::string("{DEPTH"));
            if (hasRawValues)
                section.m_rawValues.emplace_back("{DEPTH");
        }
        for (std::size_t i = 0; i < depth; ++i) {
            section.m_groups.emplace_back(102, std::string("}"));
            if (hasRawValues)
                section.m_rawValues.emplace_back("}");
        }
        return section;
    };
    constexpr std::size_t maxDepth = DRW::kMaxDxfApplicationGroupNesting;

    DRW_RawDxfSection asciiBoundary = makeSection(maxDepth, true);
    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiWriter.writeRawDxfSection(asciiBoundary)
                 && !asciiOutput.str().empty(),
             "DXF ASCII raw section maximum application-group depth is accepted");

    DRW_RawDxfSection binaryBoundary = makeSection(maxDepth, false);
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryWriter.writeRawDxfSection(binaryBoundary)
                 && !binaryOutput.str().empty(),
             "DXF binary raw section maximum application-group depth is accepted");

    DRW_RawDxfSection asciiOver = makeSection(maxDepth + 1u, true);
    std::ostringstream rejectedAsciiOutput;
    dxfRW rejectingAsciiWriter("");
    rejectingAsciiWriter.version = DRW::AC1027;
    rejectingAsciiWriter.binFile = false;
    rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedAsciiOutput);
    t.expect(!rejectingAsciiWriter.writeRawDxfSection(asciiOver)
                 && rejectedAsciiOutput.str().empty(),
             "DXF ASCII raw section over-depth rejects transactionally");

    DRW_RawDxfSection binaryOver = makeSection(maxDepth + 1u, false);
    std::ostringstream rejectedBinaryOutput;
    dxfRW rejectingBinaryWriter("");
    rejectingBinaryWriter.version = DRW::AC1027;
    rejectingBinaryWriter.binFile = true;
    rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
        &rejectedBinaryOutput);
    t.expect(!rejectingBinaryWriter.writeRawDxfSection(binaryOver)
                 && rejectedBinaryOutput.str().empty(),
             "DXF binary raw section over-depth rejects transactionally");
}

void testDxfRawSectionApplicationGroupValidMarker(TestContext& t) {
    DRW_RawDxfSection section;
    section.m_name = "LOCAL_SECTION_VALID_MARKER";
    section.m_version = DRW::AC1027;
    section.m_hasRawValues = true;
    section.m_groups = {DRW_Variant(102, std::string("{VALID_GROUP")),
                        DRW_Variant(1000, std::string("payload")),
                        DRW_Variant(102, std::string("}"))};
    section.m_rawValues = {"{VALID_GROUP", "payload", "}"};
    const std::vector<std::pair<int, std::string>> expected = {
        {0, "SECTION"}, {2, section.m_name}, {102, "{VALID_GROUP"},
        {1000, "payload"}, {102, "}"}, {0, "ENDSEC"}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiWriter.writeRawDxfSection(section),
             "DXF ASCII raw section accepts valid application-group markers");
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    std::vector<std::pair<int, std::string>> asciiActual;
    int asciiCode = 0;
    while (asciiReader.readRec(&asciiCode))
        asciiActual.emplace_back(asciiCode, asciiReader.getString());
    t.expect(asciiActual == expected,
             "DXF ASCII raw section replays valid application-group markers");

    DRW_RawDxfSection binarySection = section;
    binarySection.m_hasRawValues = false;
    binarySection.m_rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryWriter.writeRawDxfSection(binarySection),
             "DXF binary raw section accepts valid application-group markers");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    std::vector<std::pair<int, std::string>> binaryActual;
    int binaryCode = 0;
    while (binaryReader.readRec(&binaryCode))
        binaryActual.emplace_back(binaryCode, binaryReader.getString());
    t.expect(binaryActual == expected,
             "DXF binary raw section replays valid application-group markers");
}

void testDxfRawSectionApplicationGroupMarker(TestContext& t) {
    const std::vector<std::string> invalidMarkers = {"{", "NOT_A_MARKER"};
    for (const std::string& marker : invalidMarkers) {
        DRW_RawDxfSection section;
        section.m_name = "LOCAL_SECTION_MARKER";
        section.m_version = DRW::AC1027;
        section.m_hasRawValues = true;
        section.m_groups = {DRW_Variant(102, marker)};
        section.m_rawValues = {marker};

        std::ostringstream asciiOutput;
        dxfRW asciiWriter("");
        asciiWriter.version = DRW::AC1027;
        asciiWriter.binFile = false;
        asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
        t.expect(!asciiWriter.writeRawDxfSection(section)
                     && asciiOutput.str().empty(),
                 "DXF ASCII raw section invalid marker rejects transactionally");

        DRW_RawDxfSection binarySection = section;
        binarySection.m_hasRawValues = false;
        binarySection.m_rawValues.clear();
        std::ostringstream binaryOutput;
        dxfRW binaryWriter("");
        binaryWriter.version = DRW::AC1027;
        binaryWriter.binFile = true;
        binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
        t.expect(!binaryWriter.writeRawDxfSection(binarySection)
                     && binaryOutput.str().empty(),
                 "DXF binary raw section invalid marker rejects transactionally");
    }
}

void testDxfRawSectionWriterPreflight(TestContext& t) {
    DRW_RawDxfSection section;
    section.m_name = "LOCAL_SECTION_PREFLIGHT";
    section.m_version = DRW::AC1027;
    section.m_hasRawValues = true;
    section.m_groups = {DRW_Variant(1000, std::string("payload"))};
    section.m_rawValues = {"payload"};

    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    t.expect(!asciiWriter.writeRawDxfSection(section)
                 && asciiWriter.m_writeError && asciiWriter.writer == nullptr,
             "DXF ASCII raw section rejects a missing writer during preflight");

    DRW_RawDxfSection binarySection = section;
    binarySection.m_hasRawValues = false;
    binarySection.m_rawValues.clear();
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    t.expect(!binaryWriter.writeRawDxfSection(binarySection)
                 && binaryWriter.m_writeError && binaryWriter.writer == nullptr,
             "DXF binary raw section rejects a missing writer during preflight");
}

void testDxfRawSectionWriterStickyError(TestContext& t) {
    DRW_RawDxfSection section;
    section.m_name = "LOCAL_SECTION_STICKY_ERROR";
    section.m_version = DRW::AC1027;
    section.m_hasRawValues = true;
    section.m_groups = {DRW_Variant(1000, std::string("payload"))};
    section.m_rawValues = {"payload"};

    std::ostringstream output;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&output);
    asciiWriter.writer->markWriteError();
    t.expect(!asciiWriter.writeRawDxfSection(section)
                 && asciiWriter.writer->hasWriteError()
                 && output.str().empty(),
             "DXF ASCII raw section rejects a pre-existing writer error without appending");

    DRW_RawDxfSection binarySection = section;
    binarySection.m_hasRawValues = false;
    binarySection.m_rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    binaryWriter.writer->markWriteError();
    t.expect(!binaryWriter.writeRawDxfSection(binarySection)
                 && binaryWriter.writer->hasWriteError()
                 && binaryOutput.str().empty(),
             "DXF binary raw section rejects a pre-existing writer error without appending");
}

void testDxfOperationShortWriteFailure(TestContext& t) {
    class ShortWriteInterface final : public dx_iface {
    public:
        ShortWriteInterface() : rejectedOutput(&rejecting) {
            cData = &storage;
            currentBlock = storage.mBlock;
            poly.handle = 0xB100u;
            poly.addVertex(DRW_Vertex(1, 2, 0, 0));
            poly.addVertex(DRW_Vertex(3, 4, 0, 0));
            poly.vertlist[0]->handle = 0xB101u;
            poly.vertlist[1]->handle = 0xB102u;
            late.handle = 0xB200u;
            late.secPoint = DRW_Coord(7, 8, 9);
        }
        void writeEntities() override {
            // This callback-local prefix is dead before the later short write.
            emitPrefix();
            std::ostream* original = dxfW->writer->stream();
            if (reject)
                dxfW->writer->setStream(&rejectedOutput);
            polyResult = dxfW->writePolyline(&poly);
            const auto before = dxfW->highWaterHandle();
            lateResult = dxfW->writeLine(&late);
            lateAllocated = dxfW->highWaterHandle() != before;
            dxfW->writer->setStream(original);
        }
        void emitPrefix() {
            DRW_Line prefix;
            prefix.handle = 0xA100u;
            prefix.secPoint = DRW_Coord(4, 5, 6);
            prefixResult = dxfW->writeLine(&prefix);
        }
        dx_data storage;
        RejectingStreambuf rejecting;
        std::ostream rejectedOutput;
        DRW_Polyline poly;
        DRW_Line late;
        bool reject {true}, prefixResult {false}, polyResult {false};
        bool lateResult {false}, lateAllocated {false};
    };
    const auto output = std::filesystem::temp_directory_path()
        / ("libdxfrw-short-write-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
    } cleanup{output};
    const std::string fileName = output.string();
    for (const bool binary : {false, true}) {
        { std::ofstream original(output); original << "short write sentinel\n"; }
        ShortWriteInterface adapter;
        dxfRW codec(fileName.c_str());
        adapter.dxfW = &codec;
        codec.setDxfWriteRecordInventoryEnabled(true, 4096u);
        t.expect(!codec.write(&adapter, DRW::AC1027, binary)
                     && adapter.prefixResult && !adapter.polyResult
                     && !adapter.lateResult && !adapter.lateAllocated
                     && codec.dxfWriteRecordHandleCount(0xA100u) == 0u,
                 "actual short append stops all later emission after a successful stack-local prefix");
        std::ifstream preserved(output, std::ios::binary);
        t.expect(std::string(std::istreambuf_iterator<char>(preserved), {}) == "short write sentinel\n"
                     && adapter.poly.handle == 0xB100u
                     && adapter.poly.vertlist[0]->handle == 0xB101u
                     && adapter.poly.vertlist[1]->handle == 0xB102u
                     && adapter.late.handle == 0xB200u,
                 "short append rolls back its compound caller state and preserves the destination/suffix");
        t.expect(codec.getWritingContext()->sourceHandleToMintedMap.empty()
                     && codec.getWritingContext()->writtenSourceEntityHandles.empty()
                     && codec.getLastDiagnostic().code == "emit-failure",
                 "short-write failure discards derived maps without undoing expired prefix records");
        preserved.close();
        adapter.reject = false;
        t.expect(codec.write(&adapter, DRW::AC1027, binary)
                     && adapter.polyResult && adapter.lateResult,
                 "same writer and unchanged failed compound recover after the sink is corrected");
        adapter.dxfW = nullptr;
    }
}

void testDxfRawSectionAppendFailure(TestContext& t) {
    DRW_RawDxfSection section;
    section.m_name = "LOCAL_SECTION_APPEND_FAILURE";
    section.m_version = DRW::AC1027;
    section.m_hasRawValues = true;
    section.m_groups = {DRW_Variant(1000, std::string("payload"))};
    section.m_rawValues = {"payload"};

    RejectingStreambuf asciiBuffer;
    std::ostream asciiSink(&asciiBuffer);
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiSink);
    t.expect(!asciiWriter.writeRawDxfSection(section)
                 && asciiWriter.m_writeError
                 && asciiWriter.writer->hasWriteError()
                 && asciiBuffer.acceptedBytes() == 0,
             "DXF ASCII raw section rolls back a failed append");

    DRW_RawDxfSection binarySection = section;
    binarySection.m_hasRawValues = false;
    binarySection.m_rawValues.clear();
    RejectingStreambuf binaryBuffer;
    std::ostream binarySink(&binaryBuffer);
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binarySink);
    t.expect(!binaryWriter.writeRawDxfSection(binarySection)
                 && binaryWriter.m_writeError
                 && binaryWriter.writer->hasWriteError()
                 && binaryBuffer.acceptedBytes() == 0,
             "DXF binary raw section rolls back a failed append");
}

void testDxfRawObjectAppendFailure(TestContext& t) {
    DRW_RawDxfObject object;
    object.name = "LOCAL_RAW_APPEND_FAILURE";
    object.m_version = DRW::AC1027;
    object.hasRawValues = true;
    object.groups = {DRW_Variant(5, std::string("1A")),
                     DRW_Variant(1000, std::string("payload"))};
    object.rawValues = {"1A", "payload"};

    RejectingStreambuf asciiBuffer;
    std::ostream asciiSink(&asciiBuffer);
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiSink);
    t.expect(!asciiWriter.writeRawDxfObject(&object)
                 && asciiWriter.m_writeError
                 && asciiWriter.writer->hasWriteError()
                 && asciiBuffer.acceptedBytes() == 0,
             "DXF ASCII raw object rolls back a failed append");

    DRW_RawDxfObject binaryObject = object;
    binaryObject.hasRawValues = false;
    binaryObject.rawValues.clear();
    RejectingStreambuf binaryBuffer;
    std::ostream binarySink(&binaryBuffer);
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binarySink);
    t.expect(!binaryWriter.writeRawDxfObject(&binaryObject)
                 && binaryWriter.m_writeError
                 && binaryWriter.writer->hasWriteError()
                 && binaryBuffer.acceptedBytes() == 0,
             "DXF binary raw object rolls back a failed append");
}

void testDxfRawObjectWriterPreflight(TestContext& t) {
    DRW_RawDxfObject object;
    object.name = "LOCAL_RAW_PREFLIGHT";
    object.m_version = DRW::AC1027;
    object.hasRawValues = true;
    object.groups = {DRW_Variant(5, std::string("1A")),
                     DRW_Variant(1000, std::string("payload"))};
    object.rawValues = {"1A", "payload"};

    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    t.expect(!asciiWriter.writeRawDxfObject(&object)
                 && asciiWriter.m_writeError && asciiWriter.writer == nullptr,
             "DXF ASCII raw object rejects a missing writer during preflight");

    DRW_RawDxfObject binaryObject = object;
    binaryObject.hasRawValues = false;
    binaryObject.rawValues.clear();
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    t.expect(!binaryWriter.writeRawDxfObject(&binaryObject)
                 && binaryWriter.m_writeError && binaryWriter.writer == nullptr,
             "DXF binary raw object rejects a missing writer during preflight");
}

void testDxfRawObjectVersionCompatibility(TestContext& t) {
    const std::vector<DRW::Version> accepted = {
        DRW::AC1027, DRW::UNKNOWNV};
    for (const DRW::Version taggedVersion : accepted) {
        DRW_RawDxfObject object;
        object.name = "LOCAL_RAW_VERSION_ACCEPTED";
        object.m_version = taggedVersion;
        object.hasRawValues = true;
        object.groups = {DRW_Variant(5, std::string("1A")),
                         DRW_Variant(1000, std::string("payload"))};
        object.rawValues = {"1A", "payload"};

        std::ostringstream asciiOutput;
        dxfRW asciiWriter("");
        asciiWriter.version = DRW::AC1027;
        asciiWriter.binFile = false;
        asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
        t.expect(asciiWriter.writeRawDxfObject(&object)
                     && !asciiOutput.str().empty(),
                 "DXF ASCII raw object accepts matching and unknown versions");

        DRW_RawDxfObject binaryObject = object;
        binaryObject.hasRawValues = false;
        binaryObject.rawValues.clear();
        std::ostringstream binaryOutput;
        dxfRW binaryWriter("");
        binaryWriter.version = DRW::AC1027;
        binaryWriter.binFile = true;
        binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
        t.expect(binaryWriter.writeRawDxfObject(&binaryObject)
                     && !binaryOutput.str().empty(),
                 "DXF binary raw object accepts matching and unknown versions");
    }

    DRW_RawDxfObject mismatched;
    mismatched.name = "LOCAL_RAW_VERSION_MISMATCH";
    mismatched.m_version = DRW::AC1024;
    mismatched.hasRawValues = true;
    mismatched.groups = {DRW_Variant(5, std::string("1A")),
                         DRW_Variant(1000, std::string("payload"))};
    mismatched.rawValues = {"1A", "payload"};

    std::ostringstream rejectedAsciiOutput;
    dxfRW rejectingAsciiWriter("");
    rejectingAsciiWriter.version = DRW::AC1027;
    rejectingAsciiWriter.binFile = false;
    rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedAsciiOutput);
    t.expect(!rejectingAsciiWriter.writeRawDxfObject(&mismatched)
                 && rejectedAsciiOutput.str().empty(),
             "DXF ASCII raw object rejects a mismatched version transactionally");

    DRW_RawDxfObject mismatchedBinary = mismatched;
    mismatchedBinary.hasRawValues = false;
    mismatchedBinary.rawValues.clear();
    std::ostringstream rejectedBinaryOutput;
    dxfRW rejectingBinaryWriter("");
    rejectingBinaryWriter.version = DRW::AC1027;
    rejectingBinaryWriter.binFile = true;
    rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
        &rejectedBinaryOutput);
    t.expect(!rejectingBinaryWriter.writeRawDxfObject(&mismatchedBinary)
                 && rejectedBinaryOutput.str().empty(),
             "DXF binary raw object rejects a mismatched version transactionally");
}

void testDxfRawObjectEmptyPayload(TestContext& t) {
    DRW_RawDxfObject object;
    object.name = "LOCAL_RAW_EMPTY_PAYLOAD";
    object.m_version = DRW::AC1027;
    object.hasRawValues = true;
    object.groups = {DRW_Variant(5, std::string("1A"))};
    object.rawValues = {"1A"};
    const std::vector<std::pair<int, std::string>> expected = {
        {0, object.name}, {5, "1A"}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiWriter.writeRawDxfObject(&object),
             "DXF ASCII raw object accepts a self-handle-only payload");
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    std::vector<std::pair<int, std::string>> asciiActual;
    int asciiCode = 0;
    while (asciiReader.readRec(&asciiCode))
        asciiActual.emplace_back(asciiCode, asciiReader.getString());
    t.expect(asciiActual == expected,
             "DXF ASCII raw object emits only name and handle framing");

    DRW_RawDxfObject binaryObject = object;
    binaryObject.hasRawValues = false;
    binaryObject.rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryWriter.writeRawDxfObject(&binaryObject),
             "DXF binary raw object accepts a self-handle-only payload");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    std::vector<std::pair<int, std::string>> binaryActual;
    int binaryCode = 0;
    while (binaryReader.readRec(&binaryCode))
        binaryActual.emplace_back(binaryCode, binaryReader.getString());
    t.expect(binaryActual == expected,
             "DXF binary raw object emits only name and handle framing");
}

void testDxfRawObjectAggregateLimit(TestContext& t) {
    const auto makeObject = [](std::size_t pairCount, bool hasRawValues) {
        DRW_RawDxfObject object;
        object.name = "LOCAL_RAW_AGGREGATE_LIMIT";
        object.m_version = DRW::AC1027;
        object.hasRawValues = hasRawValues;
        object.groups.reserve(pairCount);
        if (hasRawValues)
            object.rawValues.reserve(pairCount);
        object.groups.emplace_back(5, std::string("1A"));
        if (hasRawValues)
            object.rawValues.emplace_back("1A");
        for (std::size_t i = 1; i < pairCount; ++i) {
            object.groups.emplace_back(1000, std::string("payload"));
            if (hasRawValues)
                object.rawValues.emplace_back("payload");
        }
        return object;
    };
    constexpr std::size_t maxPairs = DRW::kMaxDxfApplicationGroupPairs;

    DRW_RawDxfObject asciiBoundary = makeObject(maxPairs, true);
    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiWriter.writeRawDxfObject(&asciiBoundary)
                 && !asciiOutput.str().empty(),
             "DXF ASCII raw object accepts the aggregate-pair boundary");

    DRW_RawDxfObject binaryBoundary = makeObject(maxPairs, false);
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryWriter.writeRawDxfObject(&binaryBoundary)
                 && !binaryOutput.str().empty(),
             "DXF binary raw object accepts the aggregate-pair boundary");

    DRW_RawDxfObject asciiOver = makeObject(maxPairs + 1u, true);
    std::ostringstream rejectedAsciiOutput;
    dxfRW rejectingAsciiWriter("");
    rejectingAsciiWriter.version = DRW::AC1027;
    rejectingAsciiWriter.binFile = false;
    rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedAsciiOutput);
    t.expect(!rejectingAsciiWriter.writeRawDxfObject(&asciiOver)
                 && rejectedAsciiOutput.str().empty(),
             "DXF ASCII raw object rejects an over-limit aggregate transactionally");

    DRW_RawDxfObject binaryOver = makeObject(maxPairs + 1u, false);
    std::ostringstream rejectedBinaryOutput;
    dxfRW rejectingBinaryWriter("");
    rejectingBinaryWriter.version = DRW::AC1027;
    rejectingBinaryWriter.binFile = true;
    rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
        &rejectedBinaryOutput);
    t.expect(!rejectingBinaryWriter.writeRawDxfObject(&binaryOver)
                 && rejectedBinaryOutput.str().empty(),
             "DXF binary raw object rejects an over-limit aggregate transactionally");
}

void testDxfRawObjectApplicationGroupDepth(TestContext& t) {
    const auto makeObject = [](std::size_t depth, bool hasRawValues) {
        DRW_RawDxfObject object;
        object.name = "LOCAL_RAW_DEPTH_LIMIT";
        object.m_version = DRW::AC1027;
        object.hasRawValues = hasRawValues;
        object.groups.reserve(1u + depth * 2u);
        if (hasRawValues)
            object.rawValues.reserve(1u + depth * 2u);
        object.groups.emplace_back(5, std::string("1A"));
        if (hasRawValues)
            object.rawValues.emplace_back("1A");
        for (std::size_t i = 0; i < depth; ++i) {
            object.groups.emplace_back(102, std::string("{DEPTH"));
            if (hasRawValues)
                object.rawValues.emplace_back("{DEPTH");
        }
        for (std::size_t i = 0; i < depth; ++i) {
            object.groups.emplace_back(102, std::string("}"));
            if (hasRawValues)
                object.rawValues.emplace_back("}");
        }
        return object;
    };
    constexpr std::size_t maxDepth = DRW::kMaxDxfApplicationGroupNesting;

    DRW_RawDxfObject asciiBoundary = makeObject(maxDepth, true);
    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiWriter.writeRawDxfObject(&asciiBoundary)
                 && !asciiOutput.str().empty(),
             "DXF ASCII raw object accepts the application-group depth boundary");

    DRW_RawDxfObject binaryBoundary = makeObject(maxDepth, false);
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryWriter.writeRawDxfObject(&binaryBoundary)
                 && !binaryOutput.str().empty(),
             "DXF binary raw object accepts the application-group depth boundary");

    DRW_RawDxfObject asciiOver = makeObject(maxDepth + 1u, true);
    std::ostringstream rejectedAsciiOutput;
    dxfRW rejectingAsciiWriter("");
    rejectingAsciiWriter.version = DRW::AC1027;
    rejectingAsciiWriter.binFile = false;
    rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedAsciiOutput);
    t.expect(!rejectingAsciiWriter.writeRawDxfObject(&asciiOver)
                 && rejectedAsciiOutput.str().empty(),
             "DXF ASCII raw object rejects over-depth transactionally");

    DRW_RawDxfObject binaryOver = makeObject(maxDepth + 1u, false);
    std::ostringstream rejectedBinaryOutput;
    dxfRW rejectingBinaryWriter("");
    rejectingBinaryWriter.version = DRW::AC1027;
    rejectingBinaryWriter.binFile = true;
    rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
        &rejectedBinaryOutput);
    t.expect(!rejectingBinaryWriter.writeRawDxfObject(&binaryOver)
                 && rejectedBinaryOutput.str().empty(),
             "DXF binary raw object rejects over-depth transactionally");
}

void testDxfRawObjectApplicationGroupMarker(TestContext& t) {
    DRW_RawDxfObject object;
    object.name = "LOCAL_RAW_MARKER";
    object.m_version = DRW::AC1027;
    object.hasRawValues = true;
    object.groups = {DRW_Variant(5, std::string("1A")),
                     DRW_Variant(102, std::string("{VALID_GROUP")),
                     DRW_Variant(1000, std::string("payload")),
                     DRW_Variant(102, std::string("}"))};
    object.rawValues = {"1A", "{VALID_GROUP", "payload", "}"};
    const std::vector<std::pair<int, std::string>> expected = {
        {0, object.name}, {5, "1A"}, {102, "{VALID_GROUP"},
        {1000, "payload"}, {102, "}"}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiWriter.writeRawDxfObject(&object),
             "DXF ASCII raw object accepts valid application-group markers");
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    std::vector<std::pair<int, std::string>> asciiActual;
    int asciiCode = 0;
    while (asciiReader.readRec(&asciiCode))
        asciiActual.emplace_back(asciiCode, asciiReader.getString());
    t.expect(asciiActual == expected,
             "DXF ASCII raw object replays valid application-group markers");

    DRW_RawDxfObject binaryObject = object;
    binaryObject.hasRawValues = false;
    binaryObject.rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryWriter.writeRawDxfObject(&binaryObject),
             "DXF binary raw object accepts valid application-group markers");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    std::vector<std::pair<int, std::string>> binaryActual;
    int binaryCode = 0;
    while (binaryReader.readRec(&binaryCode))
        binaryActual.emplace_back(binaryCode, binaryReader.getString());
    t.expect(binaryActual == expected,
             "DXF binary raw object replays valid application-group markers");

    const std::vector<std::string> invalidMarkers = {"{", "NOT_A_MARKER"};
    for (const std::string& marker : invalidMarkers) {
        DRW_RawDxfObject malformed = object;
        malformed.groups[1] = DRW_Variant(102, marker);
        malformed.rawValues[1] = marker;
        std::ostringstream rejectedAsciiOutput;
        dxfRW rejectingAsciiWriter("");
        rejectingAsciiWriter.version = DRW::AC1027;
        rejectingAsciiWriter.binFile = false;
        rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
            &rejectedAsciiOutput);
        t.expect(!rejectingAsciiWriter.writeRawDxfObject(&malformed)
                     && rejectedAsciiOutput.str().empty(),
                 "DXF ASCII raw object rejects malformed application marker");

        DRW_RawDxfObject malformedBinary = malformed;
        malformedBinary.hasRawValues = false;
        malformedBinary.rawValues.clear();
        std::ostringstream rejectedBinaryOutput;
        dxfRW rejectingBinaryWriter("");
        rejectingBinaryWriter.version = DRW::AC1027;
        rejectingBinaryWriter.binFile = true;
        rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
            &rejectedBinaryOutput);
        t.expect(!rejectingBinaryWriter.writeRawDxfObject(&malformedBinary)
                     && rejectedBinaryOutput.str().empty(),
                 "DXF binary raw object rejects malformed application marker");
    }
}

void testDxfRawObjectApplicationGroupReferenceMatrix(TestContext& t) {
    DRW_RawDxfObject object;
    object.name = "LOCAL_RAW_REFERENCE_MATRIX";
    object.m_version = DRW::AC1027;
    object.hasRawValues = true;
    object.groups = {
        DRW_Variant(5, std::string("1A")),
        DRW_Variant(102, std::string("{REFERENCE_MATRIX")),
        DRW_Variant(320, std::string("2A")),
        DRW_Variant(330, std::string("3A")),
        DRW_Variant(350, std::string("4A")),
        DRW_Variant(390, std::string("5A")),
        DRW_Variant(399, std::string("6A")),
        DRW_Variant(480, std::string("1A")),
        DRW_Variant(481, std::string("2A")),
        DRW_Variant(102, std::string("}"))};
    object.rawValues = {"1A", "{REFERENCE_MATRIX", "2A", "3A", "4A",
                        "5A", "6A", "1A", "2A", "}"};
    const std::map<std::uint32_t, std::uint32_t> remap = {
        {0x1Au, 0x3Au}, {0x2Au, 0x4Au}, {0x3Au, 0x5Au},
        {0x4Au, 0x6Au}, {0x5Au, 0x7Au}, {0x6Au, 0x8Au}};
    const std::vector<std::pair<int, std::string>> expected = {
        {0, object.name}, {5, "3A"}, {102, "{REFERENCE_MATRIX"},
        {320, "2A"}, {330, "5A"}, {350, "6A"}, {390, "7A"},
        {399, "8A"}, {480, "3A"}, {481, "4A"}, {102, "}"}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    asciiWriter.setHandleRemap(remap);
    t.expect(asciiWriter.writeRawDxfObject(&object),
             "DXF ASCII raw object remaps every handle-reference code family");
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    std::vector<std::pair<int, std::string>> asciiActual;
    int asciiCode = 0;
    while (asciiReader.readRec(&asciiCode))
        asciiActual.emplace_back(asciiCode, asciiReader.getString());
    t.expect(asciiActual == expected,
             "DXF ASCII raw object reference-code matrix preserves framing");

    DRW_RawDxfObject binaryObject = object;
    binaryObject.hasRawValues = false;
    binaryObject.rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    binaryWriter.setHandleRemap(remap);
    t.expect(binaryWriter.writeRawDxfObject(&binaryObject),
             "DXF binary raw object remaps every handle-reference code family");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    std::vector<std::pair<int, std::string>> binaryActual;
    int binaryCode = 0;
    while (binaryReader.readRec(&binaryCode))
        binaryActual.emplace_back(binaryCode, binaryReader.getString());
    t.expect(binaryActual == expected,
             "DXF binary raw object reference-code matrix preserves framing");
}

void testDxfRawObjectApplicationGroupChunkCodeMatrix(TestContext& t) {
    DRW_RawDxfObject object;
    object.name = "LOCAL_RAW_CHUNK_CODES";
    object.m_version = DRW::AC1027;
    object.hasRawValues = true;
    object.groups.emplace_back(5, std::string("1A"));
    object.rawValues.emplace_back("1A");
    object.groups.emplace_back(102, std::string("{CHUNK_CODES"));
    object.rawValues.emplace_back("{CHUNK_CODES");
    for (int code = 310; code <= 319; ++code) {
        const std::string value = (code & 1) == 0 ? "A1B2" : "C3D4";
        object.groups.emplace_back(code, value);
        object.rawValues.emplace_back(value);
    }
    object.groups.emplace_back(1004, std::string("01020304"));
    object.rawValues.emplace_back("01020304");
    object.groups.emplace_back(102, std::string("}"));
    object.rawValues.emplace_back("}");

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiWriter.writeRawDxfObject(&object),
             "DXF ASCII raw object replays every binary chunk code");
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    std::vector<std::pair<int, std::string>> asciiActual;
    int asciiCode = 0;
    while (asciiReader.readRec(&asciiCode))
        asciiActual.emplace_back(asciiCode, asciiReader.getString());
    t.expect(asciiActual.size() == object.groups.size() + 1u
                 && asciiActual[0] == std::make_pair(0, object.name)
                 && asciiActual[1] == std::make_pair(5, std::string("1A")),
             "DXF ASCII raw object preserves chunk-code framing");

    DRW_RawDxfObject binaryObject = object;
    binaryObject.hasRawValues = false;
    binaryObject.rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryWriter.writeRawDxfObject(&binaryObject),
             "DXF binary raw object replays every binary chunk code");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    std::vector<std::pair<int, std::string>> binaryActual;
    int binaryCode = 0;
    while (binaryReader.readRec(&binaryCode))
        binaryActual.emplace_back(binaryCode, binaryReader.getString());
    t.expect(binaryActual.size() == object.groups.size() + 1u
                 && binaryActual[0] == std::make_pair(0, object.name)
                 && binaryActual[1] == std::make_pair(5, std::string("1A")),
             "DXF binary raw object preserves chunk-code framing");

    DRW_RawDxfObject malformedAscii = object;
    malformedAscii.groups[5] = DRW_Variant(313, std::string("ABC"));
    malformedAscii.rawValues[5] = "ABC";
    std::ostringstream rejectedAsciiOutput;
    dxfRW rejectingAsciiWriter("");
    rejectingAsciiWriter.version = DRW::AC1027;
    rejectingAsciiWriter.binFile = false;
    rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedAsciiOutput);
    t.expect(!rejectingAsciiWriter.writeRawDxfObject(&malformedAscii)
                 && rejectedAsciiOutput.str().empty(),
             "DXF ASCII malformed raw-object chunk rejects transactionally");

    DRW_RawDxfObject malformedBinary = binaryObject;
    malformedBinary.groups[5] = DRW_Variant(313, std::string("GG"));
    std::ostringstream rejectedBinaryOutput;
    dxfRW rejectingBinaryWriter("");
    rejectingBinaryWriter.version = DRW::AC1027;
    rejectingBinaryWriter.binFile = true;
    rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
        &rejectedBinaryOutput);
    t.expect(!rejectingBinaryWriter.writeRawDxfObject(&malformedBinary)
                 && rejectedBinaryOutput.str().empty(),
             "DXF binary malformed raw-object chunk rejects transactionally");
}

void testDxfRawObjectApplicationGroupChunkSize(TestContext& t) {
    const auto makeObject = [](std::size_t byteCount, bool hasRawValues) {
        DRW_RawDxfObject object;
        object.name = "LOCAL_RAW_CHUNK_SIZE";
        object.m_version = DRW::AC1027;
        object.hasRawValues = hasRawValues;
        const std::string chunk(byteCount * 2u, 'A');
        object.groups = {DRW_Variant(5, std::string("1A")),
                         DRW_Variant(310, chunk)};
        if (hasRawValues)
            object.rawValues = {"1A", chunk};
        return object;
    };
    constexpr std::size_t maxChunkBytes = 127u;

    DRW_RawDxfObject asciiBoundary = makeObject(maxChunkBytes, true);
    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiWriter.writeRawDxfObject(&asciiBoundary)
                 && !asciiOutput.str().empty(),
             "DXF ASCII raw object accepts the 127-byte chunk boundary");

    DRW_RawDxfObject binaryBoundary = makeObject(maxChunkBytes, false);
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryWriter.writeRawDxfObject(&binaryBoundary)
                 && !binaryOutput.str().empty(),
             "DXF binary raw object accepts the 127-byte chunk boundary");

    DRW_RawDxfObject asciiOver = makeObject(maxChunkBytes + 1u, true);
    std::ostringstream rejectedAsciiOutput;
    dxfRW rejectingAsciiWriter("");
    rejectingAsciiWriter.version = DRW::AC1027;
    rejectingAsciiWriter.binFile = false;
    rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
        &rejectedAsciiOutput);
    t.expect(!rejectingAsciiWriter.writeRawDxfObject(&asciiOver)
                 && rejectedAsciiOutput.str().empty(),
             "DXF ASCII raw object rejects a 128-byte chunk transactionally");

    DRW_RawDxfObject binaryOver = makeObject(maxChunkBytes + 1u, false);
    std::ostringstream rejectedBinaryOutput;
    dxfRW rejectingBinaryWriter("");
    rejectingBinaryWriter.version = DRW::AC1027;
    rejectingBinaryWriter.binFile = true;
    rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
        &rejectedBinaryOutput);
    t.expect(!rejectingBinaryWriter.writeRawDxfObject(&binaryOver)
                 && rejectedBinaryOutput.str().empty(),
             "DXF binary raw object rejects a 128-byte chunk transactionally");
}

void testDxfRawObjectRawValueCardinality(TestContext& t) {
    DRW_RawDxfObject object;
    object.name = "LOCAL_RAW_VALUES";
    object.m_version = DRW::AC1027;
    object.hasRawValues = true;
    object.groups = {DRW_Variant(5, std::string("1A")),
                     DRW_Variant(1000, std::string("payload"))};
    object.rawValues = {"1A", "payload"};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    t.expect(asciiWriter.writeRawDxfObject(&object)
                 && !asciiOutput.str().empty(),
             "DXF ASCII raw object accepts matching raw-value cardinality");

    DRW_RawDxfObject missing = object;
    missing.rawValues.pop_back();
    std::ostringstream missingOutput;
    dxfRW missingWriter("");
    missingWriter.version = DRW::AC1027;
    missingWriter.binFile = false;
    missingWriter.writer = std::make_unique<dxfWriterAscii>(&missingOutput);
    t.expect(!missingWriter.writeRawDxfObject(&missing)
                 && missingOutput.str().empty(),
             "DXF ASCII raw object rejects missing raw-value spelling");

    DRW_RawDxfObject extra = object;
    extra.rawValues.emplace_back("EXTRA");
    std::ostringstream extraOutput;
    dxfRW extraWriter("");
    extraWriter.version = DRW::AC1027;
    extraWriter.binFile = false;
    extraWriter.writer = std::make_unique<dxfWriterAscii>(&extraOutput);
    t.expect(!extraWriter.writeRawDxfObject(&extra)
                 && extraOutput.str().empty(),
             "DXF ASCII raw object rejects extra raw-value spelling");

    DRW_RawDxfObject binaryObject = object;
    binaryObject.hasRawValues = false;
    binaryObject.rawValues.assign(binaryObject.groups.size(), UTF8STRING());
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    t.expect(binaryWriter.writeRawDxfObject(&binaryObject)
                 && !binaryOutput.str().empty(),
             "DXF binary raw object accepts empty raw-value placeholders");
}

void testDxfRawObjectRemapChain(TestContext& t) {
    DRW_RawDxfObject object;
    object.name = "LOCAL_RAW_REMAP_CHAIN";
    object.m_version = DRW::AC1027;
    object.hasRawValues = true;
    object.groups = {DRW_Variant(5, std::string("2A")),
                     DRW_Variant(330, std::string("3A"))};
    object.rawValues = {"2A", "3A"};
    const std::map<std::uint32_t, std::uint32_t> remap = {
        {0x2Au, 0x3Au}, {0x3Au, 0x4Au}};
    const std::vector<std::pair<int, std::string>> expected = {
        {0, object.name}, {5, "3A"}, {330, "4A"}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    asciiWriter.setHandleRemap(remap);
    t.expect(asciiWriter.writeRawDxfObject(&object),
             "DXF ASCII raw object applies one remap step per handle");
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    std::vector<std::pair<int, std::string>> asciiActual;
    int asciiCode = 0;
    while (asciiReader.readRec(&asciiCode))
        asciiActual.emplace_back(asciiCode, asciiReader.getString());
    t.expect(asciiActual == expected,
             "DXF ASCII raw object preserves one-step remap semantics");

    DRW_RawDxfObject binaryObject = object;
    binaryObject.hasRawValues = false;
    binaryObject.rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    binaryWriter.setHandleRemap(remap);
    t.expect(binaryWriter.writeRawDxfObject(&binaryObject),
             "DXF binary raw object applies one remap step per handle");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    std::vector<std::pair<int, std::string>> binaryActual;
    int binaryCode = 0;
    while (binaryReader.readRec(&binaryCode))
        binaryActual.emplace_back(binaryCode, binaryReader.getString());
    t.expect(binaryActual == expected,
             "DXF binary raw object preserves one-step remap semantics");
}

void testDxfRawObjectWideHandleReplay(TestContext& t) {
    const std::string wideSelf = "123456789ABCDEF0";
    const std::string wideOwner = "FEDCBA9876543210";
    DRW_RawDxfObject object;
    object.name = "LOCAL_RAW_WIDE_HANDLE";
    object.m_version = DRW::AC1027;
    object.hasRawValues = true;
    object.groups = {DRW_Variant(5, wideSelf), DRW_Variant(330, wideOwner)};
    object.rawValues = {wideSelf, wideOwner};
    const std::map<std::uint32_t, std::uint32_t> narrowRemap = {
        {0x1Au, 0x2Au}};
    const std::vector<std::pair<int, std::string>> expected = {
        {0, object.name}, {5, wideSelf}, {330, wideOwner}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    asciiWriter.setHandleRemap(narrowRemap);
    t.expect(asciiWriter.writeRawDxfObject(&object),
             "DXF ASCII raw object accepts wide handle lexemes");
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    asciiReader.setAllowWideHandleLexemes(true);
    std::vector<std::pair<int, std::string>> asciiActual;
    int asciiCode = 0;
    while (asciiReader.readRec(&asciiCode))
        asciiActual.emplace_back(asciiCode, asciiReader.getString());
    t.expect(asciiActual == expected,
             "DXF ASCII raw object preserves wide handles beyond remap width");

    DRW_RawDxfObject binaryObject = object;
    binaryObject.hasRawValues = false;
    binaryObject.rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    binaryWriter.setHandleRemap(narrowRemap);
    t.expect(binaryWriter.writeRawDxfObject(&binaryObject),
             "DXF binary raw object accepts wide handle lexemes");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    binaryReader.setAllowWideHandleLexemes(true);
    std::vector<std::pair<int, std::string>> binaryActual;
    int binaryCode = 0;
    while (binaryReader.readRec(&binaryCode))
        binaryActual.emplace_back(binaryCode, binaryReader.getString());
    t.expect(binaryActual == expected,
             "DXF binary raw object preserves wide handles beyond remap width");
}

void testDxfRawObjectMalformedHandle(TestContext& t) {
    DRW_RawDxfObject object;
    object.name = "LOCAL_RAW_BAD_HANDLE";
    object.m_version = DRW::AC1027;
    object.hasRawValues = true;
    object.groups = {DRW_Variant(5, std::string("1A")),
                     DRW_Variant(330, std::string("2A")),
                     DRW_Variant(340, std::string("3A"))};
    object.rawValues = {"1A", "2A", "3A"};

    const std::vector<std::pair<std::size_t, std::string>> malformed = {
        {0u, "not-hex"}, {1u, "owner?"}, {2u, "reference?"}};
    for (const auto& item : malformed) {
        DRW_RawDxfObject asciiObject = object;
        asciiObject.groups[item.first] = DRW_Variant(
            asciiObject.groups[item.first].code(), item.second);
        asciiObject.rawValues[item.first] = item.second;
        std::ostringstream rejectedAsciiOutput;
        dxfRW rejectingAsciiWriter("");
        rejectingAsciiWriter.version = DRW::AC1027;
        rejectingAsciiWriter.binFile = false;
        rejectingAsciiWriter.writer = std::make_unique<dxfWriterAscii>(
            &rejectedAsciiOutput);
        t.expect(!rejectingAsciiWriter.writeRawDxfObject(&asciiObject)
                     && rejectingAsciiWriter.m_writeError
                     && rejectedAsciiOutput.str().empty(),
                 "DXF ASCII malformed raw-object handle rejects transactionally");

        DRW_RawDxfObject binaryObject = asciiObject;
        binaryObject.hasRawValues = false;
        binaryObject.rawValues.clear();
        std::ostringstream rejectedBinaryOutput;
        dxfRW rejectingBinaryWriter("");
        rejectingBinaryWriter.version = DRW::AC1027;
        rejectingBinaryWriter.binFile = true;
        rejectingBinaryWriter.writer = std::make_unique<dxfWriterBinary>(
            &rejectedBinaryOutput);
        t.expect(!rejectingBinaryWriter.writeRawDxfObject(&binaryObject)
                     && rejectingBinaryWriter.m_writeError
                     && rejectedBinaryOutput.str().empty(),
                 "DXF binary malformed raw-object handle rejects transactionally");
    }
}

void testDxfRawSectionApplicationGroupReferenceMatrix(TestContext& t) {
    DRW_RawDxfSection section;
    section.m_name = "LOCAL_SECTION_REFERENCE_MATRIX";
    section.m_version = DRW::AC1027;
    section.m_hasRawValues = true;
    section.m_groups = {
        DRW_Variant(102, std::string("{SECTION_REFERENCE_MATRIX")),
        DRW_Variant(320, std::string("2A")),
        DRW_Variant(330, std::string("3A")),
        DRW_Variant(350, std::string("4A")),
        DRW_Variant(390, std::string("5A")),
        DRW_Variant(399, std::string("6A")),
        DRW_Variant(480, std::string("1A")),
        DRW_Variant(481, std::string("2A")),
        DRW_Variant(102, std::string("}"))};
    section.m_rawValues = {"{SECTION_REFERENCE_MATRIX", "2A", "3A", "4A",
                           "5A", "6A", "1A", "2A", "}"};
    const std::map<std::uint32_t, std::uint32_t> remap = {
        {0x1Au, 0x3Au}, {0x2Au, 0x4Au}, {0x3Au, 0x5Au},
        {0x4Au, 0x6Au}, {0x5Au, 0x7Au}, {0x6Au, 0x8Au}};
    const std::vector<std::pair<int, std::string>> expected = {
        {0, "SECTION"},
        {2, "LOCAL_SECTION_REFERENCE_MATRIX"},
        {102, "{SECTION_REFERENCE_MATRIX"},
        {320, "2A"},
        {330, "5A"},
        {350, "6A"},
        {390, "7A"},
        {399, "8A"},
        {480, "3A"},
        {481, "4A"},
        {102, "}"},
        {0, "ENDSEC"}};

    std::ostringstream asciiOutput;
    dxfRW asciiWriter("");
    asciiWriter.version = DRW::AC1027;
    asciiWriter.binFile = false;
    asciiWriter.writer = std::make_unique<dxfWriterAscii>(&asciiOutput);
    asciiWriter.setHandleRemap(remap);
    t.expect(asciiWriter.writeRawDxfSection(section),
             "DXF ASCII raw section remaps every handle-reference code family");
    int code = 0;
    std::stringstream asciiRecords(asciiOutput.str());
    dxfReaderAscii asciiReader(&asciiRecords);
    bool asciiShape = true;
    for (const auto& item : expected) {
        if (!asciiReader.readRec(&code) || code != item.first
            || asciiReader.getString() != item.second) {
            asciiShape = false;
            break;
        }
    }
    t.expect(asciiShape,
             "DXF ASCII raw section reference-code matrix preserves framing");

    DRW_RawDxfSection binarySection = section;
    binarySection.m_hasRawValues = false;
    binarySection.m_rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    binaryWriter.setHandleRemap(remap);
    t.expect(binaryWriter.writeRawDxfSection(binarySection),
             "DXF binary raw section remaps every handle-reference code family");
    std::stringstream binaryRecords(binaryOutput.str());
    dxfReaderBinary binaryReader(&binaryRecords);
    binaryReader.setClassifierProfile(DxfClassifierProfile::StandaloneSafe);
    bool binaryShape = true;
    for (const auto& item : expected) {
        if (!binaryReader.readRec(&code) || code != item.first
            || binaryReader.getString() != item.second) {
            binaryShape = false;
            break;
        }
    }
    t.expect(binaryShape,
             "DXF binary raw section reference-code matrix preserves framing");
}

void testDxfArbitraryHandlePayloads(TestContext& t) {
    // Autodesk's 320-329 values are carried as-is, even if they happen to
    // equal a remapped source handle. Pointer/owner codes still translate.
    std::vector<DRW_Variant> payload;
    std::vector<std::pair<int, std::string>> expected;
    for (int code = 320; code <= 329; ++code) {
        payload.emplace_back(code, std::string("14"));
        expected.emplace_back(code, "14");
    }
    payload.emplace_back(320, std::string("DEAD"));
    payload.emplace_back(329, std::string("FEDCBA9876543210"));
    expected.emplace_back(320, "DEAD");
    expected.emplace_back(329, "FEDCBA9876543210");
    for (const int code : {330, 340, 350, 360, 390, 480}) {
        payload.emplace_back(code, std::string("14"));
        expected.emplace_back(code, "200");
    }
    const auto handles = [](const std::string& bytes, bool binary,
                            bool xrecordBody) {
        std::stringstream records(bytes);
        std::unique_ptr<dxfReader> reader = binary
            ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&records))
            : std::unique_ptr<dxfReader>(new dxfReaderAscii(&records));
        reader->setAllowWideHandleLexemes(true);
        std::vector<std::pair<int, std::string>> result;
        bool inBody = !xrecordBody;
        int code = 0;
        while (reader->readRec(&code)) {
            if (xrecordBody && code == 100
                && reader->getString() == "AcDbXrecord")
                inBody = true;
            if (inBody && ((code >= 320 && code <= 369)
                           || code == 390 || code == 480 || code == 1005))
                result.emplace_back(code, reader->getString());
        }
        return result;
    };
    for (const bool binary : {false, true}) {
        for (int route = 0; route < 4; ++route) {
            std::ostringstream output;
            dxfRW writer("");
            writer.version = DRW::AC1027;
            writer.binFile = binary;
            if (binary)
                writer.writer = std::make_unique<dxfWriterBinary>(&output);
            else
                writer.writer = std::make_unique<dxfWriterAscii>(&output);
            writer.setHandleRemap({{0x14u, 0x200u}});
            bool written = false;
            auto routeExpected = expected;
            if (route == 0) {
                DRW_RawDxfObject object;
                object.name = "LOCAL_ARBITRARY_HANDLES";
                object.m_version = DRW::AC1027;
                object.groups.emplace_back(5, std::string("70"));
                object.groups.insert(object.groups.end(), payload.begin(),
                                     payload.end());
                written = writer.writeRawDxfObject(&object);
            } else if (route == 1) {
                std::list<DRW_Variant> group;
                group.emplace_back(102, std::string("{LOCAL_HANDLES"));
                group.insert(group.end(), payload.begin(), payload.end());
                group.emplace_back(102, std::string("}"));
                written = writer.writeAppData({group});
            } else if (route == 2) {
                // The wider 390/480 roles remain covered by raw/app/section
                // routes. They are outside XRECORD's application envelope:
                // assert refusal of the original mixed payload separately.
                std::ostringstream invalidOutput;
                dxfRW invalidWriter("");
                invalidWriter.version = DRW::AC1027;
                invalidWriter.binFile = binary;
                if (binary) invalidWriter.writer = std::make_unique<dxfWriterBinary>(&invalidOutput);
                else invalidWriter.writer = std::make_unique<dxfWriterAscii>(&invalidOutput);
                DRW_XRecord invalidRecord;
                invalidRecord.handle = 0x70u;
                invalidRecord.m_dataEntries = payload;
                t.expect(!invalidWriter.writeXRecord(&invalidRecord) && invalidOutput.str().empty(),
                         "wide handle roles cannot masquerade as XRECORD application data");
                DRW_XRecord record;
                record.handle = 0x70u;
                record.m_dataEntries.assign(payload.begin(), payload.end() - 2);
                routeExpected.resize(expected.size() - 2u);
                written = writer.writeXRecord(&record);
            } else {
                DRW_RawDxfSection section;
                section.m_name = "LOCAL_ARBITRARY_HANDLES";
                section.m_version = DRW::AC1027;
                section.m_groups = payload;
                written = writer.writeRawDxfSection(section);
            }
            t.expect(written && handles(output.str(), binary, route == 2)
                                    == routeExpected,
                     "arbitrary handles survive raw/app/XRECORD/section replay while references remap");
        }
        std::ostringstream output;
        dxfRW writer("");
        writer.version = DRW::AC1027;
        writer.binFile = binary;
        if (binary)
            writer.writer = std::make_unique<dxfWriterBinary>(&output);
        else
            writer.writer = std::make_unique<dxfWriterAscii>(&output);
        writer.setHandleRemap({{0x14u, 0x200u}});
        DRW_Variant app(1001, std::string("LOCAL_HANDLES"));
        DRW_Variant reference(1005, std::string("14"));
        t.expect(writer.writeExtData({&app, &reference})
                     && handles(output.str(), binary, false)
                        == std::vector<std::pair<int, std::string>>{{1005, "200"}},
                 "XDATA handle follows reference remapping within its application group");
    }
}

void testRawCapture(TestContext& t) {
    std::stringstream records("260\n2147483647\n482\n3.14\n1004\nAB\n");
    dxfRW owner("");
    owner.binFile = false;
    owner.reader = std::make_unique<dxfReaderAscii>(&records);
    DRW_RawDxfObject object;
    int code = 0;
    while (owner.reader->readRec(&code))
        t.expect(owner.captureRawGroup(object, code),
                 "raw DXF capture accepts canonical group kind");
    t.expect(object.groups.size() == 3 && object.rawValues.size() == 3
                 && object.hasRawValues,
             "raw DXF capture retains parallel source spellings");
    if (object.groups.size() == 3 && object.rawValues.size() == 3) {
        t.expect(object.groups[0].type() == DRW_Variant::STRING
                     && object.groups[0].c_str() != nullptr
                     && object.groups[0].c_str() == std::string("2147483647"),
                 "raw DXF capture stores unknown code 260 as its source string");
        t.expect(object.groups[1].type() == DRW_Variant::STRING
                     && object.groups[1].c_str() != nullptr
                     && object.groups[1].c_str() == std::string("3.14")
                     && object.rawValues[1] == "3.14",
                 "raw DXF capture preserves unknown source spelling");
        t.expect(object.groups[2].type() == DRW_Variant::STRING
                     && object.rawValues[2] == "AB",
                 "raw DXF capture preserves binary chunk text");
    }
}

void testHatchValidationIgnoresInactiveGradient(TestContext& t) {
    dxfRW owner("");
    owner.version = DRW::AC1027;
    DRW_Hatch hatch;
    hatch.name = "SOLID";
    hatch.gradName = "\x01stale-gradient";
    DRW_Hatch::GradientStop stale;
    stale.rgb = -1039906145;
    stale.colorMethod = 7;
    stale.colorName = "\x02stale";
    hatch.gradColors.push_back(stale);
    t.expect(owner.validateHatchPayload(&hatch),
             "solid hatch ignores stale inactive gradient carriers");

    hatch.isGradient = 1;
    t.expect(!owner.validateHatchPayload(&hatch),
             "active gradient still validates gradient carriers");

    DRW_Hatch mappedGradient;
    mappedGradient.isGradient = 1;
    mappedGradient.gradName = "LINEAR";
    DRW_Hatch::GradientStop firstStop;
    firstStop.value = 0.0;
    firstStop.rgb = 0x112233;
    DRW_Hatch::GradientStop secondStop;
    secondStop.value = 1.0;
    secondStop.rgb = 0x445566;
    mappedGradient.gradColors = {firstStop, secondStop};
    t.expect(owner.validateHatchPayload(&mappedGradient),
             "DXF gradient with supported color fields remains writable");
    mappedGradient.gradColors.front().colorName = "gradient-entry";
    mappedGradient.gradColors.front().colorBookName = "gradient-book";

    for (const bool binary : {false, true}) {
        std::ostringstream legacyHatchOutput;
        dxfRW legacyHatchWriter("");
        legacyHatchWriter.version = DRW::AC1015;
        legacyHatchWriter.binFile = binary;
        if (binary)
            legacyHatchWriter.writer =
                std::make_unique<dxfWriterBinary>(&legacyHatchOutput);
        else
            legacyHatchWriter.writer =
                std::make_unique<dxfWriterAscii>(&legacyHatchOutput);
        t.expect(!legacyHatchWriter.writeHatch(&mappedGradient)
                     && legacyHatchOutput.str().empty(),
                 "ASCII/binary R2000 HATCH rejects R2004+ gradient transactionally");

        DRW_MPolygon legacyGradient;
        legacyGradient.isGradient = 1;
        legacyGradient.gradName = "LINEAR";
        legacyGradient.gradColors = mappedGradient.gradColors;
        std::ostringstream legacyMPolygonOutput;
        dxfRW legacyMPolygonWriter("");
        legacyMPolygonWriter.version = DRW::AC1015;
        legacyMPolygonWriter.binFile = binary;
        if (binary)
            legacyMPolygonWriter.writer =
                std::make_unique<dxfWriterBinary>(&legacyMPolygonOutput);
        else
            legacyMPolygonWriter.writer =
                std::make_unique<dxfWriterAscii>(&legacyMPolygonOutput);
        t.expect(!legacyMPolygonWriter.writeMPolygon(&legacyGradient)
                     && legacyMPolygonOutput.str().empty(),
                 "ASCII/binary R2000 MPOLYGON rejects R2004+ gradient transactionally");

        std::ostringstream hatchOutput;
        dxfRW hatchWriter("");
        hatchWriter.version = DRW::AC1018;
        hatchWriter.binFile = binary;
        if (binary)
            hatchWriter.writer = std::make_unique<dxfWriterBinary>(&hatchOutput);
        else
            hatchWriter.writer = std::make_unique<dxfWriterAscii>(&hatchOutput);
        const bool hatchWritten = hatchWriter.writeHatch(&mappedGradient)
            && hatchWriter.writer->writeString(0, "WIRE_END");
        bool hatchEnded = false;
        const auto hatchTags = numericWireProjection(
            hatchOutput.str(), binary, hatchEnded);
        t.expect(hatchWritten && hatchEnded
                     && std::find(hatchTags.begin(), hatchTags.end(),
                                  "432:gradient-entry") != hatchTags.end()
                     && std::find(hatchTags.begin(), hatchTags.end(),
                                  "433:gradient-book") != hatchTags.end()
                     && std::none_of(hatchTags.begin(), hatchTags.end(),
                                     [](const std::string& tag) {
                                         return tag.rfind("431:", 0) == 0;
                                     }),
                 "ASCII/binary HATCH writes string color names without group-431 integer");

        DRW_Hatch unsupportedMethod = mappedGradient;
        unsupportedMethod.gradColors.back().colorMethod = 7;
        std::ostringstream rejectedOutput;
        dxfRW rejectingWriter("");
        rejectingWriter.version = DRW::AC1027;
        rejectingWriter.binFile = binary;
        if (binary)
            rejectingWriter.writer =
                std::make_unique<dxfWriterBinary>(&rejectedOutput);
        else
            rejectingWriter.writer =
                std::make_unique<dxfWriterAscii>(&rejectedOutput);
        t.expect(!rejectingWriter.writeHatch(&unsupportedMethod)
                     && rejectedOutput.str().empty(),
                 "ASCII/binary HATCH rejects unmapped DWG color method transactionally");

        DRW_MPolygon unsupportedMPolygon;
        unsupportedMPolygon.isGradient = 1;
        unsupportedMPolygon.gradName = "LINEAR";
        unsupportedMPolygon.gradColors = unsupportedMethod.gradColors;
        std::ostringstream rejectedMPolygonOutput;
        dxfRW rejectingMPolygonWriter("");
        rejectingMPolygonWriter.version = DRW::AC1027;
        rejectingMPolygonWriter.binFile = binary;
        if (binary)
            rejectingMPolygonWriter.writer =
                std::make_unique<dxfWriterBinary>(&rejectedMPolygonOutput);
        else
            rejectingMPolygonWriter.writer =
                std::make_unique<dxfWriterAscii>(&rejectedMPolygonOutput);
        t.expect(!rejectingMPolygonWriter.writeMPolygon(&unsupportedMPolygon)
                     && rejectedMPolygonOutput.str().empty(),
                 "ASCII/binary MPOLYGON shares unmapped-method refusal");

        std::ostringstream encoded;
        if (binary) {
            dxfWriterBinary output(&encoded);
            t.expect(output.writeDouble(463, 0.25)
                         && output.writeUtf8String(431, "7")
                         && output.writeUtf8String(432, "gradient-entry"),
                     "binary gradient string control is encoded");
        } else {
            dxfWriterAscii output(&encoded);
            t.expect(output.writeDouble(463, 0.25)
                         && output.writeUtf8String(431, "7")
                         && output.writeUtf8String(432, "gradient-entry"),
                     "ASCII gradient string control is encoded");
        }
        std::stringstream input(encoded.str());
        std::unique_ptr<dxfReader> reader = binary
            ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
            : std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
        class TestableHatch final : public DRW_Hatch {
        public:
            using DRW_Hatch::parseCode;
            using DRW_Hatch::validateDxf;
        };
        TestableHatch parsed;
        int code = 0;
        while (reader->readRec(&code)) {
            if (code == 431)
                t.expect(reader->type == dxfReader::STRING,
                         "DXF gradient group 431 is read as a string");
            t.expect(parsed.parseCode(code, reader),
                     "DXF gradient string fields parse without type mismatch");
        }
        t.expect(parsed.gradColors.size() == 1
                     && parsed.gradColors.front().value == 0.25
                     && parsed.gradColors.front().colorMethod == 0
                     && parsed.gradColors.front().colorName == "gradient-entry",
                 "unmapped group 431 is ignored without corrupting later gradient fields");
    }

    // Keep the per-stop ACI/RGB grammar separate from MPOLYGON's top-level
    // group 63. The raw tag sequence is checked before semantic parsing so a
    // parser/writer pair cannot agree on the wrong stop association.
    DRW_Hatch perStopColors;
    perStopColors.isGradient = 1;
    perStopColors.gradName = "LINEAR";
    perStopColors.gradColors = {{0.0, -1, 255}, {1.0, 0x00A600, 0}};
    for (const bool binary : {false, true}) {
        std::ostringstream output;
        dxfRW writer("");
        writer.version = DRW::AC1027;
        writer.binFile = binary;
        if (binary)
            writer.writer = std::make_unique<dxfWriterBinary>(&output);
        else
            writer.writer = std::make_unique<dxfWriterAscii>(&output);
        const bool written = writer.writeHatch(&perStopColors)
            && writer.writer->writeString(0, "WIRE_END");
        bool ended = false;
        const auto tags = numericWireProjection(output.str(), binary, ended);
        const std::array<std::string, 4> expectedStopTags {{
            "463:0x0p+0", "63:255", "463:0x1p+0", "421:42496"}};
        const auto sequence = std::search(tags.cbegin(), tags.cend(),
            expectedStopTags.cbegin(), expectedStopTags.cend());
        t.expect(written && ended && sequence != tags.cend(),
                 "ASCII/binary HATCH keeps per-stop ACI 63 and truecolor 421 scoped and ordered");

        std::stringstream input(output.str());
        std::unique_ptr<dxfReader> reader = binary
            ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
            : std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
        class TestableHatch final : public DRW_Hatch {
        public:
            using DRW_Hatch::parseCode;
        } parsed;
        int code = 0;
        bool parseOk = true;
        bool foundEnd = false;
        while (reader->readRec(&code)) {
            if (code == 0 && reader->getString() == "WIRE_END") {
                foundEnd = true;
                break;
            }
            if (code == 0 && reader->getString() == "HATCH")
                continue;
            parseOk = parsed.parseCode(code, reader) && parseOk;
        }
        t.expect(parseOk && foundEnd && parsed.gradColors.size() == 2u
                     && parsed.gradColors[0].value == 0.0
                     && parsed.gradColors[0].aciColor == 255
                     && parsed.gradColors[0].rgb == -1
                     && parsed.gradColors[1].value == 1.0
                     && parsed.gradColors[1].aciColor == 0
                     && parsed.gradColors[1].rgb == 0x00A600,
                 "ASCII/binary HATCH parser associates group 63/421 values with their gradient stops");
    }

    DRW_Hatch byLayerByBlockColors;
    byLayerByBlockColors.isGradient = 1;
    byLayerByBlockColors.gradName = "LINEAR";
    byLayerByBlockColors.gradColors = {
        {0.0, -1, 256, 0xC0u}, {1.0, -1, 0, 0xC1u}};
    for (const bool binary : {false, true}) {
        std::ostringstream output;
        dxfRW writer("");
        writer.version = DRW::AC1027;
        writer.binFile = binary;
        if (binary)
            writer.writer = std::make_unique<dxfWriterBinary>(&output);
        else
            writer.writer = std::make_unique<dxfWriterAscii>(&output);
        const bool written = writer.writeHatch(&byLayerByBlockColors)
            && writer.writer->writeString(0, "WIRE_END");
        bool ended = false;
        const auto tags = numericWireProjection(output.str(), binary, ended);
        const std::array<std::string, 4> expectedStopTags {{
            "463:0x0p+0", "63:256", "463:0x1p+0", "63:0"}};
        const auto sequence = std::search(tags.cbegin(), tags.cend(),
            expectedStopTags.cbegin(), expectedStopTags.cend());
        t.expect(written && ended && sequence != tags.cend(),
                 "ASCII/binary HATCH preserves gradient ByLayer/ByBlock as ACI 256/0");

        std::stringstream input(output.str());
        std::unique_ptr<dxfReader> reader = binary
            ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
            : std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
        class TestableHatch final : public DRW_Hatch {
        public:
            using DRW_Hatch::parseCode;
        } parsed;
        int code = 0;
        bool parseOk = true;
        bool foundEnd = false;
        while (reader->readRec(&code)) {
            if (code == 0 && reader->getString() == "WIRE_END") {
                foundEnd = true;
                break;
            }
            if (code == 0 && reader->getString() == "HATCH")
                continue;
            parseOk = parsed.parseCode(code, reader) && parseOk;
        }
        t.expect(parseOk && foundEnd && parsed.gradColors.size() == 2u
                     && parsed.gradColors[0].aciColor == 256
                     && parsed.gradColors[0].colorMethod == 0xC0u
                     && parsed.gradColors[1].aciColor == 0
                     && parsed.gradColors[1].colorMethod == 0xC1u,
                 "ASCII/binary HATCH parser retains gradient ByLayer/ByBlock methods");
    }

    DRW_MPolygon byLayerByBlockMPolygon;
    byLayerByBlockMPolygon.dxfVersion = 1;
    byLayerByBlockMPolygon.name = "SOLID";
    byLayerByBlockMPolygon.solid = 1;
    byLayerByBlockMPolygon.fillColorAci = 4;
    byLayerByBlockMPolygon.xDirX = 1.0;
    byLayerByBlockMPolygon.xDirY = 0.0;
    byLayerByBlockMPolygon.isGradient = 1;
    byLayerByBlockMPolygon.gradName = "LINEAR";
    byLayerByBlockMPolygon.gradColors = {
        {0.0, -1, 256, 0xC0u}, {1.0, -1, 0, 0xC1u}};
    auto mpolygonLoop = std::make_shared<DRW_HatchLoop>(2);
    auto mpolygonBoundary = std::make_shared<DRW_LWPolyline>();
    mpolygonBoundary->flags = 1;
    mpolygonBoundary->addVertex(DRW_Vertex2D(0.0, 0.0, 0.0));
    mpolygonBoundary->addVertex(DRW_Vertex2D(1.0, 0.0, 0.0));
    mpolygonBoundary->addVertex(DRW_Vertex2D(1.0, 1.0, 0.0));
    mpolygonBoundary->addVertex(DRW_Vertex2D(0.0, 1.0, 0.0));
    mpolygonLoop->objlist.push_back(mpolygonBoundary);
    mpolygonLoop->update();
    byLayerByBlockMPolygon.appendLoop(mpolygonLoop);
    for (const bool binary : {false, true}) {
        std::ostringstream output;
        dxfRW writer("");
        writer.version = DRW::AC1027;
        writer.binFile = binary;
        if (binary)
            writer.writer = std::make_unique<dxfWriterBinary>(&output);
        else
            writer.writer = std::make_unique<dxfWriterAscii>(&output);
        const bool written = writer.writeMPolygon(&byLayerByBlockMPolygon)
            && writer.writer->writeString(0, "WIRE_END");
        bool ended = false;
        const auto tags = numericWireProjection(output.str(), binary, ended);
        const std::array<std::string, 4> expectedStopTags {{
            "463:0x0p+0", "63:256", "463:0x1p+0", "63:0"}};
        const auto sequence = std::search(tags.cbegin(), tags.cend(),
            expectedStopTags.cbegin(), expectedStopTags.cend());
        t.expect(written && ended && sequence != tags.cend(),
                 "ASCII/binary MPOLYGON preserves gradient ByLayer/ByBlock values");

        std::stringstream input(output.str());
        std::unique_ptr<dxfReader> reader = binary
            ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
            : std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
        class TestableMPolygon final : public DRW_MPolygon {
        public:
            using DRW_MPolygon::parseCode;
            using DRW_MPolygon::validateDxf;
        } parsed;
        int code = 0;
        bool parseOk = true;
        bool foundEnd = false;
        while (reader->readRec(&code)) {
            if (code == 0 && reader->getString() == "WIRE_END") {
                foundEnd = true;
                break;
            }
            if (code == 0 && reader->getString() == "MPOLYGON")
                continue;
            parseOk = parsed.parseCode(code, reader) && parseOk;
        }
        t.expect(parseOk && foundEnd && parsed.validateDxf()
                     && parsed.fillColorAci == 4
                     && parsed.gradColors.size() == 2u
                     && parsed.gradColors[0].aciColor == 256
                     && parsed.gradColors[0].colorMethod == 0xC0u
                     && parsed.gradColors[1].aciColor == 0
                     && parsed.gradColors[1].colorMethod == 0xC1u,
                 "ASCII/binary MPOLYGON parser preserves fill and gradient color methods");
    }

    DRW_Hatch invalidElevation;
    invalidElevation.basePoint = DRW_Coord{1.0, 0.0, 7.0};
    t.expect(!owner.validateHatchPayload(&invalidElevation),
             "HATCH writer rejects nonzero OCS elevation X instead of dropping it");
    invalidElevation.basePoint = DRW_Coord{0.0, 1.0, 7.0};
    t.expect(!owner.validateHatchPayload(&invalidElevation),
             "HATCH writer rejects nonzero OCS elevation Y instead of dropping it");
    invalidElevation.basePoint = DRW_Coord{0.0, 0.0, 7.0};
    invalidElevation.extPoint = DRW_Coord{0.0, 0.0, 0.0};
    t.expect(!owner.validateHatchPayload(&invalidElevation),
             "HATCH writer rejects zero extrusion direction without an OCS frame");
    DRW_MPolygon invalidMPolygon;
    invalidMPolygon.extPoint = DRW_Coord{0.0, 0.0, 0.0};
    t.expect(!owner.validateHatchPayload(&invalidMPolygon),
             "MPOLYGON writer shares HATCH zero-extrusion validation");
}

void testHatchGradientDxfGrammar(TestContext& t) {
    enum class Kind { Integer, Double, String };
    struct Tag {
        int code;
        Kind kind;
        int integer;
        double real;
        std::string text;
    };
    const auto i = [](int code, int value) {
        return Tag{code, Kind::Integer, value, 0.0, std::string{}};
    };
    const auto d = [](int code, double value) {
        return Tag{code, Kind::Double, 0, value, std::string{}};
    };
    const auto s = [](int code, const std::string& value) {
        return Tag{code, Kind::String, 0, 0.0, value};
    };
    const auto twoColorGradient = [&]() {
        return std::vector<Tag>{
            i(450, 1), i(451, 0), i(452, 0), i(453, 2),
            d(460, 0.25), d(461, 0.5), d(462, 1.5),
            d(463, 0.0), i(421, 0x112233),
            d(463, 1.0), i(421, 0x445566), s(470, "LINEAR")};
    };
    const auto appendSentinel = [&s](std::vector<Tag>& tags) {
        tags.push_back(s(0, "EOF"));
    };
    const auto parse = [&](const std::vector<Tag>& tags, bool binary,
                           std::string* parsedGradientName = nullptr) {
        std::ostringstream encoded;
        std::unique_ptr<dxfWriter> writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&encoded))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&encoded));
        for (const Tag& tag : tags) {
            bool written = false;
            switch (tag.kind) {
            case Kind::Integer:
                written = writer->writeInt32(tag.code, tag.integer);
                break;
            case Kind::Double:
                written = writer->writeDouble(tag.code, tag.real);
                break;
            case Kind::String:
                written = writer->writeUtf8String(tag.code, tag.text);
                break;
            }
            if (!written)
                return false;
        }
        std::stringstream input(encoded.str());
        std::unique_ptr<dxfReader> reader = binary
            ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
            : std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
        class TestableHatch final : public DRW_Hatch {
        public:
            using DRW_Hatch::parseCode;
            using DRW_Hatch::validateDxf;
        };
        TestableHatch hatch;
        bool sawSentinel = false;
        int code = 0;
        while (reader->readRec(&code)) {
            if (code == 0) {
                sawSentinel = reader->getString() == "EOF";
                continue;
            }
            if (!hatch.parseCode(code, reader))
                return false;
        }
        const bool valid = sawSentinel && hatch.validateDxf();
        if (valid && parsedGradientName != nullptr)
            *parsedGradientName = hatch.gradName;
        return valid;
    };

    dxfRW writerOwner("");
    writerOwner.version = DRW::AC1027;
    DRW_Hatch apiTwoColor;
    apiTwoColor.isGradient = 1;
    apiTwoColor.gradName = "LINEAR";
    apiTwoColor.gradColors = {
        DRW_Hatch::GradientStop{0.0, 0x112233, 0, 0, "", ""},
        DRW_Hatch::GradientStop{1.0, 0x445566, 0, 0, "", ""}};
    t.expect(writerOwner.validateHatchPayload(&apiTwoColor),
             "API HATCH accepts the documented two-color gradient grammar");
    DRW_Hatch apiSingleColor = apiTwoColor;
    apiSingleColor.singleColor = 1;
    apiSingleColor.gradTint = 0.5;
    t.expect(writerOwner.validateHatchPayload(&apiSingleColor),
             "API HATCH accepts single-color gradient tint in range");
    apiSingleColor.gradTint = 1.01;
    t.expect(!writerOwner.validateHatchPayload(&apiSingleColor),
             "API HATCH rejects out-of-range active single-color tint");
    apiSingleColor = apiTwoColor;
    apiSingleColor.gradColors.pop_back();
    t.expect(!writerOwner.validateHatchPayload(&apiSingleColor),
             "API HATCH rejects a gradient with fewer than two color stops");

    const std::vector<Tag> solidBlock{
        i(450, 0), i(451, 99), i(452, 77), i(453, 0),
        d(460, -5.0), d(461, 4.0), d(462, 3.0), s(470, "ignored-solid")};
    for (const bool binary : {false, true}) {
        std::vector<Tag> validTwoColor = twoColorGradient();
        appendSentinel(validTwoColor);
        t.expect(parse(validTwoColor, binary),
                 "ASCII/binary HATCH accepts complete two-color gradient block");

        std::vector<Tag> validSingleColor = twoColorGradient();
        for (Tag& tag : validSingleColor) {
            if (tag.code == 452)
                tag.integer = 1;
            else if (tag.code == 462)
                tag.real = 0.5;
        }
        appendSentinel(validSingleColor);
        t.expect(parse(validSingleColor, binary),
                 "ASCII/binary HATCH accepts complete single-color tint block");

        std::vector<Tag> validSolid = solidBlock;
        appendSentinel(validSolid);
        t.expect(parse(validSolid, binary),
                 "ASCII/binary solid HATCH ignores present gradient companion values");

        for (const int orphanCode : {451, 452, 453, 460, 461, 462, 463, 470}) {
            std::vector<Tag> orphan;
            switch (orphanCode) {
            case 460: case 461: case 462: case 463:
                orphan.push_back(d(orphanCode, 0.0));
                break;
            case 470:
                orphan.push_back(s(orphanCode, "LINEAR"));
                break;
            default:
                orphan.push_back(i(orphanCode, 0));
                break;
            }
            appendSentinel(orphan);
            t.expect(!parse(orphan, binary),
                     "ASCII/binary HATCH rejects orphan gradient companion");
        }

        for (const int missingCode : {451, 452, 453, 460, 461, 462, 470}) {
            std::vector<Tag> missing = twoColorGradient();
            const auto found = std::find_if(missing.begin(), missing.end(),
                [missingCode](const Tag& tag) {
                    return tag.code == missingCode;
                });
            if (found != missing.end())
                missing.erase(found);
            appendSentinel(missing);
            t.expect(!parse(missing, binary),
                     "ASCII/binary HATCH rejects missing mandatory gradient companion");
        }

        for (const int duplicateCode : {450, 451, 452, 453, 460, 461, 462}) {
            std::vector<Tag> duplicate = twoColorGradient();
            const auto found = std::find_if(duplicate.begin(), duplicate.end(),
                [duplicateCode](const Tag& tag) {
                    return tag.code == duplicateCode;
                });
            if (found != duplicate.end())
                duplicate.push_back(*found);
            appendSentinel(duplicate);
            t.expect(!parse(duplicate, binary),
                     "ASCII/binary HATCH rejects duplicate singleton gradient group");
        }

        std::vector<Tag> repeatedGradientName = twoColorGradient();
        repeatedGradientName.push_back(s(470, "LINEAR-OVERRIDE"));
        appendSentinel(repeatedGradientName);
        std::string parsedGradientName;
        t.expect(parse(repeatedGradientName, binary, &parsedGradientName)
                     && parsedGradientName == "LINEAR-OVERRIDE",
                 "ASCII/binary HATCH keeps last-value-wins for repeated gradient name");

        for (const int invalidCount : {0, 1, 3}) {
            std::vector<Tag> wrongCount = twoColorGradient();
            wrongCount.erase(std::remove_if(wrongCount.begin(), wrongCount.end(),
                [](const Tag& tag) { return tag.code == 463 || tag.code == 421; }),
                wrongCount.end());
            for (Tag& tag : wrongCount) {
                if (tag.code == 453)
                    tag.integer = invalidCount;
            }
            for (int stop = 0; stop < invalidCount; ++stop) {
                wrongCount.push_back(d(463, invalidCount == 1
                    ? 0.0 : static_cast<double>(stop)
                        / static_cast<double>(invalidCount - 1)));
                wrongCount.push_back(i(421, 0x112233 + stop));
            }
            appendSentinel(wrongCount);
            t.expect(!parse(wrongCount, binary),
                     "ASCII/binary HATCH rejects gradient stop counts other than two");
        }

        std::vector<Tag> reversedStops = twoColorGradient();
        for (Tag& tag : reversedStops) {
            if (tag.code == 463 && tag.real == 0.0)
                tag.real = 1.0;
            else if (tag.code == 463 && tag.real == 1.0)
                tag.real = 0.0;
        }
        appendSentinel(reversedStops);
        t.expect(!parse(reversedStops, binary),
                 "ASCII/binary HATCH requires ordered gradient endpoint roles");

        for (const double tint : {-0.01, 1.01}) {
            std::vector<Tag> invalidTint = twoColorGradient();
            for (Tag& tag : invalidTint) {
                if (tag.code == 452)
                    tag.integer = 1;
                else if (tag.code == 462)
                    tag.real = tint;
            }
            appendSentinel(invalidTint);
            t.expect(!parse(invalidTint, binary),
                     "ASCII/binary HATCH bounds tint only for single-color mode");
        }

        std::vector<Tag> invalidMode = twoColorGradient();
        for (Tag& tag : invalidMode) {
            if (tag.code == 452)
                tag.integer = 2;
        }
        appendSentinel(invalidMode);
        t.expect(!parse(invalidMode, binary),
                 "ASCII/binary HATCH rejects undefined gradient color mode");
    }
}

void testFixedSpaceBlockClassification(TestContext& t) {
    dx_ifaceBlock model;
    model.name = "*Model_Space";
    dx_ifaceBlock paper;
    paper.name = "*Paper_Space";
    dx_ifaceBlock custom;
    custom.name = "CUSTOM_BLOCK";
    t.expect(dx_iface::isFixedSpaceBlock(&model)
                 && dx_iface::isFixedSpaceBlock(&paper)
                 && !dx_iface::isFixedSpaceBlock(&custom)
                 && !dx_iface::isFixedSpaceBlock(nullptr),
             "adapter classifies only canonical fixed-space blocks");
}

struct R2007FooterVector {
    std::vector<std::uint8_t> bytes;
    std::uint64_t endBit {0};
};

R2007FooterVector r2007StringFooter(std::size_t stringSize,
                                    bool extendedSize) {
    dwgBufferW writer;
    std::vector<std::uint8_t> strings(stringSize, 0);
    for (std::size_t i = 0; i < stringSize; ++i)
        strings[i] = static_cast<std::uint8_t>(0x40u + (i & 0x3Fu));
    writer.putBytes(strings.data(), strings.size());
    for (int bit = 0; bit < 7; ++bit)
        writer.putBit(0);
    const std::uint64_t stringBitSize =
        static_cast<std::uint64_t>(stringSize) * 8u + 7u;
    if (extendedSize) {
        const std::uint16_t high = static_cast<std::uint16_t>(stringBitSize >> 15u);
        writer.putRawShort16(high);
    }
    writer.putRawShort16(static_cast<std::uint16_t>(
        (stringBitSize & 0x7FFFu) | (extendedSize ? 0x8000u : 0u)));
    writer.putBit(1);
    return {writer.data(), writer.bitCount()};
}

void testR2007StringFooterBounds(TestContext& t) {
    const auto ordinary = r2007StringFooter(7u, false);
    dwgBuffer ordinaryBuffer(const_cast<std::uint8_t*>(ordinary.bytes.data()),
                             ordinary.bytes.size());
    std::uint64_t startBit = 0;
    std::uint64_t endBit = 0;
    const bool ordinaryOk = ordinaryBuffer.getR2007StringStreamBounds(
        ordinary.endBit, startBit, endBit);
    t.expect(ordinaryOk
                 && startBit == 0u && endBit == 7u * 8u + 7u,
             "R2007 string footer ordinary bounds");

    const auto extended = r2007StringFooter(32768u, true);
    dwgBuffer extendedBuffer(const_cast<std::uint8_t*>(extended.bytes.data()),
                             extended.bytes.size());
    const bool extendedOk = extendedBuffer.getR2007StringStreamBounds(
        extended.endBit, startBit, endBit);
    t.expect(extendedOk
                 && startBit == 0u && endBit == 32768u * 8u + 7u,
             "R2007 string footer high-bit bounds");
    t.expect(extendedBuffer.seekR2007StringStream(extended.endBit)
                 && extendedBuffer.getPosition() == 0u
                 && extendedBuffer.getBitPos() == 0u
                 && extendedBuffer.getRawChar8() == 0x40u,
             "R2007 string footer seeks to stream start");

    dwgBufferW absentWriter;
    for (int bit = 0; bit < 7; ++bit)
        absentWriter.putBit(0);
    absentWriter.putRawShort16(0);
    absentWriter.putBit(0);
    const R2007FooterVector absent{absentWriter.data(), absentWriter.bitCount()};
    dwgBuffer absentBuffer(const_cast<std::uint8_t*>(absent.bytes.data()),
                           absent.bytes.size());
    const bool absentOk = absentBuffer.getR2007StringStreamBounds(
        absent.endBit, startBit, endBit);
    t.expect(absentOk
                 && startBit == absent.endBit && endBit == absent.endBit,
             "R2007 string footer absent stream");

    const auto truncated = std::vector<std::uint8_t>{0x80u, 0x80u};
    dwgBuffer truncatedBuffer(const_cast<std::uint8_t*>(truncated.data()),
                              truncated.size());
    t.expect(!truncatedBuffer.getR2007StringStreamBounds(
                  16u, startBit, endBit),
             "R2007 string footer rejects truncated extension");
}

void testDecompressor(TestContext& t) {
    constexpr std::uint32_t literalCount = 0x8000u;
    std::vector<std::uint8_t> compressed = literalRunHeader(literalCount);
    for (std::uint32_t i = 0; i < literalCount; ++i)
        compressed.push_back(static_cast<std::uint8_t>(i & 0xFFu));
    compressed.push_back(0x18);
    compressed.push_back(0x05);
    compressed.push_back(0x00);
    compressed.push_back(0x00);
    compressed.push_back(0x11);
    std::vector<std::uint8_t> output(0x8100, 0xCC);
    dwgCompressor compressor;
    t.expect(compressor.decompress18(compressed.data(), output.data(),
                                     compressed.size(), output.size()),
             "R2004 decompressor extended copy");
    t.expect(compressor.decompressedBytes() == literalCount + 14u,
             "R2004 decompressor output count");
    bool copied = true;
    for (std::uint32_t i = 0; i < 14; ++i)
        copied = copied && output[literalCount + i] == static_cast<std::uint8_t>(i);
    t.expect(copied, "R2004 decompressor back-reference bytes");

    std::uint8_t oneByte = 0;
    t.expect(!compressor.decompress18(&oneByte, output.data(), 1, output.size()),
             "R2004 decompressor rejects short input");
    t.expect(!compressor.decompress18(nullptr, output.data(), 8, output.size()),
             "R2004 decompressor rejects null input");

    // The R2004 long-copy token stores its extended length in an unbounded
    // run of 0x00 continuation bytes. A 32-bit addition of the token's fixed
    // bias used to wrap this logically huge copy to 31 bytes, even though the
    // output window has room for the full clipped copy.
    constexpr std::uint32_t zeroLengthExtensions = 16843008u;
    constexpr std::uint32_t longCopyPrefixSize = 18u;
    std::vector<std::uint8_t> longCopy{0x0Fu}; // 15 + 3 literal bytes
    for (std::uint32_t i = 0; i < longCopyPrefixSize; ++i)
        longCopy.push_back(static_cast<std::uint8_t>('A' + (i % 3u)));
    longCopy.push_back(0x20u);
    longCopy.insert(longCopy.end(), zeroLengthExtensions, 0x00u);
    longCopy.push_back(0xFEu);
    longCopy.push_back(0x00u); // back-reference offset low bits
    longCopy.push_back(0x00u); // back-reference offset high bits
    longCopy.push_back(0x11u); // end marker, also terminates the next length probe
    std::array<std::uint8_t, 64> longCopyOutput{};
    dwgCompressor longCopyCompressor;
    const bool acceptedLongCopy = longCopyCompressor.decompress18(
        longCopy.data(), longCopyOutput.data(), longCopy.size(),
        longCopyOutput.size());
    t.expect(acceptedLongCopy,
             "R2004 extended copy token is structurally accepted");
    t.expect(longCopyCompressor.decompressedBytes() == longCopyOutput.size(),
             "R2004 extended copy length is clipped to the full output window");
    t.expect(std::equal(longCopyOutput.begin(),
                        longCopyOutput.begin() + longCopyPrefixSize,
                        longCopy.begin() + 1)
                 && std::all_of(longCopyOutput.begin() + longCopyPrefixSize,
                                longCopyOutput.end(),
                                [](std::uint8_t value) { return value == 'C'; }),
             "R2004 extended copy output preserves prefix and repeated byte");

    std::vector<std::uint8_t> longLiteral{0x00u};
    longLiteral.insert(longLiteral.end(), zeroLengthExtensions, 0x00u);
    longLiteral.push_back(0xFEu);
    std::array<std::uint8_t, 64> expectedLongLiteral{};
    for (std::size_t i = 0; i < expectedLongLiteral.size(); ++i)
        expectedLongLiteral[i] = static_cast<std::uint8_t>(i);
    expectedLongLiteral[16] = 0x11u;
    longLiteral.insert(longLiteral.end(), expectedLongLiteral.begin(),
                       expectedLongLiteral.end());
    std::array<std::uint8_t, 64> longLiteralOutput{};
    dwgCompressor longLiteralCompressor;
    const bool acceptedLongLiteral = longLiteralCompressor.decompress18(
        longLiteral.data(), longLiteralOutput.data(), longLiteral.size(),
        longLiteralOutput.size());
    t.expect(acceptedLongLiteral
                 && longLiteralCompressor.decompressedBytes()
                        == longLiteralOutput.size()
                 && longLiteralOutput == expectedLongLiteral,
             "R2004 extended literal length does not wrap before output-window clipping");

    const std::uint64_t onePastUInt32 =
        static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max())
        + 1u;
    const std::uint64_t oversized18Input =
        onePastUInt32 + compressed.size();
    std::fill(output.begin(), output.end(), 0xCCu);
    const bool acceptedOversized18Input = compressor.decompress18(
        compressed.data(), output.data(), oversized18Input, output.size());
    t.expect(!acceptedOversized18Input
                 && compressor.decompressedBytes() == 0u
                 && std::all_of(output.begin(), output.end(),
                                [](std::uint8_t value) { return value == 0xCCu; }),
             "R2004 decompressor rejects input sizes wider than its 32-bit cursor before writing output");

    std::fill(output.begin(), output.end(), 0xCCu);
    const std::uint64_t oversized18Output = onePastUInt32 + output.size();
    const bool acceptedOversized18Output = compressor.decompress18(
        compressed.data(), output.data(), compressed.size(),
        oversized18Output);
    t.expect(!acceptedOversized18Output
                 && compressor.decompressedBytes() == 0u
                 && std::all_of(output.begin(), output.end(),
                                [](std::uint8_t value) { return value == 0xCCu; }),
             "R2004 decompressor rejects output windows wider than its 32-bit cursor before writing output");
    t.expect(compressor.decompress18(compressed.data(), output.data(),
                                     compressed.size(), output.size())
                 && compressor.decompressedBytes() == literalCount + 14u,
             "R2004 decompressor accepts a valid retry after oversized-size refusal");

    const std::vector<std::uint8_t> compressed21{
        0x00u, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 0x87u, 0x00u};
    const std::array<std::uint8_t, 16> expected21{
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
    std::array<std::uint8_t, 16> output21{};
    t.expect(compressor.decompress21(compressed21.data(), output21.data(),
                                     compressed21.size(), output21.size())
                 && output21 == expected21,
             "R2007 decompressor accepts the bounded literal/back-reference control");

    output21.fill(0xCCu);
    const bool acceptedOversized21Input = compressor.decompress21(
        compressed21.data(), output21.data(),
        onePastUInt32 + compressed21.size(), output21.size());
    t.expect(!acceptedOversized21Input
                 && compressor.decompressedBytes() == 0u
                 && std::all_of(output21.begin(), output21.end(),
                                [](std::uint8_t value) { return value == 0xCCu; }),
             "R2007 decompressor rejects input sizes wider than its 32-bit cursor before writing output");

    output21.fill(0xCCu);
    const bool acceptedOversized21Output = compressor.decompress21(
        compressed21.data(), output21.data(), compressed21.size(),
        onePastUInt32 + output21.size());
    t.expect(!acceptedOversized21Output
                 && compressor.decompressedBytes() == 0u
                 && std::all_of(output21.begin(), output21.end(),
                                [](std::uint8_t value) { return value == 0xCCu; }),
             "R2007 decompressor rejects output windows wider than its 32-bit cursor before writing output");
    t.expect(compressor.decompress21(compressed21.data(), output21.data(),
                                     compressed21.size(), output21.size())
                 && output21 == expected21,
             "R2007 decompressor accepts a valid retry after oversized-size refusal");
}

void testTypedAppDataNumericWireKinds(TestContext& t) {
    const std::list<DRW_Variant> group{
        {102, std::string("{APP_NUMERIC")}, {70, 7}, {280, 2}, {290, 1},
        {90, 70001}, {160, std::int64_t(1099511627777LL)}, {40, 1.25},
        {1, std::string("AFTER")}, {102, std::string("}")}};
    const std::vector<std::string> expected{
        "102:{APP_NUMERIC", "70:7", "280:2", "290:1", "90:70001",
        "160:1099511627777", "40:0x1.4p+0", "1:AFTER", "102:}",
        "0:WIRE_END"};
    for (const bool binary : {false, true}) {
        std::ostringstream output;
        dxfRW owner("");
        owner.version = DRW::AC1032;
        owner.binFile = binary;
        if (binary)
            owner.writer = std::make_unique<dxfWriterBinary>(&output);
        else
            owner.writer = std::make_unique<dxfWriterAscii>(&output);
        t.expect(owner.writeAppData({group})
                     && owner.writer->writeString(0, "WIRE_END"),
                 "typed application data numeric sentinel writes");
        bool ended = false;
        t.expect(numericWireProjection(output.str(), binary, ended) == expected
                     && ended,
                 "typed appData has complete ordered assigned-width tags");
    }

    // The existing I16 writer also accepts unsigned flag bit patterns. Test
    // their literal representations explicitly, not by normalizing every
    // signed semantic field in the general projection.
    for (const int value : {-32768, 32767, 32768, 65535}) {
        for (const bool binary : {false, true}) {
            std::ostringstream output;
            dxfRW owner("");
            owner.version = DRW::AC1032;
            owner.binFile = binary;
            if (binary)
                owner.writer = std::make_unique<dxfWriterBinary>(&output);
            else
                owner.writer = std::make_unique<dxfWriterAscii>(&output);
            const std::list<DRW_Variant> flags{
                {102, std::string("{APP_FLAGS")}, {70, value},
                {1, std::string("AFTER_FLAGS")}, {102, std::string("}")}};
            t.expect(owner.writeAppData({flags})
                         && owner.writer->writeString(0, "WIRE_END"),
                     "application I16 flag bit pattern remains representable");
            const int wireValue = binary && value > 32767 ? value - 65536 : value;
            bool ended = false;
            const std::vector<std::string> literal{
                "102:{APP_FLAGS", "70:" + std::to_string(wireValue),
                "1:AFTER_FLAGS", "102:}", "0:WIRE_END"};
            t.expect(numericWireProjection(output.str(), binary, ended) == literal
                         && ended,
                     "I16 flag encoding retains signed/unsigned wire bits exactly");
        }
    }

    const std::vector<DRW_Variant> invalid{
        {70, -32769}, {70, 65536}, {280, 65536}, {290, -1}, {290, 2},
        {160, 7}, {90, std::int64_t(70001)}, {70, std::string("7")},
        {40, 7}, {70, 1.25}, {0, std::string("LINE")},
        {-1, std::string("outside")}, {1072, std::string("outside")}};
    for (const auto& value : invalid) {
        for (const bool binary : {false, true}) {
            std::ostringstream output;
            dxfRW owner("");
            owner.version = DRW::AC1032;
            owner.binFile = binary;
            if (binary)
                owner.writer = std::make_unique<dxfWriterBinary>(&output);
            else
                owner.writer = std::make_unique<dxfWriterAscii>(&output);
            const std::list<DRW_Variant> bad{
                {102, std::string("{APP_BAD")}, value,
                {102, std::string("}")}};
            t.expect(!owner.writeAppData({group, bad}) && output.str().empty(),
                     "appData validates every kind/range before any group opener");
        }
    }
    for (const bool binary : {false, true}) {
        for (const bool legacy : {false, true}) {
            std::ostringstream output;
            dxfRW owner("");
            owner.setDxfCompatibilityProfile(legacy
                ? dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy
                : dxfRW::DxfCompatibilityProfile::StandaloneSafe);
            owner.version = DRW::AC1032;
            owner.binFile = binary;
            if (binary)
                owner.writer = std::make_unique<dxfWriterBinary>(&output);
            else
                owner.writer = std::make_unique<dxfWriterAscii>(&output);
            const int value = legacy ? 1 : 2;
            const int numericCode = legacy ? 260 : 270;
            const std::list<DRW_Variant> compatible{
                {102, std::string("{APP_PROFILE")}, {numericCode, value},
                {1, std::string("AFTER_PROFILE")}, {102, std::string("}")}};
            t.expect(owner.writeAppData({compatible})
                         && owner.writer->writeString(0, "WIRE_END"),
                     "typed appData retains explicit compatibility profile");
            bool ended = false;
            const std::vector<std::string> literal{
                "102:{APP_PROFILE", std::to_string(numericCode) + ":" + std::to_string(value),
                "1:AFTER_PROFILE", "102:}", "0:WIRE_END"};
            t.expect(numericWireProjection(output.str(), binary, ended,
                         legacy ? DxfClassifierProfile::LibreCadMasterLegacy
                                : DxfClassifierProfile::StandaloneSafe) == literal
                         && ended,
                     "profile-specific appData width preserves following tags");
        }
    }

    const std::list<DRW_Variant> unknownString{
        {102, std::string("{APP_UNKNOWN")},
        {482, std::string("opaque-extension-value")},
        {102, std::string("}")}};
    std::ostringstream asciiUnknownOutput;
    dxfRW asciiUnknownOwner("");
    asciiUnknownOwner.version = DRW::AC1032;
    asciiUnknownOwner.binFile = false;
    asciiUnknownOwner.writer =
        std::make_unique<dxfWriterAscii>(&asciiUnknownOutput);
    const bool asciiUnknownWritten =
        asciiUnknownOwner.writeAppData({unknownString})
        && asciiUnknownOwner.writer->writeString(0, "WIRE_END");
    const std::string expectedAsciiUnknown =
        "102\n{APP_UNKNOWN\n482\nopaque-extension-value\n102\n}\n"
        "  0\nWIRE_END\n";
    t.expect(asciiUnknownWritten
                 && asciiUnknownOutput.str() == expectedAsciiUnknown,
             "validated unknown ASCII appData string uses explicit raw replay and preserves the following record");

    std::ostringstream binaryUnknownOutput;
    dxfRW binaryUnknownOwner("");
    binaryUnknownOwner.version = DRW::AC1032;
    binaryUnknownOwner.binFile = true;
    binaryUnknownOwner.writer =
        std::make_unique<dxfWriterBinary>(&binaryUnknownOutput);
    t.expect(!binaryUnknownOwner.writeAppData({unknownString})
                 && binaryUnknownOutput.str().empty(),
             "unknown appData string remains refused for binary DXF before emission");
}

void testDynamicDimstyleNumericWireKinds(TestContext& t) {
    for (const int value : {std::numeric_limits<int>::min(), -32769, 0, 1,
                            65536, 70001, std::numeric_limits<int>::max()}) {
        expectTypedNumericWire(t, [value](dxfRW& writer) {
            DRW_Dimstyle style;
            style.name = "DYNAMIC_WIRE";
            style.add("WIRE90", 90, value);
            style.add("FOLLOWING_I16", 371, 25);
            style.add("FOLLOWING_BOOL", 295, 1);
            return writer.writeDimstyle(&style);
        }, {"90:" + std::to_string(value), "371:25", "295:1"},
           "dynamic DIMSTYLE I32 preserves values and following fields");
    }
    for (const int value : {-32768, 32767}) {
        expectTypedNumericWire(t, [value](dxfRW& writer) {
            DRW_Dimstyle style;
            style.name = "DYNAMIC_I16";
            style.add("WIRE371", 371, value);
            return writer.writeDimstyle(&style);
        }, {"371:" + std::to_string(value)},
           "dynamic DIMSTYLE I16 retains the signed range");
    }
    for (const int value : {-32769, 65536}) {
        expectTypedNumericRangeRejected(t, [value](dxfRW& writer) {
            DRW_Dimstyle style;
            style.add("WIRE371", 371, value);
            return writer.writeDimstyle(&style);
        });
    }
    for (const bool binary : {false, true}) {
        std::ostringstream output;
        dxfRW owner("");
        owner.version = DRW::AC1015; // Keep the existing R2007 group-90 gate.
        owner.binFile = binary;
        if (binary)
            owner.writer = std::make_unique<dxfWriterBinary>(&output);
        else
            owner.writer = std::make_unique<dxfWriterAscii>(&output);
        DRW_Dimstyle style;
        style.add("WIRE90", 90, 70001);
        t.expect(owner.writeDimstyle(&style)
                     && owner.writer->writeString(0, "WIRE_END"),
                 "pre-R2007 DIMSTYLE keeps existing inactive-variable policy");
        bool ended = false;
        const auto tags = numericWireProjection(output.str(), binary, ended);
        t.expect(ended && std::none_of(tags.begin(), tags.end(),
                     [](const auto& tag) { return tag.rfind("90:", 0) == 0; }),
                 "pre-R2007 dynamic DIMSTYLE does not emit group 90");
    }
    expectTypedNumericRangeRejected(t, [](dxfRW& writer) {
        DRW_Dimstyle style;
        style.add("WIRE90", 90, std::string("70001"));
        return writer.writeDimstyle(&style);
    });
}

void testDimensionUnassignedDirectGroups(TestContext& t) {
    struct DirectGroup {
        int code;
        const char* value;
    };
    constexpr DirectGroup groups[] = {
        {80, "180"}, {81, "181"}, {82, "182"}, {83, "183"}, {84, "184"}
    };
    class DimensionParserProbe final : public DRW_DimAligned {
    public:
        bool parseRecordCode(int code,
                             const std::unique_ptr<dxfReader>& reader) {
            return DRW_Dimension::parseCode(code, reader);
        }
    };

    for (const bool binary : {false, true}) {
        std::ostringstream encoded(std::ios::out | std::ios::binary);
        std::unique_ptr<dxfWriter> writer;
        const DxfClassifierProfile profile = binary
            ? DxfClassifierProfile::LibreCadMasterLegacy
            : DxfClassifierProfile::StandaloneSafe;
        if (binary)
            writer = std::make_unique<dxfWriterBinary>(&encoded);
        else
            writer = std::make_unique<dxfWriterAscii>(&encoded);
        writer->setClassifierProfile(profile);

        bool written = writer->writeInt16(71, 2);
        for (const DirectGroup& group : groups)
            written = (binary
                           ? writer->writeString(group.code, group.value)
                           : writer->writeRawString(group.code, group.value))
                && written;
        written = writer->writeString(0, "WIRE_END") && written;
        t.expect(written,
                 "generated DIMENSION raw/legacy controls serialize the unassigned tags");

        std::istringstream encodedInput(
            encoded.str(), std::ios::in | std::ios::binary);
        std::unique_ptr<dxfReader> reader;
        if (binary)
            reader = std::make_unique<dxfReaderBinary>(&encodedInput);
        else
            reader = std::make_unique<dxfReaderAscii>(&encodedInput);
        reader->setClassifierProfile(profile);

        DimensionParserProbe dimension;
        int code = -1;
        bool readAlign = reader->readRec(&code) && code == 71;
        if (readAlign)
            readAlign = dimension.parseRecordCode(code, reader)
                && dimension.align == 2;
        t.expect(readAlign,
                 "valid preceding DIMENSION integer establishes stale-cache control");

        bool directGroupsRead = true;
        for (const DirectGroup& group : groups) {
            const bool read = reader->readRec(&code) && code == group.code;
            t.expect(read,
                     "unassigned DIMENSION group is read with classified framing");
            if (!read) {
                directGroupsRead = false;
                break;
            }
            const bool stringValue = reader->getString() == group.value;
            const bool parsed = dimension.parseRecordCode(code, reader);
            directGroupsRead = directGroupsRead && stringValue && parsed;
        }
        t.expect(directGroupsRead,
                 "unassigned DIMENSION group strings follow the unknown-tag path");
        t.expect(dimension.tolZero == 0 && dimension.altTolDecimals == 0
                     && dimension.altZero == 0 && dimension.altTolZero == 0
                     && dimension.textMove == 0,
                 "unassigned string tags do not reuse stale integers as DSTYLE values");

        const bool sentinelRead = reader->readRec(&code) && code == 0
            && reader->getString() == "WIRE_END";
        t.expect(sentinelRead,
                 "following DIMENSION record sentinel remains aligned");
        t.expect(!reader->readRec(&code),
                 "generated DIMENSION control ends after its sentinel");
    }
}

void testDimensionDstyleWriterMappings(TestContext& t) {
    using Setter = void (*)(DRW_Dimension&);
    struct Field {
        int formerGroupCode;
        int variable;
        bool realValue;
        Setter set;
    };
    const Field fields[] = {
        {43, 47, true, [](DRW_Dimension& d) { d.tolPlus = 1.0; }},
        {44, 48, true, [](DRW_Dimension& d) { d.tolMinus = 1.0; }},
        {76, 71, false, [](DRW_Dimension& d) { d.genTol = true; }},
        {77, 72, false, [](DRW_Dimension& d) { d.limGen = true; }},
        {45, 146, true, [](DRW_Dimension& d) { d.tolScale = 1.0; }},
        {78, 272, false, [](DRW_Dimension& d) { d.tolDecimals = 1; }},
        {81, 274, false, [](DRW_Dimension& d) { d.altTolDecimals = 1; }},
        {84, 279, false, [](DRW_Dimension& d) { d.textMove = 1; }},
        {79, 283, false, [](DRW_Dimension& d) { d.tolAlign = 1; }},
        {80, 284, false, [](DRW_Dimension& d) { d.tolZero = 1; }},
        {82, 285, false, [](DRW_Dimension& d) { d.altZero = 1; }},
        {83, 286, false, [](DRW_Dimension& d) { d.altTolZero = 1; }}
    };

    for (const bool binary : {false, true}) {
        for (const Field& field : fields) {
            std::ostringstream output;
            dxfRW owner("");
            owner.version = DRW::AC1032;
            owner.binFile = binary;
            owner.writer = binary
                ? std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterBinary>(&output))
                : std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterAscii>(&output));

            DRW_DimAligned dimension;
            dimension.handle = 0x550u;
            field.set(dimension);
            t.expect(owner.writeDimension(&dimension)
                         && owner.writer->writeString(0, "WIRE_END"),
                     "typed DIMENSION override writes as ACAD/DSTYLE");
            bool ended = false;
            const auto tags = numericWireProjection(output.str(), binary, ended);
            const std::vector<std::string> expected = {
                "1001:ACAD", "1000:DSTYLE", "1002:{",
                "1070:" + std::to_string(field.variable),
                field.realValue ? "1040:0x1p+0" : "1070:1", "1002:}"
            };
            t.expect(ended
                         && std::search(tags.begin(), tags.end(),
                                        expected.begin(), expected.end())
                            != tags.end(),
                     "DIMENSION override emits its canonical variable and typed value");
            const std::set<int> obsoleteGroups = {
                43, 44, 45, 76, 77, 78, 79, 80, 81, 82, 83, 84
            };
            t.expect(ended
                         && std::none_of(tags.begin(), tags.end(),
                             [&obsoleteGroups](const std::string& tag) {
                                 const auto colon = tag.find(':');
                                 if (colon == std::string::npos)
                                     return false;
                                 return obsoleteGroups.count(
                                     std::stoi(tag.substr(0, colon))) != 0u;
                             }),
                     "canonical DSTYLE output omits noncanonical direct groups");
        }
    }

    std::ostringstream mergedOutput;
    dxfRW mergedOwner("");
    mergedOwner.version = DRW::AC1032;
    mergedOwner.writer = std::make_unique<dxfWriterAscii>(&mergedOutput);
    DRW_DimAligned merged;
    merged.handle = 0x551u;
    merged.tolPlus = 0.75;
    merged.extData = {
        std::make_shared<DRW_Variant>(1001, std::string("ACAD")),
        std::make_shared<DRW_Variant>(1000, std::string("DSTYLE")),
        std::make_shared<DRW_Variant>(1002, std::string("{")),
        std::make_shared<DRW_Variant>(1070, std::int32_t(999)),
        std::make_shared<DRW_Variant>(1040, 42.0),
        std::make_shared<DRW_Variant>(1070, std::int32_t(47)),
        std::make_shared<DRW_Variant>(1040, 0.25),
        std::make_shared<DRW_Variant>(1070, std::int32_t(285)),
        std::make_shared<DRW_Variant>(1070, std::int32_t(0)),
        std::make_shared<DRW_Variant>(1002, std::string("}"))
    };
    const bool mergedWritten = mergedOwner.writeDimension(&merged)
        && mergedOwner.writer->writeString(0, "WIRE_END");
    bool mergedEnded = false;
    const auto mergedTags = numericWireProjection(
        mergedOutput.str(), false, mergedEnded);
    const std::vector<std::string> mergedExpected = {
        "1001:ACAD", "1000:DSTYLE", "1002:{", "1070:999",
        "1040:0x1.5p+5", "1070:47", "1040:0x1.8p-1",
        "1070:285", "1070:0", "1002:}"
    };
    t.expect(mergedWritten && mergedEnded
                 && std::search(mergedTags.begin(), mergedTags.end(),
                                mergedExpected.begin(), mergedExpected.end())
                    != mergedTags.end(),
             "DSTYLE writer updates typed override and preserves zero/unknown raw pairs");

    std::ostringstream acadMergeOutput;
    dxfRW acadMergeOwner("");
    acadMergeOwner.version = DRW::AC1032;
    acadMergeOwner.writer = std::make_unique<dxfWriterAscii>(&acadMergeOutput);
    DRW_DimAligned acadMerge;
    acadMerge.handle = 0x554u;
    acadMerge.tolMinus = 0.5;
    acadMerge.extData = {
        std::make_shared<DRW_Variant>(1001, std::string("ACAD")),
        std::make_shared<DRW_Variant>(1000, std::string("OTHER_ACAD_DATA"))
    };
    const bool acadMergeWritten = acadMergeOwner.writeDimension(&acadMerge)
        && acadMergeOwner.writer->writeString(0, "WIRE_END");
    bool acadMergeEnded = false;
    const auto acadMergeTags = numericWireProjection(
        acadMergeOutput.str(), false, acadMergeEnded);
    const std::vector<std::string> acadMergeExpected = {
        "1001:ACAD", "1000:OTHER_ACAD_DATA", "1000:DSTYLE", "1002:{",
        "1070:48", "1040:0x1p-1", "1002:}"
    };
    t.expect(acadMergeWritten && acadMergeEnded
                 && std::search(acadMergeTags.begin(), acadMergeTags.end(),
                                acadMergeExpected.begin(), acadMergeExpected.end())
                    != acadMergeTags.end(),
             "new DSTYLE subsection merges into an existing ACAD app group");

    std::ostringstream malformedOutput;
    const std::string prefix = "PRESERVE_DIMENSION_OUTPUT";
    malformedOutput << prefix;
    dxfRW malformedOwner("");
    malformedOwner.version = DRW::AC1032;
    malformedOwner.writer = std::make_unique<dxfWriterAscii>(&malformedOutput);
    DRW_DimAligned malformed;
    malformed.handle = 0x552u;
    malformed.tolPlus = 1.0;
    malformed.extData = {
        std::make_shared<DRW_Variant>(1001, std::string("ACAD")),
        std::make_shared<DRW_Variant>(1000, std::string("DSTYLE")),
        std::make_shared<DRW_Variant>(1002, std::string("{")),
        std::make_shared<DRW_Variant>(1070, std::int32_t(47)),
        std::make_shared<DRW_Variant>(1070, std::int32_t(2)),
        std::make_shared<DRW_Variant>(1002, std::string("}"))
    };
    t.expect(!malformedOwner.writeDimension(&malformed)
                 && malformedOwner.m_writeError
                 && malformedOutput.str() == prefix,
             "malformed retained DSTYLE is refused before entity output");

    const Setter invalidValues[] = {
        [](DRW_Dimension& d) { d.tolPlus = -0.25; },
        [](DRW_Dimension& d) { d.tolDecimals = 9; }
    };
    for (const bool binary : {false, true}) {
        for (const Setter setInvalid : invalidValues) {
            std::ostringstream invalidOutput;
            invalidOutput << prefix;
            dxfRW invalidOwner("");
            invalidOwner.version = DRW::AC1032;
            invalidOwner.writer = binary
                ? std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterBinary>(&invalidOutput))
                : std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterAscii>(&invalidOutput));
            DRW_DimAligned invalid;
            invalid.handle = 0x553u;
            setInvalid(invalid);
            t.expect(!invalidOwner.writeDimension(&invalid)
                         && invalidOwner.m_writeError
                         && invalidOutput.str() == prefix,
                     "out-of-range typed DSTYLE value is refused before output");
        }
    }
}

void testDimensionPictureBlockStrictNameAdmission(TestContext& t) {
    // Empty styles isolate picture-BLOCK checks from strict DIMSTYLE lookup.
    const auto makeDimension = [](int kind) -> std::unique_ptr<DRW_Dimension> {
        std::unique_ptr<DRW_Dimension> dimension;
        if (kind == 1)
            dimension = std::make_unique<DRW_DimArc>();
        else if (kind == 2)
            dimension = std::make_unique<DRW_DimLargeRadial>();
        else
            dimension = std::make_unique<DRW_DimAligned>();
        dimension->setStyle("");
        return dimension;
    };
    class PictureBlockInterface final : public dx_iface {
    public:
        PictureBlockInterface() {
            cData = &storage;
            currentBlock = storage.mBlock;
        }

        void bind(dxfRW& owner) { dxfW = &owner; }

        void addGeometryBlock(const std::string& name) {
            auto block = std::make_unique<dx_ifaceBlock>();
            block->name = name;
            block->handle = nextHandle++;
            auto line = std::make_unique<DRW_Line>();
            line->handle = nextHandle++;
            line->basePoint = DRW_Coord(1.0, 2.0, 0.0);
            line->secPoint = DRW_Coord(3.0, 4.0, 0.0);
            block->ent.push_back(line.release());
            storage.blocks.push_back(block.release());
        }

        void addDimensionBlock(const std::string& ownerName,
                               const std::string& pictureName) {
            auto block = std::make_unique<dx_ifaceBlock>();
            block->name = ownerName;
            block->handle = nextHandle++;
            auto dimension = std::make_unique<DRW_DimAligned>();
            dimension->handle = nextHandle++;
            dimension->setStyle("");
            dimension->setName(pictureName);
            block->ent.push_back(dimension.release());
            storage.blocks.push_back(block.release());
        }

        dx_data storage;

    private:
        std::uint32_t nextHandle {0x5A0u};
    };
    const std::string prefix = "KEEP_DIMENSION_PREFIX";

    for (const bool binary : {false, true}) {
        for (int kind = 0; kind != 3; ++kind) {
            std::ostringstream output;
            output << prefix;
            dxfRW strictOwner("");
            strictOwner.version = DRW::AC1032;
            strictOwner.writer = binary
                ? std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterBinary>(&output))
                : std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterAscii>(&output));
            strictOwner.setSourceBlockRecords({}, true);
            auto dimension = makeDimension(kind);
            dimension->handle = 0x580u + static_cast<std::uint32_t>(kind);
            dimension->setName("*D_MISSING");
            t.expect(!strictOwner.writeDimension(dimension.get())
                         && strictOwner.m_writeError
                         && strictOwner.getLastDiagnostic().code
                             == "unresolved-dimension-picture-block"
                         && output.str() == prefix,
                     "strict DXF conversion rejects a missing named picture BLOCK_RECORD before output");
        }

        std::ostringstream registeredOutput;
        dxfRW registeredOwner("");
        registeredOwner.version = DRW::AC1032;
        registeredOwner.writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&registeredOutput))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&registeredOutput));
        registeredOwner.setSourceBlockRecords({}, true);
        const bool blockRecordPlanned =
            registeredOwner.writeBlockRecord("*D_REGISTERED");
        DRW_DimAligned registered;
        registered.handle = 0x584u;
        registered.setName("*D_REGISTERED");
        t.expect(blockRecordPlanned
                     && registeredOwner.writeDimension(&registered),
                 "strict DXF conversion admits a picture name present in the planned BLOCK_RECORD index");

        std::ostringstream legacyOutput;
        dxfRW legacyOwner("");
        legacyOwner.version = DRW::AC1032;
        legacyOwner.writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&legacyOutput))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&legacyOutput));
        DRW_DimAligned legacy;
        legacy.handle = 0x585u;
        legacy.setName("*D_UNRESOLVED_LEGACY");
        t.expect(legacyOwner.writeDimension(&legacy),
                 "standalone DXF writer keeps legacy behavior for unresolved picture names");

        std::ostringstream emptyNameOutput;
        dxfRW emptyNameOwner("");
        emptyNameOwner.version = DRW::AC1032;
        emptyNameOwner.writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&emptyNameOutput))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&emptyNameOutput));
        emptyNameOwner.setSourceBlockRecords({}, true);
        DRW_DimAligned emptyName;
        emptyName.handle = 0x586u;
        t.expect(emptyNameOwner.writeDimension(&emptyName),
                 "strict DXF conversion leaves empty-name dimensions unclassified rather than assuming an ordinary picture block");

        const auto configureOwner = [binary](dxfRW& owner,
                                              std::ostringstream& output,
                                              PictureBlockInterface& adapter) {
            owner.version = DRW::AC1032;
            owner.writer = binary
                ? std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterBinary>(&output))
                : std::unique_ptr<dxfWriter>(
                      std::make_unique<dxfWriterAscii>(&output));
            owner.setSourceBlockRecords({}, true);
            owner.iface = &adapter;
            adapter.bind(owner);
        };

        for (int kind = 0; kind != 3; ++kind) {
            std::ostringstream missingBodyOutput;
            missingBodyOutput << prefix;
            PictureBlockInterface missingBodyAdapter;
            dxfRW missingBodyOwner("");
            configureOwner(missingBodyOwner, missingBodyOutput,
                           missingBodyAdapter);
            const bool missingBodyPlanned =
                missingBodyOwner.writeBlockRecord("*D_NO_BODY");
            const bool emptyBlockSection = missingBodyOwner.writeBlocks();
            const std::string beforeMissingDimension =
                missingBodyOutput.str();
            auto missingBodyDimension = makeDimension(kind);
            missingBodyDimension->handle =
                0x590u + static_cast<std::uint32_t>(kind);
            missingBodyDimension->setName("*D_NO_BODY");
            const bool missingBodyRejected =
                !missingBodyOwner.writeDimension(missingBodyDimension.get());
            t.expect(missingBodyPlanned && emptyBlockSection
                         && missingBodyRejected
                         && missingBodyOwner.getLastDiagnostic().code
                             == "incomplete-dimension-picture-block"
                         && missingBodyOutput.str() == beforeMissingDimension,
                     "strict conversion refuses a planned DIMENSION picture name without an emitted body for every supported dimension writer");
        }

        for (int kind = 0; kind != 3; ++kind) {
            std::ostringstream completeBodyOutput;
            completeBodyOutput << prefix;
            PictureBlockInterface completeBodyAdapter;
            completeBodyAdapter.addGeometryBlock("*D_COMPLETE");
            dxfRW completeBodyOwner("");
            configureOwner(completeBodyOwner, completeBodyOutput,
                           completeBodyAdapter);
            const bool completeBodyPlanned =
                completeBodyOwner.writeBlockRecord("*D_COMPLETE");
            const bool completeBlockSection = completeBodyOwner.writeBlocks();
            auto completeBodyDimension = makeDimension(kind);
            completeBodyDimension->handle =
                0x593u + static_cast<std::uint32_t>(kind);
            completeBodyDimension->setName("*D_COMPLETE");
            const bool completeDimensionWritten =
                completeBodyOwner.writeDimension(completeBodyDimension.get());
            t.expect(completeBodyPlanned && completeBlockSection
                         && completeBodyOwner.m_emittedDxfBlockBodyNames.count(
                                "*D_COMPLETE") == 1u
                         && completeDimensionWritten,
                     "strict conversion admits each DIMENSION writer when its planned picture BLOCK body was emitted and closed");
        }

        std::ostringstream forwardOutput;
        forwardOutput << prefix;
        PictureBlockInterface forwardAdapter;
        forwardAdapter.addDimensionBlock("*D_OWNER", "*D_FORWARD");
        forwardAdapter.addGeometryBlock("*D_FORWARD");
        dxfRW forwardOwner("");
        configureOwner(forwardOwner, forwardOutput, forwardAdapter);
        const bool forwardRecordsPlanned =
            forwardOwner.writeBlockRecord("*D_OWNER")
            && forwardOwner.writeBlockRecord("*D_FORWARD");
        const bool forwardBlockSection = forwardOwner.writeBlocks();
        DRW_DimAligned forwardModelDimension;
        forwardModelDimension.handle = 0x592u;
        forwardModelDimension.setStyle("");
        forwardModelDimension.setName("*D_FORWARD");
        const bool forwardDimensionWritten =
            forwardOwner.writeDimension(&forwardModelDimension);
        t.expect(forwardRecordsPlanned && forwardBlockSection
                     && forwardOwner.m_emittedDxfBlockBodyNames.count(
                            "*D_OWNER") == 1u
                     && forwardOwner.m_emittedDxfBlockBodyNames.count(
                            "*D_FORWARD") == 1u
                     && forwardDimensionWritten,
                 "strict conversion defers a forward picture reference inside BLOCKS until its later body is emitted");

        std::ostringstream unresolvedForwardOutput;
        unresolvedForwardOutput << prefix;
        PictureBlockInterface unresolvedForwardAdapter;
        unresolvedForwardAdapter.addDimensionBlock("*D_OWNER", "*D_ABSENT");
        dxfRW unresolvedForwardOwner("");
        configureOwner(unresolvedForwardOwner, unresolvedForwardOutput,
                       unresolvedForwardAdapter);
        const bool unresolvedForwardPlanned =
            unresolvedForwardOwner.writeBlockRecord("*D_OWNER")
            && unresolvedForwardOwner.writeBlockRecord("*D_ABSENT");
        const std::string beforeUnresolvedBlocks = unresolvedForwardOutput.str();
        const bool unresolvedForwardRejected =
            !unresolvedForwardOwner.writeBlocks();
        t.expect(unresolvedForwardPlanned
                     && unresolvedForwardRejected
                     && unresolvedForwardOwner.getLastDiagnostic().code
                         == "incomplete-dimension-picture-block"
                     && unresolvedForwardOutput.str() == beforeUnresolvedBlocks
                     && unresolvedForwardOwner.m_emittedDxfBlockBodyNames.empty()
                     && unresolvedForwardOwner.m_pendingDxfDimensionPictureBlocks.empty(),
                 "strict conversion atomically rejects a BLOCKS forward picture reference whose target body is absent");
    }
}

void testHandlesAndHeader(TestContext& t) {
    HandleAllocator allocator;
    allocator.seedReserved();
    t.expect(allocator.next() == 0x30u, "handle allocator first generated handle");
    allocator.reserve(0x31u);
    t.expect(allocator.next() == 0x32u, "handle allocator skips reservation");
    t.expect(allocator.current() == 0x33u, "handle allocator high-water mark");

    DRW_Header header;
    header.setHandSeed(0x12345678u);
    dwgBufferW data;
    t.expect(DwgHandseedTestAccess::encode(header, DRW::AC1015, &data, &data),
             "DWG header encoder accepts R2000 defaults");
    t.expect(!data.data().empty(), "DWG header encoder emits bytes");
    t.expect(header.dwgHandseedBitOffset() != DRW_Header::kInvalidDwgHandseedBitOffset,
             "DWG header records HANDSEED patch offset");
    t.expect(header.dwgHandseedBitOffset() < data.bitCount(),
             "DWG HANDSEED patch offset is within output");

    RScodec invalidField(0x96, 0, 8);
    t.expect(!invalidField.isOkey() && invalidField.decode(nullptr) == -1,
             "RS codec invalid configuration fails closed");
}

void testHeaderMaintenanceWidths(TestContext& t) {
    class HeaderProbe final : public DRW_Header {
    public:
        using DRW_Header::parseCode;
    };
    struct Control {
        const char* version;
        DRW::Version target;
        int code;
        int value;
        bool present;
    };
    const Control controls[] = {
        {"AC1032", DRW::AC1032, 90, 70001, true},
        {"AC1032", DRW::AC1032, 90, 0, true},
        {"AC1032", DRW::AC1032, 90, 2147483647, true},
        {"AC1032", DRW::AC1032, 90, 0, false},
        {"AC1027", DRW::AC1027, 70, 12345, true},
        {"AC1027", DRW::AC1027, 70, 0, true},
        {"AC1027", DRW::AC1027, 70, 0, false},
    };
    for (bool binary : {false, true}) {
        for (const Control& control : controls) {
            std::ostringstream source;
            std::unique_ptr<dxfWriter> sourceWriter;
            if (binary)
                sourceWriter = std::make_unique<dxfWriterBinary>(&source);
            else
                sourceWriter = std::make_unique<dxfWriterAscii>(&source);
            sourceWriter->setVersion(control.version, true);
            sourceWriter->writeString(9, "$ACADVER");
            sourceWriter->writeString(1, control.version);
            if (control.present) {
                sourceWriter->writeString(9, "$ACADMAINTVER");
                if (control.code == 90)
                    sourceWriter->writeInt32(90, control.value);
                else
                    sourceWriter->writeInt16(70, control.value);
            }
            t.expect(!sourceWriter->hasWriteError(),
                     "literal header-width control encodes");
            std::istringstream input(source.str());
            std::unique_ptr<dxfReader> reader;
            if (binary)
                reader = std::make_unique<dxfReaderBinary>(&input);
            else
                reader = std::make_unique<dxfReaderAscii>(&input);
            HeaderProbe parsed;
            int code = 0;
            while (reader->readRec(&code))
                t.expect(parsed.parseCode(code, reader),
                         "header-width control parses");
            const auto found = parsed.vars.find("$ACADMAINTVER");
            if (control.present) {
                t.expect(found != parsed.vars.end() && found->second != nullptr
                             && found->second->type() == DRW_Variant::INTEGER
                             && found->second->code() == control.code
                             && found->second->i_val() == control.value,
                         "header retains source integer width and full value");
            } else {
                t.expect(found == parsed.vars.end(),
                         "absent maintenance variable stays absent on read");
            }
            DRW_Header copied(parsed);
            const auto copiedValue = copied.vars.find("$ACADMAINTVER");
            if (control.present)
                t.expect(copiedValue != copied.vars.end()
                             && copiedValue->second != found->second
                             && copiedValue->second->i_val() == control.value,
                         "header copy owns an independent full-width value");

            std::ostringstream destination;
            std::unique_ptr<dxfWriter> writer;
            if (binary)
                writer = std::make_unique<dxfWriterBinary>(&destination);
            else
                writer = std::make_unique<dxfWriterAscii>(&destination);
            copied.write(writer, control.target);
            t.expect(!writer->hasWriteError(),
                     "maintenance header output is representable");
            std::istringstream output(destination.str());
            if (binary)
                reader = std::make_unique<dxfReaderBinary>(&output);
            else
                reader = std::make_unique<dxfReaderAscii>(&output);
            std::string name;
            bool maintenanceSeen = false;
            while (reader->readRec(&code)) {
                if (code == 9)
                    name = reader->getString();
                else if (name == "$ACADMAINTVER") {
                    maintenanceSeen = true;
                    t.expect(code == control.code
                                 && reader->getInt32() == control.value,
                             "serialized maintenance code/value matches target width");
                }
            }
            t.expect(maintenanceSeen,
                     "writer retains existing zero default for absent maintenance");
            static_cast<DRW_Header&>(parsed) = DRW_Header{};
            t.expect(parsed.vars.count("$ACADMAINTVER") == 0,
                     "reset header assignment clears prior integer state");
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 2
        && std::string(argv[1]) == "--sectionobject-indicator-color-wire") {
        TestContext focused;
        testSectionObjectIndicatorColorWire(focused);
        std::cout << "SECTIONOBJECT indicator-color wire: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--dwg-decompressor-size-bounds") {
        TestContext focused;
        testDecompressor(focused);
        std::cout << "DWG decompressor size bounds: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--acad-field-writer-candidate") {
        TestContext focused;
        testAcadFieldExtensionDictionaryWireCandidate(focused);
        testFieldDxfVersionFloor(focused);
        testFieldCadValueEmissionPreflight(focused);
        std::cout << "ACAD_FIELD extension-dictionary writer candidate: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--solid-nondefault-ocs") {
        TestContext focused;
        testSolidNonDefaultOcsTransform(focused);
        std::cout << "SOLID non-default OCS transform: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--field-cadvalue-emission-preflight") {
        TestContext focused;
        testFieldCadValueEmissionPreflight(focused);
        std::cout << "FIELD CADVALUE emission preflight: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--typed-appdata-wire-kinds") {
        TestContext focused;
        testTypedAppDataNumericWireKinds(focused);
        std::cout << "Typed appData wire kinds: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--dxf-code-type-boundaries") {
        TestContext focused;
        testTextAndPreR13(focused);
        testDxfClassifierBoundaryMatrix(focused);
        testDxfBinaryWriterTypeGuard(focused);
        testDxfAsciiWriterTypeGuard(focused);
        testDimensionUnassignedDirectGroups(focused);
        testDimensionDstyleWriterMappings(focused);
        testDimensionPictureBlockStrictNameAdmission(focused);
        testDxfClassifierProfileProbe(focused);
        testDxfProfileMatrix(focused);
        std::cout << "DXF code-type boundaries and writer guards: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--dimension-unassigned-direct-groups") {
        TestContext focused;
        testDimensionUnassignedDirectGroups(focused);
        std::cout << "DIMENSION unassigned direct groups: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--common-appdata-base-scope") {
        TestContext focused;
        testCommonAppDataBaseScope(focused);
        testTypedObjectCommonMetadata(focused);
        std::cout << "Common application-data base scope: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--integer-valued-boolean-wire-kinds") {
        TestContext focused;
        testIntegerValuedBooleanWireKinds(focused);
        std::cout << "Integer-valued DXF field wire kinds: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--associative-dependency-writer") {
        TestContext focused;
        testAssociativeDependencyWriter(focused);
        std::cout << "AC1024 associative dependency writer: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--dimassoc-graph-closure") {
        TestContext focused;
        testDimensionAssociationGraphClosure(focused);
        std::cout << "DIMASSOC graph closure: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--hatch-gradient-dxf-grammar") {
        TestContext focused;
        testHatchGradientDxfGrammar(focused);
        std::cout << "HATCH gradient DXF grammar: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--mline-style-dxf-defaults") {
        TestContext focused;testMLineStyleDxfDefaults(focused);return focused.failures==0?0:1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--mline-style-validation") {
        TestContext focused;
        testMlineStyleColorAdmission(focused);
        testMlineStyleDxfSchemaAdmission(focused);
        testMLineStyleReferenceHandleRemapping(focused);
        std::cout << "MLINESTYLE validation: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--mleader-style-validation") {
        TestContext focused;
        testMLeaderStyleVersionAndHandleRemapping(focused);
        std::cout << "MLEADERSTYLE validation: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--mleader-writer-validation") {
        TestContext focused;
        testMLeaderWriterMinimumVersion(focused);
        testMLeaderDxfCmcColorLossGuard(focused);
        testMLeaderStyleDxfCmcColorLossGuard(focused);
        testMLeaderWriterReferenceHandleRemapping(focused);
        testMLeaderStyleEnumDomains(focused);
        std::cout << "MLEADER writer validation: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--sun-cmc-dxf-projection") {
        TestContext focused;
        testSunDxfCmcProjectionGuard(focused);
        std::cout << "SUN CMC DXF projection guard: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--section-settings-cmc-dxf-projection") {
        TestContext focused;
        testSectionSettingsDxfCmcProjectionGuard(focused);
        std::cout << "SECTION_SETTINGS CMC DXF projection guard: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--geodata-versioned-writer") {
        TestContext focused;
        testGeoDataVersionedWriter(focused);
        std::cout << "GEODATA versioned writer: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    TestContext context;
    if (argc == 2 && std::string(argv[1]) == "--dxf-objects-write-inspection") {
        testDxfObjectsWriteInspection(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--standard-object-wire-comparison") {
        testStandardObjectWireComparison(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--dxf-standard-object-identities") {
        testDxfStandardObjectIdentities(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--generated-dictionary-source-metadata") {
        testGeneratedDictionarySourceMetadata(context);
        return context.failures == 0 ? 0 : 1;
    }
    testBufferRoundTrip(context);
    testDxfObjectsWriteInspection(context);
    testStandardObjectWireComparison(context);
    testTextAndPreR13(context);
    testDxfClassifierBoundaryMatrix(context);
    testDxfBinaryWriterTypeGuard(context);
    testDxfAsciiWriterTypeGuard(context);
    testDimensionUnassignedDirectGroups(context);
    testDimensionDstyleWriterMappings(context);
    testDimensionPictureBlockStrictNameAdmission(context);
    testDxfClassifierProfileProbe(context);
    testDxfBinaryLegacyProfileReplay(context);
    testDxfFacadeClassifierProfile(context);
    testDxfProfilePromotionPolicy(context);
    testDxfProfileMatrix(context);
    testDxfFacadeProfileReplayAgreement(context);
    testDxfProfileDiagnostics(context);
    testDxfRawBoundaryReplay(context);
    testDxfRawSectionBoundaryReplay(context);
    testDxfRawSectionCaptureReplay(context);
    testDxfAcdsDataOpaqueSectionCaptureReplay(context);
    testDxfBinaryRawBoundaryReplay(context);
    testViewFlagWireKinds(context);
    testIntegerValuedBooleanWireKinds(context);
    testAssociativeDependencyWriter(context);
    testDimensionAssociationGraphClosure(context);
    testCustomTextNumericWireKinds(context);
    testLayoutStyleNumericWireKinds(context);
    testEntityNumericWireKinds(context);
    testImageObjectNumericWireKinds(context);
    testCommonAppDataBaseScope(context);
    testTypedObjectCommonMetadata(context);
    testMlineStyleColorAdmission(context);
    testMLineStyleDxfDefaults(context);
    testMlineStyleDxfSchemaAdmission(context);
    testMLineStyleReferenceHandleRemapping(context);
    testMLeaderStyleVersionAndHandleRemapping(context);
    testMLeaderWriterMinimumVersion(context);
    testSunDxfCmcProjectionGuard(context);
    testMLeaderDxfCmcColorLossGuard(context);
    testMLeaderStyleDxfCmcColorLossGuard(context);
    testMLeaderWriterReferenceHandleRemapping(context);
    testFieldAndFieldListHandleRemapping(context);
    testAcadFieldExtensionDictionaryWireCandidate(context);
    testFieldDxfVersionedWireProfiles(context);
    testFieldDxfVersionFloor(context);
    testFieldCadValueEmissionPreflight(context);
    testDxfWriterEmittedHandleInventory(context);
    testDxfWriterControlCharacterFraming(context);
    testGeoDataVersionedWriter(context);
    testGeneratedDictionaryGroupCommonMetadata(context);
    testDxfBinaryRawSectionCaptureReplay(context);
    testDxfBinaryRawObjectCaptureReplay(context);
    testDxfRawObjectHandleScope(context);
    testDxfRawObjectHandleDiagnostics(context);
    testDxfRawObjectMalformedHandleDiagnostics(context);
    testDxfRawEntityHandleDiagnostics(context);
    testDxfRawEntityWideHandleReplay(context);
    testDxfRawEntityHandleRemap(context);
    testDxfRawEntityApplicationGroupRemap(context);
    testDxfRawEntityApplicationGroupDepth(context);
    testDxfRawEntityApplicationGroupAggregate(context);
    testDxfRawEntityApplicationGroupMarker(context);
    testDxfRawEntityApplicationGroupReferenceMatrix(context);
    testDxfRawEntityApplicationGroupBinaryChunks(context);
    testDxfRawEntityApplicationGroupChunkSize(context);
    testDxfRawEntityApplicationGroupChunkCodeMatrix(context);
    testDxfRawEntityApplicationGroupRawValueCardinality(context);
    testDxfRawEntityApplicationGroupSourceSpelling(context);
    testDxfRawEntityApplicationGroupRemapChain(context);
    testDxfRawSectionApplicationGroupRemap(context);
    testDxfRawSectionApplicationGroupSourceSpelling(context);
    testDxfRawSectionApplicationGroupRawValueCardinality(context);
    testDxfRawSectionApplicationGroupRemapChain(context);
    testDxfRawSectionApplicationGroupBinaryChunks(context);
    testDxfRawSectionApplicationGroupChunkCodeMatrix(context);
    testDxfRawSectionApplicationGroupChunkSize(context);
    testDxfRawSectionHandleDiagnostics(context);
    testDxfRawSectionHandleScope(context);
    testDxfRawSectionWideHandleReplay(context);
    testDxfRawSectionWideHandleRemap(context);
    testDxfRawSectionRemapRollback(context);
    testDxfRawSectionReservedNames(context);
    testDxfRawSectionCustomFraming(context);
    testDxfRawSectionVersionCompatibility(context);
    testDxfRawSectionEmptyPayload(context);
    testDxfRawSectionCommentPreservation(context);
    testDxfRawSectionCommentReadPolicy(context);
    testDxfRawSectionCaseInsensitiveEndsec(context);
    testDxfRawSectionCaseInsensitiveSection(context);
    testDxfRawSectionCaseInsensitiveEof(context);
    testDxfRawSectionFinalEofWithoutNewline(context);
    testDxfRawSectionMissingEof(context);
    testDxfRawSectionMissingEndsec(context);
    testDxfRawSectionEmptyNameRead(context);
    testDxfRawSectionRecordBoundaries(context);
    testDxfRawSectionRecordBoundaryReplay(context);
    testDxfRawSectionEndsecTerminator(context);
    testDxfRawSectionGroupCodeBounds(context);
    testDxfRawSectionAggregateLimit(context);
    testDxfRawSectionDepthLimit(context);
    testDxfRawSectionApplicationGroupValidMarker(context);
    testDxfRawSectionApplicationGroupMarker(context);
    testDxfRawSectionWriterPreflight(context);
    testDxfRawSectionWriterStickyError(context);
    testDxfOperationShortWriteFailure(context);
    testDxfRawSectionAppendFailure(context);
    testDxfRawObjectAppendFailure(context);
    testDxfRawObjectWriterPreflight(context);
    testDxfRawObjectVersionCompatibility(context);
    testDxfRawObjectEmptyPayload(context);
    testDxfRawObjectAggregateLimit(context);
    testDxfRawObjectApplicationGroupDepth(context);
    testDxfRawObjectApplicationGroupMarker(context);
    testDxfRawObjectApplicationGroupReferenceMatrix(context);
    testDxfRawObjectApplicationGroupChunkCodeMatrix(context);
    testDxfRawObjectApplicationGroupChunkSize(context);
    testDxfRawObjectRawValueCardinality(context);
    testDxfRawObjectRemapChain(context);
    testDxfRawObjectWideHandleReplay(context);
    testDxfRawObjectMalformedHandle(context);
    testDxfRawSectionApplicationGroupReferenceMatrix(context);
    testDxfArbitraryHandlePayloads(context);
    testRawCapture(context);
    testHatchValidationIgnoresInactiveGradient(context);
    testHatchGradientDxfGrammar(context);
    testFixedSpaceBlockClassification(context);
    testR2007StringFooterBounds(context);
    testDecompressor(context);
    testHandlesAndHeader(context);
    testHeaderMaintenanceWidths(context);
    testDynamicDimstyleNumericWireKinds(context);
    testTypedAppDataNumericWireKinds(context);
    if (context.failures != 0) {
        std::cerr << context.failures << " Wave 1 assertion(s) failed\n";
        return 1;
    }
    std::cout << "Wave 1 tests: PASS\n";
    return 0;
}
