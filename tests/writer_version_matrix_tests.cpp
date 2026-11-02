#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
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
#include "dx_dwg_class_section_wire.h"

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

// Section authoring seam only. Actual six-container imports are checked by
// the adapter raster test; the byte observer here has no production dependency.
class ClassSectionWriter final : public dwgWriter24 {
public:
    ClassSectionWriter(DRW::Version version, unsigned count, unsigned length)
        : dwgWriter24(nullptr, nullptr) {
        m_version = version; configureTextCodec(); m_dwgClassDefinitions.clear();
        for (unsigned n = 0; n < count; ++n) {
            DwgClassDefinition row;
            row.m_classNum = static_cast<std::uint16_t>(500u + n); row.m_proxyFlag = 7u;
            row.m_appName.assign(length, 'A'); row.m_className.assign(length, 'C'); row.m_recordName.assign(length, 'R');
            row.m_wasProxy = true; row.m_entityFlagRaw = 0x1F3u; row.m_instanceCount = 1;
            row.m_dwgVersion = 257; row.m_maintenanceVersion = 329; row.m_unknown1 = 41; row.m_unknown2 = 73;
            m_dwgClassDefinitions.push_back(std::move(row));
        }
    }
};
void testClassTextBitLengths(TestContext& t) {
    namespace wire = dx_class_wire;
    unsigned cases = 0u, shortFooters = 0u, extendedFooters = 0u, alignments = 0u;
    for (const auto version : {DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032})
    for (unsigned count = 0; count < 8u; ++count)
    for (const unsigned length : {1u, 254u, 255u, 700u}) {
        ClassSectionWriter writer(version, count, length);
        const auto label = "CLASSES exact text bits " + std::to_string(version) + "/" + std::to_string(count) + "/" + std::to_string(length);
        t.expect(writer.prepareDwgClassManifest() && writer.writeDwgClasses(), label + " authoring");
        wire::SectionProfile profile;
        profile.revision = version == DRW::AC1021 ? 2007u : version == DRW::AC1024 ? 2010u
            : version == DRW::AC1027 ? 2013u : 2018u;
        profile.maintenance = version == DRW::AC1027 ? 6u : 0u;
        profile.highSizeRule = wire::ClassHighSizeRule::R2018Always;
        profile.trailer = wire::Trailer::ProducerBitLong;
        profile.tail = wire::ClassTail::BeforeEight;
        profile.requireTrueHeaderFlag = false; profile.scalarByteAlignment = true;
        const auto& bytes = writer.buffer();
        const auto proof = wire::observeSection(bytes.data(), bytes.size(), profile);
        t.expect(proof.status == wire::Status::Observed && proof.records == count && proof.crcMatches,
                 label + " independently framed with complete text consumption");
        if (proof.status != wire::Status::Observed) continue;
        // TU includes one terminal NUL. BS256 uses only its two tag bits.
        const auto units = length + 1u;
        const auto prefix = units == 256u ? 2u : units < 256u ? 10u : 18u;
        const auto expectedBits = count * 3u * (prefix + units * 16u);
        const auto footerBits = count == 0u ? 1u : expectedBits > 0x7FFFu ? 33u : 17u;
        t.expect(proof.strings.end - proof.strings.begin == expectedBits
                     && proof.stringFooter.end - proof.stringFooter.begin == footerBits
                     && proof.extendedFooter == (expectedBits > 0x7FFFu), label + " exact footer bounds");
        if (count != 0u) {
            alignments |= 1u << (proof.stringFooter.begin % 8u);
            if (proof.extendedFooter) ++extendedFooters; else ++shortFooters;
        }
        auto data = proof.data, strings = proof.strings;
        for (unsigned n = 0; n < count; ++n) {
            const auto record = wire::scan(data, strings, profile.revision, profile.trailer);
            bool names = record.status == wire::Status::Observed;
            for (unsigned name = 0; name < 3u; ++name) {
                const std::string expected(length, name == 0u ? 'A' : name == 1u ? 'C' : 'R');
                names = names && wire::equalsTerminatedUtf8(strings, record.text[name], true, expected.data(), expected.size());
            }
            t.expect(names && record.number == 500u + n && record.proxy == 7u && record.zombie
                         && record.dwgVersion == 257 && record.maintenance == 329
                         && record.unknown1 == 41 && record.unknown2 == 73,
                     label + " independent row and nondefault metadata");
            data.begin = record.dataEnd; strings.begin = record.stringEnd;
        }
        t.expect(data.begin == data.end && strings.begin == strings.end, label + " no text-padding waiver");
        auto corrupt = bytes;
        corrupt.at(proof.crcByte) ^= 1u;
        t.expect(wire::observeSection(corrupt.data(), corrupt.size(), profile).reason == wire::SectionReason::Crc,
                 label + " independent CRC still required");
        ++cases;
    }
    t.expect(cases == 128u && shortFooters && extendedFooters && alignments == 0x55u,
             "CLASSES matrix covers empty/partial/aligned, short/extended and all four text-footer bit alignments");
    std::cout << "CLASSES writer exact-bit cases=" << cases << " short=" << shortFooters
              << " extended=" << extendedFooters << " alignment-mask=" << alignments << '\n';
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
    testClassTextBitLengths(context);
    if (context.failures != 0) {
        std::cerr << context.failures << " writer version assertion(s) failed\n";
        return 1;
    }
    std::cout << "Writer version matrix: PASS\n";
    return 0;
}
