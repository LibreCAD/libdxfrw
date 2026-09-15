#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <string>
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
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"
#include "intern/rscodec.h"

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
    t.expect(dxfValueKindForCode(260) == DxfValueKind::I32,
             "DXF code 260 uses a preserving integer kind");
    t.expect(dxfValueKindForCode(269) == DxfValueKind::I32,
             "DXF code 269 uses a preserving integer kind");
    t.expect(dxfValueKindForCode(482) == DxfValueKind::Unknown
                 && dxfValueKindForCode(998) == DxfValueKind::Unknown,
             "DXF unassigned 482-998 span is unknown");
    t.expect(dxfValueKindForCode(999) == DxfValueKind::Str
                 && dxfValueKindForCode(1004) == DxfValueKind::Bin,
             "DXF comment and binary chunk boundaries are explicit");
    std::stringstream records("260\n2147483647\n482\n3.14\n999\ncomment\n1004\nAB\n1071\n-7\n");
    dxfReaderAscii reader(&records);
    int code = 0;
    t.expect(reader.readRec(&code) && code == 260
                 && reader.type == dxfReader::INT32
                 && reader.getInt32() == 2147483647,
             "DXF ASCII reader preserves code 260 as INT32");
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
    t.expect(dxfValueKindForCode(259) == DxfValueKind::Dbl
                 && dxfValueKindForCode(260) == DxfValueKind::I32
                 && dxfValueKindForCode(269) == DxfValueKind::I32
                 && dxfValueKindForCode(270) == DxfValueKind::I16,
             "DXF 259-270 boundary kinds are explicit");
    t.expect(dxfValueKindForCode(481) == DxfValueKind::Str
                 && dxfValueKindForCode(482) == DxfValueKind::Unknown
                 && dxfValueKindForCode(998) == DxfValueKind::Unknown
                 && dxfValueKindForCode(999) == DxfValueKind::Str,
             "DXF 481-999 unknown span is non-overlapping");
    t.expect(dxfValueKindForCode(1003) == DxfValueKind::Str
                 && dxfValueKindForCode(1004) == DxfValueKind::Bin
                 && dxfValueKindForCode(1005) == DxfValueKind::Str
                 && dxfValueKindForCode(1071) == DxfValueKind::I32
                 && dxfValueKindForCode(1072) == DxfValueKind::Unknown,
             "DXF binary/XDATA boundaries are explicit");

    std::stringstream records(
        "259\n1.25\n260\n2147483647\n269\n-7\n270\n7\n"
        "481\nABCD\n482\n3.14\n998\nopaque-after\n"
        "999\ncomment\n1004\nAB\n1005\nABC\n1071\n9\n");
    dxfReaderAscii reader(&records);
    int code = 0;
    const std::vector<dxfReader::TYPE> expectedTypes {
        dxfReader::DOUBLE, dxfReader::INT32, dxfReader::INT32,
        dxfReader::INT32, dxfReader::STRING, dxfReader::STRING,
        dxfReader::STRING, dxfReader::STRING, dxfReader::BINARY,
        dxfReader::STRING, dxfReader::INT32};
    std::size_t index = 0;
    while (reader.readRec(&code)) {
        t.expect(index < expectedTypes.size() && reader.type == expectedTypes[index],
                 "DXF parser boundary type agrees with classifier");
        ++index;
    }
    t.expect(index == expectedTypes.size(),
             "DXF parser consumes every boundary vector");

    std::stringstream capturedRecords("260\n2147483647\n482\n3.14\n998\nopaque\n");
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
    t.expect(index == 3 && object.groups.size() == 3
                 && object.groups[0].type() == DRW_Variant::INTEGER
                 && object.groups[1].type() == DRW_Variant::STRING
                 && object.groups[2].type() == DRW_Variant::STRING
                 && object.rawValues.size() == object.groups.size(),
             "raw capture preserves typed/opaque boundary values");
}

DRW_RawDxfObject rawBoundaryObject() {
    DRW_RawDxfObject object;
    object.name = "RAW_BOUNDARY";
    object.handle = 0x1Au;
    object.m_version = DRW::AC1027;
    object.hasRawValues = true;
    object.groups = {
        DRW_Variant(5, std::string("1A")),
        DRW_Variant(260, static_cast<std::int32_t>(2147483647)),
        DRW_Variant(269, static_cast<std::int32_t>(-7)),
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
        dxfReader::STRING, dxfReader::STRING, dxfReader::INT32,
        dxfReader::INT32, dxfReader::STRING, dxfReader::STRING,
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
    badNumeric.groups[1] = DRW_Variant(260, std::string("not-an-int"));
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
}

DRW_RawDxfSection rawBoundarySection() {
    DRW_RawDxfSection section;
    section.m_name = "LOCAL_RAW_BOUNDARY";
    section.m_version = DRW::AC1027;
    section.m_hasRawValues = true;
    section.m_groups = {
        DRW_Variant(260, static_cast<std::int32_t>(2147483647)),
        DRW_Variant(269, static_cast<std::int32_t>(-7)),
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
        dxfReader::STRING, dxfReader::STRING, dxfReader::INT32,
        dxfReader::INT32, dxfReader::STRING, dxfReader::STRING,
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
    badNumeric.m_groups[0] = DRW_Variant(260, std::string("not-an-int"));
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
        t.expect(object.groups[0].type() == DRW_Variant::INTEGER
                     && object.groups[0].i_val() == 2147483647,
                 "raw DXF capture stores code 260 as an integer variant");
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
    DRW_Hatch hatch;
    hatch.name = "SOLID";
    hatch.gradName = "\x01stale-gradient";
    DRW_Hatch::GradientStop stale;
    stale.rgb = -1039906145;
    stale.colorName = "\x02stale";
    hatch.gradColors.push_back(stale);
    t.expect(owner.validateHatchPayload(&hatch),
             "solid hatch ignores stale inactive gradient carriers");

    hatch.isGradient = 1;
    t.expect(!owner.validateHatchPayload(&hatch),
             "active gradient still validates gradient carriers");
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

}  // namespace

int main() {
    TestContext context;
    testBufferRoundTrip(context);
    testTextAndPreR13(context);
    testDxfClassifierBoundaryMatrix(context);
    testDxfRawBoundaryReplay(context);
    testDxfRawSectionBoundaryReplay(context);
    testRawCapture(context);
    testHatchValidationIgnoresInactiveGradient(context);
    testFixedSpaceBlockClassification(context);
    testR2007StringFooterBounds(context);
    testDecompressor(context);
    testHandlesAndHeader(context);
    if (context.failures != 0) {
        std::cerr << context.failures << " Wave 1 assertion(s) failed\n";
        return 1;
    }
    std::cout << "Wave 1 tests: PASS\n";
    return 0;
}
