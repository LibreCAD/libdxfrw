// SPDX-License-Identifier: GPL-2.0-or-later
// Generated ATTRIB writer revision-matrix rollback and readback control.
#include "dx_iface.h"
#include "intern/dxfreader.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <new>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {
long allocationsBeforeFailure = -1;
std::size_t allocationAttempts = 0u;
bool allocationFailed = false;
bool allocationTracking = false;

bool failAllocation() noexcept {
    if (!allocationTracking || allocationsBeforeFailure < 0)
        return false;
    ++allocationAttempts;
    if (allocationsBeforeFailure == 0) {
        allocationsBeforeFailure = -1;
        allocationFailed = true;
        return true;
    }
    --allocationsBeforeFailure;
    return false;
}

void armAllocationFailure(long offset) noexcept {
    allocationsBeforeFailure = offset;
    allocationAttempts = 0u;
    allocationFailed = false;
    allocationTracking = true;
}

void disarmAllocationFailure() noexcept {
    allocationTracking = false;
    allocationsBeforeFailure = -1;
}

DRW_Attrib makeAttrib() {
    DRW_Attrib value;
    value.handle = 0xC310u;
    value.layer = "0";
    value.tag = "ITEM_CODE";
    value.text = "PANEL-42";
    value.style = "STANDARD";
    value.basePoint = {12.25, -4.5, 7.75};
    value.secPoint = {21.5, 8.25, -3.125};
    value.extPoint = {0.0, 0.0, 1.0};
    value.height = 2.75;
    value.widthscale = 0.875;
    value.angle = 32.5;
    value.oblique = 3.25;
    value.textgen = 2;
    value.alignH = DRW_Text::HCenter;
    value.alignV = DRW_Text::VMiddle;
    value.attribFlags = 5u;
    value.m_fieldLength = 17;
    return value;
}

bool sameAttrib(const DRW_Attrib& actual, const DRW_Attrib& expected,
                bool compareOwner) {
    const auto samePoint = [](const DRW_Coord& a, const DRW_Coord& b) {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    };
    return actual.eType == DRW::ATTRIB && actual.handle == expected.handle
        && (!compareOwner || actual.parentHandle == expected.parentHandle)
        && actual.layer == expected.layer
        && actual.lineType == expected.lineType
        && actual.color == expected.color && actual.lWeight == expected.lWeight
        && actual.ltypeScale == expected.ltypeScale
        && actual.visible == expected.visible
        && actual.material == expected.material && actual.space == expected.space
        && actual.numProxyGraph == expected.numProxyGraph
        && actual.proxyGraphics == expected.proxyGraphics
        && actual.appData.size() == expected.appData.size()
        && actual.tag == expected.tag
        && actual.text == expected.text && actual.style == expected.style
        && samePoint(actual.basePoint, expected.basePoint)
        && samePoint(actual.secPoint, expected.secPoint)
        && samePoint(actual.extPoint, expected.extPoint)
        && actual.thickness == expected.thickness
        && actual.height == expected.height
        && actual.widthscale == expected.widthscale
        && actual.angle == expected.angle && actual.oblique == expected.oblique
        && actual.textgen == expected.textgen
        && actual.alignH == expected.alignH && actual.alignV == expected.alignV
        && actual.attribFlags == expected.attribFlags
        && actual.m_fieldLength == expected.m_fieldLength
        && actual.lockPosition == expected.lockPosition
        && actual.attVersion == expected.attVersion
        && actual.m_attributeType == expected.m_attributeType
        && actual.keepDuplicateRecords == expected.keepDuplicateRecords
        && actual.hasDxfXrecord == expected.hasDxfXrecord
        && actual.m_r2018OuterAnnotativeData
            == expected.m_r2018OuterAnnotativeData
        && actual.m_r2018OuterAnnotativeUnknown
            == expected.m_r2018OuterAnnotativeUnknown
        && actual.m_r2018OuterAnnotativeAppHandle
            == expected.m_r2018OuterAnnotativeAppHandle
        && actual.m_r2018OuterAnnotativeAppIdName
            == expected.m_r2018OuterAnnotativeAppIdName
        && actual.mtext == nullptr && expected.mtext == nullptr
        && actual.extData.size() == expected.extData.size();
}

bool sameInsertSource(const DRW_Insert& actual, const DRW_Insert& expected) {
    const auto samePoint = [](const DRW_Coord& a, const DRW_Coord& b) {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    };
    return actual.eType == expected.eType
        && actual.handle == expected.handle
        && actual.parentHandle == expected.parentHandle
        && actual.name == expected.name && actual.layer == expected.layer
        && actual.lineType == expected.lineType && actual.color == expected.color
        && actual.lWeight == expected.lWeight
        && actual.ltypeScale == expected.ltypeScale
        && actual.visible == expected.visible
        && actual.material == expected.material && actual.space == expected.space
        && actual.numProxyGraph == expected.numProxyGraph
        && actual.proxyGraphics == expected.proxyGraphics
        && actual.appData.size() == expected.appData.size()
        && samePoint(actual.basePoint, expected.basePoint)
        && samePoint(actual.extPoint, expected.extPoint)
        && actual.thickness == expected.thickness
        && actual.xAxisAngle == expected.xAxisAngle
        && actual.xscale == expected.xscale && actual.yscale == expected.yscale
        && actual.zscale == expected.zscale && actual.angle == expected.angle
        && actual.colcount == expected.colcount
        && actual.rowcount == expected.rowcount
        && actual.colspace == expected.colspace
        && actual.rowspace == expected.rowspace
        && actual.attlist.size() == expected.attlist.size()
        && std::equal(actual.attlist.begin(), actual.attlist.end(),
                      expected.attlist.begin(),
                      [](const auto& a, const auto& b) {
                          return static_cast<bool>(a) == static_cast<bool>(b);
                      })
        && static_cast<bool>(actual.seqEndEntity)
            == static_cast<bool>(expected.seqEndEntity);
}

class Export final : public dx_iface {
public:
    explicit Export(dxfRW& output) : codec(output), attribute(
            std::make_shared<DRW_Attrib>(makeAttrib())) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &codec;
        insert.handle = 0xC300u;
        insert.name = "ATTRIBUTE_TEST_BLOCK";
        insert.basePoint = {1.0, 2.0, 3.0};
        insert.attlist.push_back(attribute);
    }

    void writeEntities() override {
        struct TrackingScope {
            long& reached;
            ~TrackingScope() {
                reached = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
            }
        } trackingScope{reached};
        armAllocationFailure(failAt);
        try {
            accepted = emitOwnerlessAttrib
                ? codec.writeAttrib(attribute.get())
                : codec.writeInsert(&insert);
        } catch (const std::bad_alloc&) {
            accepted = false;
        }
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    dx_data storage;
    DRW_Insert insert;
    std::shared_ptr<DRW_Attrib> attribute;
    long failAt = -1;
    long reached = 0;
    bool accepted = false;
    bool emitOwnerlessAttrib = false;
};

class Readback final : public dx_iface {
public:
    void addInsert(const DRW_Insert& value) override { inserts.push_back(value); }
    std::vector<DRW_Insert> inserts;
};

bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

bool readbackMatches(const std::filesystem::path& path,
                     DRW::Version version) {
    dx_data data;
    Readback reader;
    const bool imported = reader.fileImport(path.string(), &data, false);
    const bool oneInsert = reader.inserts.size() == 1u;
    const bool oneAttrib = oneInsert
        && reader.inserts.front().attlist.size() == 1u
        && reader.inserts.front().attlist.front() != nullptr;
    bool identity = false;
    bool fields = false;
    if (oneAttrib) {
        const auto& insert = reader.inserts.front();
        const auto& attribute = *insert.attlist.front();
        DRW_Attrib expected = makeAttrib();
        expected.handle = attribute.handle;
        expected.parentHandle = version > DRW::AC1014 ? insert.handle : 0u;
        identity = insert.handle != DRW::NoHandle
            && attribute.handle != DRW::NoHandle
            && attribute.parentHandle == expected.parentHandle;
        fields = sameAttrib(attribute, expected, true);
    }
    if (!imported || data.sourceVersion != version || !oneInsert
        || !oneAttrib || !identity || !fields) {
        std::cerr << "typed INSERT readback: imported=" << imported
                  << " version=" << static_cast<int>(data.sourceVersion)
                  << '/' << static_cast<int>(version)
                  << " inserts=" << reader.inserts.size()
                  << " oneAttrib=" << oneAttrib << " identity=" << identity
                  << " fields=" << fields;
        if (oneInsert && !reader.inserts.front().attlist.empty()
            && reader.inserts.front().attlist.front()) {
            const auto& actual = *reader.inserts.front().attlist.front();
            std::cerr << " attrHandle=" << actual.handle
                      << " owner=" << actual.parentHandle
                      << " tag=" << actual.tag << " text=" << actual.text
                      << " style=" << actual.style << " height=" << actual.height
                      << " field=" << actual.m_fieldLength;
        }
        std::cerr << '\n';
    }
    return imported && data.sourceVersion == version && oneInsert
        && oneAttrib && identity && fields;
}

bool rawRecordMatches(const std::string& bytes, bool binary,
                      DRW::Version version) {
    std::istringstream input(bytes);
    if (binary) {
        const std::string signature("AutoCAD Binary DXF\r\n\x1a\0", 22u);
        std::string actualSignature(22u, '\0');
        input.read(actualSignature.data(),
                   static_cast<std::streamsize>(actualSignature.size()));
        if (actualSignature != signature)
            return false;
    }
    std::unique_ptr<dxfReader> reader;
    if (!binary)
        reader = std::make_unique<dxfReaderAscii>(&input);
    else if (version == DRW::AC1009)
        reader = std::make_unique<dxfReaderBinaryR12>(&input);
    else
        reader = std::make_unique<dxfReaderBinary>(&input);
    bool inInsert = false;
    bool inAttrib = false;
    bool hasInsertHandle = false;
    bool hasAttributeHandle = false;
    bool hasAttributeOwner = version <= DRW::AC1014;
    std::uint32_t insertHandle = DRW::NoHandle;
    std::uint32_t attributeHandle = DRW::NoHandle;
    std::uint32_t attributeOwner = DRW::NoHandle;
    bool hasLayer = false;
    bool hasTag = false;
    bool hasText = false;
    bool hasHeight = false;
    bool hasAlignment = false;
    bool hasFlags = false;
    bool hasFieldLength = false;
    bool hasAttributeSubclass = false;
    bool hasLockPosition = version == DRW::AC1009 || version == DRW::AC1014;
    bool sawLockPosition = false;
    int recordCount = 0;
    int insertCount = 0;
    int code = 0;
    while (reader->readRec(&code)) {
        if (code == 0) {
            const std::string type = reader->getString();
            if (inAttrib)
                break;
            if (type == "INSERT") {
                inInsert = true;
                ++insertCount;
            } else if (type == "ATTRIB") {
                inInsert = false;
                inAttrib = true;
                ++recordCount;
            } else
                inInsert = false;
            continue;
        }
        if (!inInsert && !inAttrib)
            continue;
        switch (code) {
        case 5:
            if (inInsert) {
                insertHandle = reader->getHandleString();
                hasInsertHandle = insertHandle != DRW::NoHandle;
            } else {
                attributeHandle = reader->getHandleString();
                hasAttributeHandle = attributeHandle != DRW::NoHandle;
            }
            break;
        case 330:
            if (inAttrib) {
                attributeOwner = reader->getHandleString();
                hasAttributeOwner = attributeOwner == insertHandle;
            }
            break;
        }
        if (!inAttrib)
            continue;
        switch (code) {
        case 8:
            hasLayer = reader->getString() == "0";
            break;
        case 1:
            hasText = reader->getUtf8String() == "PANEL-42";
            break;
        case 2:
            hasTag = reader->getUtf8String() == "ITEM_CODE";
            break;
        case 40:
            hasHeight = std::abs(reader->getDouble() - 2.75) < 1e-12;
            break;
        case 70:
            hasFlags = reader->getInt32() == 5;
            break;
        case 72:
            hasAlignment = reader->getInt32() == DRW_Text::HCenter;
            break;
        case 73:
            hasFieldLength = reader->getInt32() == 17;
            break;
        case 100:
            hasAttributeSubclass = hasAttributeSubclass
                || reader->getString() == "AcDbAttribute";
            break;
        case 280:
            sawLockPosition = true;
            hasLockPosition = reader->getInt32() == 0;
            break;
        default:
            break;
        }
    }
    const bool hasExpectedOwner = version <= DRW::AC1014
        ? attributeOwner == DRW::NoHandle : hasAttributeOwner;
    const bool matches = insertCount == 1 && recordCount == 1 && inAttrib
        && hasInsertHandle && hasAttributeHandle && hasExpectedOwner && hasLayer
        && hasTag && hasText && hasHeight && hasAlignment && hasFlags
        && hasFieldLength && hasLockPosition
        && sawLockPosition == (version > DRW::AC1014)
        && (version == DRW::AC1009 || hasAttributeSubclass);
    if (!matches)
        std::cerr << "raw ATTRIB fields: inserts=" << insertCount
                  << " count=" << recordCount
                  << " insertHandle=" << hasInsertHandle
                  << " attribHandle=" << hasAttributeHandle
                  << " owner=" << hasExpectedOwner << " layer=" << hasLayer
                  << " tag=" << hasTag << " text=" << hasText
                  << " height=" << hasHeight << " align=" << hasAlignment
                  << " flags=" << hasFlags << " length=" << hasFieldLength
                  << " subclass=" << hasAttributeSubclass
                  << " lock=" << hasLockPosition << '/' << sawLockPosition
                  << " version=" << static_cast<int>(version) << '\n';
    return matches;
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "ATTRIB writer transaction matrix: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    const std::vector<std::pair<DRW::Version, const char*>> versions = {
        {DRW::AC1009, "AC1009"}, {DRW::AC1014, "AC1014"},
        {DRW::AC1015, "AC1015"}, {DRW::AC1018, "AC1018"},
        {DRW::AC1021, "AC1021"}, {DRW::AC1024, "AC1024"},
        {DRW::AC1027, "AC1027"}, {DRW::AC1032, "AC1032"}};
    const std::string sentinel = "prior ATTRIB destination bytes\n";
    for (const auto& [version, name] : versions) {
        for (const bool binary : {false, true}) {
            const auto output = directory / (std::string("attrib-") + name
                + (binary ? "-binary.dxf" : "-ascii.dxf"));
            std::string expectedBytes;
            long allocationCount = 0;
            {
                dxfRW codec(output.string().c_str());
                Export exporter(codec);
                exporter.failAt = 1000000;
                const bool written = codec.write(&exporter, version, binary);
                const bool bytesRead = readBytes(output, expectedBytes);
                const bool rawMatches = bytesRead
                    && rawRecordMatches(expectedBytes, binary, version);
                const bool typedMatches = bytesRead
                    && readbackMatches(output, version);
                if (!written || !exporter.accepted || !bytesRead
                    || !rawMatches || !typedMatches) {
                    std::cerr << "clean " << name << ' '
                              << (binary ? "Binary" : "ASCII")
                              << ": write=" << written
                              << " callback=" << exporter.accepted
                              << " bytes=" << bytesRead
                              << " raw=" << rawMatches
                              << " typed=" << typedMatches << '\n';
                    return fail("clean write or raw/typed readback failed");
                }
                allocationCount = exporter.reached;
            }
            if (allocationCount <= 0)
                return fail("calibration observed no writer allocations");

            {
                {
                    std::ofstream prior(output,
                                        std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior)
                        return fail("could not seed ownerless refusal sentinel");
                }
                dxfRW codec(output.string().c_str());
                Export exporter(codec);
                const DRW_Attrib source = *exporter.attribute;
                const DRW_Insert sourceInsert = exporter.insert;
                exporter.emitOwnerlessAttrib = true;
                const bool rejected = !codec.write(&exporter, version, binary)
                    && !exporter.accepted
                    && sameInsertSource(exporter.insert, sourceInsert)
                    && sameAttrib(*exporter.attribute, source, true);
                std::string afterRefusal;
                if (!rejected || !readBytes(output, afterRefusal)
                    || afterRefusal != sentinel)
                    return fail("ownerless ATTRIB refusal changed output or caller");

                exporter.emitOwnerlessAttrib = false;
                const bool retried = codec.write(&exporter, version, binary);
                std::string retryBytes;
                if (!retried || !exporter.accepted
                    || !sameInsertSource(exporter.insert, sourceInsert)
                    || !sameAttrib(*exporter.attribute, source, true)
                    || !readBytes(output, retryBytes)
                    || retryBytes != expectedBytes
                    || !rawRecordMatches(retryBytes, binary, version)
                    || !readbackMatches(output, version))
                    return fail("corrected same-codec INSERT retry failed");
                std::cout << "ATTRIB " << name << ' '
                          << (binary ? "Binary" : "ASCII")
                          << ": ownerless direct refusal/corrected INSERT retry=PASS\n";
            }

            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                                        std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior)
                        return fail("could not seed destination sentinel");
                }
                dxfRW codec(output.string().c_str());
                Export exporter(codec);
                const DRW_Attrib source = *exporter.attribute;
                const DRW_Insert sourceInsert = exporter.insert;
                exporter.failAt = offset;
                const bool written = codec.write(&exporter, version, binary);
                std::string afterFailure;
                if (written || exporter.accepted
                    || exporter.reached != offset + 1 || !allocationFailed
                    || !sameInsertSource(exporter.insert, sourceInsert)
                    || !sameAttrib(*exporter.attribute, source, true)
                    || !readBytes(output, afterFailure)
                    || afterFailure != sentinel)
                    return fail("injected failure changed output or caller state");

                exporter.failAt = -1;
                const bool retried = codec.write(&exporter, version, binary);
                std::string retryBytes;
                if (!retried || !exporter.accepted
                    || !sameInsertSource(exporter.insert, sourceInsert)
                    || !sameAttrib(*exporter.attribute, source, true)
                    || !readBytes(output, retryBytes)
                    || retryBytes != expectedBytes
                    || !rawRecordMatches(retryBytes, binary, version)
                    || !readbackMatches(output, version))
                    return fail("same-codec retry or output readback failed");
            }
            std::cout << "ATTRIB " << name << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", atomic refusal/retry/raw+typed readback=PASS\n";
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
    }
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (failAllocation())
        throw std::bad_alloc();
    if (void* value = std::malloc(size == 0u ? 1u : size))
        return value;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }
void operator delete(void* value, std::size_t) noexcept { std::free(value); }
void operator delete[](void* value, std::size_t) noexcept { std::free(value); }

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: dxf_attrib_version_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    const bool passed = run(argv[1]);
    std::cout << "ATTRIB writer transaction matrix: "
              << (passed ? "PASS" : "FAIL") << '\n';
    return passed ? 0 : 1;
}
