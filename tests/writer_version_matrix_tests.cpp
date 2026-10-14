#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "drw_base.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/drw_textcodec.h"
#include "intern/dwgwriter15.h"
#include "intern/dwgwriter18.h"
#include "intern/dwgwriter21.h"
#include "intern/dwgwriter24.h"
#include "intern/dwgwriter27.h"
#include "intern/dwgwriter32.h"
#include "intern/dwgtemplate.h"
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

        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer semanticReader(
            const_cast<std::uint8_t*>(writer.data().data()),
            writer.data().size(), &codec);
        const std::string semantic = semanticReader.getVariableText(version);
        t.expect(semantic == "writer-A" && semanticReader.isGood(),
                 "versioned TU semantic path accepts inline terminator "
                     + std::to_string(static_cast<int>(version)));

        // The ODA description defines TU as a length followed by N Unicode
        // characters; exercise the alternate length-excludes-terminator form
        // as well.  The adaptive reader must consume the following UTF-16 NUL
        // without consuming any subsequent field.
        dwgBufferW lengthOnly;
        lengthOnly.putBitShort(8);
        for (const char c : std::string("writer-A")) {
            lengthOnly.putRawChar8(static_cast<std::uint8_t>(c));
            lengthOnly.putRawChar8(0);
        }
        lengthOnly.putRawChar8(0);
        lengthOnly.putRawChar8(0);
        lengthOnly.putRawChar8(0x7f);
        dwgBuffer lengthOnlyReader(
            const_cast<std::uint8_t*>(lengthOnly.data().data()),
            lengthOnly.data().size(), &codec);
        t.expect(lengthOnlyReader.getVariableText(version) == "writer-A"
                     && lengthOnlyReader.isGood()
                     && lengthOnlyReader.getRawChar8() == 0x7f
                     && lengthOnlyReader.isGood(),
                 "versioned TU semantic path accepts trailing terminator "
                     + std::to_string(static_cast<int>(version)));

        dwgBufferW noTerminator;
        noTerminator.putBitShort(8);
        for (const char c : std::string("writer-A")) {
            noTerminator.putRawChar8(static_cast<std::uint8_t>(c));
            noTerminator.putRawChar8(0);
        }
        dwgBuffer noTerminatorReader(
            const_cast<std::uint8_t*>(noTerminator.data().data()),
            noTerminator.data().size(), &codec);
        t.expect(noTerminatorReader.getVariableText(version) == "writer-A"
                     && noTerminatorReader.isGood(),
                 "versioned TU semantic path tolerates absent terminator "
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

std::unique_ptr<dwgWriter> templateWriter(DRW::Version version,
                                         DRW_Header& header) {
    switch (version) {
    case DRW::AC1015: return std::make_unique<dwgWriter15>(nullptr, &header);
    case DRW::AC1018: return std::make_unique<dwgWriter18>(nullptr, &header);
    case DRW::AC1021: return std::make_unique<dwgWriter21>(nullptr, &header);
    case DRW::AC1024: return std::make_unique<dwgWriter24>(nullptr, &header);
    case DRW::AC1027: return std::make_unique<dwgWriter27>(nullptr, &header);
    case DRW::AC1032: return std::make_unique<dwgWriter32>(nullptr, &header);
    default: return nullptr;
    }
}

void testTemplateMeasurementAdmission(TestContext& t) {
    for (const auto version : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                               DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        for (int value : {0, 1}) {
            DRW_Header header;
            header.addInt("$MEASUREMENT", value, 70);
            header.addInt("$INSUNITS", value == 0 ? 4 : 1, 70);
            std::uint16_t measurement = 99;
            const auto count = header.vars.size();
            t.expect(dwgTemplate::headerMeasurement(&header, measurement)
                         && measurement == value && header.vars.size() == count,
                     "explicit Template measurement is independent of INSUNITS");
            t.expect(dwgTemplate::emptyDescription(measurement)
                         == std::array<std::uint8_t, 4>{{0, 0,
                                static_cast<std::uint8_t>(value), 0}},
                     "Template canonical empty-description field bytes");
            auto writer = templateWriter(version, header);
            t.expect(writer && writer->writeFileHeaderStub(),
                     "all six writer targets admit explicit0/1 measurement");
        }
        for (int value : {-1, 2, (std::numeric_limits<int>::max)()}) {
            DRW_Header header;
            header.addInt("MEASUREMENT", value, 70);
            auto writer = templateWriter(version, header);
            const auto before = writer->buffer();
            t.expect(writer && !writer->writeFileHeaderStub()
                         && writer->buffer() == before,
                     "invalid explicit measurement rejects before writer bytes");
            header.addInt("MEASUREMENT", 1, 70);
            t.expect(writer->writeFileHeaderStub(),
                     "corrected measurement retries on the same untouched writer");
        }
    }
    DRW_Header absent;
    absent.addInt("INSUNITS", 4, 70);
    std::uint16_t measurement = 99;
    t.expect(dwgTemplate::headerMeasurement(&absent, measurement)
                 && measurement == 0,
             "absent measurement retains the existing DWG fallback");
    absent.addStr("MEASUREMENT", "1", 1);
    t.expect(!dwgTemplate::headerMeasurement(&absent, measurement),
             "explicit noninteger measurement is not treated as absent");
    DRW_Header aliases;
    aliases.addInt("MEASUREMENT", 0, 70);
    aliases.addInt("$MEASUREMENT", 1, 70);
    t.expect(!dwgTemplate::headerMeasurement(&aliases, measurement),
             "conflicting measurement aliases reject deterministically");
}

} // namespace

int main() {
    TestContext context;
    testVersionedObjectTypes(context);
    testVersionedText(context);
    testOverflowIsFailClosed(context);
    testWriterInheritanceMatrix(context);
    testCapabilityInventory(context);
    testTemplateMeasurementAdmission(context);
    if (context.failures != 0) {
        std::cerr << context.failures << " writer version assertion(s) failed\n";
        return 1;
    }
    std::cout << "Writer version matrix: PASS\n";
    return 0;
}
