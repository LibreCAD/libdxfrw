#include <array>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// The reader factory and bounded openBuffer path are intentionally private
// implementation seams.  This test exposes them only in this translation
// unit so dispatch can be qualified without constructing a complete drawing.
#define private public
#include "libdwgr.h"
#undef private

#include "intern/dwgreader15.h"
#include "intern/dwgreader18.h"
#include "intern/dwgreader21.h"
#include "intern/dwgreader24.h"
#include "intern/dwgreader27.h"
#include "intern/dwgreader32.h"
#include "intern/dwgreaderR11.h"
#include "intern/dwgreaderR1_40.h"
#include "intern/dwgutil.h"

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

bool matchesReader(const dwgReader* reader, DRW::Version version) {
    switch (version) {
    case DRW::AC14:
        return dynamic_cast<const dwgReaderR1_40*>(reader) != nullptr;
    case DRW::AC210:
    case DRW::AC1003:
    case DRW::AC1004:
    case DRW::AC1006:
    case DRW::AC1009:
        return dynamic_cast<const dwgReaderR11*>(reader) != nullptr;
    case DRW::AC1012:
    case DRW::AC1014:
    case DRW::AC1015:
        return dynamic_cast<const dwgReader15*>(reader) != nullptr;
    case DRW::AC1018:
        return dynamic_cast<const dwgReader18*>(reader) != nullptr;
    case DRW::AC1021:
        return dynamic_cast<const dwgReader21*>(reader) != nullptr;
    case DRW::AC1024:
        return dynamic_cast<const dwgReader24*>(reader) != nullptr;
    case DRW::AC1027:
        return dynamic_cast<const dwgReader27*>(reader) != nullptr;
    case DRW::AC1032:
        return dynamic_cast<const dwgReader32*>(reader) != nullptr;
    default:
        return false;
    }
}

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
    for (const ReaderCase& test : cases) {
        std::array<std::uint8_t, 6> bytes {};
        std::memcpy(bytes.data(), test.magic, bytes.size());
        const bool opened = owner.openBuffer(
            std::make_unique<dwgBuffer>(bytes.data(), bytes.size()));
        const std::string label = std::string("DWG dispatch ") + test.magic;
        t.expect(opened, label + " accepts known AC magic");
        t.expect(owner.version == test.version, label + " sniffs expected version");
        t.expect(opened && matchesReader(owner.reader.get(), test.version),
                 label + " selects expected reader class");
    }

    std::array<std::uint8_t, 6> unsupported {
        {'B', 'A', 'D', '0', '0', '0'}
    };
    t.expect(!owner.openBuffer(
                  std::make_unique<dwgBuffer>(unsupported.data(), unsupported.size()))
                 && owner.error == DRW::BAD_VERSION && owner.reader == nullptr,
             "DWG dispatch rejects unknown AC magic with BAD_VERSION");

    std::array<std::uint8_t, 5> truncated {};
    t.expect(!owner.openBuffer(
                  std::make_unique<dwgBuffer>(truncated.data(), truncated.size()))
                 && owner.error == DRW::BAD_VERSION,
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

}  // namespace

int main() {
    TestContext context;
    testVersionDispatch(context);
    testReadBufferRejection(context);
    testSectionNameMatrix(context);
    if (context.failures != 0) {
        std::cerr << context.failures << " DWG reader matrix assertion(s) failed\n";
        return 1;
    }
    std::cout << "DWG reader matrix tests: PASS\n";
    return 0;
}
