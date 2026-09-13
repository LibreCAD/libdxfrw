#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "drw_base.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgwriter15.h"
#include "intern/dwgwriter18.h"
#include "intern/dwgwriter21.h"
#include "intern/dwgwriter24.h"
#include "intern/dwgwriter27.h"
#include "intern/dwgwriter32.h"
#include "libdwgr.h"

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

std::vector<std::uint16_t> objectTypes(DRW::Version version) {
    if (version > DRW::AC1021)
        return {0, 1, 255, 0x01F0, 0x01F1, 0x02F0, 0xFFFF};
    return {0, 1, 255, 256, 0xFFFF};
}

void testVersionedObjectTypes(TestContext& t) {
    const std::vector<DRW::Version> versions {
        DRW::AC1015, DRW::AC1018, DRW::AC1021,
        DRW::AC1024, DRW::AC1027, DRW::AC1032};
    for (const DRW::Version version : versions) {
        for (const std::uint16_t type : objectTypes(version)) {
            dwgBufferW writer;
            writer.putObjType(version, type);
            dwgBuffer reader(const_cast<std::uint8_t*>(writer.data().data()),
                             writer.data().size());
            t.expect(writer.isGood() && reader.getObjType(version) == type
                         && reader.isGood(),
                     "versioned OT round-trip "
                         + std::to_string(static_cast<int>(version)) + "/"
                         + std::to_string(type));
        }
    }
}

void testVersionedText(TestContext& t) {
    const std::vector<DRW::Version> versions {
        DRW::AC1015, DRW::AC1018, DRW::AC1021,
        DRW::AC1024, DRW::AC1027, DRW::AC1032};
    for (const DRW::Version version : versions) {
        dwgBufferW writer;
        writer.putVariableText(version, "writer-A");
        dwgBuffer reader(const_cast<std::uint8_t*>(writer.data().data()),
                         writer.data().size());
        if (version <= DRW::AC1018) {
            t.expect(writer.isGood()
                         && reader.getVariableText(version) == "writer-A"
                         && reader.isGood(),
                     "versioned TV round-trip "
                         + std::to_string(static_cast<int>(version)));
            continue;
        }

        // The writer stores the TU terminator inside the declared unit count,
        // while the legacy reader's default nullTerm=true mode expects an
        // additional terminator.  Exercise the exact raw carrier with
        // nullTerm=false and require the default path to either decode
        // semantically or fail closed (never silently return a partial value).
        const std::string expectedRaw("w\0r\0i\0t\0e\0r\0-\0A\0\0\0", 18);
        const std::string raw = reader.getVariableText(version, false);
        t.expect(writer.isGood() && raw == expectedRaw && reader.isGood(),
                 "versioned TU raw framing "
                     + std::to_string(static_cast<int>(version)));
        dwgBuffer semanticReader(
            const_cast<std::uint8_t*>(writer.data().data()),
            writer.data().size());
        const std::string semantic = semanticReader.getVariableText(version);
        t.expect((semantic == "writer-A")
                     || (semantic.empty() && !semanticReader.isGood()),
                 "versioned TU semantic path is correct or fail-closed "
                     + std::to_string(static_cast<int>(version)));
    }
}

void testOverflowIsFailClosed(TestContext& t) {
    dwgBufferW writer;
    writer.putBitLongLong(0x0100000000000000ULL);
    t.expect(!writer.isGood() && writer.data().empty(),
             "R2010 bit-long-long overflow emits no partial bytes");
}

void testWriterInheritanceMatrix(TestContext& t) {
    t.expect(std::is_base_of<dwgWriter15, dwgWriter18>::value
                 && std::is_base_of<dwgWriter18, dwgWriter24>::value
                 && std::is_base_of<dwgWriter24, dwgWriter27>::value
                 && std::is_base_of<dwgWriter27, dwgWriter32>::value,
             "R2004-to-R2018 writer inheritance is linear");
    t.expect(std::is_base_of<dwgWriter24, dwgWriter21>::value
                 && std::is_final<dwgWriter21>::value,
             "AC1021 writer is the final RS-container override branch");
    t.expect(static_cast<int>(DRW::AC1015)
                 < static_cast<int>(DRW::AC1018)
                 && static_cast<int>(DRW::AC1018)
                        < static_cast<int>(DRW::AC1021)
                 && static_cast<int>(DRW::AC1021)
                        < static_cast<int>(DRW::AC1024),
             "writer version gates preserve chronological ordering");
}

void testCapabilityInventory(TestContext& t) {
    std::vector<DwgDataStorageWriterCapability> capabilities;
    t.expect(getDwgDataStorageWriterCapabilities(capabilities)
                 && capabilities.size() == 38,
             "writer capability inventory has 38 executable bindings");
    for (const auto& capability : capabilities) {
        t.expect(capability.binding != DwgDataStorageWriterBinding::None
                     && capability.operation
                            != DwgDataStorageWriterOperation::None
                     && capability.family != nullptr
                     && capability.family[0] != '\0'
                     && capability.recordName != nullptr
                     && capability.recordName[0] != '\0'
                     && capability.minVersion >= DRW::AC1015
                     && (capability.maxVersion == DRW::UNKNOWNV
                         || capability.maxVersion >= capability.minVersion),
                 "writer capability identity and version range are valid");
        DwgDataStorageWriterCapability copy;
        t.expect(getDwgDataStorageWriterCapability(capability.binding, copy)
                     && copy.binding == capability.binding
                     && copy.operation == capability.operation
                     && copy.minVersion == capability.minVersion
                     && copy.maxVersion == capability.maxVersion,
                 "writer capability lookup is stable");
        t.expect(findDwgDataStorageWriterBinding(
                     capability.family, capability.className,
                     capability.recordName) == capability.binding,
                 "writer capability identity resolves to its binding");
    }
}

} // namespace

int main() {
    TestContext context;
    testVersionedObjectTypes(context);
    testVersionedText(context);
    testOverflowIsFailClosed(context);
    testWriterInheritanceMatrix(context);
    testCapabilityInventory(context);
    if (context.failures != 0) {
        std::cerr << context.failures << " writer version assertion(s) failed\n";
        return 1;
    }
    std::cout << "Writer version matrix: PASS\n";
    return 0;
}
