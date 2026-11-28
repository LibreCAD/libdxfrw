#include "drw_acis.h"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <vector>

namespace {
std::atomic<int> failAfter{-1};

bool allocationShouldFail() noexcept {
    const int remaining = failAfter.load(std::memory_order_relaxed);
    if (remaining < 0) return false;
    if (remaining == 0) {
        failAfter.store(-1, std::memory_order_relaxed);
        return true;
    }
    failAfter.fetch_sub(1, std::memory_order_relaxed);
    return false;
}

void appendU32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    for (unsigned shift = 0u; shift < 32u; shift += 8u)
        bytes.push_back(static_cast<std::uint8_t>(value >> shift));
}

void appendString(std::vector<std::uint8_t>& bytes, std::uint8_t tag,
                  const char* value, std::size_t length) {
    bytes.push_back(tag);
    bytes.push_back(static_cast<std::uint8_t>(length));
    bytes.insert(bytes.end(), value, value + length);
}

void appendDouble(std::vector<std::uint8_t>& bytes, double value) {
    bytes.push_back(6u);
    std::uint64_t bits = 0u;
    std::memcpy(&bits, &value, sizeof(bits));
    for (unsigned shift = 0u; shift < 64u; shift += 8u)
        bytes.push_back(static_cast<std::uint8_t>(bits >> shift));
}

void appendRawDouble(std::vector<std::uint8_t>& bytes, double value) {
    std::uint64_t bits = 0u;
    std::memcpy(&bits, &value, sizeof(bits));
    for (unsigned shift = 0u; shift < 64u; shift += 8u)
        bytes.push_back(static_cast<std::uint8_t>(bits >> shift));
}

std::vector<std::uint8_t> makeSab(std::uint32_t version = 21200u,
                                  bool addVersionedFloat = false,
                                  std::uint32_t flags = 0u,
                                  bool addVersion700Transform = false,
                                  std::uint32_t numEntities = 1u,
                                  std::uint32_t extraRecords = 0u) {
    std::vector<std::uint8_t> bytes{
        'A','C','I','S',' ','B','i','n','a','r','y','F','i','l','e'};
    appendU32(bytes, version);
    appendU32(bytes, 0u);
    appendU32(bytes, numEntities);
    appendU32(bytes, flags);
    appendString(bytes, 7u, "P", 1u);
    appendString(bytes, 7u, "A", 1u);
    appendString(bytes, 7u, "D", 1u);
    appendDouble(bytes, 1.0);
    appendDouble(bytes, 0.1);
    appendDouble(bytes, 0.01);
    appendString(bytes, 13u, "body", 4u);
    if (version == 700u) {
        bytes.push_back(DRW_SabTag::Int);
        appendU32(bytes, static_cast<std::uint32_t>(-1));
        bytes.push_back(DRW_SabTag::Pointer);
        appendU32(bytes, static_cast<std::uint32_t>(-1));
        bytes.push_back(DRW_SabTag::Pointer);
        appendU32(bytes, 2u);
    }
    bytes.push_back(10u);
    if (addVersionedFloat) {
        bytes.push_back(DRW_SabTag::Unknown0x17);
        appendRawDouble(bytes, 1.25);
    }
    bytes.push_back(17u);
    if (version == 700u) {
        appendString(bytes, DRW_SabTag::EntityType, "plane-surface", 13u);
        bytes.insert(bytes.end(), {DRW_SabTag::BoolFalse,
                                   DRW_SabTag::BoolTrue,
                                   DRW_SabTag::BoolFalse,
                                   DRW_SabTag::BoolTrue,
                                   DRW_SabTag::BoolFalse,
                                   DRW_SabTag::RecordEnd});
        appendString(bytes, DRW_SabTag::EntityType, "coedge", 6u);
        bytes.insert(bytes.end(), {DRW_SabTag::BoolFalse,
                                   DRW_SabTag::RecordEnd});
        appendString(bytes, DRW_SabTag::EntityType, "straight-curve", 14u);
        bytes.insert(bytes.end(), {DRW_SabTag::BoolTrue,
                                   DRW_SabTag::BoolFalse,
                                   DRW_SabTag::RecordEnd});
    }
    if (version == 700u && addVersion700Transform) {
        appendString(bytes, DRW_SabTag::EntityType, "transform", 9u);
        bytes.push_back(DRW_SabTag::Pointer);
        appendU32(bytes, static_cast<std::uint32_t>(-1));
        bytes.push_back(DRW_SabTag::Int);
        appendU32(bytes, static_cast<std::uint32_t>(-1));
        const std::string transform =
            "1 0 0 0 1 0 0 0 1 388.5 388.5 388.5 1 "
            "no_rotate no_reflect no_shear ";
        bytes.push_back(DRW_SabTag::LiteralStr);
        appendU32(bytes, static_cast<std::uint32_t>(transform.size()));
        bytes.insert(bytes.end(), transform.begin(), transform.end());
        bytes.push_back(DRW_SabTag::RecordEnd);
    }
    for (std::uint32_t i = 0u; i < extraRecords; ++i) {
        appendString(bytes, DRW_SabTag::EntityType, "point", 5u);
        bytes.push_back(DRW_SabTag::RecordEnd);
    }
    bytes.insert(bytes.end(), {0x0Eu,0x03u,'E','n','d',
                               0x0Eu,0x02u,'o','f',
                               0x0Eu,0x04u,'A','C','I','S',
                               0x0Du,0x04u,'d','a','t','a'});
    return bytes;
}

std::vector<std::uint8_t> makeModelerSab() {
    std::vector<std::uint8_t> bytes{
        'A','C','I','S',' ','B','i','n','a','r','y','F','i','l','e'};
    appendU32(bytes, 21800u);
    appendU32(bytes, 3u);
    appendU32(bytes, 2u);
    appendU32(bytes, 0u);
    appendString(bytes, 7u, "LOCAL_PRODUCT_IDENTIFIER_WITH_HEAP_STORAGE", 42u);
    appendString(bytes, 7u, "LOCAL_ACIS_VERSION_FOR_ALLOCATION_SWEEP", 39u);
    appendString(bytes, 7u, "LOCAL_CREATION_DATE_FOR_TESTING", 31u);
    appendDouble(bytes, 1.0);
    appendDouble(bytes, 1.0e-6);
    appendDouble(bytes, 1.0e-6);

    appendString(bytes, DRW_SabTag::EntityType, "point", 5u);
    bytes.push_back(DRW_SabTag::Pointer);
    appendU32(bytes, static_cast<std::uint32_t>(-1));
    bytes.push_back(DRW_SabTag::Int);
    appendU32(bytes, 101u);
    bytes.push_back(DRW_SabTag::LocationVec);
    appendRawDouble(bytes, 1.0);
    appendRawDouble(bytes, 2.0);
    appendRawDouble(bytes, 3.0);
    bytes.push_back(DRW_SabTag::RecordEnd);

    appendString(bytes, DRW_SabTag::EntityType, "vertex", 6u);
    bytes.push_back(DRW_SabTag::Pointer);
    appendU32(bytes, static_cast<std::uint32_t>(-1));
    bytes.push_back(DRW_SabTag::Int);
    appendU32(bytes, 202u);
    bytes.push_back(DRW_SabTag::Pointer);
    appendU32(bytes, 0u);
    bytes.push_back(DRW_SabTag::RecordEnd);

    appendString(bytes, DRW_SabTag::EntityType, "End-of-ACIS-data", 16u);
    bytes.push_back(DRW_SabTag::RecordEnd);
    return bytes;
}
} // namespace

void* operator new(std::size_t size) {
    if (allocationShouldFail()) throw std::bad_alloc();
    if (void* memory = std::malloc(size == 0u ? 1u : size)) return memory;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) {
    if (allocationShouldFail()) throw std::bad_alloc();
    if (void* memory = std::malloc(size == 0u ? 1u : size)) return memory;
    throw std::bad_alloc();
}

void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

int main() {
    // The third word's meaning is version-specific and unresolved for 21500.
    // Retain it as raw metadata; it must not be used as a SAB record limit.
    const std::vector<std::uint8_t> sab21500EntityWord =
        makeSab(21500u, false, 0x18u, false, 2u, 3u);
    DRW_SabData parsed21500EntityWord;
    DRW_SabParseOutcome outcome21500EntityWord;
    if (!drw_parseSabWithOutcome(sab21500EntityWord.data(),
                                 sab21500EntityWord.size(),
                                 parsed21500EntityWord,
                                 outcome21500EntityWord)
        || !outcome21500EntityWord.complete
        || outcome21500EntityWord.recordCountKnown
        || !outcome21500EntityWord.terminalRecordSeen
        || parsed21500EntityWord.header.version != 21500
        || parsed21500EntityWord.header.numRecords != 0
        || parsed21500EntityWord.header.numEntities != 2
        || parsed21500EntityWord.records.size() != 5u
        || parsed21500EntityWord.records.back().type != "End-of-ACIS-data") {
        std::fprintf(stderr,
                     "ACIS 21500 raw entity word altered record framing\n");
        return 20;
    }
    std::vector<std::uint8_t> sat21500EntityWord;
    if (!drw_convertSabToSatV1(sab21500EntityWord.data(),
                               sab21500EntityWord.size(), DRW::AC1024,
                               sat21500EntityWord)
        || std::string(sat21500EntityWord.begin(),
                       sat21500EntityWord.end()).find("21500 4 2 0 \n")
            != 0u) {
        std::fprintf(stderr,
                     "ACIS 21500 raw header word changed in SAT output\n");
        return 21;
    }

    const std::vector<std::uint8_t> sab700 = makeSab(700u);
    DRW_SabData parsed700;
    DRW_SabParseOutcome outcome700;
    if (!drw_parseSabWithOutcome(sab700.data(), sab700.size(), parsed700,
                                 outcome700)
        || !outcome700.complete || outcome700.recordCountKnown
        || !outcome700.terminalRecordSeen || parsed700.records.size() != 5u
        || parsed700.header.version != 700
        || parsed700.header.numRecords != 0
        || parsed700.records.back().type != "End-of-ACIS-data") {
        std::fprintf(stderr,
                     "ACIS 700 zero-count terminal framing mismatch\n");
        return 9;
    }
    std::vector<std::uint8_t> converted700;
    const std::string expected700Prefix =
        "700 4 1 0 \n@1 P @1 A @1 D \n"
        "1 0.10000000000000001 0.01 \nbody -1 $-1 $2 F #\n";
    const bool converted700Ok = drw_convertSabToSatV1(
        sab700.data(), sab700.size(), DRW::AC1024, converted700);
    const std::string converted700Text(converted700.begin(),
                                       converted700.end());
    if (!converted700Ok || converted700Text.find(expected700Prefix) != 0u
        || converted700Text.find(
               "plane-surface forward_v F I F I #") == std::string::npos
        || converted700Text.find("coedge forward #") == std::string::npos
        || converted700Text.find("straight-curve F I #")
               == std::string::npos) {
        const std::string actual700(converted700.begin(), converted700.end());
        std::fprintf(stderr, "ACIS 700 AC1024 SAT conversion mismatch: %s\n",
                     actual700.substr(0u, 160u).c_str());
        return 11;
    }
    const std::vector<std::uint8_t> sab700Transform =
        makeSab(700u, false, 0u, true);
    std::vector<std::uint8_t> converted700Transform;
    const std::string expectedTransform =
        "transform $-1 -1 1 0 0 0 1 0 0 0 1 388.5 388.5 388.5 1 "
        "no_rotate no_reflect no_shear";
    if (!drw_convertSabToSatV1(sab700Transform.data(),
                               sab700Transform.size(), DRW::AC1024,
                               converted700Transform)
        || std::string(converted700Transform.begin(),
                       converted700Transform.end()).find(expectedTransform)
            == std::string::npos) {
        const std::string actual(converted700Transform.begin(),
                                 converted700Transform.end());
        std::fprintf(stderr,
                     "ACIS 700 body-transform SAT serialization mismatch: %s\n",
                     actual.c_str());
        return 22;
    }
    std::vector<std::uint8_t> refused700{0xA5u, 0x5Au};
    if (drw_convertSabToSatV1(sab700.data(), sab700.size(), DRW::AC1027,
                              refused700)
        || refused700 != std::vector<std::uint8_t>({0xA5u, 0x5Au})) {
        std::fprintf(stderr,
                     "ACIS 700 conversion escaped its AC1024 profile\n");
        return 12;
    }
    int first700Success = -1;
    for (int failIndex = 0; failIndex < 512; ++failIndex) {
        std::vector<std::uint8_t> output{0xA5u, 0x5Au};
        failAfter.store(failIndex, std::memory_order_relaxed);
        bool converted = false;
        bool threw = false;
        try {
            converted = drw_convertSabToSatV1(
                sab700.data(), sab700.size(), DRW::AC1024, output);
        } catch (...) {
            threw = true;
        }
        failAfter.store(-1, std::memory_order_relaxed);
        if (threw) {
            std::fprintf(stderr,
                         "ACIS 700 allocation %d escaped as an exception\n",
                         failIndex);
            return 13;
        }
        if (converted) {
            if (output != converted700) {
                std::fprintf(stderr,
                             "ACIS 700 allocation sweep output mismatch\n");
                return 14;
            }
            first700Success = failIndex;
            break;
        }
        if (output != std::vector<std::uint8_t>({0xA5u, 0x5Au})) {
            std::fprintf(stderr,
                         "ACIS 700 allocation %d changed output on failure\n",
                         failIndex);
            return 15;
        }
        if (!drw_convertSabToSatV1(sab700.data(), sab700.size(),
                                   DRW::AC1024, output)
            || output != converted700) {
            std::fprintf(stderr,
                         "ACIS 700 allocation %d same-buffer retry failed\n",
                         failIndex);
            return 16;
        }
    }
    if (first700Success < 8) {
        std::fprintf(stderr, "ACIS 700 allocation sweep too short: %d\n",
                     first700Success);
        return 17;
    }
    std::vector<std::uint8_t> sab700WithoutTerminal = sab700;
    sab700WithoutTerminal.resize(sab700WithoutTerminal.size() - 19u);
    DRW_SabData partial700;
    DRW_SabParseOutcome partialOutcome700;
    if (drw_parseSabWithOutcome(sab700WithoutTerminal.data(),
                                sab700WithoutTerminal.size(), partial700,
                                partialOutcome700)
        || partialOutcome700.complete || partialOutcome700.terminalRecordSeen
        || !partial700.records.empty()
        || drw_parseSab(sab700WithoutTerminal.data(),
                        sab700WithoutTerminal.size(), partial700)) {
        std::fprintf(stderr,
                     "ACIS 700 missing-terminal mismatch: complete=%d "
                     "terminal=%d records=%zu\n",
                     partialOutcome700.complete ? 1 : 0,
                     partialOutcome700.terminalRecordSeen ? 1 : 0,
                     partial700.records.size());
        return 10;
    }

    const std::vector<std::uint8_t> sab = makeSab();
    std::vector<std::uint8_t> expected;
    if (!drw_convertSabToSatV1(sab.data(), sab.size(), DRW::AC1024, expected)
        || expected.empty()) {
        std::fprintf(stderr, "baseline SAT conversion failed\n");
        return 1;
    }

    const std::vector<std::uint8_t> sab21500 = makeSab(21500u, true, 0x18u);
    std::vector<std::uint8_t> converted21500;
    if (!drw_convertSabToSatV1(sab21500.data(), sab21500.size(),
                               DRW::AC1024, converted21500)
        || converted21500.empty()) {
        std::fprintf(stderr, "ACIS 21500 SAT conversion failed\n");
        return 7;
    }
    const std::string text21500(converted21500.begin(), converted21500.end());
    if (text21500.find("21500 1 1 0") != 0u
        || text21500.find("1.25 ") == std::string::npos
        || text21500.find("1065353216") != std::string::npos) {
        std::fprintf(stderr, "ACIS 21500 versioned float serialization mismatch\n");
        return 8;
    }

    int firstSuccess = -1;
    for (int failIndex = 0; failIndex < 512; ++failIndex) {
        std::vector<std::uint8_t> output{0xA5u, 0x5Au};
        failAfter.store(failIndex, std::memory_order_relaxed);
        bool converted = false;
        bool threw = false;
        try {
            converted = drw_convertSabToSatV1(
                sab.data(), sab.size(), DRW::AC1024, output);
        } catch (...) {
            threw = true;
        }
        failAfter.store(-1, std::memory_order_relaxed);

        if (threw) {
            std::fprintf(stderr, "allocation %d escaped as an exception\n",
                         failIndex);
            return 2;
        }
        if (converted) {
            if (output != expected) {
                std::fprintf(stderr, "allocation sweep output mismatch\n");
                return 3;
            }
            firstSuccess = failIndex;
            break;
        }
        if (output != std::vector<std::uint8_t>({0xA5u, 0x5Au})) {
            std::fprintf(stderr, "allocation %d changed output on failure\n",
                         failIndex);
            return 4;
        }
        if (!drw_convertSabToSatV1(sab.data(), sab.size(), DRW::AC1024,
                                   output)
            || output != expected) {
            std::fprintf(stderr, "allocation %d same-buffer retry failed\n",
                         failIndex);
            return 5;
        }
    }

    if (firstSuccess < 8) {
        std::fprintf(stderr, "allocation sweep too short: %d\n", firstSuccess);
        return 6;
    }

    const std::vector<std::uint8_t> modelerSab = makeModelerSab();
    DRW_AcisBrep expectedBrep;
    DRW_SabParseOutcome expectedOutcome;
    if (!drw_decodeAcisWireframeWithOutcome(modelerSab, expectedBrep,
                                            expectedOutcome)
        || !expectedOutcome.complete || expectedBrep.vertices.size() != 1u
        || !expectedBrep.vertices.front().valid
        || expectedBrep.vertices.front().point.x != 1.0
        || expectedBrep.vertices.front().point.y != 2.0
        || expectedBrep.vertices.front().point.z != 3.0
        || !expectedBrep.hasBBox || expectedBrep.bboxMin.x != 1.0
        || expectedBrep.bboxMin.y != 2.0 || expectedBrep.bboxMin.z != 3.0
        || expectedBrep.bboxMax.x != 1.0 || expectedBrep.bboxMax.y != 2.0
        || expectedBrep.bboxMax.z != 3.0) {
        std::fprintf(stderr,
                     "baseline ACIS decode failed: complete=%d vertices=%zu "
                     "bbox=%d diagnostics=%zu\n",
                     expectedOutcome.complete ? 1 : 0,
                     expectedBrep.vertices.size(), expectedBrep.hasBBox ? 1 : 0,
                     expectedBrep.diagnostics.size());
        return 7;
    }
    const auto matchesExpectedBrep = [&expectedBrep](const DRW_AcisBrep& value) {
        return value.vertices.size() == expectedBrep.vertices.size()
            && value.vertices.front().point.x
                == expectedBrep.vertices.front().point.x
            && value.vertices.front().point.y
                == expectedBrep.vertices.front().point.y
            && value.vertices.front().point.z
                == expectedBrep.vertices.front().point.z
            && value.edges.size() == expectedBrep.edges.size()
            && value.faces.size() == expectedBrep.faces.size()
            && value.lumps.size() == expectedBrep.lumps.size()
            && value.shells.size() == expectedBrep.shells.size()
            && value.hasBBox == expectedBrep.hasBBox
            && value.bboxMin.x == expectedBrep.bboxMin.x
            && value.bboxMin.y == expectedBrep.bboxMin.y
            && value.bboxMin.z == expectedBrep.bboxMin.z
            && value.bboxMax.x == expectedBrep.bboxMax.x
            && value.bboxMax.y == expectedBrep.bboxMax.y
            && value.bboxMax.z == expectedBrep.bboxMax.z
            && value.diagnostics == expectedBrep.diagnostics;
    };
    int decodeFirstSuccess = -1;
    for (int failIndex = 0; failIndex < 512; ++failIndex) {
        DRW_AcisBrep output;
        DRW_AcisVertex sentinel;
        sentinel.nodeIndex = 99;
        sentinel.valid = true;
        sentinel.point.x = 99.0;
        output.vertices.push_back(sentinel);
        output.hasBBox = true;
        output.diagnostics.push_back("sentinel");
        DRW_SabParseOutcome outcome;
        outcome.complete = true;
        outcome.sourceByteCount = 99u;

        failAfter.store(failIndex, std::memory_order_relaxed);
        bool decoded = false;
        bool threw = false;
        try {
            decoded = drw_decodeAcisWireframeWithOutcome(
                modelerSab, output, outcome);
        } catch (...) {
            threw = true;
        }
        failAfter.store(-1, std::memory_order_relaxed);

        if (threw) {
            std::fprintf(stderr, "ACIS decode allocation %d escaped\n",
                         failIndex);
            return 8;
        }
        if (decoded) {
            if (!matchesExpectedBrep(output) || !outcome.complete) {
                std::fprintf(stderr, "ACIS decode output mismatch\n");
                return 9;
            }
            decodeFirstSuccess = failIndex;
            break;
        }
        if (!output.empty() || output.hasBBox || !output.diagnostics.empty()
            || !output.vertices.empty() || !output.edges.empty()
            || !output.faces.empty() || !output.lumps.empty()
            || !output.shells.empty()
            || !outcome.allocationFailure
            || outcome.sourceByteCount != modelerSab.size()) {
            std::fprintf(stderr,
                         "ACIS decode allocation %d rollback mismatch: "
                         "empty=%d bbox=%d diag=%zu vertices=%zu edges=%zu "
                         "faces=%zu lumps=%zu shells=%zu complete=%d "
                         "alloc=%d bytes=%zu expected=%zu\n",
                         failIndex, output.empty() ? 1 : 0,
                         output.hasBBox ? 1 : 0, output.diagnostics.size(),
                         output.vertices.size(), output.edges.size(),
                         output.faces.size(), output.lumps.size(),
                         output.shells.size(), outcome.complete ? 1 : 0,
                         outcome.allocationFailure ? 1 : 0,
                         outcome.sourceByteCount, modelerSab.size());
            return 10;
        }
        if (!drw_decodeAcisWireframeWithOutcome(modelerSab, output, outcome)
            || !outcome.complete || !matchesExpectedBrep(output)) {
            std::fprintf(stderr, "ACIS decode allocation %d retry failed\n",
                         failIndex);
            return 11;
        }
    }
    if (decodeFirstSuccess < 8) {
        std::fprintf(stderr, "ACIS decode allocation sweep too short: %d\n",
                     decodeFirstSuccess);
        return 12;
    }

    DRW_AcisBrep headerBrep;
    DRW_SabHeader expectedHeader;
    DRW_SabParseOutcome headerOutcome;
    if (!drw_decodeAcisWireframeWithHeader(
            modelerSab, headerBrep, expectedHeader, headerOutcome,
            DRW_SabParseLimits())
        || !headerOutcome.complete
        || expectedHeader.signature != "ACIS BinaryFile"
        || expectedHeader.version != 21800
        || expectedHeader.numRecords != 3
        || expectedHeader.numEntities != 2
        || expectedHeader.productId
            != "LOCAL_PRODUCT_IDENTIFIER_WITH_HEAP_STORAGE"
        || expectedHeader.acisVersion
            != "LOCAL_ACIS_VERSION_FOR_ALLOCATION_SWEEP"
        || expectedHeader.creationDate != "LOCAL_CREATION_DATE_FOR_TESTING") {
        std::fprintf(stderr, "baseline ACIS header decode failed\n");
        return 13;
    }
    const auto matchesExpectedHeader = [&expectedHeader](
            const DRW_SabHeader& value) {
        return value.signature == expectedHeader.signature
            && value.version == expectedHeader.version
            && value.numRecords == expectedHeader.numRecords
            && value.numEntities == expectedHeader.numEntities
            && value.flags == expectedHeader.flags
            && value.productId == expectedHeader.productId
            && value.acisVersion == expectedHeader.acisVersion
            && value.creationDate == expectedHeader.creationDate
            && value.unitsInMm == expectedHeader.unitsInMm
            && value.resTol == expectedHeader.resTol
            && value.norTol == expectedHeader.norTol;
    };
    const auto isResetHeader = [](const DRW_SabHeader& value) {
        return value.signature.empty() && value.version == 0
            && value.numRecords == 0 && value.numEntities == 0
            && value.flags == 0 && value.productId.empty()
            && value.acisVersion.empty() && value.creationDate.empty()
            && value.unitsInMm == 1.0 && value.resTol == 0.0
            && value.norTol == 0.0;
    };
    int headerFirstSuccess = -1;
    for (int failIndex = 0; failIndex < 512; ++failIndex) {
        DRW_AcisBrep output;
        DRW_AcisVertex sentinel;
        sentinel.nodeIndex = 99;
        sentinel.valid = true;
        sentinel.point.x = 99.0;
        output.vertices.push_back(sentinel);
        output.hasBBox = true;
        output.diagnostics.push_back("sentinel");
        DRW_SabHeader header;
        header.signature = "caller-signature";
        header.version = 99;
        header.productId = "caller-product-id";
        header.acisVersion = "caller-acis-version";
        header.creationDate = "caller-date";
        DRW_SabParseOutcome outcome;
        outcome.complete = true;
        outcome.sourceByteCount = 99u;

        failAfter.store(failIndex, std::memory_order_relaxed);
        bool decoded = false;
        bool threw = false;
        try {
            decoded = drw_decodeAcisWireframeWithHeader(
                modelerSab, output, header, outcome, DRW_SabParseLimits());
        } catch (...) {
            threw = true;
        }
        failAfter.store(-1, std::memory_order_relaxed);

        if (threw) {
            std::fprintf(stderr, "ACIS header allocation %d escaped\n",
                         failIndex);
            return 14;
        }
        if (decoded) {
            if (!matchesExpectedBrep(output) || !outcome.complete
                || !matchesExpectedHeader(header)) {
                std::fprintf(stderr, "ACIS header decode output mismatch\n");
                return 15;
            }
            headerFirstSuccess = failIndex;
            break;
        }
        if (!output.empty() || !output.vertices.empty() || output.hasBBox
            || !output.diagnostics.empty() || !isResetHeader(header)
            || !outcome.allocationFailure
            || outcome.sourceByteCount != modelerSab.size()) {
            std::fprintf(stderr,
                         "ACIS header allocation %d did not roll back\n",
                         failIndex);
            return 16;
        }
        if (!drw_decodeAcisWireframeWithHeader(
                modelerSab, output, header, outcome, DRW_SabParseLimits())
            || !outcome.complete || !matchesExpectedBrep(output)
            || !matchesExpectedHeader(header)) {
            std::fprintf(stderr, "ACIS header allocation %d retry failed\n",
                         failIndex);
            return 17;
        }
    }
    if (headerFirstSuccess < 8) {
        std::fprintf(stderr, "ACIS header allocation sweep too short: %d\n",
                     headerFirstSuccess);
        return 18;
    }

    DRW_SabData expectedSab;
    DRW_SabParseOutcome expectedSabOutcome;
    if (!drw_parseSabWithOutcome(modelerSab.data(), modelerSab.size(),
                                 expectedSab, expectedSabOutcome)
        || !expectedSabOutcome.complete
        || !matchesExpectedHeader(expectedSab.header)
        || expectedSab.records.size() != 3u
        || expectedSab.records[0].type != "point"
        || expectedSab.records[0].tokens.size() != 4u
        || expectedSab.records[0].tokens[1].ival != -1
        || expectedSab.records[0].tokens[2].ival != 101
        || expectedSab.records[0].tokens[3].vec.x != 1.0
        || expectedSab.records[0].tokens[3].vec.y != 2.0
        || expectedSab.records[0].tokens[3].vec.z != 3.0
        || expectedSab.records[1].type != "vertex"
        || expectedSab.records[1].tokens.size() != 4u
        || expectedSab.records[1].tokens[1].ival != -1
        || expectedSab.records[1].tokens[2].ival != 202
        || expectedSab.records[1].tokens[3].ival != 0
        || expectedSab.records[2].type != "End-of-ACIS-data") {
        std::fprintf(stderr,
                     "baseline raw SAB parse failed: complete=%d records=%zu "
                     "types=%s,%s,%s tokenCounts=%zu,%zu,%zu "
                     "pointId=%lld vertexId=%lld point=(%.17g,%.17g,%.17g) "
                     "vertexRef=%lld\n",
                     expectedSabOutcome.complete ? 1 : 0,
                     expectedSab.records.size(),
                     expectedSab.records.size() > 0u
                         ? expectedSab.records[0].type.c_str() : "-",
                     expectedSab.records.size() > 1u
                         ? expectedSab.records[1].type.c_str() : "-",
                     expectedSab.records.size() > 2u
                         ? expectedSab.records[2].type.c_str() : "-",
                     expectedSab.records.size() > 0u
                         ? expectedSab.records[0].tokens.size() : 0u,
                     expectedSab.records.size() > 1u
                         ? expectedSab.records[1].tokens.size() : 0u,
                     expectedSab.records.size() > 2u
                         ? expectedSab.records[2].tokens.size() : 0u,
                     expectedSab.records.size() > 0u
                         && expectedSab.records[0].tokens.size() > 2u
                         ? expectedSab.records[0].tokens[2].ival : -1LL,
                     expectedSab.records.size() > 1u
                         && expectedSab.records[1].tokens.size() > 2u
                         ? expectedSab.records[1].tokens[2].ival : -1LL,
                     expectedSab.records.size() > 0u
                         && expectedSab.records[0].tokens.size() > 3u
                         ? expectedSab.records[0].tokens[3].vec.x : 0.0,
                     expectedSab.records.size() > 0u
                         && expectedSab.records[0].tokens.size() > 3u
                         ? expectedSab.records[0].tokens[3].vec.y : 0.0,
                     expectedSab.records.size() > 0u
                         && expectedSab.records[0].tokens.size() > 3u
                         ? expectedSab.records[0].tokens[3].vec.z : 0.0,
                     expectedSab.records.size() > 1u
                         && expectedSab.records[1].tokens.size() > 3u
                         ? expectedSab.records[1].tokens[3].ival : -1LL);
        return 19;
    }
    const auto matchesExpectedSab = [&expectedSab, &matchesExpectedHeader](
            const DRW_SabData& value) {
        return matchesExpectedHeader(value.header)
            && value.records.size() == expectedSab.records.size()
            && value.records[0].type == expectedSab.records[0].type
            && value.records[0].tokens.size() == 4u
            && value.records[0].tokens[3].vec.x == 1.0
            && value.records[0].tokens[3].vec.y == 2.0
            && value.records[0].tokens[3].vec.z == 3.0
            && value.records[1].type == expectedSab.records[1].type
            && value.records[1].tokens.size() == 4u
            && value.records[1].tokens[3].ival == 0
            && value.records[2].type == expectedSab.records[2].type;
    };
    int parseFirstSuccess = -1;
    for (int failIndex = 0; failIndex < 512; ++failIndex) {
        DRW_SabData output;
        output.header.signature = "caller-signature";
        output.header.productId = "caller-product-id";
        output.records.push_back(DRW_SabRecord());
        output.records.back().type = "caller-record";
        DRW_SabParseOutcome outcome;
        outcome.complete = true;
        outcome.sourceByteCount = 99u;

        failAfter.store(failIndex, std::memory_order_relaxed);
        bool parsed = false;
        bool threw = false;
        try {
            parsed = drw_parseSabWithOutcome(
                modelerSab.data(), modelerSab.size(), output, outcome);
        } catch (...) {
            threw = true;
        }
        failAfter.store(-1, std::memory_order_relaxed);

        if (threw) {
            std::fprintf(stderr, "raw SAB parse allocation %d escaped\n",
                         failIndex);
            return 20;
        }
        if (parsed) {
            if (!matchesExpectedSab(output) || !outcome.complete) {
                std::fprintf(stderr, "raw SAB parse output mismatch\n");
                return 21;
            }
            parseFirstSuccess = failIndex;
            break;
        }
        if (!output.records.empty() || !isResetHeader(output.header)
            || !outcome.allocationFailure || outcome.complete
            || outcome.sourceByteCount != modelerSab.size()) {
            std::fprintf(stderr,
                         "raw SAB parse allocation %d did not roll back\n",
                         failIndex);
            return 22;
        }
        if (!drw_parseSabWithOutcome(modelerSab.data(), modelerSab.size(),
                                     output, outcome)
            || !matchesExpectedSab(output) || !outcome.complete) {
            std::fprintf(stderr, "raw SAB parse allocation %d retry failed\n",
                         failIndex);
            return 23;
        }
    }
    if (parseFirstSuccess < 8) {
        std::fprintf(stderr, "raw SAB parse sweep too short: %d\n",
                     parseFirstSuccess);
        return 24;
    }

    DRW_AcisModel expectedModel;
    DRW_SabParseOutcome expectedModelOutcome;
    if (!drw_buildAcisModelWithLimits(expectedSab, expectedModel,
                                      expectedModelOutcome,
                                      DRW_SabParseLimits())
        || !matchesExpectedHeader(expectedModel.header)
        || expectedModel.nodes.size() != 3u
        || expectedModel.nodes[0].type != "point"
        || expectedModel.nodes[0].refs.size() != 0u
        || expectedModel.nodes[1].type != "vertex"
        || expectedModel.nodes[1].refs.size() != 1u
        || expectedModel.nodes[1].refs[0] != 0
        || expectedModel.nodes[2].type != "End-of-ACIS-data") {
        std::fprintf(stderr, "baseline ACIS graph build failed\n");
        return 25;
    }
    const auto matchesExpectedModel = [&expectedModel,
                                       &matchesExpectedHeader](
            const DRW_AcisModel& value) {
        return matchesExpectedHeader(value.header)
            && value.nodes.size() == expectedModel.nodes.size()
            && value.nodes[0].type == expectedModel.nodes[0].type
            && value.nodes[0].refs == expectedModel.nodes[0].refs
            && value.nodes[1].type == expectedModel.nodes[1].type
            && value.nodes[1].refs == expectedModel.nodes[1].refs
            && value.nodes[2].type == expectedModel.nodes[2].type
            && value.nodes[2].refs == expectedModel.nodes[2].refs;
    };
    int modelFirstSuccess = -1;
    for (int failIndex = 0; failIndex < 512; ++failIndex) {
        DRW_AcisModel output;
        output.header.signature = "caller-signature";
        output.nodes.push_back(DRW_AcisNode());
        output.nodes.back().type = "caller-node";
        DRW_SabParseOutcome outcome;
        outcome.complete = true;
        outcome.graphItemsAccounted = 99u;

        failAfter.store(failIndex, std::memory_order_relaxed);
        bool built = false;
        bool threw = false;
        try {
            built = drw_buildAcisModelWithLimits(
                expectedSab, output, outcome, DRW_SabParseLimits());
        } catch (...) {
            threw = true;
        }
        failAfter.store(-1, std::memory_order_relaxed);

        if (threw) {
            std::fprintf(stderr, "ACIS graph allocation %d escaped\n",
                         failIndex);
            return 26;
        }
        if (built) {
            if (!matchesExpectedModel(output)) {
                std::fprintf(stderr, "ACIS graph output mismatch\n");
                return 27;
            }
            modelFirstSuccess = failIndex;
            break;
        }
        if (!output.nodes.empty() || !isResetHeader(output.header)
            || !outcome.allocationFailure || outcome.complete) {
            std::fprintf(stderr,
                         "ACIS graph allocation %d did not roll back\n",
                         failIndex);
            return 28;
        }
        if (!drw_buildAcisModelWithLimits(
                expectedSab, output, outcome, DRW_SabParseLimits())
            || !matchesExpectedModel(output)) {
            std::fprintf(stderr, "ACIS graph allocation %d retry failed\n",
                         failIndex);
            return 29;
        }
    }
    if (modelFirstSuccess < 8) {
        std::fprintf(stderr, "ACIS graph allocation sweep too short: %d\n",
                     modelFirstSuccess);
        return 30;
    }
    std::printf("SAT allocation rollback/retry passed at %d points; "
                "ACIS decode rollback/retry passed at %d points; "
                "ACIS header decode rollback/retry passed at %d points; "
                "raw SAB parse rollback/retry passed at %d points; "
                "ACIS graph rollback/retry passed at %d points\n",
                firstSuccess, decodeFirstSuccess, headerFirstSuccess,
                parseFirstSuccess, modelFirstSuccess);
    return 0;
}
