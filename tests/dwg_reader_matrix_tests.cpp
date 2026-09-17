#include <array>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "libdwgr.h"
#include "dx_iface.h"

#include "intern/dwgreader15.h"
#include "intern/dwgreader18.h"
#include "intern/dwgreader21.h"
#include "intern/dwgreader24.h"
#include "intern/dwgreader27.h"
#include "intern/dwgreader32.h"
#include "intern/dwgreaderR11.h"
#include "intern/dwgreaderR1_40.h"
#include "intern/dwgreader.h"
#include "intern/dwgutil.h"
#include "intern/drw_textcodec.h"

namespace {

struct TestContext {
    int failures {0};

    void expect(bool condition, const std::string& label) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << label << '\n';
        }
    }
};

void testVersionDispatch(TestContext& t) {
    struct ReaderCase {
        const char* magic;
        DRW::Version version;
    };
    const std::vector<ReaderCase> cases {
        {"AC1.40", DRW::AC14},
        {"AC2.10", DRW::AC210},
        {"AC1003", DRW::AC1003},
        {"AC1004", DRW::AC1004},
        {"AC1006", DRW::AC1006},
        {"AC1009", DRW::AC1009},
        {"AC1012", DRW::AC1012},
        {"AC1014", DRW::AC1014},
        {"AC1015", DRW::AC1015},
        {"AC1018", DRW::AC1018},
        {"AC1021", DRW::AC1021},
        {"AC1024", DRW::AC1024},
        {"AC1027", DRW::AC1027},
        {"AC1032", DRW::AC1032},
    };

    dwgRW owner("");
    dx_iface interface_;
    dx_data data;
    interface_.cData = &data;
    interface_.currentBlock = data.mBlock;
    for (const ReaderCase& test : cases) {
        std::array<std::uint8_t, 6> bytes {};
        std::memcpy(bytes.data(), test.magic, bytes.size());
        const bool opened = owner.readBuffer(bytes.data(), bytes.size(),
                                             &interface_, false);
        const std::string label = std::string("DWG dispatch ") + test.magic;
        t.expect(owner.getVersion() == test.version,
                 label + " sniffs expected version before body rejection");
        t.expect(!opened, label + " rejects synthetic body after dispatch");
    }

    std::array<std::uint8_t, 6> unsupported {
        {'B', 'A', 'D', '0', '0', '0'}
    };
    t.expect(!owner.readBuffer(unsupported.data(), unsupported.size(),
                               &interface_, false)
                 && owner.getError() == DRW::BAD_VERSION,
             "DWG dispatch rejects unknown AC magic with BAD_VERSION");

    std::array<std::uint8_t, 5> truncated {};
    t.expect(!owner.readBuffer(truncated.data(), truncated.size(),
                               &interface_, false)
                 && owner.getError() == DRW::BAD_UNKNOWN,
             "DWG dispatch rejects truncated version header");
}

void testReadBufferRejection(TestContext& t) {
    dwgRW owner("");
    std::array<std::uint8_t, 6> unknown {
        {'B', 'A', 'D', '0', '0', '0'}
    };
    t.expect(!owner.readBuffer(unknown.data(), unknown.size(), nullptr, false)
                 && owner.getError() == DRW::BAD_UNKNOWN,
             "public readBuffer rejects null interface before format parsing");

    std::array<std::uint8_t, 5> shortHeader {};
    t.expect(!owner.readBuffer(shortHeader.data(), shortHeader.size(), nullptr, false)
                 && owner.getError() == DRW::BAD_UNKNOWN,
             "public readBuffer rejects null interface for short input");
    t.expect(!owner.readBuffer(nullptr, 6, nullptr, false)
                 && owner.getError() == DRW::BAD_UNKNOWN,
             "public readBuffer rejects null input without crashing");
}

void testSectionNameMatrix(TestContext& t) {
    struct SectionCase {
        const char* name;
        secEnum::DWGSection section;
    };
    const std::vector<SectionCase> cases {
        {"AcDb:Header", secEnum::HEADER},
        {"AcDb:Classes", secEnum::CLASSES},
        {"AcDb:Handles", secEnum::HANDLES},
        {"AcDb:AcDbObjects", secEnum::OBJECTS},
        {"AcDb:Preview", secEnum::PREVIEW},
        {"AcDb:SummaryInfo", secEnum::SUMARYINFO},
        {"AcDb:RevHistory", secEnum::REVHISTORY},
        {"AcDb:AppInfo", secEnum::APPINFO},
        {"AcDb:AppInfoHistory", secEnum::APPINFOHISTORY},
        {"AcDb:ObjFreeSpace", secEnum::OBJFREESPACE},
        {"AcDb:Template", secEnum::TEMPLATE},
        {"AcDb:FileDepList", secEnum::FILEDEP},
        {"AcDb:Security", secEnum::SECURITY},
        {"AcDb:AuxHeader", secEnum::AUXHEADER},
        {"AcDb:Signature", secEnum::SIGNATURE},
        {"AcDb:VBAProject", secEnum::VBAPROY},
        {"AcDb:AcDsPrototype_1b", secEnum::PROTOTYPE},
    };
    for (const SectionCase& test : cases) {
        t.expect(secEnum::getEnum(test.name) == test.section,
                 std::string("section matrix maps ") + test.name);
    }
    t.expect(secEnum::getEnum("AcDb:Unknown") == secEnum::UNKNOWNS,
             "section matrix keeps unknown names explicit");
    t.expect(secEnum::getEnum("") == secEnum::UNKNOWNS,
             "section matrix keeps empty names unsupported");
}

void testSectionNameUtf16Framing(TestContext& t) {
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1021, false);

    // SectionNameLength is a byte count.  A one-code-unit name must be
    // accepted, and the following byte must remain untouched for the next
    // section-map field.
    std::vector<std::uint8_t> oneUnit {'A', 0, 0x7f};
    dwgBuffer oneUnitBuffer(oneUnit.data(), oneUnit.size(), &codec);
    t.expect(oneUnitBuffer.getUCSStr(2) == "A",
             "section-name framing accepts one UTF-16 code unit");
    t.expect(oneUnitBuffer.getPosition() == 2
                 && oneUnitBuffer.getRawChar8() == 0x7f,
             "section-name framing preserves the following byte");

    // Some section-map producers include a declared UTF-16 NUL code unit in
    // the byte count.  Normalize that declared unit without consuming the
    // next field.
    std::vector<std::uint8_t> declaredNull {'A', 0, 0, 0, 0x6b};
    dwgBuffer declaredNullBuffer(declaredNull.data(), declaredNull.size(),
                                  &codec);
    t.expect(declaredNullBuffer.getUCSStr(4) == "A",
             "section-name framing strips a declared UTF-16 NUL");
    t.expect(declaredNullBuffer.getPosition() == 4
                 && declaredNullBuffer.getRawChar8() == 0x6b,
             "declared UTF-16 NUL normalization preserves alignment");

    std::vector<std::uint8_t> malformed {'A', 0, 0x7f};
    dwgBuffer malformedBuffer(malformed.data(), malformed.size(), &codec);
    t.expect(malformedBuffer.getUCSStr(3).empty()
                 && malformedBuffer.getPosition() == 0,
             "section-name framing rejects an odd byte length");
}

void testR2007ClassStringFooter(TestContext& t) {
    // The AC1024 RTM class footer uses the high-bit extension when the UTF-16
    // string stream exceeds 0x7fff bits. Keep this vector local and synthetic:
    // it exercises the wire arithmetic without retaining a drawing fixture.
    constexpr std::uint64_t footerEndBit = 40800;
    constexpr std::uint64_t highSize = 1;
    constexpr std::uint64_t lowSize = 0x8001;
    constexpr std::uint64_t expectedSize = (highSize << 15) | 1;
    constexpr std::uint64_t expectedStart = footerEndBit - 32 - expectedSize;
    std::vector<std::uint8_t> bytes(5200, 0);
    bytes[footerEndBit / 8 - 4] = static_cast<std::uint8_t>(highSize);
    bytes[footerEndBit / 8 - 3] = 0;
    bytes[footerEndBit / 8 - 2] = static_cast<std::uint8_t>(lowSize & 0xff);
    bytes[footerEndBit / 8 - 1] = static_cast<std::uint8_t>(lowSize >> 8);
    dwgBuffer buffer(bytes.data(), bytes.size());
    std::uint64_t start = 0;
    std::uint64_t size = 0;
    t.expect(readDwgClassStringFooter(buffer, footerEndBit, start, size),
             "AC1024 class footer accepts high-bit string-size extension");
    t.expect(start == expectedStart && size == expectedSize,
             "AC1024 class footer reconstructs extended string bounds");
    t.expect(buffer.getPosition() == expectedStart / 8
                 && buffer.getBitPos() == expectedStart % 8,
             "AC1024 class footer leaves cursor at string start");

    std::vector<std::uint8_t> malformed(5200, 0);
    malformed[footerEndBit / 8 - 4] = 0xff;
    malformed[footerEndBit / 8 - 3] = 0xff;
    malformed[footerEndBit / 8 - 2] = 0xff;
    malformed[footerEndBit / 8 - 1] = 0xff;
    dwgBuffer bad(malformed.data(), malformed.size());
    t.expect(!readDwgClassStringFooter(bad, footerEndBit, start, size),
             "AC1024 class footer rejects an overlong string-size extension");
}

}  // namespace

int main() {
    TestContext context;
    testVersionDispatch(context);
    testReadBufferRejection(context);
    testSectionNameMatrix(context);
    testSectionNameUtf16Framing(context);
    testR2007ClassStringFooter(context);
    if (context.failures != 0) {
        std::cerr << context.failures << " DWG reader matrix assertion(s) failed\n";
        return 1;
    }
    std::cout << "DWG reader matrix tests: PASS\n";
    return 0;
}
