#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
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
#include "intern/dwgtemplate.h"
#include "intern/dwgheadersection.h"
#include "intern/dwgobjectframe.h"
#include "intern/dwgwriter15.h"
#include "intern/dwgwriter18.h"
#include "intern/dwgwriter21.h"
#include "intern/dwgwriter24.h"
#include "intern/dwgwriter27.h"
#include "intern/dwgwriter32.h"
#include "intern/dwgsafety.h"

class DwgBlockOwnershipTestAccess {
public:
    static void setEntityHandles(DRW_Block_Record& record,
                                 std::vector<std::uint32_t> handles) {
        record.entMap = std::move(handles);
    }
};

namespace {

static_assert(static_cast<unsigned>(DwgIntegrityCheckKind::TemplateDescriptionUnsupported) == 20u,
              "existing Template integrity categories must not be renumbered");
static_assert(static_cast<unsigned>(DwgIntegrityCheckKind::HeaderSectionBounds) == 21u
                  && static_cast<unsigned>(DwgIntegrityCheckKind::HeaderSectionSentinel) == 22u
                  && static_cast<unsigned>(DwgIntegrityCheckKind::HeaderSectionCrc) == 23u,
              "HEADER integrity categories append to the stable diagnostic contract");
static_assert(static_cast<unsigned>(DwgIntegrityCheckKind::HeaderSectionCompatibility) == 24u
                  && static_cast<unsigned>(DwgIntegrityCheckKind::HeaderSectionUnclaimedData) == 25u,
              "compatible HEADER observations append without renumbering prior categories");
static_assert(static_cast<unsigned>(DwgIntegrityCheckKind::NamedObjectDictionaryReference) == 26u,
              "NOD reference observations append without renumbering prior categories");

struct TestContext {
    int failures {0};

    void expect(bool condition, const std::string& label) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << label << '\n';
        }
    }
};

struct RangeComparisonCounter {
    explicit RangeComparisonCounter(std::size_t* count = nullptr) : count(count) {}
    bool operator()(std::uint64_t lhs, std::uint64_t rhs) const noexcept {
        if (count != nullptr)
            ++*count;
        return lhs < rhs;
    }
    std::size_t* count;
};

struct NameComparisonCounter {
    explicit NameComparisonCounter(std::size_t* count = nullptr) : count(count) {}
    bool operator()(const std::string& lhs, const std::string& rhs) const noexcept {
        if (count != nullptr)
            ++*count;
        return lhs < rhs;
    }
    std::size_t* count;
};

struct RangeAllocationFailure {
    static bool failNext;
};

bool RangeAllocationFailure::failNext = false;

template <typename T>
struct ThrowOnceAllocator {
    using value_type = T;
    template <typename U> struct rebind { using other = ThrowOnceAllocator<U>; };
    ThrowOnceAllocator() = default;
    template <typename U>
    ThrowOnceAllocator(const ThrowOnceAllocator<U>&) noexcept {}

    T* allocate(std::size_t count) {
        if (RangeAllocationFailure::failNext) {
            RangeAllocationFailure::failNext = false;
            throw std::bad_alloc();
        }
        return std::allocator<T>{}.allocate(count);
    }
    void deallocate(T* value, std::size_t count) noexcept {
        std::allocator<T>{}.deallocate(value, count);
    }
    template <typename U> bool operator==(const ThrowOnceAllocator<U>&) const noexcept {
        return true;
    }
    template <typename U> bool operator!=(const ThrowOnceAllocator<U>&) const noexcept {
        return false;
    }
};

void testDisjointRangeIndex(TestContext& t) {
    using Range = std::pair<std::uint64_t, std::uint64_t>;
    std::vector<Range> candidates;
    for (std::uint64_t start = 0; start < 6; ++start) {
        for (std::uint64_t end = start + 1; end <= 6; ++end)
            candidates.emplace_back(start, end);
    }

    bool differentialMatch = true;
    for (const auto& first : candidates) {
        for (const auto& second : candidates) {
            for (const auto& third : candidates) {
                dwgSafety::DisjointRangeIndex<> index;
                std::vector<Range> reference;
                for (const auto& range : {first, second, third}) {
                    const bool expected = std::none_of(
                        reference.cbegin(), reference.cend(),
                        [&range](const Range& existing) {
                            return range.first < existing.second
                                && existing.first < range.second;
                        });
                    const bool observed = dwgSafety::tryInsertSectionPageRange(
                        index, range.first, range.second - range.first)
                        == dwgSafety::SectionPageRangeResult::Inserted;
                    differentialMatch = differentialMatch
                        && (observed == expected);
                    if (expected)
                        reference.push_back(range);
                }
            }
        }
    }
    t.expect(differentialMatch,
             "ordered interval admission matches first-arrival vector semantics for 9,261 triples");

    dwgSafety::DisjointRangeIndex<> boundaries;
    t.expect(dwgSafety::tryInsertSectionPageRange(boundaries, 0, 4)
                     == dwgSafety::SectionPageRangeResult::Inserted
                 && dwgSafety::tryInsertSectionPageRange(boundaries, 4, 5)
                     == dwgSafety::SectionPageRangeResult::Inserted
                 && dwgSafety::tryInsertSectionPageRange(boundaries, 3, 2)
                     == dwgSafety::SectionPageRangeResult::Overlap
                 && dwgSafety::tryInsertSectionPageRange(boundaries, 0, 1)
                     == dwgSafety::SectionPageRangeResult::Overlap
                 && dwgSafety::tryInsertSectionPageRange(boundaries, 8, 2)
                     == dwgSafety::SectionPageRangeResult::Overlap
                 && dwgSafety::tryInsertSectionPageRange(boundaries, 7, 0)
                     == dwgSafety::SectionPageRangeResult::Empty,
             "interval index accepts adjacency and rejects predecessor/successor overlap and empty ranges");
    t.expect(dwgSafety::tryInsertSectionPageRange(
                 boundaries, (std::numeric_limits<std::uint64_t>::max)(), 1)
                 == dwgSafety::SectionPageRangeResult::Overflow,
             "checked section range admission rejects overflowing interval ends");

    const auto compareWork = [](std::size_t count, int order) {
        std::size_t comparisons = 0;
        dwgSafety::DisjointRangeIndex<RangeComparisonCounter> index(
            RangeComparisonCounter{&comparisons});
        for (std::size_t i = 0; i < count; ++i) {
            std::size_t slot = i;
            if (order == 1)
                slot = count - i - 1;
            else if (order == 2)
                slot = (i * 197u) % count;
            if (!index.insert(slot * 4u, slot * 4u + 2u))
                return std::numeric_limits<std::size_t>::max();
        }
        return comparisons;
    };
    for (int order = 0; order < 3; ++order) {
        const std::size_t small = compareWork(128, order);
        const std::size_t large = compareWork(512, order);
        t.expect(small < 128u * 40u && large < 512u * 40u
                     && large < small * 8u,
                 "128/512 interval comparison envelopes remain ordered-logarithmic across insertion orders");
    }

    using FailingRangeIndex = dwgSafety::DisjointRangeIndex<
        std::less<std::uint64_t>,
        ThrowOnceAllocator<std::pair<const std::uint64_t, std::uint64_t>>>;
    FailingRangeIndex recoverable;
    const bool firstInserted = dwgSafety::tryInsertSectionPageRange(
        recoverable, 0, 2)
        == dwgSafety::SectionPageRangeResult::Inserted;
    RangeAllocationFailure::failNext = true;
    const bool reportedResourceFailure = dwgSafety::tryInsertSectionPageRange(
        recoverable, 4, 6)
        == dwgSafety::SectionPageRangeResult::ResourceFailure;
    const bool retainedAfterFailure = recoverable.size() == 1;
    const bool recovered = dwgSafety::tryInsertSectionPageRange(
        recoverable, 4, 6)
        == dwgSafety::SectionPageRangeResult::Inserted;
    const bool insertedAfterRecovery = recoverable.size() == 2;
    recoverable.clear();
    t.expect(firstInserted && reportedResourceFailure && retainedAfterFailure && recovered
                 && insertedAfterRecovery && recoverable.empty(),
             "allocation failure leaves the staged range index reusable and clearable");
}

void testExactUnknownSectionNames(TestContext& t) {
    const auto makeName = [](std::size_t index) {
        return std::string("Vendor.CommonPrefix.UnknownSection.")
            + std::to_string(index);
    };
    const auto compareWork = [&makeName](std::size_t count, int order) {
        std::size_t comparisons = 0;
        dwgSafety::ExactNameIndex<NameComparisonCounter> index(
            NameComparisonCounter{&comparisons});
        bool admitted = true;
        for (std::size_t i = 0; i < count; ++i) {
            std::size_t slot = i;
            if (order == 1)
                slot = count - i - 1;
            else if (order == 2)
                slot = (i * 197u) % count;
            admitted = admitted
                && dwgSafety::tryInsertExactName(index, makeName(slot))
                    == dwgSafety::ExactNameInsertResult::Inserted;
        }
        return std::make_pair(comparisons, admitted && index.size() == count);
    };
    for (int order = 0; order < 3; ++order) {
        const auto small = compareWork(128, order);
        const auto large = compareWork(512, order);
        t.expect(small.second && large.second
                     && small.first < 128u * 48u
                     && large.first < 512u * 48u
                     && large.first < small.first * 8u,
                 "128/512 exact unknown-name admission stays ordered-logarithmic with common prefixes");
    }

    dwgSafety::ExactNameIndex<> names;
    const std::string early = makeName(0);
    const std::string late = makeName(511);
    bool uniqueRows = true;
    for (std::size_t i = 0; i < 512; ++i) {
        uniqueRows = uniqueRows
            && dwgSafety::tryInsertExactName(names, makeName(i))
                == dwgSafety::ExactNameInsertResult::Inserted;
    }
    const auto duplicateEarly = dwgSafety::tryInsertExactName(names, early);
    const auto duplicateLate = dwgSafety::tryInsertExactName(names, late);
    dwgSafety::ExactNameIndex<> caseDistinct;
    const bool casesRemainDistinct = dwgSafety::tryInsertExactName(
            caseDistinct, "Vendor:Context")
            == dwgSafety::ExactNameInsertResult::Inserted
        && dwgSafety::tryInsertExactName(caseDistinct, "vendor:Context")
            == dwgSafety::ExactNameInsertResult::Inserted;
    t.expect(uniqueRows && duplicateEarly
                     == dwgSafety::ExactNameInsertResult::Duplicate
                 && duplicateLate == dwgSafety::ExactNameInsertResult::Duplicate
                 && casesRemainDistinct,
             "unknown-name admission rejects exact early/late duplicates but preserves case-distinct names");

    using FailingNameIndex = dwgSafety::ExactNameIndex<
        std::less<std::string>, ThrowOnceAllocator<std::string>>;
    FailingNameIndex recoverable;
    const bool firstInserted = dwgSafety::tryInsertExactName(
        recoverable, "Vendor:Stable")
        == dwgSafety::ExactNameInsertResult::Inserted;
    RangeAllocationFailure::failNext = true;
    const bool resourceReported = dwgSafety::tryInsertExactName(
        recoverable, "Vendor:Retry")
        == dwgSafety::ExactNameInsertResult::ResourceFailure;
    const bool retainedAfterFailure = recoverable.size() == 1;
    const bool retried = dwgSafety::tryInsertExactName(
        recoverable, "Vendor:Retry")
        == dwgSafety::ExactNameInsertResult::Inserted;
    t.expect(firstInserted && resourceReported && retainedAfterFailure
                 && retried && recoverable.size() == 2,
             "unknown-name index handles node-allocation failure without losing prior staged names");
}

bool readLe32(const std::vector<std::uint8_t>& bytes, std::size_t offset,
              std::uint32_t& value) {
    if (offset > bytes.size() || bytes.size() - offset < 4u)
        return false;
    value = static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u)
        | (static_cast<std::uint32_t>(bytes[offset + 2u]) << 16u)
        | (static_cast<std::uint32_t>(bytes[offset + 3u]) << 24u);
    return true;
}

bool writeLe32(std::vector<std::uint8_t>& bytes, std::size_t offset,
               std::uint32_t value) {
    if (offset > bytes.size() || bytes.size() - offset < 4u)
        return false;
    for (unsigned int i = 0; i < 4u; ++i)
        bytes[offset + i] = static_cast<std::uint8_t>(value >> (i * 8u));
    return true;
}

std::vector<std::uint8_t> encodeR2004LiteralRun(
        const std::vector<std::uint8_t>& source) {
    if (source.empty() || source.size() > std::numeric_limits<std::uint32_t>::max())
        return {};
    const auto length = static_cast<std::uint32_t>(source.size());
    std::vector<std::uint8_t> compressed;
    if (length <= 18u) {
        compressed.push_back(static_cast<std::uint8_t>(length - 3u));
    } else {
        compressed.push_back(0);
        std::uint32_t remaining = length - 18u;
        while (remaining > 0xFFu) {
            compressed.push_back(0);
            remaining -= 0xFFu;
        }
        compressed.push_back(static_cast<std::uint8_t>(remaining));
    }
    compressed.insert(compressed.end(), source.begin(), source.end());
    return compressed;
}

std::vector<std::uint8_t> encodeR2004SystemPage(
        std::uint32_t pageType, const std::vector<std::uint8_t>& content) {
    const auto compressed = encodeR2004LiteralRun(content);
    if (compressed.empty()
        || content.size() > std::numeric_limits<std::uint32_t>::max()
        || compressed.size() > std::numeric_limits<std::uint32_t>::max())
        return {};
    std::vector<std::uint8_t> page(20u, 0);
    (void)writeLe32(page, 0, pageType);
    (void)writeLe32(page, 4, static_cast<std::uint32_t>(content.size()));
    (void)writeLe32(page, 8, static_cast<std::uint32_t>(compressed.size()));
    (void)writeLe32(page, 12, 2u);
    std::uint32_t checksum = dwgUtil::checksum18(0, page.data(), page.size());
    checksum = dwgUtil::checksum18(checksum, compressed.data(), compressed.size());
    (void)writeLe32(page, 16, checksum);
    page.insert(page.end(), compressed.begin(), compressed.end());
    return page;
}

bool decodeR2004SystemPage(const std::vector<std::uint8_t>& bytes,
                           std::uint64_t address, std::uint32_t expectedType,
                           std::vector<std::uint8_t>& content) {
    if (address > bytes.size() || bytes.size() - static_cast<std::size_t>(address) < 20u)
        return false;
    const auto offset = static_cast<std::size_t>(address);
    std::uint32_t pageType = 0, decompressedSize = 0, compressedSize = 0;
    if (!readLe32(bytes, offset, pageType)
        || !readLe32(bytes, offset + 4u, decompressedSize)
        || !readLe32(bytes, offset + 8u, compressedSize)
        || pageType != expectedType || decompressedSize == 0
        || decompressedSize > dwgSafety::MaxBufferSize
        || compressedSize == 0
        || compressedSize > bytes.size() - offset - 20u)
        return false;

    content.resize(decompressedSize);
    dwgCompressor decompressor;
    return decompressor.decompress18(bytes.data() + offset + 20u,
                                     content.data(), compressedSize,
                                     decompressedSize)
        && decompressor.decompressedBytes() == decompressedSize;
}

bool makeDuplicateUnknownR2004SectionMap(std::vector<std::uint8_t>& bytes) {
    if (bytes.size() < 0x100u)
        return false;

    std::array<std::uint8_t, 0x6c> variableHeader{};
    for (std::size_t i = 0; i < variableHeader.size(); ++i)
        variableHeader[i] = bytes[0x80u + i]
            ^ static_cast<std::uint8_t>(DRW_magicNum18[i]);
    const auto headerValue32 = [&variableHeader](std::size_t offset) {
        return static_cast<std::uint32_t>(variableHeader[offset])
            | (static_cast<std::uint32_t>(variableHeader[offset + 1u]) << 8u)
            | (static_cast<std::uint32_t>(variableHeader[offset + 2u]) << 16u)
            | (static_cast<std::uint32_t>(variableHeader[offset + 3u]) << 24u);
    };
    const std::uint32_t sectionPageMapId = headerValue32(0x50u);
    const std::uint32_t dataSectionMapId = headerValue32(0x5cu);
    const std::uint64_t sectionPageMapOffset = headerValue32(0x54u)
        | (static_cast<std::uint64_t>(headerValue32(0x58u)) << 32u);
    if (sectionPageMapOffset > std::numeric_limits<std::uint64_t>::max() - 0x100u)
        return false;
    const std::uint64_t sectionPageMapAddress = sectionPageMapOffset + 0x100u;

    std::vector<std::uint8_t> sectionPageMapContent;
    if (!decodeR2004SystemPage(bytes, sectionPageMapAddress, 0x41630e3bu,
                               sectionPageMapContent)
        || sectionPageMapContent.size() % 8u != 0u)
        return false;

    std::uint64_t physicalAddress = 0x100u;
    std::uint64_t dataSectionMapAddress = 0;
    std::uint32_t dataSectionMapPageSize = 0;
    bool foundSectionPageMap = false;
    bool foundDataSectionMap = false;
    for (std::size_t offset = 0; offset < sectionPageMapContent.size(); offset += 8u) {
        std::uint32_t pageId = 0, pageSize = 0;
        if (!readLe32(sectionPageMapContent, offset, pageId)
            || !readLe32(sectionPageMapContent, offset + 4u, pageSize)
            || pageSize == 0
            || physicalAddress > std::numeric_limits<std::uint64_t>::max() - pageSize)
            return false;
        if (pageId == sectionPageMapId) {
            if (physicalAddress != sectionPageMapAddress)
                return false;
            foundSectionPageMap = true;
        }
        if (pageId == dataSectionMapId) {
            dataSectionMapAddress = physicalAddress;
            dataSectionMapPageSize = pageSize;
            foundDataSectionMap = true;
        }
        physicalAddress += pageSize;
    }
    if (!foundSectionPageMap || !foundDataSectionMap
        || dataSectionMapAddress > bytes.size()
        || dataSectionMapPageSize > bytes.size() - dataSectionMapAddress)
        return false;

    std::vector<std::uint8_t> dataSectionMapContent;
    if (!decodeR2004SystemPage(bytes, dataSectionMapAddress, 0x4163003bu,
                               dataSectionMapContent)
        || dataSectionMapContent.size() < 20u)
        return false;
    std::uint32_t descriptionCount = 0;
    if (!readLe32(dataSectionMapContent, 0, descriptionCount)
        || descriptionCount < 2u)
        return false;

    std::vector<std::size_t> nameOffsets;
    std::size_t offset = 20u;
    for (std::uint32_t i = 0; i < descriptionCount; ++i) {
        std::uint32_t pageCount = 0;
        if (offset > dataSectionMapContent.size()
            || dataSectionMapContent.size() - offset < 96u
            || !readLe32(dataSectionMapContent, offset + 8u, pageCount))
            return false;
        nameOffsets.push_back(offset + 32u);
        const std::uint64_t next = static_cast<std::uint64_t>(offset) + 96u
            + static_cast<std::uint64_t>(pageCount) * 16u;
        if (next > dataSectionMapContent.size())
            return false;
        offset = static_cast<std::size_t>(next);
    }
    if (nameOffsets.size() < 2u)
        return false;

    constexpr char duplicateName[] = "AcDb:CodexDuplicate";
    for (std::size_t i = 0; i < 2u; ++i) {
        const auto nameOffset = nameOffsets[i];
        std::fill(dataSectionMapContent.begin() + static_cast<std::ptrdiff_t>(nameOffset),
                  dataSectionMapContent.begin() + static_cast<std::ptrdiff_t>(nameOffset + 64u),
                  std::uint8_t{0});
        std::copy(duplicateName, duplicateName + sizeof(duplicateName) - 1u,
                  dataSectionMapContent.begin() + static_cast<std::ptrdiff_t>(nameOffset));
    }

    auto replacement = encodeR2004SystemPage(0x4163003bu, dataSectionMapContent);
    if (replacement.empty() || replacement.size() > dataSectionMapPageSize)
        return false;
    replacement.resize(dataSectionMapPageSize, 0);
    std::copy(replacement.begin(), replacement.end(),
              bytes.begin() + static_cast<std::ptrdiff_t>(dataSectionMapAddress));
    return true;
}

class SectionMapPublicationProbe final : public dwgReader18 {
public:
    SectionMapPublicationProbe(std::vector<std::uint8_t>& bytes, dwgRW* owner)
        : dwgReader18(std::make_unique<dwgBuffer>(bytes.data(), bytes.size()), owner) {
        version = DRW::AC1018;
        decoder.setVersion(version, false);
    }
    bool parse() { return readFileHeader(); }
    std::size_t knownSections() const { return sections.size(); }
    std::size_t unknownSections() const { return m_unknownSections.size(); }
    const std::vector<DwgIntegrityDiagnostic>& diagnostics() const {
        return m_integrityDiagnostics;
    }
};

class SectionMapSink final : public dx_iface {
public:
    SectionMapSink() {
        cData = &data;
        currentBlock = data.mBlock;
    }
    void addHeader(const DRW_Header*) override { ++headers; }
    void addLine(const DRW_Line& value) override {
        ++lines;
        end = value.secPoint;
    }
    dx_data data;
    std::size_t headers{0};
    std::size_t lines{0};
    DRW_Coord end;
};

class ModelerDispatchSource final : public DRW_ModelerGeometry {
public:
    explicit ModelerDispatchSource(DRW::ETYPE type) : DRW_ModelerGeometry(type) {}

    void setObjectType(std::uint16_t value) { oType = value; }
    bool encodeCommon(DRW::Version version, dwgBufferW& body,
                      dwgBufferW& strings) {
        return encodeDwgCommon(version, &body, &strings);
    }
    bool encodeHandles(DRW::Version version, dwgBufferW& body,
                       dwgBufferW& handles) {
        return encodeDwgEntHandle(version, &body, &handles);
    }
};

class InsertDispatchSource final : public DRW_Insert {
public:
    bool encode(DRW::Version version, dwgBufferW& body,
                dwgBufferW& strings, dwgBufferW& handles) {
        return encodeDwg(version, &body, 0u, &strings, &handles);
    }
};

class ModelerDispatchFrameWriter final : public dwgWriter27 {
public:
    ModelerDispatchFrameWriter(std::ofstream* stream, DRW_Header* header)
        : dwgWriter27(stream, header) {}

    bool encodeEmptySolid(std::uint32_t handle, std::uint32_t ownerHandle) {
        ModelerDispatchSource source(DRW::E3DSOLID);
        source.setObjectType(38u); // ODA v5.4.1 §20.4.41: 3DSOLID
        source.handle = handle;
        source.parentHandle = ownerHandle;

        dwgBufferW& body = beginObject(handle);
        if (!source.encodeCommon(m_version, body, m_objectStrings))
            return false;
        body.putBit(1u); // empty modeler data: no ACIS body follows
        body.putBit(0u); // modeler-data unknown bit
        if (!body.isGood()
            || !source.encodeHandles(m_version, body, m_objectHandles))
            return false;

        finishObject();
        return !objectWriteFailed() && m_buf.isGood() && !m_buf.data().empty();
    }

    bool encodeInsert(std::uint32_t handle, std::uint32_t ownerHandle,
                      std::uint32_t targetBlockHandle) {
        InsertDispatchSource source;
        source.handle = handle;
        source.parentHandle = ownerHandle;
        source.blockRecH.ref = targetBlockHandle;

        dwgBufferW& body = beginObject(handle);
        if (!source.encode(m_version, body, m_objectStrings, m_objectHandles))
            return false;
        finishObject();
        return !objectWriteFailed() && m_buf.isGood() && !m_buf.data().empty();
    }
};

class ModelerDispatchReader final : public dwgReader27 {
public:
    using dwgReader::walkBlockRecordEntities;

    ModelerDispatchReader(std::unique_ptr<dwgBuffer> buffer, dwgRW* parent)
        : dwgReader27(std::move(buffer), parent) {}

    void setFormatVersion(DRW::Version value) {
        version = value;
        decoder.setVersion(value, false);
    }
};

class ModelerDispatchSink final : public dx_iface {
public:
    struct OwnedSolid {
        std::uint32_t walkOwner;
        DRW_ModelerGeometry entity;
    };
    struct OwnedInsert {
        std::uint32_t walkOwner;
        DRW_Insert entity;
    };

    ModelerDispatchSink() {
        cData = &data;
        currentBlock = data.mBlock;
    }

    void addModelerGeometry(const DRW_ModelerGeometry& value) override {
        modeler.push_back({activeWalkOwner, value});
    }

    void addInsert(const DRW_Insert& value) override {
        inserts.push_back({activeWalkOwner, value});
    }

    dx_data data;
    std::uint32_t activeWalkOwner{DRW::NoHandle};
    std::vector<OwnedSolid> modeler;
    std::vector<OwnedInsert> inserts;
};

void testModelerBlockRecordDispatch(TestContext& t) {
    constexpr std::uint32_t modelSpaceHandle = 0x400u;
    constexpr std::uint32_t liveBlockHandle = 0x500u;
    constexpr std::uint32_t unusedBlockHandle = 0x510u;
    constexpr std::uint32_t liveSolidHandle = 0x900u;
    constexpr std::uint32_t unusedSolidHandle = 0x901u;
    constexpr std::uint32_t insertHandle = 0x902u;
    std::ofstream output;
    DRW_Header header;
    ModelerDispatchFrameWriter liveWriter(&output, &header);
    ModelerDispatchFrameWriter unusedWriter(&output, &header);
    ModelerDispatchFrameWriter insertWriter(&output, &header);
    const bool generated =
        liveWriter.encodeEmptySolid(liveSolidHandle, liveBlockHandle)
        && unusedWriter.encodeEmptySolid(unusedSolidHandle, unusedBlockHandle)
        && insertWriter.encodeInsert(insertHandle, modelSpaceHandle,
                                     liveBlockHandle);
    t.expect(generated,
             "in-memory AC1027 writer frames two block-owned solids and one INSERT");
    if (!generated)
        return;

    std::vector<std::uint8_t> liveBytes = liveWriter.buffer();
    std::vector<std::uint8_t> unusedBytes = unusedWriter.buffer();
    std::vector<std::uint8_t> insertBytes = insertWriter.buffer();
    dwgRW parent("");
    auto makeBlock = [](std::uint32_t handle, const std::string& name,
                        std::uint32_t entityHandle) {
        auto block = std::make_unique<DRW_Block_Record>();
        block->handle = handle;
        block->name = name;
        DwgBlockOwnershipTestAccess::setEntityHandles(*block, {entityHandle});
        return block;
    };
    ModelerDispatchReader reader(
        std::unique_ptr<dwgBuffer>(new dwgBuffer(liveBytes.data(), liveBytes.size())),
        &parent);
    reader.setFormatVersion(DRW::AC1027);
    const auto registerBlock = [&reader](std::unique_ptr<DRW_Block_Record> value) {
        DRW_Block_Record* raw = value.get();
        reader.blockRecordmap.emplace(raw->handle, raw);
        (void)value.release(); // dwgReader owns and deletes blockRecordmap values.
    };
    registerBlock(makeBlock(modelSpaceHandle, "*Model_Space", insertHandle));
    registerBlock(makeBlock(liveBlockHandle, "LIVE_SOLID", liveSolidHandle));
    registerBlock(makeBlock(unusedBlockHandle, "UNUSED_SOLID", unusedSolidHandle));
    reader.ObjectMap.emplace(liveSolidHandle,
                             objHandle(38u, liveSolidHandle, 0u));
    reader.ObjectMap.emplace(unusedSolidHandle,
                             objHandle(38u, unusedSolidHandle, 0u));
    reader.ObjectMap.emplace(insertHandle, objHandle(7u, insertHandle, 0u));

    dwgBuffer liveStream(liveBytes.data(), liveBytes.size());
    dwgBuffer unusedStream(unusedBytes.data(), unusedBytes.size());
    dwgBuffer insertStream(insertBytes.data(), insertBytes.size());
    ModelerDispatchSink sink;
    sink.activeWalkOwner = modelSpaceHandle;
    const bool modelWalk = reader.walkBlockRecordEntities(
        reader.blockRecordmap.at(modelSpaceHandle), &insertStream, sink,
        modelSpaceHandle, modelSpaceHandle);
    sink.activeWalkOwner = liveBlockHandle;
    const bool liveWalk = reader.walkBlockRecordEntities(
        reader.blockRecordmap.at(liveBlockHandle), &liveStream, sink,
        liveBlockHandle, liveBlockHandle);
    sink.activeWalkOwner = unusedBlockHandle;
    const bool unusedWalk = reader.walkBlockRecordEntities(
        reader.blockRecordmap.at(unusedBlockHandle), &unusedStream, sink,
        unusedBlockHandle, unusedBlockHandle);
    t.expect(modelWalk && liveWalk && unusedWalk && sink.inserts.size() == 1u
                 && sink.modeler.size() == 2u,
             "AC1027 BLOCK_RECORD walks dispatch one INSERT and two stored solids");

    bool graphMatches = sink.inserts.size() == 1u && sink.modeler.size() == 2u;
    if (graphMatches) {
        const ModelerDispatchSink::OwnedInsert& insert = sink.inserts.front();
        graphMatches = insert.walkOwner == modelSpaceHandle
            && insert.entity.handle == insertHandle
            && insert.entity.parentHandle == modelSpaceHandle
            && insert.entity.blockRecH.ref == liveBlockHandle
            && insert.entity.name == "LIVE_SOLID";
        std::size_t reachableSolids = 0;
        bool liveSolidFound = false;
        bool unusedSolidStored = false;
        for (const ModelerDispatchSink::OwnedSolid& solid : sink.modeler) {
            const bool isLive = solid.entity.handle == liveSolidHandle
                && solid.walkOwner == liveBlockHandle
                && solid.entity.parentHandle == liveBlockHandle
                && solid.entity.eType == DRW::E3DSOLID
                && solid.entity.m_isEmpty
                && solid.entity.m_dwgAcisPayload.empty();
            const bool isUnused = solid.entity.handle == unusedSolidHandle
                && solid.walkOwner == unusedBlockHandle
                && solid.entity.parentHandle == unusedBlockHandle
                && solid.entity.eType == DRW::E3DSOLID
                && solid.entity.m_isEmpty
                && solid.entity.m_dwgAcisPayload.empty();
            graphMatches = graphMatches && (isLive || isUnused);
            liveSolidFound = liveSolidFound || isLive;
            unusedSolidStored = unusedSolidStored || isUnused;
            if (insert.entity.blockRecH.ref == solid.walkOwner)
                ++reachableSolids;
        }
        graphMatches = graphMatches && liveSolidFound && unusedSolidStored
            && reachableSolids == 1u;
    }
    t.expect(graphMatches,
             "INSERT reaches its named block's solid; a second block's solid stays stored but outside this reference");
}

void testModelerBlockWalkRejectsBadOwnership(TestContext& t) {
    constexpr std::uint32_t blockHandle = 0x600u;
    constexpr std::uint32_t wrongOwnerHandle = 0x601u;
    constexpr std::uint32_t solidHandle = 0xA00u;
    constexpr std::uint32_t wrongOwnerSolidHandle = 0xA01u;
    constexpr std::uint32_t missingSolidHandle = 0xA02u;
    std::ofstream output;
    DRW_Header header;
    ModelerDispatchFrameWriter validWriter(&output, &header);
    ModelerDispatchFrameWriter wrongOwnerWriter(&output, &header);
    const bool generated = validWriter.encodeEmptySolid(solidHandle, blockHandle)
        && wrongOwnerWriter.encodeEmptySolid(wrongOwnerSolidHandle,
                                             wrongOwnerHandle);
    t.expect(generated, "in-memory AC1027 ownership-failure frames are generated");
    if (!generated)
        return;

    struct WalkResult {
        bool walked;
        std::size_t callbacks;
    };
    const auto runWalk = [](const std::vector<std::uint8_t>& bytes,
                            std::uint32_t mappedHandle,
                            std::uint32_t blockHandle,
                            std::vector<std::uint32_t> ownership,
                            bool seedObject) {
        std::vector<std::uint8_t> sourceBytes = bytes;
        dwgRW parent("");
        auto blockOwner = std::make_unique<DRW_Block_Record>();
        DRW_Block_Record* block = blockOwner.get();
        block->handle = blockHandle;
        block->name = "BAD_OWNERSHIP_TEST";
        DwgBlockOwnershipTestAccess::setEntityHandles(*block,
                                                     std::move(ownership));
        ModelerDispatchReader reader(
            std::unique_ptr<dwgBuffer>(new dwgBuffer(
                sourceBytes.data(), sourceBytes.size())),
            &parent);
        reader.setFormatVersion(DRW::AC1027);
        reader.blockRecordmap.emplace(blockHandle, block);
        (void)blockOwner.release(); // dwgReader owns blockRecordmap values.
        if (seedObject) {
            reader.ObjectMap.emplace(mappedHandle,
                                     objHandle(38u, mappedHandle, 0u));
        }
        dwgBuffer objectStream(sourceBytes.data(), sourceBytes.size());
        ModelerDispatchSink sink;
        const bool walked = reader.walkBlockRecordEntities(
            block, &objectStream, sink, blockHandle, blockHandle);
        return WalkResult{walked, sink.modeler.size()};
    };

    const WalkResult duplicate = runWalk(
        validWriter.buffer(), solidHandle, blockHandle,
        {solidHandle, solidHandle}, true);
    const WalkResult missing = runWalk(
        validWriter.buffer(), missingSolidHandle, blockHandle,
        {missingSolidHandle}, false);
    const WalkResult wrongOwner = runWalk(
        wrongOwnerWriter.buffer(), wrongOwnerSolidHandle, blockHandle,
        {wrongOwnerSolidHandle}, true);
    t.expect(!duplicate.walked && duplicate.callbacks == 0u,
             "duplicate BLOCK_RECORD entity handle is rejected before callback");
    t.expect(missing.walked && missing.callbacks == 0u,
             "missing BLOCK_RECORD entity frame is skipped without failing the block walk");
    t.expect(!wrongOwner.walked && wrongOwner.callbacks == 0u,
             "entity whose encoded owner differs from BLOCK_RECORD is quarantined");
}

void testR2004SectionMapPublication(TestContext& t) {
    std::stringbuf streamBuffer(std::ios::out | std::ios::binary);
    std::ofstream output;
    static_cast<std::ostream&>(output).rdbuf(&streamBuffer);
    DRW_Header header;
    header.addInt("$MEASUREMENT", 1, 70);
    DRW_Line line;
    line.basePoint = {2, 3, 5};
    line.secPoint = {7, 11, 13};
    dwgWriter18 writer(&output, &header);
    const bool generated = writer.prepareDwgClassManifest()
        && writer.writeFileHeaderStub() && writer.writeDwgHeader()
        && writer.writeDwgClasses() && writer.writeDwgObjects()
        && writer.encodeEntity(&line) && writer.emitDeferredBlockControl()
        && writer.finalizeHeaderHandseed() && writer.writeDwgHandles()
        && writer.writeSecondHeader() && writer.finalize();
    t.expect(generated, "complete AC1018 section-map witness is generated in memory");
    if (!generated)
        return;

    const auto encoded = streamBuffer.str();
    const std::vector<std::uint8_t> valid(encoded.begin(), encoded.end());
    std::vector<std::uint8_t> invalid = valid;
    t.expect(makeDuplicateUnknownR2004SectionMap(invalid),
             "two generated R2004 descriptors receive an exact duplicate unknown name with a resealed DSM page");
    if (invalid == valid)
        return;
    const std::vector<std::uint8_t> publicInvalid = invalid;

    dwgRW diagnosticOwner("");
    SectionMapPublicationProbe probe(invalid, &diagnosticOwner);
    const bool rejected = !probe.parse();
    const auto diagnostics = probe.diagnostics();
    const bool exactFailure = std::any_of(
        diagnostics.begin(), diagnostics.end(), [](const DwgIntegrityDiagnostic& entry) {
            return entry.phase == DwgIntegrityPhase::SectionMap
                && entry.kind == DwgIntegrityCheckKind::SectionPageReference
                && entry.severity == DwgIntegritySeverity::Error;
        });
    const bool checksumsStayedValid = std::none_of(
        diagnostics.begin(), diagnostics.end(), [](const DwgIntegrityDiagnostic& entry) {
            return (entry.phase == DwgIntegrityPhase::FileHeader
                    && entry.kind == DwgIntegrityCheckKind::FileHeaderCrc)
                || entry.kind == DwgIntegrityCheckKind::SystemPageChecksum;
        });
    t.expect(rejected && exactFailure && checksumsStayedValid,
             "duplicate-name failure reaches the real SectionMap stage after valid envelope and checksums");
    t.expect(probe.knownSections() == 0u && probe.unknownSections() == 0u,
             "failed full-map admission publishes neither standard nor unknown section descriptors");

    std::copy(valid.begin(), valid.end(), invalid.begin());
    t.expect(probe.parse() && probe.knownSections() > 0u
                 && probe.unknownSections() == 0u,
             "the same reader instance retries a corrected full map and publishes the valid descriptors");

    SectionMapSink sink;
    dwgRW publicReader("");
    t.expect(!publicReader.readBuffer(
                 publicInvalid.data(), publicInvalid.size(), &sink, false)
                 && sink.headers == 0u && sink.lines == 0u,
             "public DWG read publishes no callbacks for a duplicate full section map");
    const auto publicDiagnostics = publicReader.getIntegrityDiagnostics();
    t.expect(std::any_of(publicDiagnostics.begin(), publicDiagnostics.end(),
                         [](const DwgIntegrityDiagnostic& entry) {
                             return entry.phase == DwgIntegrityPhase::SectionMap
                                 && entry.kind == DwgIntegrityCheckKind::SectionPageReference;
                         }),
             "public readBuffer preserves the reader-level duplicate-map diagnostic identity");
    t.expect(publicReader.readBuffer(valid.data(), valid.size(), &sink, false)
                 && sink.headers == 1u && sink.lines == 1u
                 && sink.end.x == 7 && sink.end.y == 11
                 && sink.end.z == 13,
             "public readBuffer recovers on the same facade after the corrected full map is supplied");
}

void appendAdmissionDiagnostic(std::ostream& out, const DRW_OperationDiagnostic& value) {
    out << static_cast<int>(value.operation) << ':';
    const auto append = [&out](const auto& entry) {
        out << static_cast<int>(entry.phase) << ':' << static_cast<int>(entry.cause)
            << ':' << std::quoted(entry.code) << ':' << std::quoted(entry.message)
            << ':' << entry.offset << ':' << entry.handle << ':' << entry.hasOffset
            << ':' << entry.hasHandle << ';';
    };
    append(value);
    out << value.secondary.size() << ':';
    for (const auto& entry : value.secondary) append(entry);
}

// Public snapshots plus physical prefix/suffix readback avoid a private-member
// macro or a new production test-access API at this lifecycle boundary.
std::string admissionState(dwgRW& owner) {
    std::ostringstream out;
    appendAdmissionDiagnostic(out, owner.getLastDiagnostic());
    const auto skip = owner.getWriteSkipCounters();
    out << owner.getError() << ':' << owner.getVersion() << ':' << std::quoted(owner.getCodePage())
        << ':' << owner.dwgReadObjectBudget() << ':' << owner.hasDwgWriteFailure()
        << ':' << owner.getSuccessfulEntityWriteCount() << ':' << owner.getLastSuccessfulEntityHandle()
        << ':' << skip.entityWrites << ':' << skip.tableRecordWrites << ':' << skip.objectWrites
        << ':' << skip.classRegistrations << ':' << skip.rawObjectWrites << ':' << skip.rawSectionWrites
        << ':' << skip.blockDefinitions << ':' << owner.getEntityParseFailures()
        << ':' << owner.getObjectParseFailures() << ':' << owner.getClassesCrcMismatch()
        << ':' << owner.getR2007CrcMismatch() << ':' << owner.getR2004CrcMismatch()
        << ':' << owner.getDataStorageLinkFailures() << ':' << owner.getDataStorageOrphanRecords()
        << ':' << owner.getDecodedProxyPrimitives() << ':' << owner.getIntegrityDiagnosticsDropped();
    for (const auto& diagnostic : owner.getIntegrityDiagnostics()) {
        out << '|' << diagnostic.version << ':' << static_cast<int>(diagnostic.severity)
            << ':' << static_cast<int>(diagnostic.offsetSpace) << ':' << static_cast<int>(diagnostic.phase)
            << ':' << static_cast<int>(diagnostic.kind) << ':' << diagnostic.logicalSectionId
            << ':' << diagnostic.sectionDescriptorId << ':' << std::quoted(diagnostic.sectionName)
            << ':' << diagnostic.pageId << ':' << diagnostic.fileOffset << ':' << diagnostic.logicalHandle
            << ':' << diagnostic.expected << ':' << diagnostic.observed << ':' << diagnostic.hasPageId
            << ':' << diagnostic.hasFileOffset << ':' << diagnostic.hasLogicalHandle
            << ':' << diagnostic.hasExpected << ':' << diagnostic.hasObserved;
    }
    const auto appendMap = [&out](const auto& values) {
        out << '|';
        for (const auto& entry : std::map<std::string, std::size_t>(values.begin(), values.end()))
            out << std::quoted(entry.first) << ':' << entry.second << ';';
    };
    appendMap(owner.getSkippedCustomClasses());
    appendMap(owner.getSkippedUnsupportedObjects());
    out << '|';
    for (const auto& name : owner.getLayerNameOrder()) out << std::quoted(name) << ';';
    out << '|';
    for (const auto& name : owner.getLtypeNameOrder()) out << std::quoted(name) << ';';
    std::vector<std::uint32_t> handles;
    out << '|' << owner.getEmittedDwgTableRecordHandles(handles) << ':';
    for (const auto handle : handles) out << handle << ';';
    DRW::DwgObjectFrameReceipt frame;
    out << '|' << owner.getLastDwgObjectFrame(frame) << ':' << frame.valid << ':' << frame.generation
        << ':' << frame.objectHandle << ':' << frame.version << ':' << frame.dataBitSize
        << ':' << frame.handleBitSize << ':' << frame.classNumber << ':' << frame.writerOperation
        << ':' << frame.admissionToken;
    for (const auto& occurrence : frame.occurrences) {
        out << '|' << occurrence.objectHandle << ':' << static_cast<int>(occurrence.stream)
            << ':' << static_cast<int>(occurrence.serializedSegment) << ':' << occurrence.segmentOrdinal
            << ':' << static_cast<int>(occurrence.token.code) << ':' << occurrence.token.reference
            << ':' << occurrence.token.startBit << ':' << occurrence.token.endBit;
    }
    return out.str();
}

class AdmissionInterface final : public dx_iface {
public:
    explicit AdmissionInterface(dwgRW* owner = nullptr) : codec(owner) {
        cData = &data;
        currentBlock = data.mBlock;
        layer.name = "ADMISSION_LAYER";
        app.name = "ADMISSION_APP";
        for (int i = 0; i < 2; ++i) {
            input[i].layer = layer.name;
            input[i].basePoint = {1.0 + i * 6, 2.0 + i * 6, 3.0 + i * 6};
            input[i].secPoint = {4.0 + i * 6, 5.0 + i * 6, 6.0 + i * 6};
        }
    }
    void addLine(const DRW_Line& line) override {
        lines.push_back(line);
        if (lines.size() == 1u && callback) callback();
    }
    void addHeader(const DRW_Header* value) override {
        dx_iface::addHeader(value);
        if (headerCallback) headerCallback();
    }
    void writeHeader(DRW_Header&) override {}
    void writeDwgClasses() override { ++phaseHooks; }
    void collectDwgAppIds() override { ++phaseHooks; }
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeLTypes() override {}
    void writeLayers() override { recordsOkay = codec->addLayer(&layer) && recordsOkay; }
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeObjects() override {}
    void writeAppId() override { recordsOkay = codec->addAppId(&app) && recordsOkay; }
    void writeEntities() override {
        ++writeCalls;
        const auto transaction = codec->beginDwgWriteTransaction();
        recordsOkay = transaction != 0 && recordsOkay;
        for (int i = 0; i < 2; ++i) {
            // Exercise provenance hooks without qualifying DataStorage payloads.
            recordsOkay = codec->setDwgObjectFrameProvenance(0, 0, 0) && recordsOkay;
            codec->clearDwgObjectFrameProvenance();
            recordsOkay = codec->writeLine(&input[i]) && recordsOkay;
            if (i == 0 && callback) callback();
        }
        recordsOkay = codec->commitDwgWriteTransaction(transaction) && recordsOkay;
    }
    dwgRW* codec;
    std::function<void()> callback;
    std::function<void()> headerCallback;
    std::vector<DRW_Line> lines;
    bool recordsOkay {true};
    int writeCalls {0}, phaseHooks {0};
private:
    dx_data data;
    DRW_Layer layer;
    DRW_AppId app;
    std::array<DRW_Line, 2> input;
};

bool admissionLinesMatch(const AdmissionInterface& sink) {
    return sink.lines.size() == 2u && sink.lines[0].basePoint.x == 1
        && sink.lines[0].secPoint.z == 6 && sink.lines[1].basePoint.x == 7
        && sink.lines[1].secPoint.z == 12 && sink.lines[0].layer == "ADMISSION_LAYER"
        && sink.lines[1].layer == "ADMISSION_LAYER";
}

void testDwgOperationAdmission(TestContext& t) {
    const auto root = std::filesystem::temp_directory_path()
        / ("libdxfrw-dwg-admission-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::error_code error;
    if (!std::filesystem::create_directory(root, error)) {
        t.expect(false, "DWG admission owns an exclusive scratch directory");
        return;
    }
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
    } cleanup{root};
    const auto bytes = [](const std::filesystem::path& path) {
        std::ifstream stream(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(stream), {});
    };
    for (const auto version : {DRW::AC1015, DRW::AC1027}) {
        const auto path = root / (std::to_string(static_cast<int>(version)) + ".dwg");
        const auto otherPath = root / "other.dwg";
        const std::string filename = path.u8string();
        dwgRW generator(filename.c_str());
        AdmissionInterface source(&generator);
        t.expect(generator.write(&source, version, true) && source.recordsOkay && source.phaseHooks == 2,
                 "DWG lifecycle source uses ordinary table, provenance and record hooks");
        const std::string original = bytes(path);
        const std::vector<std::uint8_t> buffer(original.begin(), original.end());
        const auto restore = [&](const std::filesystem::path& target) {
            std::ofstream stream(target, std::ios::binary | std::ios::trunc);
            stream.write(original.data(), static_cast<std::streamsize>(original.size()));
            return stream.good();
        };
        for (int outer = 0; outer < 3; ++outer) {
            for (int nested = 0; nested < 5; ++nested) {
                t.expect(restore(path), "DWG admission restores only its generated input");
                dwgRW owner(filename.c_str());
                AdmissionInterface sink(&owner);
                int attempts = 0;
                sink.callback = [&]() {
                    ++attempts;
                    const std::string state = admissionState(owner), destination = bytes(path);
                    AdmissionInterface rejected(&owner);
                    const bool accepted = nested == 0 ? owner.read(&rejected, true)
                        : nested == 1 ? owner.readBuffer(buffer.data(), buffer.size(), &rejected, true)
                        : nested == 2 ? owner.write(&rejected, version, false)
                        : nested == 3 ? owner.getPreview() : owner.testReader();
                    t.expect(!accepted && rejected.lines.empty() && rejected.writeCalls == 0,
                             "same DWG facade rejects each whole-operation entry before callbacks");
                    t.expect(!owner.read(nullptr, true)
                                 && !owner.readBuffer(nullptr, 6, nullptr, true)
                                 && !owner.write(nullptr, DRW::UNKNOWNV, false),
                             "DWG admission precedes nested argument and version validation");
                    t.expect(admissionState(owner) == state && bytes(path) == destination,
                             "nested DWG rejection preserves live results, diagnostics and destination");
                    t.expect(restore(otherPath), "different-facade source is independent of outer output");
                    const std::string otherName = otherPath.u8string();
                    dwgRW other(otherName.c_str());
                    AdmissionInterface independent(&other);
                    const bool otherOkay = nested == 0 ? other.read(&independent, false)
                        : nested == 2 ? other.write(&independent, version, true)
                        : nested == 1 ? other.readBuffer(buffer.data(), buffer.size(), &independent, false)
                        : nested == 3 ? other.getPreview() : other.testReader();
                    const bool otherResult = nested < 3
                        ? otherOkay && (nested == 2 ? independent.recordsOkay : admissionLinesMatch(independent))
                        : !otherOkay && other.getLastDiagnostic().operation == DRW::OperationKind::Read
                            && (nested == 3 ? other.getError() == DRW::BAD_READ_METADATA && other.getVersion() == version
                                            : other.getError() == DRW::BAD_NONE && other.getVersion() == DRW::UNKNOWNV);
                    t.expect(otherResult, "another DWG facade retains each entry's normal callback-time contract");
                    t.expect(admissionState(owner) == state && bytes(path) == destination,
                             "independent callback work does not change the outer DWG facade");
                };
                const bool okay = outer == 0 ? owner.read(&sink, false)
                    : outer == 1 ? owner.readBuffer(buffer.data(), buffer.size(), &sink, false)
                                 : owner.write(&sink, version, true);
                t.expect(okay && attempts == 1 && owner.getError() == DRW::BAD_NONE
                             && (outer == 2 ? sink.recordsOkay : admissionLinesMatch(sink)),
                         "DWG outer operation retains both records after rejected nesting");
                AdmissionInterface sequential(&owner);
                t.expect(owner.read(&sequential, false) && admissionLinesMatch(sequential),
                         "DWG operation releases admission for sequential file reuse");
            }
        }
        // Preview has no consumer callback in the current implementation.
        // Preserve its unsupported-preview result, not an invented outer matrix.
        dwgRW preview(filename.c_str());
        t.expect(!preview.getPreview() && preview.getError() == DRW::BAD_READ_METADATA,
                 "generated DWG retains the existing unsupported-preview contract");
        AdmissionInterface afterPreview(&preview);
        t.expect(preview.read(&afterPreview, false) && admissionLinesMatch(afterPreview),
                 "unsupported preview releases admission for a full read");
        dwgRW diagnosticReader(filename.c_str());
        t.expect(!diagnosticReader.testReader() && diagnosticReader.getError() == DRW::BAD_NONE,
                 "legacy diagnostic reader retains its false-without-parse-error result");
        AdmissionInterface afterDiagnostic(&diagnosticReader);
        t.expect(diagnosticReader.read(&afterDiagnostic, false) && admissionLinesMatch(afterDiagnostic),
                 "legacy diagnostic entry releases admission for a full read");

        for (int nested = 0; nested < 3; ++nested) {
            dwgR legacy(filename.c_str());
            AdmissionInterface sink;
            int attempts = 0;
            sink.callback = [&]() {
                ++attempts;
                std::ostringstream before;
                appendAdmissionDiagnostic(before, legacy.getLastDiagnostic());
                const auto errorBefore = legacy.getError();
                const auto versionBefore = legacy.getVersion();
                const auto codePage = legacy.getCodePage(), destination = bytes(path);
                AdmissionInterface rejected;
                const bool accepted = nested == 0 ? legacy.read(&rejected, true)
                    : nested == 1 ? legacy.getPreview() : legacy.testReader();
                std::ostringstream after;
                appendAdmissionDiagnostic(after, legacy.getLastDiagnostic());
                t.expect(!accepted && before.str() == after.str() && legacy.getError() == errorBefore
                             && legacy.getVersion() == versionBefore && legacy.getCodePage() == codePage
                             && bytes(path) == destination,
                         "deprecated DWG forwarding shares implementation admission exactly once");
            };
            t.expect(legacy.read(&sink, false) && attempts == 1 && admissionLinesMatch(sink),
                     "deprecated DWG facade continues its outer callbacks");
            sink.callback = {};
            sink.lines.clear();
            t.expect(legacy.read(&sink, false) && admissionLinesMatch(sink),
                     "deprecated DWG facade releases admission for sequential reuse");
        }
        dwgRW reusable(filename.c_str());
        AdmissionInterface reuse(&reusable);
        t.expect(!reusable.read(nullptr, false), "DWG null-interface read rejects");
        t.expect(reusable.read(&reuse, false), "DWG early read failure releases admission");
        t.expect(!reusable.readBuffer(nullptr, 6, &reuse, false), "DWG invalid-buffer read rejects");
        reuse.lines.clear();
        t.expect(reusable.readBuffer(buffer.data(), buffer.size(), &reuse, false) && admissionLinesMatch(reuse),
                 "DWG early buffer failure releases admission");
        t.expect(!reusable.write(&reuse, DRW::UNKNOWNV, true), "DWG unsupported write rejects");
        t.expect(reusable.write(&reuse, version, true) && reuse.recordsOkay,
                 "DWG early write failure releases admission");
        for (const bool memory : {false, true}) {
            reuse.lines.clear();
            reuse.headerCallback = []() { throw std::runtime_error("DWG header admission callback"); };
            bool caught = false;
            bool readOkay = true;
            try {
                readOkay = memory ? reusable.readBuffer(buffer.data(), buffer.size(), &reuse, false)
                                  : reusable.read(&reuse, false);
            } catch (const std::runtime_error&) { caught = true; }
            t.expect(!caught && !readOkay && reusable.getError() == DRW::BAD_READ_HEADER
                         && reusable.getLastDiagnostic().cause == DRW::OperationCause::CallbackException,
                     "DWG header callback retains existing contained-exception classification");
            reuse.headerCallback = {};
            reuse.lines.clear();
            t.expect(reusable.readBuffer(buffer.data(), buffer.size(), &reuse, false) && admissionLinesMatch(reuse),
                     "DWG read callback exception releases admission");
        }
        const std::string previous = bytes(path);
        reuse.callback = []() { throw std::runtime_error("DWG write admission callback"); };
        t.expect(!reusable.write(&reuse, version, true)
                     && reusable.getLastDiagnostic().cause == DRW::OperationCause::CallbackException
                     && bytes(path) == previous,
                 "DWG write callback failure retains classification and atomic destination");
        reuse.callback = {};
        t.expect(reusable.write(&reuse, version, true) && reuse.recordsOkay,
                 "DWG write catch releases admission for reuse");
        for (int entry = 0; entry < 3; ++entry) {
            const std::string missingName = (root / ("missing-" + std::to_string(entry) + ".dwg")).u8string();
            dwgRW missing(missingName.c_str());
            AdmissionInterface afterMissing(&missing);
            const bool opened = entry == 0 ? missing.read(&afterMissing, false)
                : entry == 1 ? missing.getPreview() : missing.testReader();
            t.expect(!opened && missing.getError() == DRW::BAD_OPEN,
                     "DWG file-backed entry preserves its missing-file failure");
            t.expect(missing.write(&afterMissing, version, true) && afterMissing.recordsOkay,
                     "DWG missing-file failure releases admission for subsequent output");
            std::error_code ignored;
            std::filesystem::remove(std::filesystem::u8path(missingName), ignored);
        }
    }
}

class HeaderSectionReader final : public dwgReader15 {
public:
    explicit HeaderSectionReader(std::vector<std::uint8_t>& bytes,
                                 std::uint32_t address = 0,
                                 DRW::Version headerVersion = DRW::AC1015,
                                 std::uint8_t appMaintenance = 0)
        : dwgReader15(std::make_unique<dwgBuffer>(bytes.data(), bytes.size()), nullptr) {
        version = headerVersion;
        appMaintenanceVersion = appMaintenance;
        decoder.setVersion(headerVersion, false);
        dwgSectionInfo section;
        section.Id = 0;
        section.size = bytes.size() - address;
        section.address = address;
        sections.emplace(secEnum::HEADER, section);
    }
    bool readDwgHeader(DRW_Header& hdr) override {
        const auto sectionIt = sections.find(secEnum::HEADER);
        if (sectionIt == sections.end() || sectionIt->second.Id < 0)
            return false;
        const auto& section = sectionIt->second;
        if (section.size == 0 || section.size > std::numeric_limits<int>::max()
            || !dwgSafety::range(section.address, section.size, fileBuf->size())
            || !fileBuf->setPosition(section.address))
            return false;
        std::vector<std::uint8_t> data(static_cast<std::size_t>(section.size));
        if (!fileBuf->getBytes(data.data(), data.size()))
            return false;
        dwgBuffer dataBuffer(data.data(), data.size(), &decoder);
        (void)checkSentinel(&dataBuffer, secEnum::HEADER, true);
        const auto addressSpace = version == DRW::AC1015
            ? DwgIntegrityAddressSpace::PhysicalFile
            : DwgIntegrityAddressSpace::DecodedBuffer;
        if (version > DRW::AC1018) {
            dwgBuffer handleBuffer(data.data(), data.size(), &decoder);
            return dwgReader::readDwgHeader(
                hdr, &dataBuffer, &handleBuffer, addressSpace, section.Id,
                section.address);
        }
        return dwgReader::readDwgHeader(
            hdr, &dataBuffer, &dataBuffer, addressSpace, section.Id,
            section.address);
    }
    const auto& diagnostics() const { return m_integrityDiagnostics; }
    std::size_t dropped() const { return m_integrityDiagnosticsDropped; }
    void fillDiagnostics() {
        for (std::size_t i = 0; i < dwgSafety::MaxIntegrityDiagnostics; ++i)
            addIntegrityDiagnostic({});
    }
};

class HeaderBodyProbe final : public DRW_Header {
public:
    using DRW_Header::parseDwg;
};

std::uint16_t headerTestCrc(const std::vector<std::uint8_t>& bytes,
                           std::size_t start, std::size_t end) {
    // Bit-wise CRC from ODA's reflected CRC16 table, independent of the
    // production writer/buffer checksum helpers (not a drawing oracle).
    std::uint16_t crc = 0xC0C1u;
    for (auto i = start; i < end; ++i) {
        crc ^= bytes[i];
        for (unsigned int bit = 0; bit < 8u; ++bit)
            crc = static_cast<std::uint16_t>((crc >> 1u)
                ^ ((crc & 1u) != 0u ? 0xA001u : 0u));
    }
    return crc;
}

void testBoundedFork(TestContext& t) {
    std::array<std::uint8_t, 4> bytes{{0xB5, 0x6A, 0xCC, 0xDD}};
    DRW_TextCodec codec;
    dwgBuffer parent(bytes.data(), bytes.size(), &codec);
    t.expect(parent.get3Bits() == 5u, "bounded fork source has a cached partial byte");
    auto bounded = parent.forkBounded(1u);
    t.expect(bounded.decoder == &codec && bounded.getBitPos() == 3u
                 && bounded.getPosition() == 0u && bounded.size() == 1u,
             "bounded fork preserves same-origin cursor/cache/decoder");
    unsigned int tail = 0;
    for (unsigned int i = 0; i < 5u; ++i)
        tail = (tail << 1u) | bounded.getBit();
    t.expect(tail == 21u && bounded.isGood() && bounded.getPosition() == 1u,
             "bounded fork reads remaining cached bits through its exact ceiling");
    (void)bounded.getBit();
    t.expect(!bounded.isGood() && parent.isGood() && parent.getBitPos() == 3u,
             "bounded stream exhaustion cannot read a following byte or move its parent");

    auto shared = parent.forkBounded(2u);
    auto copied = shared;
    copied.invalidate();
    t.expect(!parent.isGood() && !shared.isGood(),
             "related bounded/copy cursors intentionally share structural invalidation");
    dwgBuffer independentParent(bytes.data(), bytes.size());
    auto speculative = independentParent.forkIndependent().forkBounded(2u);
    speculative.invalidate();
    t.expect(independentParent.isGood(), "speculative independent fork isolates structural invalidation");

    auto nested = independentParent.forkBounded(2u).forkBounded(99u);
    t.expect(nested.size() == 2u && nested.getRawShort16() == 0x6AB5u,
             "nested ceilings intersect rather than expanding source access");
    t.expect(!nested.setPosition(3u) && !nested.isGood(),
             "bounded seek cannot escape the intersected ceiling");
    auto copySource = independentParent.forkBounded(2u);
    (void)copySource.get3Bits();
    auto cachedCopy = copySource;
    t.expect(cachedCopy.getBitPos() == 3u && cachedCopy.get2Bits() == 2u
                 && copySource.getBitPos() == 3u,
             "copy clones partial-byte state and cursor independently");
    auto excludedCache = copySource.forkBounded(0u);
    t.expect(!excludedCache.isGood() && excludedCache.getBit() == 0u,
             "a ceiling excluding the cached byte cannot expose its bits");
    auto mixed = independentParent.forkBounded(2u);
    (void)mixed.get3Bits();
    std::uint8_t mixedByte = 0;
    t.expect(mixed.getBytes(&mixedByte, 1u) && mixedByte == 0xABu
                 && mixed.getBitPos() == 3u && mixed.getPosition() == 1u,
             "raw byte reads preserve cached partial-byte alignment at a ceiling");
    unsigned int mixedTail = 0;
    for (unsigned int i = 0; i < 5u; ++i)
        mixedTail = (mixedTail << 1u) | mixed.getBit();
    t.expect(mixedTail == 10u && mixed.isGood(),
             "raw byte reads leave the remaining cached bits available");
    (void)mixed.getBit();
    t.expect(!mixed.isGood(), "mixed raw/bit reads cannot borrow a byte beyond the ceiling");

    std::array<std::uint8_t, 3> destination{{0xEE, 0xEE, 0xEE}};
    auto byteRead = independentParent.forkBounded(2u);
    t.expect(!byteRead.getBytes(destination.data(), destination.size())
                 && destination == std::array<std::uint8_t, 3>{{0xEE, 0xEE, 0xEE}},
             "out-of-window raw read does not expose even a partial prefix");
    auto crcRead = independentParent.forkBounded(2u);
    (void)crcRead.crc8(0xC0C1u, 0, 4);
    t.expect(!crcRead.isGood(), "CRC direct-pointer/fallback paths cannot escape a ceiling");
    auto validCrc = independentParent.forkBounded(2u);
    std::vector<std::uint8_t> prefix{bytes[0], bytes[1]};
    t.expect(validCrc.crc8(0xC0C1u, 0, 2) == headerTestCrc(prefix, 0, 2)
                 && validCrc.isGood() && validCrc.getPosition() == 0u,
             "bounded valid CRC preserves its source cursor");
    auto survivor = [&bytes]() {
        dwgBuffer temporary(bytes.data(), bytes.size());
        return temporary.forkBounded(2u);
    }();
    t.expect(survivor.getRawShort16() == 0x6AB5u && survivor.isGood(),
             "bounded fork owns its stream after parent expiry with backing storage alive");
    const std::vector<std::uint8_t> zeroCrc{{0xC1, 0xC0}};
    t.expect(headerTestCrc(zeroCrc, 0, 2) == 0u,
             "a calculated zero CRC is a real checksum value, not absence");
}

void testHeaderSectionBounds(TestContext& t) {
    DRW_Header source;
    source.addInt("LUNITS", 4, 70);
    dwgWriter15 writer(nullptr, &source);
    t.expect(writer.writeDwgHeader(), "full AC1015 HEADER generation succeeds");
    const auto original = writer.buffer();
    const std::uint32_t declared = original[16]
        | (std::uint32_t(original[17]) << 8u)
        | (std::uint32_t(original[18]) << 16u)
        | (std::uint32_t(original[19]) << 24u);
    const std::size_t crcOffset = 20u + declared;
    t.expect(crcOffset + 18u == original.size(),
             "HEADER RL delimits the complete payload before CRC and sentinel");
    t.expect(headerTestCrc(original, 16u, crcOffset)
                 == (original[crcOffset] | (std::uint16_t(original[crcOffset + 1u]) << 8u)),
             "literal bit-wise CRC confirms the generated full HEADER span");
    for (const std::string name : {"valid", "crc-flip", "crc-zero", "end-first",
            "end-zero", "begin-first", "truncated", "declared-zero",
            "declared-short", "declared-max", "declared-large"}) {
        auto bytes = original;
        if (name == "crc-flip") bytes[crcOffset] ^= 1u;
        if (name == "crc-zero") bytes[crcOffset] = bytes[crcOffset + 1u] = 0u;
        if (name == "end-first") bytes[crcOffset + 2u] ^= 1u;
        if (name == "end-zero") std::fill(bytes.begin() + crcOffset + 2u, bytes.end(), 0u);
        if (name == "begin-first") bytes[0] ^= 1u;
        if (name == "truncated") bytes.pop_back();
        std::uint32_t count = declared;
        if (name == "declared-zero") count = 0u;
        if (name == "declared-short") count -= 1u;
        if (name == "declared-max") count = 0xFFFFFFFFu;
        if (name == "declared-large") count += 1u;
        if (name.rfind("declared-", 0u) == 0u)
            for (std::size_t i = 0u; i < 4u; ++i)
                bytes[16u + i] = static_cast<std::uint8_t>(count >> (8u * i));
        HeaderSectionReader reader(bytes);
        DRW_Header header;
        header.addInt("stale", 99, 70);
        const bool accepted = reader.readDwgHeader(header);
        const bool warning = name == "crc-flip" || name == "crc-zero";
        const bool valid = name == "valid" || warning;
        t.expect(accepted == valid, "HEADER outcome: " + name);
        if (valid) {
            const auto entry = header.vars.find("LUNITS");
            t.expect(entry != header.vars.end() && entry->second->content.i == 4,
                     "bounded HEADER retains semantic values: " + name);
            t.expect(reader.diagnostics().size() == (warning ? 1u : 0u),
                     "HEADER checksum warnings remain observable: " + name);
            if (warning && !reader.diagnostics().empty())
                t.expect(reader.diagnostics()[0].severity == DwgIntegritySeverity::Warning,
                         "HEADER checksum corruption is not a structural failure");
        } else {
            t.expect(header.vars.empty(), "failed HEADER publishes no variables: " + name);
        }
    }

    // A canonical envelope is insufficient if the body itself is truncated.
    std::vector<std::uint8_t> shortBody(original.begin(), original.begin() + 36u);
    shortBody[16] = 16u;
    shortBody[17] = shortBody[18] = shortBody[19] = 0u;
    const auto shortCrc = headerTestCrc(shortBody, 16u, shortBody.size());
    shortBody.push_back(static_cast<std::uint8_t>(shortCrc));
    shortBody.push_back(static_cast<std::uint8_t>(shortCrc >> 8u));
    shortBody.insert(shortBody.end(), std::begin(dwgSentinels::HEADER_END),
                     std::end(dwgSentinels::HEADER_END));
    HeaderSectionReader shortReader(shortBody);
    DRW_Header shortHeader;
    t.expect(!shortReader.readDwgHeader(shortHeader) && shortHeader.vars.empty()
                 && shortReader.diagnostics().size() == 1u
                 && shortReader.diagnostics()[0].kind == DwgIntegrityCheckKind::HeaderSectionBounds,
             "valid CRC/sentinel cannot let a truncated body borrow trailer bytes");

    std::vector<std::uint8_t> compact(original.begin() + 16u, original.begin() + crcOffset);
    HeaderBodyProbe body;
    dwgBuffer compactBuffer(compact.data(), compact.size());
    t.expect(body.parseDwg(DRW::AC1015, &compactBuffer, &compactBuffer)
                 && body.vars.count("LUNITS") == 1u,
             "exact compact body-only HEADER remains readable without a trailer");
    compact.pop_back();
    dwgBuffer truncatedCompact(compact.data(), compact.size());
    t.expect(!body.parseDwg(DRW::AC1015, &truncatedCompact, &truncatedCompact)
                 && body.vars.empty(), "truncated compact data cannot bypass full-section admission");

    auto reuseBytes = original;
    HeaderSectionReader reuse(reuseBytes);
    DRW_Header reused;
    t.expect(reuse.readDwgHeader(reused), "same HEADER reader starts with valid data");
    reuseBytes[crcOffset + 2u] ^= 1u;
    t.expect(!reuse.readDwgHeader(reused) && reused.vars.empty(),
             "same HEADER reader clears rejected replacement data");
    std::copy(original.begin(), original.end(), reuseBytes.begin());
    t.expect(reuse.readDwgHeader(reused) && reused.vars.count("LUNITS") == 1u,
             "same HEADER reader can reread corrected data without stale parser state");

    std::vector<std::uint8_t> prefixed(64u, 0u);
    prefixed.insert(prefixed.end(), original.begin(), original.end());
    prefixed[64u + crcOffset] ^= 1u;
    HeaderSectionReader located(prefixed, 64u);
    DRW_Header locatedHeader;
    t.expect(located.readDwgHeader(locatedHeader) && located.diagnostics().size() == 1u,
             "physical HEADER checksum warning survives a nonzero section address");
    if (!located.diagnostics().empty()) {
        const auto& diagnostic = located.diagnostics()[0];
        t.expect(diagnostic.kind == DwgIntegrityCheckKind::HeaderSectionCrc
                     && diagnostic.phase == DwgIntegrityPhase::SectionParser
                     && diagnostic.offsetSpace == DwgIntegrityAddressSpace::PhysicalFile
                     && diagnostic.logicalSectionId == secEnum::HEADER
                     && diagnostic.sectionDescriptorId == 0
                     && diagnostic.hasFileOffset && diagnostic.fileOffset == 64u + crcOffset
                     && diagnostic.hasExpected && diagnostic.hasObserved,
                 "HEADER diagnostics bind exact section/address/value presence");
    }
    HeaderSectionReader saturated(prefixed, 64u);
    saturated.fillDiagnostics();
    DRW_Header saturatedHeader;
    t.expect(saturated.readDwgHeader(saturatedHeader) && saturated.dropped() == 1u
                 && saturated.diagnostics().size() == dwgSafety::MaxIntegrityDiagnostics,
             "dropped checksum diagnostics retain warning-only read behavior");

    // Fixed finite USERR1 bits give a genuinely zero checksum for this
    // generated full section; no search loop or opaque payload is needed.
    DRW_Header zeroSource;
    zeroSource.addInt("LUNITS", 4, 70);
    constexpr std::uint64_t zeroBits = UINT64_C(0x400d8939b2fec9f5);
    double zeroValue = 0;
    std::memcpy(&zeroValue, &zeroBits, sizeof zeroValue);
    zeroSource.addDouble("USERR1", zeroValue, 40);
    dwgWriter15 zeroWriter(nullptr, &zeroSource);
    t.expect(zeroWriter.writeDwgHeader(), "zero-CRC full HEADER generation succeeds");
    auto zeroBytes = zeroWriter.buffer();
    const auto zeroOffset = zeroBytes.size() - 18u;
    t.expect(headerTestCrc(zeroBytes, 16u, zeroOffset) == 0u
                 && zeroBytes[zeroOffset] == 0u && zeroBytes[zeroOffset + 1u] == 0u,
             "full HEADER's independently calculated and stored CRC are both zero");
    HeaderSectionReader zeroReader(zeroBytes);
    DRW_Header zeroHeader;
    t.expect(zeroReader.readDwgHeader(zeroHeader) && zeroReader.diagnostics().empty()
                 && zeroHeader.vars.count("USERR1") == 1u
                 && zeroHeader.vars.at("USERR1")->content.d == zeroValue,
             "a matching zero CRC is accepted cleanly with exact semantic data");
}

void setHeaderHighSizeWord(std::vector<std::uint8_t>& bytes, bool present) {
    const std::uint32_t oldCount = std::uint32_t(bytes[16])
        | (std::uint32_t(bytes[17]) << 8u)
        | (std::uint32_t(bytes[18]) << 16u)
        | (std::uint32_t(bytes[19]) << 24u);
    if (present) {
        bytes.insert(bytes.begin() + 20, 4u, 0u);
    } else {
        bytes.erase(bytes.begin() + 20, bytes.begin() + 24);
    }
    const std::uint32_t newCount = present ? oldCount + 4u : oldCount - 4u;
    for (std::size_t i = 0; i < 4u; ++i)
        bytes[16u + i] = static_cast<std::uint8_t>(newCount >> (i * 8u));
    const std::size_t crcOffset = 20u + newCount;
    const std::uint16_t crc = headerTestCrc(bytes, 16u, crcOffset);
    bytes[crcOffset] = static_cast<std::uint8_t>(crc);
    bytes[crcOffset + 1u] = static_cast<std::uint8_t>(crc >> 8u);
}

template<class Writer>
std::vector<std::uint8_t> makeHeaderSection(TestContext& t,
                                             const std::string& label) {
    DRW_Header source;
    source.addInt("LUNITS", 4, 70);
    Writer writer(nullptr, &source);
    t.expect(writer.writeDwgHeader(), "generated HEADER section: " + label);
    return writer.buffer();
}

std::vector<std::uint8_t> headerWithNodReference(TestContext& t,
                                                std::uint8_t code,
                                                std::uint64_t value) {
    auto bytes = makeHeaderSection<dwgWriter32>(t, "NOD reference vector");
    dwgBuffer input(bytes.data(), bytes.size());
    t.expect(input.setPosition(24u), "NOD vector reaches the modern HEADER bit-size");
    const auto handlesBit = 192u + input.getRawLong32();
    t.expect(input.setPosition(handlesBit / 8u), "NOD vector reaches the handle stream");
    input.setBitPos(handlesBit & 7u);
    // ODA9: CLAYER..CMLSTYLE6, paper/model UCS3+3, DIM8,
    // controls9, ACAD_GROUP/MLINESTYLE2; the next H is NOD.
    for (int i = 0; i < 31; ++i) (void)input.getHandle();
    const auto offset = input.getPosition() * 8u + input.getBitPos();
    const auto old = input.getHandle();
    const bool located = input.isGood()
        && old.code == DRW::DwgNullReference && old.size == 0u;
    t.expect(located, "independent HEADER H walk locates the null NOD placeholder");
    if (!located) return {};
    std::uint8_t size = 0;
    for (auto remaining = value; remaining != 0u; remaining >>= 8u) ++size;
    std::vector<std::uint8_t> encoded{static_cast<std::uint8_t>((code << 4u) | size)};
    for (int i = size; i > 0; --i)
        encoded.push_back(static_cast<std::uint8_t>(value >> (8u * (i - 1))));
    // HEADER handle streams need not be byte aligned. Replace one encoded
    // null H (8bits) while preserving every surrounding bit and stream origin.
    const auto original = bytes;
    bytes.assign(original.size() + encoded.size() - 1u, 0u);
    const auto copyBit = [&bytes](std::size_t destination, unsigned bit) {
        bytes[destination / 8u] |= static_cast<std::uint8_t>(bit << (7u - destination % 8u));
    };
    for (std::size_t i = 0; i < offset; ++i)
        copyBit(i, (original[i / 8u] >> (7u - i % 8u)) & 1u);
    for (std::size_t i = 0; i < encoded.size() * 8u; ++i)
        copyBit(offset + i, (encoded[i / 8u] >> (7u - i % 8u)) & 1u);
    for (std::size_t i = offset + 8u; i < original.size() * 8u; ++i)
        copyBit(i + (encoded.size() - 1u) * 8u,
                (original[i / 8u] >> (7u - i % 8u)) & 1u);
    const auto declared = static_cast<std::uint32_t>(bytes.size() - 38u);
    for (unsigned i = 0; i < 4; ++i)
        bytes[16u + i] = static_cast<std::uint8_t>(declared >> (8u * i));
    const auto crcOffset = bytes.size() - 18u;
    const auto crc = headerTestCrc(bytes, 16u, crcOffset);
    bytes[crcOffset] = static_cast<std::uint8_t>(crc);
    bytes[crcOffset + 1u] = static_cast<std::uint8_t>(crc >> 8u);
    return bytes;
}

class NodObjectWriter final : public dwgWriter32 {
public:
    using dwgWriter32::dwgWriter32;
    const auto& objects() const { return m_objectMap; }
};

class NodObjectReader final : public dwgReader32 {
public:
    explicit NodObjectReader(std::vector<std::uint8_t>& bytes)
        : dwgReader32(std::make_unique<dwgBuffer>(bytes.data(), bytes.size()), nullptr) {
        version = DRW::AC1032;
        decoder.setVersion(version, false);
    }
    void adoptHeader(const DRW_Header& header) {
        m_dwgNodReference = header.dwgNamedObjectDictionaryReference();
        m_dwgNodPublished = m_dwgNodFailureReported = false;
    }
    bool readObjects(std::vector<std::uint8_t>& bytes,
                     const std::vector<std::pair<std::uint32_t, std::uint32_t>>& frames,
                     DRW_Interface& sink) {
        objObjectMap.clear();
        for (std::size_t i = 0; i < frames.size(); ++i)
            objObjectMap.emplace(frames[i].first,
                objHandle(0u, frames[i].first, frames[i].second, i));
        dwgBuffer input(bytes.data(), bytes.size(), &decoder);
        return dwgReader::readDwgObjects(sink, &input);
    }
    const auto& diagnostics() const { return m_integrityDiagnostics; }
};

class NodSink : public dx_iface {
public:
    NodSink() { cData = &data; currentBlock = data.mBlock; }
    void addDictionary(const DRW_Dictionary& object) override {
        ++dictionaries;
        dx_iface::addDictionary(object);
    }
    void addNamedObjectDictionary(const DRW_Dictionary& object) override {
        ++roots;
        dx_iface::addNamedObjectDictionary(object);
    }
    void addXRecord(const DRW_XRecord& object) override {
        ++records;
        dx_iface::addXRecord(object);
    }
    dx_data data;
    std::size_t roots{0}, dictionaries{0}, records{0};
};

class NodCommonProbe final : public DRW_Dictionary {
public:
    bool readCommon(dwgBuffer* body, dwgBuffer* strings, std::uint32_t handleBits) {
        return DRW_TableEntry::parseDwg(DRW::AC1032, body, strings, handleBits);
    }
};

bool malformNodCount(TestContext& t, std::vector<std::uint8_t>& bytes,
                     std::uint32_t offset) {
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1032, false);
    dwgBuffer input(bytes.data(), bytes.size(), &codec);
    DwgObjectFrame frame;
    if (!frame.readAt(input, DRW::AC1032, offset)) return false;
    dwgBuffer body(frame.body().data(), frame.body().size(), &codec);
    dwgBuffer strings = body;
    NodCommonProbe common;
    if (!common.readCommon(&body, &strings, frame.bodyBitSize())) return false;
    const auto countBit = body.getPosition() * 8u + body.getBitPos();
    if (body.getBitLong() != 2) return false;
    (void)input.setPosition(offset);
    (void)input.getModularShort();
    (void)input.getUModularChar();
    const auto bodyOffset = input.getPosition();
    const auto bit = bodyOffset * 8u + countBit + 1u;
    // BL2's01 prefix becomes00 (a raw32-bit count) inside the same frame.
    // Body bounds/CRC stay valid, but the declared dictionary body does not.
    bytes[bit / 8u] &= static_cast<std::uint8_t>(~(0x80u >> (bit & 7u)));
    const auto crcOffset = bodyOffset + frame.body().size();
    const auto crc = headerTestCrc(bytes, offset, crcOffset);
    bytes[crcOffset] = static_cast<std::uint8_t>(crc);
    bytes[crcOffset + 1u] = static_cast<std::uint8_t>(crc >> 8u);
    dwgBuffer verify(bytes.data(), bytes.size(), &codec);
    t.expect(frame.readAt(verify, DRW::AC1032, offset),
             "malformed NOD count is still an exact CRC-valid bounded object frame");
    return true;
}

void testDwgNodReference(TestContext& t) {
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-nod-" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!std::filesystem::create_directory(directory)) {
        t.expect(false, "NOD test owns a fresh scratch directory");
        return;
    }
    struct Cleanup {
        std::filesystem::path directory;
        ~Cleanup() { std::error_code error; std::filesystem::remove_all(directory, error); }
    } cleanup{directory};
    DRW_Header fresh;
    t.expect(!fresh.dwgNamedObjectDictionaryReference(), "fresh HEADER has no DWG root provenance");
    for (const auto& row : {
            std::pair<std::uint8_t, std::uint64_t>{DRW::DwgHardOwnership, 0xA000u},
            {DRW::DwgSoftOwnership, 0u}, {DRW::DwgHardPointer, 0xA000u},
            {6u, 0u}, {DRW::DwgHardOwnership, 0x10000A000ULL}}) {
        auto bytes = headerWithNodReference(t, row.first, row.second);
        if (bytes.empty()) continue;
        HeaderSectionReader reader(bytes, 0u, DRW::AC1032, 6u);
        DRW_Header parsed;
        const bool accepted = reader.readDwgHeader(parsed);
        const auto reference = parsed.dwgNamedObjectDictionaryReference();
        t.expect(accepted && reference && reference->code == row.first
                     && reference->ref64 == row.second
                     && reference->ref == static_cast<std::uint32_t>(row.second),
                 "HEADER retains complete role/null/wide NOD H without truncation or guessing");
        if (!reference) continue;
        const bool usable = row.first == DRW::DwgSoftOwnership
            || (row.first == DRW::DwgHardOwnership && row.second <= 0xFFFFFFFFu);
        t.expect(DRW_Header::isUsableDwgNamedObjectDictionaryReference(*reference) == usable
                     && reader.diagnostics().size() == (usable ? 0u : 1u),
                 "invalid NOD role/width is observable while ordinary HEADER reading stays tolerant");
        if (!usable && !reader.diagnostics().empty())
            t.expect(reader.diagnostics().back().kind
                         == DwgIntegrityCheckKind::NamedObjectDictionaryReference
                         && reader.diagnostics().back().severity == DwgIntegritySeverity::Warning
                         && !reader.diagnostics().back().hasFileOffset
                         && reader.diagnostics().back().hasObserved
                         && reader.diagnostics().back().observed
                              == (row.second > 0xFFFFFFFFu ? row.second : row.first),
                     "NOD warning uses its appended optional diagnostic category");
        if (!usable) {
            dx_data incomplete;
            incomplete.headerC = parsed;
            dx_iface converter;
            const auto destination = directory / "bad-reference.dxf";
            { std::ofstream existing(destination); existing << "NOD_SENTINEL"; }
            t.expect(!converter.fileExport(destination.string(), DRW::AC1032, false,
                                           &incomplete, false, true),
                     "strict converter refuses invalid/wide NOD source before output publication");
            std::ifstream unchanged(destination);
            const std::string content((std::istreambuf_iterator<char>(unchanged)),
                                       std::istreambuf_iterator<char>());
            t.expect(content == "NOD_SENTINEL", "invalid NOD reference preserves destination");
        }
        bytes.back() ^= 1u;
        t.expect(!reader.readDwgHeader(parsed) && !parsed.dwgNamedObjectDictionaryReference(),
                 "failed HEADER reuse clears prior NOD provenance");
        bytes.back() ^= 1u;
        t.expect(reader.readDwgHeader(parsed) && parsed.dwgNamedObjectDictionaryReference(),
                 "corrected same-reader HEADER restores NOD provenance");
        HeaderBodyProbe direct;
        static_cast<DRW_Header&>(direct) = parsed;
        t.expect(!direct.parseDwg(DRW::AC1032, nullptr, nullptr)
                     && !direct.dwgNamedObjectDictionaryReference(),
                 "null direct HEADER parse cannot retain an old NOD reference");
    }

    auto headerBytes = headerWithNodReference(t, DRW::DwgHardOwnership, 0xA000u);
    if (headerBytes.empty()) return;
    HeaderSectionReader headerReader(headerBytes, 0u, DRW::AC1032, 6u);
    DRW_Header header;
    t.expect(headerReader.readDwgHeader(header), "object-root controls have a decoded source HEADER");
    for (const std::string variant : {"valid", "missing", "wrong-type", "wrong-owner", "malformed"}) {
        NodObjectWriter writer(nullptr, &fresh);
        DRW_Dictionary child;
        child.handle = 0x100u;
        child.parentHandle = 0xA000u;
        t.expect(writer.writeDictionary(child), "lower-handle dictionary emits before the root");
        if (variant != "missing") {
            if (variant == "wrong-type") {
                DRW_XRecord wrong;
                wrong.handle = 0xA000u;
                t.expect(writer.writeXRecord(wrong), "wrong root type stays a valid object record");
            } else {
                DRW_Dictionary root;
                root.handle = 0xA000u;
                root.parentHandle = variant == "wrong-owner" ? 0xA100u : 0u;
                root.hardOwner = 1;
                root.m_entries = {{"LOCAL_CHILD", 0x100u}, {"LOCAL_RECORD", 0xA001u}};
                t.expect(writer.writeDictionary(root), "root dictionary emits complete counted entries");
            }
        }
        DRW_XRecord suffix;
        suffix.handle = 0xA001u;
        suffix.parentHandle = 0xA000u;
        suffix.m_values.emplace_back(1, UTF8STRING("NOD_SUFFIX"));
        t.expect(writer.writeXRecord(suffix), "following XRECORD is an independent suffix control");
        auto bytes = writer.buffer();
        const auto original = bytes;
        if (variant == "malformed") {
            const auto frame = std::find_if(writer.objects().begin(), writer.objects().end(),
                [](const auto& object) { return object.first == 0xA000u; });
            const bool changed = frame != writer.objects().end()
                && malformNodCount(t, bytes, frame->second);
            t.expect(changed, "NOD negative mutates only its declared body count");
            if (!changed) continue;
        }
        NodObjectReader reader(bytes);
        reader.adoptHeader(header);
        NodSink sink;
        sink.data.headerC = header;
        const bool valid = variant == "valid";
        t.expect(reader.readObjects(bytes, writer.objects(), sink)
                     && sink.roots == (valid ? 1u : 0u)
                     && sink.records == (variant == "wrong-type" ? 2u : 1u)
                     && sink.dictionaries == (variant == "wrong-owner" ? 2u : 1u),
                 "NOD selection is typed/complete/owner-checked without dropping suffixes: " + variant);
        t.expect(reader.diagnostics().size() == (valid ? 0u : 1u),
                 "missing or inconsistent NOD remains a bounded warning: " + variant);
        if (valid) {
            for (const auto& absent : {
                    std::pair<std::uint8_t, std::uint64_t>{DRW::DwgSoftOwnership, 0u},
                    {DRW::DwgHardPointer, 0xA000u},
                    {DRW::DwgHardOwnership, 0x10000A000ULL}, {6u, 0u}}) {
                auto noRootBytes = headerWithNodReference(t, absent.first, absent.second);
                if (noRootBytes.empty()) continue;
                HeaderSectionReader noRootReader(noRootBytes, 0u, DRW::AC1032, 6u);
                DRW_Header noRootHeader;
                t.expect(noRootReader.readDwgHeader(noRootHeader),
                         "NOD selector negative has a complete source HEADER");
                reader.adoptHeader(noRootHeader);
                NodSink unrelated;
                t.expect(reader.readObjects(bytes, writer.objects(), unrelated)
                             && unrelated.roots == 0u && unrelated.dictionaries == 2u
                             && unrelated.records == 1u,
                         "null/wrong-role/wide reference never aliases the matching low32 dictionary");
            }
            reader.adoptHeader(fresh);
            NodSink noSource;
            t.expect(reader.readObjects(bytes, writer.objects(), noSource)
                         && noSource.roots == 0u && noSource.dictionaries == 2u,
                     "absent source reference never promotes an ownerless dictionary");
            class LegacyDefault final : public NodSink {
            public:
                void addNamedObjectDictionary(const DRW_Dictionary& object) override {
                    DRW_Interface::addNamedObjectDictionary(object);
                }
            } legacy;
            reader.adoptHeader(header);
            t.expect(reader.readObjects(bytes, writer.objects(), legacy)
                         && legacy.roots == 0u && legacy.dictionaries == 2u,
                     "existing optional root default calls historical addDictionary exactly once");
        } else {
            if (variant == "malformed") {
                bytes = original;
                reader.adoptHeader(header);
                NodSink retry;
                t.expect(reader.readObjects(bytes, writer.objects(), retry)
                             && retry.roots == 1u && retry.records == 1u,
                         "corrected dictionary body succeeds on the same reader");
            }
            dx_iface converter;
            const auto destination = directory / (variant + ".dxf");
            t.expect(!converter.fileExport(destination.string(), DRW::AC1032, false,
                                           &sink.data, false, true)
                         && !std::filesystem::exists(destination),
                     "missing/wrong NOD cannot publish an absent destination");
            { std::ofstream existing(destination); existing << "NOD_SENTINEL"; }
            t.expect(!converter.fileExport(destination.string(), DRW::AC1032, true,
                                           &sink.data, false, true),
                     "missing/wrong NOD refuses binary publication too");
            std::ifstream unchanged(destination);
            const std::string content((std::istreambuf_iterator<char>(unchanged)),
                                       std::istreambuf_iterator<char>());
            t.expect(content == "NOD_SENTINEL", "root refusal preserves an existing destination");
            unchanged.close();
            sink.data.dictionaries.erase(std::remove_if(sink.data.dictionaries.begin(),
                sink.data.dictionaries.end(), [](const DRW_Dictionary& object) {
                    return object.handle == 0xA000u;
                }), sink.data.dictionaries.end());
            sink.data.xRecords.erase(std::remove_if(sink.data.xRecords.begin(),
                sink.data.xRecords.end(), [](const DRW_XRecord& object) {
                    return object.handle == 0xA000u;
                }), sink.data.xRecords.end());
            auto& corrected = sink.data.namedObjectDictionary;
            corrected.handle = 0xA000u;
            corrected.parentHandle = 0u;
            corrected.hardOwner = 1;
            corrected.m_entries = {{"LOCAL_CHILD", 0x100u}, {"LOCAL_RECORD", 0xA001u}};
            sink.data.hasNamedObjectDictionary = true;
            for (const bool binary : {false, true}) {
                t.expect(converter.fileExport(destination.string(), DRW::AC1032, binary,
                                              &sink.data, false, true),
                         "explicit corrected root permits same-model/converter retry");
                dx_data reopened;
                dx_iface input;
                t.expect(input.fileImport(destination.string(), &reopened, false)
                             && reopened.hasNamedObjectDictionary
                             && !reopened.headerC.dwgNamedObjectDictionaryReference()
                             && reopened.xRecords.size() == 1u
                             && sink.data.headerC.dwgNamedObjectDictionaryReference()->ref64 == 0xA000u
                             && sink.data.namedObjectDictionary.handle == 0xA000u,
                         "corrected ASCII/binary DXF preserves root/suffix and source identities");
            }
        }
    }
}

void checkHeaderVersionProfile(TestContext& t,
                               std::vector<std::uint8_t> bytes,
                               DRW::Version version, std::uint8_t maintenance,
                               bool highSizePresent, const std::string& label) {
    dwgBuffer view(bytes.data(), bytes.size());
    const bool positioned = view.setPosition(16u);
    const auto envelope = positioned
        ? inspectDwgHeaderEnvelope(view, version, maintenance)
        : DwgHeaderEnvelope{};
    t.expect(envelope.error == DwgHeaderEnvelopeError::None
                 && envelope.hasHighSizeWord == highSizePresent
                 && (!highSizePresent || envelope.highSizeWord == 0u)
                 && envelope.payloadEnd + 18u == bytes.size(),
             "versioned logical HEADER boundary/profile: " + label);

    HeaderSectionReader reader(bytes, 0u, version, maintenance);
    DRW_Header parsed;
    parsed.addInt("stale", 99, 70);
    t.expect(reader.readDwgHeader(parsed) && parsed.vars.count("LUNITS") == 1u
                 && reader.diagnostics().empty(),
             "versioned HEADER variables and CRC are accepted: " + label);

    auto trailing = bytes;
    trailing.push_back(0u);
    HeaderSectionReader trailingReader(trailing, 0u, version, maintenance);
    DRW_Header trailingHeader;
    trailingHeader.addInt("stale", 99, 70);
    t.expect(!trailingReader.readDwgHeader(trailingHeader)
                 && trailingHeader.vars.empty()
                 && trailingReader.diagnostics().size() == 1u
                 && trailingReader.diagnostics()[0].kind
                        == DwgIntegrityCheckKind::HeaderSectionBounds,
             "decoded page padding cannot extend the logical HEADER: " + label);

    auto crcMutation = bytes;
    const auto crcOffset = static_cast<std::size_t>(envelope.payloadEnd);
    crcMutation[crcOffset] ^= 1u;
    HeaderSectionReader crcReader(crcMutation, 0u, version, maintenance);
    DRW_Header crcHeader;
    t.expect(crcReader.readDwgHeader(crcHeader)
                 && crcReader.diagnostics().size() == 1u
                 && crcReader.diagnostics()[0].kind
                        == DwgIntegrityCheckKind::HeaderSectionCrc
                 && crcReader.diagnostics()[0].severity
                        == DwgIntegritySeverity::Warning,
             "versioned HEADER CRC mismatch remains warning-only: " + label);

    auto endMutation = bytes;
    endMutation[static_cast<std::size_t>(envelope.payloadEnd) + 2u] ^= 1u;
    HeaderSectionReader endReader(endMutation, 0u, version, maintenance);
    DRW_Header endHeader;
    endHeader.addInt("stale", 99, 70);
    t.expect(!endReader.readDwgHeader(endHeader) && endHeader.vars.empty()
                 && endReader.diagnostics().size() == 1u
                 && endReader.diagnostics()[0].kind
                        == DwgIntegrityCheckKind::HeaderSectionSentinel,
             "versioned HEADER end sentinel is checked before publication: " + label);
}

void testHeaderVersionEnvelopes(TestContext& t) {
    checkHeaderVersionProfile(t,
        makeHeaderSection<dwgWriter15>(t, "AC1015"), DRW::AC1015, 0u, false,
        "AC1015");
    checkHeaderVersionProfile(t,
        makeHeaderSection<dwgWriter18>(t, "AC1018"), DRW::AC1018, 0u, false,
        "AC1018");
    checkHeaderVersionProfile(t,
        makeHeaderSection<dwgWriter21>(t, "AC1021"), DRW::AC1021, 0u, false,
        "AC1021");

    auto ac1021 = makeHeaderSection<dwgWriter21>(t, "AC1021");
    checkHeaderVersionProfile(t, ac1021, DRW::AC1021, 6u, false,
                              "AC1021 app maintenance does not add a high-size word");

    auto ac1024Low = makeHeaderSection<dwgWriter24>(t, "AC1024 MV<=3");
    checkHeaderVersionProfile(t, ac1024Low, DRW::AC1024, 3u,
                                           false, "AC1024 MV<=3");
    auto ac1024High = ac1024Low;
    setHeaderHighSizeWord(ac1024High, true);
    checkHeaderVersionProfile(t, ac1024High, DRW::AC1024, 4u,
                                           true, "AC1024 MV>3");
    for (const auto& mismatch : {
             std::pair<const std::vector<std::uint8_t>*, std::uint8_t>{&ac1024High, 3u},
             {&ac1024Low, 4u}}) {
        auto mismatchBytes = *mismatch.first;
        HeaderSectionReader reader(mismatchBytes, 0u, DRW::AC1024, mismatch.second);
        DRW_Header header;
        header.addInt("stale", 99, 70);
        t.expect(!reader.readDwgHeader(header) && header.vars.empty(),
                 "AC1024 maintenance gate must match actual optional word presence");
    }

    auto ac1027High = makeHeaderSection<dwgWriter27>(t, "AC1027 MV>3");
    checkHeaderVersionProfile(t, ac1027High, DRW::AC1027, 6u,
                                           true, "AC1027 MV>3");
    auto ac1027Low = ac1027High;
    setHeaderHighSizeWord(ac1027Low, false);
    checkHeaderVersionProfile(t, ac1027Low, DRW::AC1027, 3u,
                                           false, "AC1027 MV<=3");
    for (const auto& mismatch : {
             std::pair<const std::vector<std::uint8_t>*, std::uint8_t>{&ac1027High, 3u},
             {&ac1027Low, 4u}}) {
        auto mismatchBytes = *mismatch.first;
        HeaderSectionReader reader(mismatchBytes, 0u, DRW::AC1027, mismatch.second);
        DRW_Header header;
        header.addInt("stale", 99, 70);
        t.expect(!reader.readDwgHeader(header) && header.vars.empty(),
                 "AC1027 maintenance gate must match actual optional word presence");
    }

    const auto ac1032 = makeHeaderSection<dwgWriter32>(t, "AC1032");
    checkHeaderVersionProfile(t, ac1032, DRW::AC1032, 0u,
                                           true, "AC1032 always");

    // R13/R14 are separate reader15 admission profiles. This envelope-only
    // partner checks their shared on-disk sentinels/RL/CRC layout without
    // treating AC1015 body semantics as R13/R14 qualification.
    auto ac1015 = makeHeaderSection<dwgWriter15>(t, "R13/R14 envelope basis");
    for (const DRW::Version version : {DRW::AC1012, DRW::AC1014}) {
        dwgBuffer view(ac1015.data(), ac1015.size());
        const bool positioned = view.setPosition(16u);
        const auto envelope = positioned
            ? inspectDwgHeaderEnvelope(view, version, 0u)
            : DwgHeaderEnvelope{};
        t.expect(envelope.error == DwgHeaderEnvelopeError::None
                     && !envelope.hasHighSizeWord
                     && envelope.payloadEnd + 18u == ac1015.size(),
                 "R13/R14 read-only HEADER section envelope: "
                     + std::string(version == DRW::AC1012 ? "AC1012" : "AC1014"));
        auto malformed = ac1015;
        malformed.front() ^= 1u;
        HeaderSectionReader reader(malformed, 0u, version, 0u);
        DRW_Header header;
        header.addInt("stale", 99, 70);
        t.expect(!reader.readDwgHeader(header) && header.vars.empty()
                     && reader.diagnostics().size() == 1u
                     && reader.diagnostics()[0].kind
                            == DwgIntegrityCheckKind::HeaderSectionSentinel,
                 "R13/R14 malformed HEADER sentinel rejects before parse: "
                     + std::string(version == DRW::AC1012 ? "AC1012" : "AC1014"));
    }
}

void patchHeaderTestLong(std::vector<std::uint8_t>& bytes, std::size_t offset,
                         std::uint32_t value) {
    for (std::size_t i = 0; i < 4u; ++i)
        bytes.at(offset + i) = static_cast<std::uint8_t>(value >> (8u * i));
}

void sealHeaderTest(std::vector<std::uint8_t>& bytes, std::size_t crcOffset) {
    const auto crc = headerTestCrc(bytes, 16u, crcOffset);
    bytes.at(crcOffset) = static_cast<std::uint8_t>(crc);
    bytes.at(crcOffset + 1u) = static_cast<std::uint8_t>(crc >> 8u);
}

void testHeaderCompatibilityProfile(TestContext& t) {
    DRW_Header source;
    source.addInt("LUNITS", 4, 70);
    source.addStr("MENU", "header-boundary-menu", 1);
    dwgWriter32 writer(nullptr, &source);
    t.expect(writer.writeDwgHeader(), "compatible profile source generation");
    const auto canonical = writer.buffer();
    const auto crcOffset = canonical.size() - 18u;
    auto compatible = canonical;
    patchHeaderTestLong(compatible, 16u,
                        static_cast<std::uint32_t>(crcOffset - 24u));
    sealHeaderTest(compatible, crcOffset);
    for (const std::size_t suffix : {0u, 1u, 739u}) {
        auto bytes = compatible;
        bytes.insert(bytes.end(), suffix, 0xA5u); // Authored opaque bytes, not zero padding.
        const auto original = bytes;
        dwgBuffer view(bytes.data(), bytes.size());
        (void)view.setPosition(16u);
        const auto envelope = inspectDwgHeaderEnvelope(view, DRW::AC1032, 55u);
        t.expect(envelope.error == DwgHeaderEnvelopeError::None
                     && envelope.excludesHighSizeWord
                     && envelope.payloadEnd == crcOffset
                     && envelope.trailerEnd == compatible.size()
                     && envelope.unclaimedBytes == suffix
                     && view.getPosition() == 16u && view.isGood()
                     && bytes == original,
                 "CRC-bound compatible envelope is nonmutating, suffix=" + std::to_string(suffix));
        HeaderSectionReader reader(bytes, 0u, DRW::AC1032, 55u);
        DRW_Header header;
        t.expect(reader.readDwgHeader(header) && header.vars.count("LUNITS") == 1u
                     && header.vars.at("LUNITS")->content.i == 4
                     && header.vars.count("MENU") == 1u
                     && header.vars.at("MENU")->type() == DRW_Variant::STRING
                     && header.vars.at("MENU")->c_str() != nullptr
                     && std::string(header.vars.at("MENU")->c_str()) == "header-boundary-menu"
                     && reader.diagnostics().size() == (suffix == 0u ? 1u : 2u),
                 "compatible HEADER retains fields and explicit observations");
        if (!reader.diagnostics().empty()) {
            const auto& layout = reader.diagnostics().front();
            t.expect(layout.kind == DwgIntegrityCheckKind::HeaderSectionCompatibility
                         && layout.severity == DwgIntegritySeverity::Warning
                         && layout.offsetSpace == DwgIntegrityAddressSpace::DecodedBuffer
                         && layout.hasFileOffset && layout.fileOffset == 16u
                         && layout.hasExpected && layout.expected == crcOffset - 4u
                         && layout.hasObserved && layout.observed == crcOffset,
                     "compatible layout diagnostic binds actual CRC offsets");
        }
        if (suffix != 0u && reader.diagnostics().size() == 2u) {
            const auto& tail = reader.diagnostics().back();
            t.expect(tail.kind == DwgIntegrityCheckKind::HeaderSectionUnclaimedData
                         && tail.severity == DwgIntegritySeverity::Warning
                         && tail.hasFileOffset && tail.fileOffset == compatible.size()
                         && tail.hasExpected && tail.expected == compatible.size()
                         && tail.hasObserved && tail.observed == bytes.size(),
                     "unclaimed logical suffix is observable, not declared padding");
        }
    }
    dwgBuffer fieldProbe(compatible.data(), compatible.size());
    (void)fieldProbe.setPosition(24u);
    const auto bits = fieldProbe.getRawLong32();
    const auto endBit = 192u + std::uint64_t(bits);
    std::uint64_t stringStart = 0, stringEnd = 0;
    const bool stringsLocated = fieldProbe.getR2007StringStreamBounds(
        endBit, stringStart, stringEnd);
    t.expect(stringsLocated && stringStart < stringEnd,
             "generated nonempty string stream has independent literal bounds");
    if (stringsLocated) {
        const auto patchBit = [](std::vector<std::uint8_t>& bytes,
                                  std::uint64_t offset, bool value) {
            const auto mask = static_cast<std::uint8_t>(1u << (7u - (offset & 7u)));
            auto& byte = bytes.at(static_cast<std::size_t>(offset >> 3u));
            byte = value ? static_cast<std::uint8_t>(byte | mask)
                         : static_cast<std::uint8_t>(byte & ~mask);
        };
        for (const std::string name : {"string-length", "string-absence", "footer-length"}) {
            auto bytes = compatible;
            bytes.insert(bytes.end(), 739u, 0xA5u);
            if (name == "string-length") {
                patchBit(bytes, stringStart, false);
                patchBit(bytes, stringStart + 1u, false); // BS00 + raw 0xffff.
                for (unsigned i = 0; i < 16u; ++i)
                    patchBit(bytes, stringStart + 2u + i, true);
            } else if (name == "string-absence") {
                patchBit(bytes, endBit - 1u, false);
            } else {
                for (unsigned i = 0; i < 16u; ++i)
                    patchBit(bytes, endBit - 17u + i, true);
            }
            sealHeaderTest(bytes, crcOffset);
            HeaderSectionReader reader(bytes, 0u, DRW::AC1032, 55u);
            DRW_Header header;
            t.expect(!reader.readDwgHeader(header) && header.vars.empty(),
                     "CRC-valid string mutation cannot borrow handle/suffix bytes: " + name);
        }
    }

    for (const std::string name : {"crc", "begin", "end", "word", "short",
                                   "max", "bits-zero", "bits-max"}) {
        auto bytes = compatible;
        bytes.insert(bytes.end(), 739u, 0xA5u);
        if (name == "crc") bytes[crcOffset] ^= 1u;
        if (name == "begin") bytes[0] ^= 1u;
        if (name == "end") bytes[crcOffset + 2u] ^= 1u;
        if (name == "word") patchHeaderTestLong(bytes, 20u, 1u);
        if (name == "short") patchHeaderTestLong(bytes, 16u, 1u);
        if (name == "max") patchHeaderTestLong(bytes, 16u, 0xFFFFFFFFu);
        if (name == "bits-zero") patchHeaderTestLong(bytes, 24u, 0u);
        if (name == "bits-max") patchHeaderTestLong(bytes, 24u, 0xFFFFFFFFu);
        if (name != "crc") sealHeaderTest(bytes, crcOffset);
        HeaderSectionReader reader(bytes, 0u, DRW::AC1032, 55u);
        DRW_Header header;
        header.addInt("stale", 1, 70);
        t.expect(!reader.readDwgHeader(header) && header.vars.empty()
                     && reader.diagnostics().size() == 1u
                     && reader.diagnostics().front().severity == DwgIntegritySeverity::Error,
                 "opaque suffix cannot rescue malformed compatible body: " + name);
    }
    for (const DRW::Version version : {DRW::AC1024, DRW::AC1027}) {
        auto bytes = compatible;
        HeaderSectionReader reader(bytes, 0u, version, 55u);
        DRW_Header header;
        t.expect(!reader.readDwgHeader(header) && header.vars.empty(),
                 "AC1032 compatibility is not extrapolated to unwitnessed versions");
    }
    auto fakeTail = canonical;
    fakeTail.insert(fakeTail.end(), 739u, 0xA5u);
    fakeTail.insert(fakeTail.end(), compatible.end() - 18, compatible.end());
    HeaderSectionReader fakeReader(fakeTail, 0u, DRW::AC1032, 55u);
    DRW_Header fakeHeader;
    t.expect(!fakeReader.readDwgHeader(fakeHeader) && fakeHeader.vars.empty(),
             "canonical surplus and distant matching sentinel do not trigger a scan");

    // A correct alternate CRC/sentinel with abundant suffix bytes still cannot
    // supply a missing variable body, string footer or handle stream.
    std::vector<std::uint8_t> shortBody(compatible.begin(), compatible.begin() + 32u);
    patchHeaderTestLong(shortBody, 16u, 8u);
    shortBody.resize(34u);
    sealHeaderTest(shortBody, 32u);
    shortBody.insert(shortBody.end(), std::begin(dwgSentinels::HEADER_END),
                     std::end(dwgSentinels::HEADER_END));
    shortBody.insert(shortBody.end(), 739u, 0xA5u);
    HeaderSectionReader shortReader(shortBody, 0u, DRW::AC1032, 55u);
    DRW_Header shortHeader;
    t.expect(!shortReader.readDwgHeader(shortHeader) && shortHeader.vars.empty(),
             "CRC-valid truncated alternate body cannot borrow unclaimed suffix");

    auto reuseBytes = compatible;
    reuseBytes.insert(reuseBytes.end(), 739u, 0xA5u);
    HeaderSectionReader reuse(reuseBytes, 0u, DRW::AC1032, 55u);
    DRW_Header reused;
    t.expect(reuse.readDwgHeader(reused), "compatible reader first valid operation");
    reuseBytes[crcOffset] ^= 1u;
    t.expect(!reuse.readDwgHeader(reused) && reused.vars.empty(),
             "compatible reader clears state on rejected replacement");
    reuseBytes[crcOffset] ^= 1u;
    t.expect(reuse.readDwgHeader(reused) && reused.vars.count("LUNITS") == 1u,
             "compatible reader accepts corrected retry");
    HeaderSectionReader saturated(reuseBytes, 0u, DRW::AC1032, 55u);
    saturated.fillDiagnostics();
    DRW_Header saturatedHeader;
    t.expect(saturated.readDwgHeader(saturatedHeader) && saturated.dropped() == 2u,
             "compatible observations preserve bounded dropped-diagnostic accounting");
}

class FullHeaderWriterProbe final : public dwgWriter15 {
public:
    FullHeaderWriterProbe(std::ofstream* stream, DRW_Header* header)
        : dwgWriter15(stream, header) {}
    std::size_t headerAddress() const { return m_sectionOffsets.at(0); }
    std::size_t classesAddress() const { return m_sectionOffsets.at(1); }
    struct SealingState {
        std::vector<std::uint8_t> bytes;
        std::uint32_t offset, size, patch, dataStart;
    };
    SealingState sealingState() const {
        return {buffer(), m_sectionOffsets.at(0), m_sectionSizes.at(0),
                m_headerHandseedBitOffset, m_headerDataStartBit};
    }
    void corruptSealing(const std::string& name) {
        const auto start = m_sectionOffsets.at(0);
        const auto crc = start + m_sectionSizes.at(0) - 18u;
        if (name == "missing-range") m_sectionOffsets.erase(0);
        if (name == "outside-range") m_sectionOffsets[0] = 0xFFFFFFFFu;
        if (name == "short-range") m_sectionSizes[0] = 37u;
        if (name == "long-range") ++m_sectionSizes[0];
        if (name == "declared-max") m_buf.patchRawLong32(start + 16u, 0xFFFFFFFFu);
        if (name == "declared-short") m_buf.patchRawLong32(start + 16u, 1u);
        if (name == "begin-sentinel") {
            const std::uint8_t value = std::uint8_t(buffer()[start] ^ 1u);
            (void)m_buf.patchRawBytesAtBit(start * 8u, &value, 1u);
        }
        if (name == "end-sentinel") {
            const std::uint8_t value = std::uint8_t(buffer()[crc + 2u] ^ 1u);
            (void)m_buf.patchRawBytesAtBit((crc + 2u) * 8u, &value, 1u);
        }
        if (name == "patch-prefix") m_headerHandseedBitOffset = m_headerDataStartBit - 1u;
        if (name == "patch-crc-bit") m_headerHandseedBitOffset = crc * 8u - 31u;
        if (name == "invalid-data-origin") m_headerDataStartBit = crc * 8u + 1u;
    }
    bool restoreSealing(const SealingState& state) {
        m_sectionOffsets[0] = state.offset;
        m_sectionSizes[0] = state.size;
        m_headerHandseedBitOffset = state.patch;
        m_headerDataStartBit = state.dataStart;
        return m_buf.patchRawBytesAtBit(0u, state.bytes.data(), state.bytes.size());
    }
};

class HeaderPublicationSink final : public dx_iface {
public:
    HeaderPublicationSink() {
        cData = &data;
        currentBlock = data.mBlock;
    }
    void addHeader(const DRW_Header* header) override {
        ++headers;
        handseed = header->getHandSeed();
    }
    void addLine(const DRW_Line& line) override {
        ++lines;
        lineEnd = line.secPoint;
    }
    std::size_t headers{0};
    std::size_t lines{0};
    std::uint32_t handseed{0};
    DRW_Coord lineEnd;
    dx_data data;
};

void testTrackedHeaderRead(TestContext& t) {
    class Sink final : public dx_iface {
    public:
        Sink() {
            cData = &data;
            currentBlock = data.mBlock;
        }
        void addHeader(const DRW_Header* header) override {
            ++headers;
            dx_iface::addHeader(header);
            const std::array<const char*, 4> keys{{"LUNITS", "INSUNITS", "LUPREC", "DIMDEC"}};
            for (std::size_t i = 0; i < keys.size(); ++i) {
                const auto found = header->vars.find(keys[i]);
                if (found != header->vars.end() && found->second != nullptr
                    && found->second->type() == DRW_Variant::INTEGER)
                    values[i] = found->second->content.i;
            }
        }
        void addText(const DRW_Text& text) override {
            rtext += dynamic_cast<const DRW_RText*>(&text) != nullptr;
            arcText += dynamic_cast<const DRW_ArcAlignedText*>(&text) != nullptr;
        }
        void addMPolygon(const DRW_MPolygon*) override { ++mpolygon; }
        void addNamedObjectDictionary(const DRW_Dictionary& dictionary) override {
            ++roots;
            dx_iface::addNamedObjectDictionary(dictionary);
        }
        std::array<int, 4> values{{-1, -1, -1, -1}};
        std::size_t headers{0};
        std::size_t roots{0};
        std::size_t rtext{0}, arcText{0}, mpolygon{0};
        dx_data data;
    };
    struct Row {
        const char* name;
        std::size_t frames;
        std::array<int, 4> values;
        bool compatible;
    };
    const std::array<Row, 4> rows{{
        {"rtext_arctext.dwg", 267u, {{2, 4, 4, 2}}, true},
        {"mpolygon_solid.dwg", 95u, {{2, 0, 4, 4}}, true},
        {"large_radial.dwg", 110u, {{2, 0, 4, 4}}, true},
        {"ordinary_enc_AC1027.dwg", 26u, {{2, 0, 4, 4}}, false}
    }};
    for (const auto& row : rows) {
        const auto path = (std::filesystem::path(LIBDXFRW_READER_FIXTURE_DIR) / row.name).string();
        dwgRW reader(path.c_str());
        Sink sink;
        const bool read = reader.read(&sink, false);
        const auto observations = reader.getReadObservations();
        t.expect(read && reader.getError() == DRW::BAD_NONE && sink.headers == 1u
                     && sink.values == row.values,
                 "tracked HEADER exact public fields: " + std::string(row.name));
        t.expect(observations.frames.status == DRW_DwgFrameCoverageStatus::FinalizedComplete
                     && observations.frames.sourceFrames == row.frames,
                 "tracked physical frame accounting (not semantic qualification): " + std::string(row.name));
        const auto root = sink.data.headerC.dwgNamedObjectDictionaryReference();
        t.expect(root.has_value() && root->ref64 == (row.compatible ? 12u : 0u)
                     && sink.roots == (row.compatible ? 1u : 0u)
                     && sink.data.hasNamedObjectDictionary == row.compatible,
                 "DWG root identity follows the source HEADER, not handle/order guessing");
        if (row.compatible && root) {
            t.expect(root->code == DRW::DwgHardOwnership
                         && sink.data.namedObjectDictionary.handle == root->ref64
                         && sink.data.namedObjectDictionary.hasCompleteDwgEntries()
                         && std::none_of(sink.data.dictionaries.begin(), sink.data.dictionaries.end(),
                                [&root](const DRW_Dictionary& dictionary) {
                                    return dictionary.handle == root->ref64;
                                }),
                     "validated DWG NOD is published once on its explicit root path");
            DRW_Header copy(sink.data.headerC);
            DRW_Header moved(std::move(copy));
            DRW_Header assigned;
            assigned = moved;
            const auto movedRoot = moved.dwgNamedObjectDictionaryReference();
            const auto assignedRoot = assigned.dwgNamedObjectDictionaryReference();
            t.expect(!copy.dwgNamedObjectDictionaryReference()
                         && movedRoot && movedRoot->ref64 == root->ref64
                         && assignedRoot && assignedRoot->ref64 == root->ref64,
                     "DWG root-reference copies/moves own the captured value after HEADER delivery");
        }
        std::size_t layouts = 0, tails = 0;
        for (std::size_t i = 0; i < observations.integrity.retained; ++i) {
            const auto& diagnostic = observations.integrity.rows[i];
            layouts += diagnostic.kind == DwgIntegrityCheckKind::HeaderSectionCompatibility;
            tails += diagnostic.kind == DwgIntegrityCheckKind::HeaderSectionUnclaimedData;
        }
        t.expect(layouts == (row.compatible ? 1u : 0u)
                     && tails == (row.compatible ? 1u : 0u),
                 "tracked HEADER compatibility evidence survives facade snapshot");
        t.expect(sink.data.visualStyles.size() == (row.compatible ? 24u : 0u),
                 "tracked VISUALSTYLE source cardinality survives bounded decoding");
        for (const auto& style : sink.data.visualStyles) {
            const auto& metadata = style.m_body.m_encoding;
            t.expect(style.m_bodyDecoded && metadata.companionCount == 58u
                         && metadata.colorCount == 8u && metadata.hasSourceIsolines,
                     "tracked modern VISUALSTYLE publishes complete source metadata");
            if (style.desc == "Flat") {
                t.expect(metadata.colors[0].rawIndex == 0u
                             && metadata.colors[0].rawRgb == 0xC2FFFFFFu
                             && metadata.colors[2].rawRgb == 0xC8000000u
                             && metadata.colors[5].rawRgb == 0xC2000000u
                             && metadata.companions[51] == 0u
                             && metadata.companions[52] == 0u
                             && style.m_body.faceMonoColor == 256
                             && style.m_body.edgeObscuredColor == 256,
                         "tracked Flat style retains RGB white/black/C8 and source operation zeros");
            }
        }
        if (std::string(row.name) == "rtext_arctext.dwg")
            t.expect(sink.rtext == 1u && sink.arcText == 1u,
                     "tracked RTEXT/ARCTEXT concrete callbacks survive HEADER admission");
        if (std::string(row.name) == "mpolygon_solid.dwg")
            t.expect(sink.mpolygon == 1u, "tracked MPOLYGON callback survives HEADER admission");
    }
}

template<class Writer>
void testHeaderWriterSealing(TestContext& t, const std::string& version) {
    for (const std::uint32_t suppliedSeed : {0u, 0x12345678u}) {
        std::stringbuf output(std::ios::out | std::ios::binary);
        std::ofstream stream;
        static_cast<std::ostream&>(stream).rdbuf(&output);
        DRW_Header header;
        header.setHandSeed(suppliedSeed);
        Writer writer(&stream, &header);
        writer.reserveHandle(0x7000u);
        const auto label = version + " seed=" + std::to_string(suppliedSeed);
        const bool prepared = writer.prepareDwgClassManifest()
            && writer.writeFileHeaderStub();
        t.expect(prepared, "HEADER writer preparation: " + label);
        if (!prepared) continue;
        const auto start = writer.buffer().size();
        if (!writer.writeDwgHeader()) {
            t.expect(false, "HEADER writer emission: " + label);
            continue;
        }
        const auto sectionSize = writer.buffer().size() - start;
        const auto crcOffset = start + sectionSize - 18u;
        const auto storedCrc = [&writer, crcOffset]() {
            return std::uint16_t(writer.buffer()[crcOffset])
                | (std::uint16_t(writer.buffer()[crcOffset + 1u]) << 8u);
        };
        t.expect(storedCrc() == headerTestCrc(writer.buffer(), start + 16u, crcOffset),
                 "initial HEADER CRC matches independent bitwise projection: " + label);
        DRW_Line line;
        line.basePoint = DRW_Coord(1, 2, 3);
        line.secPoint = DRW_Coord(4, 5, 6);
        const bool objects = writer.writeDwgClasses() && writer.writeDwgObjects()
            && writer.encodeEntity(&line) && writer.emitDeferredBlockControl();
        t.expect(objects, "HEADER writer following-record preparation: " + label);
        if (!objects) continue;
        // An unemitted reservation must also contribute to the high-water
        // seed; maxEmittedHandle+1 alone is not the complete existing policy.
        writer.reserveHandle(0x9000u);
        std::uint32_t maxHandle = 0;
        t.expect(writer.validateEmittedHandleMap(maxHandle), "HEADER writer handle map: " + label);
        const auto before = writer.buffer();
        t.expect(writer.finalizeHeaderHandseed(), "final HANDSEED patch: " + label);
        const auto finalSeed = header.getHandSeed();
        t.expect(finalSeed > maxHandle && finalSeed >= writer.highWaterHandle()
                     && (suppliedSeed == 0u || finalSeed == suppliedSeed),
                 "HANDSEED retains supplied/high-water bounds: " + label);
        t.expect(storedCrc() == headerTestCrc(writer.buffer(), start + 16u, crcOffset),
                 "final HEADER CRC matches independent bitwise projection: " + label);
        const std::size_t dataPrefix = version == "AC1015" || version == "AC1018"
            ? 20u : version == "AC1027" || version == "AC1032" ? 28u : 24u;
        const auto patchBit = (start + dataPrefix) * 8u + header.dwgHandseedBitOffset();
        bool otherBitsExact = before.size() == writer.buffer().size();
        for (std::size_t byte = 0; otherBitsExact && byte < before.size(); ++byte) {
            const auto changed = std::uint8_t(before[byte] ^ writer.buffer()[byte]);
            for (unsigned int bit = 0; bit < 8u; ++bit) {
                const auto at = byte * 8u + bit;
                if (at >= patchBit && at < patchBit + 32u) continue;
                if (byte >= crcOffset && byte < crcOffset + 2u) continue;
                if ((changed & (1u << (7u - bit))) != 0u) otherBitsExact = false;
            }
        }
        t.expect(otherBitsExact,
                 "only HANDSEED bits/CRC change, including unaligned neighbors: " + label);
        if (suppliedSeed != 0u)
            t.expect(writer.buffer() == before, "unchanged high HANDSEED leaves all bytes exact: " + label);
        const auto once = writer.buffer();
        t.expect(writer.finalizeHeaderHandseed() && writer.buffer() == once,
                 "HANDSEED patch/reseal is idempotent, not container finalization: " + label);
        t.expect(output.str().empty(), "HANDSEED finalization publishes no file prefix: " + label);
        const bool published = writer.writeDwgHandles() && writer.writeSecondHeader()
            && writer.finalize();
        t.expect(published, "sealed HEADER container publication: " + label);
        if (!published) continue;
        const auto file = output.str();
        std::vector<std::uint8_t> bytes(file.begin(), file.end());
        HeaderPublicationSink sink;
        dwgRW reader("");
        t.expect(reader.readBuffer(bytes.data(), bytes.size(), &sink, false)
                     && sink.headers == 1u && sink.handseed == finalSeed
                     && sink.lines == 1u && sink.lineEnd.x == 4
                     && sink.lineEnd.y == 5 && sink.lineEnd.z == 6,
                 "sealed complete file retains HEADER and following LINE: " + label);
        const auto diagnostics = reader.getIntegrityDiagnostics();
        t.expect(std::none_of(diagnostics.begin(), diagnostics.end(), [](const auto& entry) {
                     return entry.kind == DwgIntegrityCheckKind::HeaderSectionCrc;
                 }), "clean writer needs no HEADER CRC warning: " + label);
    }
}

void testHeaderWriters(TestContext& t) {
    testHeaderWriterSealing<dwgWriter15>(t, "AC1015");
    testHeaderWriterSealing<dwgWriter18>(t, "AC1018");
    testHeaderWriterSealing<dwgWriter21>(t, "AC1021");
    testHeaderWriterSealing<dwgWriter24>(t, "AC1024");
    testHeaderWriterSealing<dwgWriter27>(t, "AC1027");
    testHeaderWriterSealing<dwgWriter32>(t, "AC1032");
}

void testHeaderWriterSealingFailures(TestContext& t) {
    std::stringbuf output(std::ios::out | std::ios::binary);
    std::ofstream stream;
    static_cast<std::ostream&>(stream).rdbuf(&output);
    DRW_Header header;
    FullHeaderWriterProbe writer(&stream, &header);
    const bool prepared = writer.prepareDwgClassManifest() && writer.writeFileHeaderStub()
        && writer.writeDwgHeader() && writer.writeDwgClasses()
        && writer.writeDwgObjects() && writer.emitDeferredBlockControl();
    t.expect(prepared, "writer sealing negative controls have valid preceding records");
    if (!prepared) return;
    const auto valid = writer.sealingState();
    t.expect(valid.patch % 8u != 0u, "actual HEADER HANDSEED patch is unaligned");
    for (const std::string name : {"missing-range", "outside-range", "short-range",
            "long-range", "declared-max", "declared-short", "begin-sentinel",
            "end-sentinel", "patch-prefix", "patch-crc-bit", "invalid-data-origin"}) {
        writer.corruptSealing(name);
        const auto corrupted = writer.buffer();
        const auto priorSeed = header.getHandSeed();
        t.expect(!writer.finalizeHeaderHandseed() && writer.buffer() == corrupted
                     && header.getHandSeed() == priorSeed && output.str().empty(),
                 "invalid HEADER sealing rejects before any mutation/publication: " + name);
        t.expect(writer.restoreSealing(valid) && writer.finalizeHeaderHandseed(),
                 "corrected internal HEADER range can be retried: " + name);
        const auto crc = valid.offset + valid.size - 18u;
        const auto stored = std::uint16_t(writer.buffer()[crc])
            | (std::uint16_t(writer.buffer()[crc + 1u]) << 8u);
        t.expect(stored == headerTestCrc(writer.buffer(), valid.offset + 16u, crc),
                 "successful retry reseals the independently checked HEADER: " + name);
    }
}

void testHeaderPublicRead(TestContext& t) {
    // The legacy writer takes an ofstream; its ostream buffer can be owned
    // memory, so complete-file controls need no fixture file or filesystem.
    std::stringbuf output(std::ios::out | std::ios::binary);
    std::ofstream stream;
    static_cast<std::ostream&>(stream).rdbuf(&output);
    DRW_Header source;
    FullHeaderWriterProbe writer(&stream, &source);
    auto phase = [&t](bool result, const std::string& name) {
        t.expect(result, "complete AC1015 memory writer phase: " + name);
        return result;
    };
    const bool generated = phase(writer.prepareDwgClassManifest(), "class manifest")
        && phase(writer.writeFileHeaderStub(), "file header")
        && phase(writer.writeDwgHeader(), "HEADER")
        && phase(writer.writeDwgClasses(), "CLASSES")
        && phase(writer.writeDwgObjects(), "objects")
        && phase(writer.emitDeferredBlockControl(), "block control")
        && phase(writer.finalizeHeaderHandseed(), "header HANDSEED")
        && phase(writer.writeDwgHandles(), "handles")
        && phase(writer.writeSecondHeader(), "second header")
        && phase(writer.finalize(), "finalize");
    t.expect(generated, "complete from-scratch AC1015 source writes to an owned memory stream");
    if (!generated) return;
    const auto data = output.str();
    std::vector<std::uint8_t> original(data.begin(), data.end());
    const auto headerStart = writer.headerAddress();
    const auto classesStart = writer.classesAddress();
    const auto count = std::uint32_t(original[headerStart + 16u])
        | (std::uint32_t(original[headerStart + 17u]) << 8u)
        | (std::uint32_t(original[headerStart + 18u]) << 16u)
        | (std::uint32_t(original[headerStart + 19u]) << 24u);
    const auto crc = headerStart + 20u + count;
    const auto writerStoredCrc = std::uint16_t(original[crc])
        | (std::uint16_t(original[crc + 1u]) << 8u);
    const auto correctedCrc = headerTestCrc(original, headerStart + 16u, crc);
    HeaderPublicationSink unmodifiedSink;
    dwgRW unmodifiedReader("");
    const bool unmodifiedAccepted = unmodifiedReader.readBuffer(
        original.data(), original.size(), &unmodifiedSink, false);
    const auto unmodifiedDiagnostics = unmodifiedReader.getIntegrityDiagnostics();
    const auto unmodifiedWarnings = std::count_if(
        unmodifiedDiagnostics.begin(), unmodifiedDiagnostics.end(), [](const auto& entry) {
            return entry.kind == DwgIntegrityCheckKind::HeaderSectionCrc;
        });
    t.expect(unmodifiedAccepted && unmodifiedSink.headers == 1u
                 && unmodifiedWarnings == (writerStoredCrc == correctedCrc ? 0 : 1),
             "unmodified writer output retains checksum-warning compatibility");
    // Keep the reader mutation controls independent of writer sealing. This
    // test-only normalization cannot qualify production writer integrity;
    // testHeaderWriters checks the unmodified production CRC independently.
    original[crc] = static_cast<std::uint8_t>(correctedCrc);
    original[crc + 1u] = static_cast<std::uint8_t>(correctedCrc >> 8u);
    dwgRW reader("");
    for (const std::string name : {"valid", "crc", "end-and-classes", "valid-again"}) {
        auto bytes = original;
        if (name == "crc") bytes[crc] ^= 1u;
        if (name == "end-and-classes") {
            bytes[crc + 2u] ^= 1u;
            std::fill(bytes.begin() + classesStart + 16u,
                      bytes.begin() + classesStart + 20u, 0xFFu);
        }
        HeaderPublicationSink sink;
        const bool accepted = reader.readBuffer(bytes.data(), bytes.size(), &sink, false);
        const bool structural = name == "end-and-classes";
        t.expect(accepted == !structural && sink.headers == (structural ? 0u : 1u),
                 "public HEADER success/publication boundary: " + name
                     + " (error=" + std::to_string(reader.getError())
                     + ", callbacks=" + std::to_string(sink.headers) + ")");
        t.expect(reader.getError() == (structural ? DRW::BAD_READ_HEADER : DRW::BAD_NONE),
                 "public first HEADER error remains sticky and resets on reuse: " + name);
        const auto diagnostics = reader.getIntegrityDiagnostics();
        const auto warnings = std::count_if(diagnostics.begin(), diagnostics.end(), [](const auto& entry) {
            return entry.kind == DwgIntegrityCheckKind::HeaderSectionCrc;
        });
        t.expect(warnings == (name == "crc" ? 1 : 0),
                 "public HEADER diagnostics reset between read operations: " + name);
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

class TemplateProbe final : public dwgReader15 {
public:
    explicit TemplateProbe(DRW::Version target)
        : dwgReader15(nullptr, nullptr) { version = target; }
    using dwgReader::decodeDwgTemplate;
    using dwgReader::missingDwgTemplate;
    const std::vector<DwgIntegrityDiagnostic>& diagnostics() const {
        return m_integrityDiagnostics;
    }
};

void testTemplateFields(TestContext& t) {
    for (auto version : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                          DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        for (std::uint8_t value : {0, 1}) {
            const std::array<std::uint8_t, 6> padded{{0, 0, value, 0, 0x7f, 0x6a}};
            std::uint16_t measurement = 99;
            t.expect(dwgTemplate::decode(padded.data(), 4, version, measurement)
                         == dwgTemplate::DecodeResult::Valid
                         && measurement == value,
                     "Template consumes logical bytes, not page padding");
            t.expect(dwgTemplate::decode(padded.data(), padded.size(), version,
                                         measurement)
                         == dwgTemplate::DecodeResult::Malformed,
                     "Template does not scan trailing page bytes for a value");
            if (version >= DRW::AC1021) {
                const std::array<std::uint8_t, 5> canonicalNul{{1, 0, 0, value, 0}};
                t.expect(dwgTemplate::decode(canonicalNul.data(), canonicalNul.size(),
                                             version, measurement)
                             == dwgTemplate::DecodeResult::Valid
                             && measurement == value,
                         "modern canonical one-byte NUL description uses ODA byte count");
                const std::array<std::uint8_t, 6> oldEmpty{{1, 0, 0, 0, value, 0}};
                t.expect(dwgTemplate::decode(oldEmpty.data(), oldEmpty.size(),
                                             version, measurement)
                             == dwgTemplate::DecodeResult::Valid
                             && measurement == value,
                         "modern one-unit UTF16 NUL Template remains compatible");
            }
        }
        TemplateProbe missing(version);
        t.expect(missing.missingDwgTemplate(), "missing Template remains compatible");
        t.expect(version < DRW::AC1018 ? missing.diagnostics().empty()
                     : missing.diagnostics().size() == 1
                         && missing.diagnostics()[0].kind
                             == DwgIntegrityCheckKind::TemplateMissing
                         && missing.diagnostics()[0].severity
                             == DwgIntegritySeverity::Warning,
                 "mandatory modern Template absence has a bounded named warning");
        std::vector<std::uint8_t> maximumDescription(0x7fffu + 4u, 'A');
        maximumDescription[0] = 0xff;
        maximumDescription[1] = 0x7f;
        maximumDescription[maximumDescription.size() - 2] = 1;
        maximumDescription.back() = 0;
        std::uint16_t maximumMeasurement = 99;
        t.expect(dwgTemplate::decode(maximumDescription.data(),
                                     maximumDescription.size(), version,
                                     maximumMeasurement)
                     == dwgTemplate::DecodeResult::Valid
                     && maximumMeasurement == 1,
                 "Template accepts the largest signed byte-count description");
    }
    for (const std::vector<std::uint8_t>& bytes : {
             std::vector<std::uint8_t>{}, {0, 0}, {0, 0, 1}, {0, 0, 2, 0},
             {0xff, 0xff, 0, 0}, {0, 0x80, 0, 0},
             {4, 0, 'A', 0, 1, 0}, {1, 0, 0, 0, 1}, {1, 0, 0, 0, 2, 0}}) {
        TemplateProbe reader(DRW::AC1024);
        DRW_Header header;
        t.expect(!reader.decodeDwgTemplate(header, bytes.data(), bytes.size(), 5)
                     && header.vars.empty()
                     && reader.diagnostics().back().kind
                         == DwgIntegrityCheckKind::TemplateMalformed,
                 "malformed Template publishes no partial header variable");
    }
    const std::array<std::uint8_t, 7> legacyDescription{{3, 0, 'A', 'B', 'C', 1, 0}};
    std::uint16_t measurement = 99;
    for (auto version : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                          DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        measurement = 99;
        t.expect(dwgTemplate::decode(legacyDescription.data(), legacyDescription.size(),
                                     version, measurement)
                     == dwgTemplate::DecodeResult::Valid && measurement == 1,
                 "all versions use the specified Template description byte count");
    }
    TemplateProbe modern(DRW::AC1024);
    DRW_Header header;
    const std::array<std::uint8_t, 8> noncanonicalModern{{2, 0, 'A', 0, 'B', 0, 1, 0}};
    t.expect(modern.decodeDwgTemplate(header, noncanonicalModern.data(),
                                      noncanonicalModern.size(), 5)
                 && header.vars.empty()
                 && modern.diagnostics().back().kind
                     == DwgIntegrityCheckKind::TemplateDescriptionUnsupported,
             "noncanonical modern description remains explicit, not guessed");
}

class TemplateSink final : public dx_iface {
public:
    TemplateSink() { cData = &data; currentBlock = data.mBlock; }
    void addHeader(const DRW_Header* header) override {
        ++headers;
        for (const auto& field : header->vars) {
            if (field.first == "MEASUREMENT") measurement = field.second->i_val();
            if (field.first == "INSUNITS") units = field.second->i_val();
        }
    }
    void addLine(const DRW_Line& line) override {
        if (line.basePoint.x == 11 && line.secPoint.z == 29) ++lines;
    }
    dx_data data;
    int headers = 0, measurement = -9, units = -9, lines = 0;
};

template<class Writer>
void testTemplatePublication(TestContext& t, dwgRW& reader, int measurement) {
    std::stringbuf bytes(std::ios::out | std::ios::binary);
    std::ofstream output;
    static_cast<std::ostream&>(output).rdbuf(&bytes);
    DRW_Header header;
    header.addInt("$MEASUREMENT", measurement, 70);
    const int units = measurement == 0 ? 4 : 1;
    header.addInt("$INSUNITS", units, 70);
    Writer writer(&output, &header);
    DRW_Line line;
    line.basePoint = {11, 13, 17}; line.secPoint = {19, 23, 29};
    const bool written = writer.prepareDwgClassManifest()
        && writer.writeFileHeaderStub() && writer.writeDwgHeader()
        && writer.writeDwgClasses() && writer.writeDwgObjects()
        && writer.encodeEntity(&line) && writer.emitDeferredBlockControl()
        && writer.finalizeHeaderHandseed() && writer.writeDwgHandles()
        && writer.writeSecondHeader() && writer.finalize();
    t.expect(written, "Template six-target memory writer completes");
    if (!written) return;
    const auto encoded = bytes.str();
    std::vector<std::uint8_t> source(encoded.begin(), encoded.end());
    const auto before = source;
    TemplateSink sink;
    t.expect(reader.readBuffer(source.data(), source.size(), &sink, false)
                 && sink.headers == 1 && sink.measurement == measurement
                 && sink.units == units && sink.lines == 1 && source == before,
             "one public header publishes Template measurement independent of units");
    if (std::is_same<Writer, dwgWriter15>::value) {
        // The Template locator is record4 in this locally generated file.
        std::size_t at = 0;
        for (unsigned i = 0; i < source[21]; ++i) {
            const std::size_t record = 25 + 9u * i;
            if (source[record] != 4) continue;
            at = static_cast<std::size_t>(source[record + 1])
                | static_cast<std::size_t>(source[record + 2]) << 8
                | static_cast<std::size_t>(source[record + 3]) << 16
                | static_cast<std::size_t>(source[record + 4]) << 24;
        }
        t.expect(at != 0 && at + 4 <= source.size(), "legacy Template locator is present");
        if (at != 0 && at + 4 <= source.size()) {
            source[at + 2] = 2;
            TemplateSink invalid;
            t.expect(!reader.readBuffer(source.data(), source.size(), &invalid, false)
                         && reader.getError() == DRW::BAD_READ_HEADER
                         && invalid.headers == 0 && invalid.lines == 0,
                     "invalid Template fails Header before public callback");
            TemplateSink retry;
            t.expect(reader.readBuffer(before.data(), before.size(), &retry, false)
                         && retry.measurement == measurement && retry.headers == 1,
                     "same public reader recovers without stale Template state");
            source = before;
            const unsigned count = source[21];
            for (unsigned i = 0; i < count; ++i) {
                const std::size_t record = 25 + 9u * i;
                if (source[record] == 4)
                    std::fill(source.begin() + record + 1,
                              source.begin() + record + 9, 0);
            }
            const std::size_t crcOffset = 25 + 9u * count;
            dwgBuffer crcBuffer(source.data(), source.size());
            const std::uint16_t seeds[] = {0, 0, 0, 0xa598, 0x8101, 0x3cc4, 0x8461};
            const auto crc = static_cast<std::uint16_t>(
                crcBuffer.crc8(0, 0, static_cast<std::int32_t>(crcOffset))
                ^ seeds[count]);
            source[crcOffset] = static_cast<std::uint8_t>(crc);
            source[crcOffset + 1] = static_cast<std::uint8_t>(crc >> 8);
            TemplateSink absent;
            t.expect(reader.readBuffer(source.data(), source.size(), &absent, false)
                         && absent.headers == 1 && absent.measurement == -9,
                     "absent optional legacy Template cannot reuse prior measurement");
        }
    }
}

void testTemplateFilePublication(TestContext& t) {
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-template-" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!std::filesystem::create_directory(directory)) {
        t.expect(false, "Template test scratch directory creation");
        return;
    }
    const auto path = directory / "destination.dwg";
    for (auto version : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                          DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        for (int value : {-1, 2, (std::numeric_limits<int>::max)()}) {
            { std::ofstream existing(path, std::ios::binary); existing << "sentinel"; }
            TemplateSink source;
            source.data.headerC.addInt("$MEASUREMENT", value, 70);
            dwgRW writer(path.string().c_str());
            t.expect(!writer.write(&source, version, true)
                         && source.data.headerC.vars.at("$MEASUREMENT")->i_val() == value,
                     "invalid explicit measurement does not mutate caller header");
            std::ifstream unchanged(path, std::ios::binary);
            const std::string preserved((std::istreambuf_iterator<char>(unchanged)),
                                         std::istreambuf_iterator<char>());
            t.expect(preserved == "sentinel", "invalid Template preserves destination file");
            unchanged.close();
            source.data.headerC.addInt("$MEASUREMENT", 1, 70);
            t.expect(writer.write(&source, version, true),
                     "corrected public writer retries and publishes");
            TemplateSink result;
            dwgRW reader(path.string().c_str());
            t.expect(reader.read(&result, false) && result.headers == 1
                         && result.measurement == 1,
                     "file reader publishes corrected Template measurement");
        }
    }
    std::filesystem::remove(path);
    std::filesystem::remove(directory);
}

void testTemplatePublicMatrix(TestContext& t) {
    dwgRW reader("");
    for (int measurement : {0, 1}) {
        testTemplatePublication<dwgWriter15>(t, reader, measurement);
        testTemplatePublication<dwgWriter18>(t, reader, measurement);
        testTemplatePublication<dwgWriter21>(t, reader, measurement);
        testTemplatePublication<dwgWriter24>(t, reader, measurement);
        testTemplatePublication<dwgWriter27>(t, reader, measurement);
        testTemplatePublication<dwgWriter32>(t, reader, measurement);
    }
}

}  // namespace

int main() {
    TestContext context;
    testDisjointRangeIndex(context);
    testExactUnknownSectionNames(context);
    testR2004SectionMapPublication(context);
    testModelerBlockRecordDispatch(context);
    testModelerBlockWalkRejectsBadOwnership(context);
    testBoundedFork(context);
    testHeaderSectionBounds(context);
    testHeaderVersionEnvelopes(context);
    testDwgNodReference(context);
    testHeaderCompatibilityProfile(context);
    testTrackedHeaderRead(context);
    testHeaderPublicRead(context);
    testHeaderWriters(context);
    testHeaderWriterSealingFailures(context);
    testDwgOperationAdmission(context);
    testVersionDispatch(context);
    testReadBufferRejection(context);
    testSectionNameMatrix(context);
    testSectionNameUtf16Framing(context);
    testR2007ClassStringFooter(context);
    testTemplateFields(context);
    testTemplatePublicMatrix(context);
    testTemplateFilePublication(context);
    if (context.failures != 0) {
        std::cerr << context.failures << " DWG reader matrix assertion(s) failed\n";
        return 1;
    }
    std::cout << "DWG reader matrix tests: PASS\n";
    return 0;
}
