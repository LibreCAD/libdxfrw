#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <map>
#include <sstream>
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
#include "intern/rscodec.h"

// The façade probe only exercises read callbacks. Keep the test target
// independent from the dwg2dxf adapter translation unit, whose writeEntity
// implementation is not needed here.
void dx_iface::writeEntity(DRW_Entity*) {}

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
    t.expect(dxfValueKindForCode(260, DxfClassifierProfile::StandaloneSafe)
                 == DxfValueKind::I32
                 && dxfValueKindForCode(482,
                                        DxfClassifierProfile::StandaloneSafe)
                        == DxfValueKind::Unknown,
             "standalone-safe classifier profile remains the default");
    t.expect(dxfValueKindForCode(260,
                                 DxfClassifierProfile::LibreCadMasterLegacy)
                 == DxfValueKind::Bln
                 && dxfValueKindForCode(
                        482, DxfClassifierProfile::LibreCadMasterLegacy)
                        == DxfValueKind::Dbl,
             "target legacy classifier profile is explicit and opt-in");

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

void testDxfClassifierProfileProbe(TestContext& t) {
    std::stringstream asciiRecords("260\n7\n482\n3.5\n");
    dxfReaderAscii safeReader(&asciiRecords);
    t.expect(safeReader.classifierProfile()
                 == DxfClassifierProfile::StandaloneSafe,
             "DXF reader defaults to standalone-safe classifier profile");
    int code = 0;
    t.expect(safeReader.readRec(&code) && code == 260
                 && safeReader.type == dxfReader::INT32
                 && safeReader.getInt32() == 7,
             "standalone-safe probe preserves code 260 integer width");
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
                        == DRW_Variant::INTEGER
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
         dxfReader::INT32, dxfReader::STRING, false},
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

        std::ostringstream binaryBytes;
        dxfWriterBinary binaryWriter(&binaryBytes);
        const bool wrote260 = profileCase.legacy
            ? binaryWriter.writeBool(260, true)
            : binaryWriter.writeInt32(260, 7);
        t.expect(wrote260 && binaryWriter.writeDouble(482, 3.5),
                 "DXF profile matrix creates local binary vector");
        std::stringstream binaryRecords(binaryBytes.str());
        dxfReaderBinary binaryReader(&binaryRecords);
        binaryReader.setClassifierProfile(profileCase.profile);
        const bool binary260Read = binaryReader.readRec(&code);
        t.expect(binary260Read && code == 260
                     && binaryReader.type == profileCase.code260Type,
                 "DXF profile matrix agrees on binary code 260");
        const bool binary482Read = binaryReader.readRec(&code);
        if (profileCase.legacy) {
            t.expect(binary482Read && code == 482
                         && binaryReader.type == profileCase.code482Type,
                     "DXF profile matrix agrees on legacy binary code 482");
        } else {
            t.expect(!binary482Read && code == 482
                         && binaryReader.type == dxfReader::INVALID,
                     "DXF profile matrix rejects safe binary unknown code 482");
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
        DRW_Variant code482;
        dxfReader::TYPE code260Type;
        dxfReader::TYPE code482Type;
    };
    const ReplayCase cases[] = {
        {dxfRW::DxfCompatibilityProfile::StandaloneSafe,
         DxfClassifierProfile::StandaloneSafe,
         DRW_Variant(482, std::string("3.5")), dxfReader::INT32,
         dxfReader::STRING},
        {dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy,
         DxfClassifierProfile::LibreCadMasterLegacy,
         DRW_Variant(482, 3.5), dxfReader::BOOL, dxfReader::DOUBLE}};

    for (const ReplayCase &replayCase : cases) {
        DRW_RawDxfObject object;
        object.name = "PROFILE_REPLAY";
        object.m_version = DRW::AC1027;
        object.groups = {DRW_Variant(5, std::string("2A")),
                         DRW_Variant(260, static_cast<std::int32_t>(7)),
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
        section.m_groups = {DRW_Variant(260, static_cast<std::int32_t>(7)),
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
            : "0\nSECTION\n2\nPROFILE_FAILURE\n260\nnot-an-int\n"
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
    dxfWriterBinary malformedWriter(&malformedBytes);
    t.expect(malformedWriter.writeInt32(482, 7),
             "profile malformed binary vector is created locally");
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

DRW_RawDxfObject rawBinaryBoundaryObject() {
    DRW_RawDxfObject object;
    object.name = "RAW_BINARY_BOUNDARY";
    object.handle = 0x2Au;
    object.m_version = DRW::AC1027;
    object.groups = {
        DRW_Variant(5, std::string("2A")),
        DRW_Variant(260, static_cast<std::int32_t>(2147483647)),
        DRW_Variant(269, static_cast<std::int32_t>(-7)),
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
        DRW_Variant(260, static_cast<std::int32_t>(2147483647)),
        DRW_Variant(269, static_cast<std::int32_t>(-7)),
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

void testDxfBinaryRawBoundaryReplay(TestContext& t) {
    const DRW_RawDxfObject object = rawBinaryBoundaryObject();
    std::string output;
    t.expect(writeRawBinaryBoundaryObject(object, output),
             "binary DXF raw boundary object writes");
    t.expect(!output.empty(), "binary DXF raw boundary object commits bytes");

    std::stringstream records(output);
    dxfReaderBinary reader(&records);
    const std::vector<int> expectedCodes {0, 5, 260, 269, 1004};
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
                     "binary DXF raw replay preserves code 260 value");
        if (index == 3)
            t.expect(reader.getInt32() == -7,
                     "binary DXF raw replay preserves code 269 value");
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
    const std::vector<int> expectedSectionCodes {0, 2, 260, 269, 1004, 0};
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
    badNumeric.groups[1] = DRW_Variant(260, std::string("2147483647"));
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
        std::ostringstream source;
        dxfWriterBinary sourceWriter(&source);
        sourceWriter.writeString(0, "SECTION");
        sourceWriter.writeString(2, "LOCAL_BINARY_CAPTURE");
        if (legacy)
            sourceWriter.writeBool(260, true);
        else
            sourceWriter.writeInt32(260, 2147483647);
        if (legacy)
            sourceWriter.writeBool(269, true);
        else
            sourceWriter.writeInt32(269, -7);
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
            0, 2, 260, 269, profileCode, 1004, 0};
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
        std::ostringstream source;
        dxfWriterBinary sourceWriter(&source);
        sourceWriter.writeString(0, "SECTION");
        sourceWriter.writeString(2, "OBJECTS");
        sourceWriter.writeString(0, "LOCAL_BINARY_OBJECT");
        sourceWriter.writeString(5, "2A");
        if (legacy) {
            sourceWriter.writeBool(260, true);
            sourceWriter.writeBool(269, true);
        } else {
            sourceWriter.writeInt32(260, 2147483647);
            sourceWriter.writeInt32(269, -7);
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
                     && captured.groups[1].code() == 260
                     && captured.groups[2].code() == 269
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
            0, 5, 260, 269, profileCode, 1004};
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
        DRW_Variant(260, std::string("not-an-int")));
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
        {320, "4A"},
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
                          DRW_Variant(260, std::string("not-an-int")),
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
    t.expect(asciiWriter.writeRawDxfSection(section)
                 && asciiWriter.writer->hasWriteError()
                 && !output.str().empty(),
             "DXF ASCII raw section preserves a pre-existing writer error");

    DRW_RawDxfSection binarySection = section;
    binarySection.m_hasRawValues = false;
    binarySection.m_rawValues.clear();
    std::ostringstream binaryOutput;
    dxfRW binaryWriter("");
    binaryWriter.version = DRW::AC1027;
    binaryWriter.binFile = true;
    binaryWriter.writer = std::make_unique<dxfWriterBinary>(&binaryOutput);
    binaryWriter.writer->markWriteError();
    t.expect(binaryWriter.writeRawDxfSection(binarySection)
                 && binaryWriter.writer->hasWriteError()
                 && !binaryOutput.str().empty(),
             "DXF binary raw section preserves a pre-existing writer error");
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
        {320, "4A"}, {330, "5A"}, {350, "6A"}, {390, "7A"},
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
        {320, "4A"},
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
    testDxfBinaryRawBoundaryReplay(context);
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
    testDxfRawSectionApplicationGroupReferenceMatrix(context);
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
