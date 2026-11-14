// SPDX-License-Identifier: GPL-2.0-or-later
// Generated SORTENTSTABLE OBJECTS allocation rollback/retry control.
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"

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

class DxfSortEntsTableTestAccess {
public:
    static void attachWriter(dxfRW& codec, DRW::Version version,
                             bool binary, std::ostream& output) {
        codec.version = version;
        codec.binFile = binary;
        codec.writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&output))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&output));
    }

    static bool writeEnd(dxfRW& codec) {
        return codec.writer != nullptr
            && codec.writer->writeString(0, "WIRE_END");
    }
};

namespace {
long allocationsBeforeFailure = -1;
std::size_t failedAllocations = 0u;

struct AllocationFailure {
    explicit AllocationFailure(long count) {
        allocationsBeforeFailure = count;
    }
    ~AllocationFailure() { allocationsBeforeFailure = -1; }
};

DRW_SortEntsTable makeSortTable(
    std::uint32_t owner = DRW::DxfModelSpaceBlockRecordHandle) {
    DRW_SortEntsTable table;
    table.handle = 0xD720u;
    table.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
    table.m_blockOwnerHandle = owner;
    table.m_entityHandles = {0xD731u, 0xD732u, 0xD733u, 0xD734u};
    table.m_sortHandles = {0xA101u, 0xA202u, 0xA303u, 0xA404u};
    table.extData.push_back(new DRW_Variant(1001, std::string("ACAD")));
    table.extData.push_back(new DRW_Variant(
        1000, std::string("sortents-allocation-retry")));
    return table;
}

class Export final : public LC3CompatCheck {
public:
    explicit Export(dxfRW& writer) : writer(writer), table(makeSortTable()) {}

    void writeObjects() override {
        if (failAt < 0) {
            accepted = writer.writeSortEntsTable(&table);
        } else {
            const std::size_t before = failedAllocations;
            AllocationFailure failure(failAt);
            try {
                accepted = writer.writeSortEntsTable(&table);
            } catch (const std::bad_alloc&) {
                accepted = false;
            }
            reached = failedAllocations != before
                ? static_cast<long>(failedAllocations - before)
                : failAt - allocationsBeforeFailure;
        }
        if (!accepted)
            (void)writer.writeRawDxfObject(nullptr);
    }

    void writeEntities() override {
        for (std::size_t i = 0; i < table.m_entityHandles.size(); ++i) {
            DRW_Line line;
            line.handle = table.m_entityHandles[i];
            line.parentHandle = table.m_blockOwnerHandle;
            line.space = table.m_blockOwnerHandle
                    == DRW::DxfPaperSpaceBlockRecordHandle
                ? DRW::PaperSpace : DRW::ModelSpace;
            line.basePoint = DRW_Coord(static_cast<double>(i), 1.0, 0.0);
            line.secPoint = DRW_Coord(static_cast<double>(i) + 1.0, 2.0, 0.0);
            if (!writer.writeLine(&line))
                accepted = false;
        }
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& writer;
    DRW_SortEntsTable table;
    long failAt = -1;
    long reached = 0;
    bool accepted = false;
};

class Reader final : public LC3CompatCheck {
public:
    struct LineOwner {
        std::uint32_t handle;
        std::uint32_t owner;
        DRW::Space space;
    };
    void addLine(const DRW_Line& line) override {
        lines.push_back({line.handle, line.parentHandle, line.space});
    }
    void addSortEntsTable(const DRW_SortEntsTable& table) override {
        tables.push_back(table);
    }
    std::vector<LineOwner> lines;
    std::vector<DRW_SortEntsTable> tables;
};

bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

bool readbackMatches(
    const std::filesystem::path& path,
    std::uint32_t owner = DRW::DxfModelSpaceBlockRecordHandle) {
    dxfRW readerCodec(path.string().c_str());
    Reader reader;
    const bool read = readerCodec.read(&reader, false);
    if (!read || reader.tables.size() != 1u || reader.lines.size() != 4u) {
        std::cerr << "SORTENTSTABLE readback census failed: read=" << read
                  << " tables=" << reader.tables.size()
                  << " lines=" << reader.lines.size() << '\n';
        return false;
    }
    const DRW_SortEntsTable& table = reader.tables.front();
    const DRW_SortEntsTable expected = makeSortTable(owner);
    if (table.handle != expected.handle
        || table.parentHandle != expected.parentHandle
        || table.m_blockOwnerHandle != owner
        || table.m_entityHandles.size() != expected.m_entityHandles.size()
        || table.m_sortHandles != expected.m_sortHandles
        || table.extData.size() != expected.extData.size()) {
        std::cerr << "SORTENTSTABLE field mismatch: handle=" << table.handle
                  << " parent=" << table.parentHandle
                  << " owner=" << table.m_blockOwnerHandle
                  << " pairs=" << table.m_entityHandles.size() << "/"
                  << table.m_sortHandles.size()
                  << " EED=" << table.extData.size()
                  << " entityCountMatch="
                  << (table.m_entityHandles.size()
                      == expected.m_entityHandles.size())
                  << " sortMatch="
                  << (table.m_sortHandles == expected.m_sortHandles)
                  << " parentMatch="
                  << (table.parentHandle == expected.parentHandle) << '\n';
        return false;
    }
    for (std::size_t i = 0; i < expected.m_entityHandles.size(); ++i) {
        const DRW::Space expectedSpace =
            owner == DRW::DxfPaperSpaceBlockRecordHandle
                ? DRW::PaperSpace : DRW::ModelSpace;
        if (table.m_entityHandles[i] != reader.lines[i].handle
            || reader.lines[i].owner != owner
            || reader.lines[i].space != expectedSpace) {
            std::cerr << "SORTENTSTABLE target line mismatch at " << i
                      << ": handle=" << reader.lines[i].handle
                      << " tableTarget=" << table.m_entityHandles[i]
                      << " owner=" << reader.lines[i].owner << '\n';
            return false;
        }
    }
    for (std::size_t i = 0; i < expected.extData.size(); ++i) {
        if (!table.extData[i] || !expected.extData[i]
            || !table.extData[i]->content.s || !expected.extData[i]->content.s
            || table.extData[i]->code() != expected.extData[i]->code()
            || *table.extData[i]->content.s != *expected.extData[i]->content.s)
            return false;
    }
    return true;
}

bool sourceMatches(const DRW_SortEntsTable& actual) {
    const DRW_SortEntsTable expected = makeSortTable();
    if (actual.handle != expected.handle
        || actual.parentHandle != expected.parentHandle
        || actual.m_blockOwnerHandle != expected.m_blockOwnerHandle
        || actual.m_entityHandles != expected.m_entityHandles
        || actual.m_sortHandles != expected.m_sortHandles
        || actual.extData.size() != expected.extData.size())
        return false;
    for (std::size_t i = 0; i < expected.extData.size(); ++i) {
        if (!actual.extData[i] || !expected.extData[i]
            || !actual.extData[i]->content.s || !expected.extData[i]->content.s
            || actual.extData[i]->code() != expected.extData[i]->code()
            || *actual.extData[i]->content.s != *expected.extData[i]->content.s)
            return false;
    }
    return true;
}

bool directRecordReadback(const std::string& bytes, bool binary) {
    class SortEntsTableProbe final : public DRW_SortEntsTable {
    public:
        using DRW_SortEntsTable::parseCode;
    };
    std::istringstream input(bytes);
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(std::make_unique<dxfReaderBinary>(&input))
        : std::unique_ptr<dxfReader>(std::make_unique<dxfReaderAscii>(&input));
    int code = 0;
    if (!reader->readRec(&code) || code != 0
        || reader->getString() != "SORTENTSTABLE")
        return false;
    SortEntsTableProbe decoded;
    bool ended = false;
    while (reader->readRec(&code)) {
        if (code == 0) {
            ended = reader->getString() == "WIRE_END";
            break;
        }
        if (!decoded.parseCode(code, reader))
            return false;
    }
    return ended && sourceMatches(decoded);
}

bool directRecordAllocationSweep() {
    constexpr long calibrationLimit = 1000000;
    const DRW::Version versions[] = {
        DRW::AC1014, DRW::AC1015, DRW::AC1018, DRW::AC1021,
        DRW::AC1024, DRW::AC1027, DRW::AC1032};
    const auto writeDirect = [](dxfRW& codec, DRW_SortEntsTable& table) {
        return codec.writeSortEntsTable(&table);
    };
    for (const DRW::Version version : versions) {
      for (const bool binary : {false, true}) {
        std::string expectedBytes;
        long allocationCount = 0;
        {
            std::ostringstream bytes;
            dxfRW codec("");
            DxfSortEntsTableTestAccess::attachWriter(
                codec, version, binary, bytes);
            DRW_SortEntsTable table = makeSortTable();
            bool written = false;
            {
                AllocationFailure failure(calibrationLimit);
                written = writeDirect(codec, table);
                allocationCount = calibrationLimit
                    - allocationsBeforeFailure;
            }
            if (!written || !sourceMatches(table)
                || codec.getWritingContext()
                    ->sourceHandleToWrittenObjectMap.count(table.handle) != 1u
                || codec.getWritingContext()
                    ->sourceHandleToWrittenObjectMap.at(table.handle)
                    != codec.remapHandle(table.handle)
                || !DxfSortEntsTableTestAccess::writeEnd(codec))
                return false;
            expectedBytes = bytes.str();
            if (!directRecordReadback(expectedBytes, binary))
                return false;
        }
        if (allocationCount <= 0)
            return false;

        for (long offset = 0; offset < allocationCount; ++offset) {
            std::ostringstream bytes;
            dxfRW codec("");
            DxfSortEntsTableTestAccess::attachWriter(
                codec, version, binary, bytes);
            DRW_SortEntsTable table = makeSortTable();
            const std::size_t failuresBefore = failedAllocations;
            bool written = false;
            try {
                AllocationFailure failure(offset);
                written = writeDirect(codec, table);
            } catch (const std::bad_alloc&) {
                allocationsBeforeFailure = -1;
            }
            allocationsBeforeFailure = -1;
            const DRW_WritingContext* context = codec.getWritingContext();
            // A low-level record commit can encounter a short write after a
            // generic ostream accepted a prefix. Whole-file allocationSweep
            // separately verifies that the enclosing output transaction
            // preserves the destination sentinel and retries successfully.
            if (written || failedAllocations == failuresBefore
                || !sourceMatches(table)
                || context == nullptr
                || context->sourceHandleToWrittenObjectMap.count(table.handle)
                    != 0u) {
                std::cerr << "direct SORTENTSTABLE allocation rollback failed at "
                          << (binary ? "Binary" : "ASCII")
                          << " offset=" << offset
                          << " wrote=" << written
                          << " allocationFailed="
                          << (failedAllocations != failuresBefore)
                          << " sourcePreserved=" << sourceMatches(table)
                          << " identityAbsent="
                          << (context != nullptr
                              && context->sourceHandleToWrittenObjectMap
                                     .count(table.handle) == 0u)
                          << '\n';
                return false;
            }
        }
        std::cout << (binary ? "Binary" : "ASCII")
                  << " direct SORTENTSTABLE allocation offsets version="
                  << static_cast<int>(version) << " count="
                  << allocationCount << '\n';
      }
    }
    return true;
}

bool allocationSweep(const std::filesystem::path& directory) {
    const auto path = directory / "sortents-allocation.dxf";
    const std::string prior = "prior SORTENTSTABLE destination\n";
    for (const bool binary : {false, true}) {
        long allocationCount = 0;
        std::string expectedBytes;
        {
            dxfRW writer(path.string().c_str());
            Export exporter(writer);
            exporter.failAt = 1000000;
            const bool written = writer.write(&exporter, DRW::AC1027, binary);
            if (!written || !exporter.accepted || !sourceMatches(exporter.table)
                || !readbackMatches(path) || !readBytes(path, expectedBytes)) {
                std::cerr << "clean SORTENTSTABLE write failed: write=" << written
                          << " accepted=" << exporter.accepted
                          << " source=" << sourceMatches(exporter.table)
                          << " readback=" << readbackMatches(path) << '\n';
                return false;
            }
            allocationCount = exporter.reached;
        }
        if (allocationCount <= 0)
            return false;

        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream output(path, std::ios::binary | std::ios::trunc);
                output << prior;
                if (!output)
                    return false;
            }
            dxfRW writer(path.string().c_str());
            Export exporter(writer);
            exporter.failAt = offset;
            const bool written = writer.write(&exporter, DRW::AC1027, binary);
            exporter.failAt = -1;
            std::string current;
            if (written || exporter.accepted || exporter.reached != 1
                || !readBytes(path, current) || current != prior
                || !sourceMatches(exporter.table))
                return false;

            const bool retried = writer.write(&exporter, DRW::AC1027, binary);
            std::string retryBytes;
            if (!retried || !exporter.accepted
                || !sourceMatches(exporter.table)
                || !readBytes(path, retryBytes)
                || retryBytes != expectedBytes || !readbackMatches(path))
                return false;
        }
        std::cout << (binary ? "Binary" : "ASCII")
                  << " SORTENTSTABLE writer allocation offsets="
                  << allocationCount << '\n';
    }
    return true;
}

bool ownerScopeControls(const std::filesystem::path& directory) {
    const auto path = directory / "sortents-owner-scope.dxf";
    const std::string prior = "prior SORTENTSTABLE owner-scope destination\n";
    for (const bool binary : {false, true}) {
        {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output << prior;
            if (!output)
                return false;
        }
        dxfRW writer(path.string().c_str());
        Export exporter(writer);
        exporter.table.m_blockOwnerHandle = 0x600u;
        const bool invalidWrite = writer.write(&exporter, DRW::AC1027, binary);
        std::string unchanged;
        if (invalidWrite || exporter.accepted
            || exporter.table.m_blockOwnerHandle != 0x600u
            || !readBytes(path, unchanged) || unchanged != prior)
            return false;

        exporter.table.m_blockOwnerHandle =
            DRW::DxfModelSpaceBlockRecordHandle;
        const bool modelRetry = writer.write(&exporter, DRW::AC1027, binary);
        if (!modelRetry || !exporter.accepted
            || !readbackMatches(path, DRW::DxfModelSpaceBlockRecordHandle))
            return false;

        exporter.table.m_blockOwnerHandle =
            DRW::DxfPaperSpaceBlockRecordHandle;
        const bool paperRetry = writer.write(&exporter, DRW::AC1027, binary);
        if (!paperRetry || !exporter.accepted
            || !readbackMatches(path, DRW::DxfPaperSpaceBlockRecordHandle))
            return false;
    }
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (allocationsBeforeFailure == 0) {
        ++failedAllocations;
        allocationsBeforeFailure = -1;
        throw std::bad_alloc();
    }
    if (allocationsBeforeFailure > 0)
        --allocationsBeforeFailure;
    if (void* value = std::malloc(size == 0u ? 1u : size))
        return value;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }
void operator delete(void* value, std::size_t) noexcept { std::free(value); }
void operator delete[](void* value, std::size_t) noexcept { std::free(value); }

int main() {
    const auto directory = std::filesystem::temp_directory_path()
        / ("sortents-objects-allocation-"
           + std::to_string(static_cast<long long>(
                 std::filesystem::file_time_type::clock::now()
                     .time_since_epoch().count())));
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return 1;
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
    } cleanup{directory};
    return directRecordAllocationSweep()
        && allocationSweep(directory) && ownerScopeControls(directory) ? 0 : 2;
}
