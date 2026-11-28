// Generated DXF-only DATATABLE projection controls; input files live in temp.
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include "intern/dxfwriter.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <new>
#include <string>
#include <vector>

namespace {
long allocationsBeforeFailure = -1;
std::size_t failedAllocations = 0u;
struct AllocationFailure {
    explicit AllocationFailure(long count) { allocationsBeforeFailure = count; }
    ~AllocationFailure() { allocationsBeforeFailure = -1; }
};
}

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

namespace {
struct Reader final : LC3CompatCheck {
    void addDataTable(const DRW_DataTable& value) override { tables.push_back(value); }
    void addXRecord(const DRW_XRecord& value) override { xrecords.push_back(value); }
    void addRawDxfObject(const DRW_RawDxfObject& value) override {
        if (value.name == "DATATABLE") raw.push_back(value);
    }
    std::vector<DRW_DataTable> tables;
    std::vector<DRW_XRecord> xrecords;
    std::vector<DRW_RawDxfObject> raw;
};

struct Export final : LC3CompatCheck {
    explicit Export(dxfRW& codec) : codec(codec) {
        table.handle = 0x115u;
        table.parentHandle = 0x114u;
        table.flags = 2;
        table.columnCount = 4;
        table.rowCount = 2;
        table.tableName = "TABLE_NAME";
        table.columns = {
            {2, "DOUBLE", {{0, 1.25, ""}, {0, 2.5, ""}}},
            {1, "INTEGER", {{7, 0.0, ""}, {8, 0.0, ""}}},
            {6, "TEXT", {{0, 0.0, "alpha"}, {0, 0.0, "beta"}}},
            {1, "INTEGER_2", {{-3, 0.0, ""}, {4, 0.0, ""}}}
        };
        table.rowHandles = {0x116u, 0x117u};
        firstChild.handle = 0x116u;
        firstChild.parentHandle = table.handle;
        firstChild.m_cloning = 1;
        firstChild.m_dataEntries.emplace_back(1, "alpha child record");
        secondChild.handle = 0x117u;
        secondChild.parentHandle = table.handle;
        secondChild.m_cloning = 1;
        secondChild.m_dataEntries.emplace_back(1, "beta child record");
    }

    dxfRW& codec;
    DRW_DataTable table;
    DRW_XRecord firstChild;
    DRW_XRecord secondChild;
    bool emitted{false};
    long failAt = -1;
    long reached = 0;

    void setTargetVersion(DRW::Version version) {
        const int cloning = version == DRW::AC1014 ? 0 : 1;
        firstChild.m_cloning = cloning;
        secondChild.m_cloning = cloning;
    }

    void writeObjects() override {
        bool tableWritten = false;
        if (failAt < 0) {
            tableWritten = codec.writeDataTable(&table);
        } else {
            const std::size_t before = failedAllocations;
            AllocationFailure failure(failAt);
            try {
                tableWritten = codec.writeDataTable(&table);
            } catch (const std::bad_alloc&) {
                tableWritten = false;
            }
            reached = failedAllocations != before
                ? static_cast<long>(failedAllocations - before)
                : failAt - allocationsBeforeFailure;
        }
        emitted = tableWritten && codec.writeXRecord(&firstChild)
            && codec.writeXRecord(&secondChild);
        if (!emitted)
            (void)codec.writeRawDxfObject(nullptr);
    }
    bool finalizeDxfWrite() override { return emitted; }
};

bool writeInput(const std::filesystem::path& path, bool binary,
                unsigned referenceCount = 2u,
                bool unsupportedColumnType = false,
                int malformedProjection = 0) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) return false;
    if (binary) {
        const char sentinel[] = "AutoCAD Binary DXF\r\n\x1a";
        file.write(sentinel, sizeof(sentinel));
    }
    std::unique_ptr<dxfWriter> writer = binary
        ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&file))
        : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&file));
    const auto str = [&](int code, const char* value) {
        return writer->writeString(code, value);
    };
    const auto integer = [&](int code, int value) {
        return code == 70 ? writer->writeInt16(code, value)
                          : writer->writeInt32(code, value);
    };
    if (!str(0, "SECTION") || !str(2, "HEADER")
        || !str(9, "$ACADVER") || !str(1, "AC1024") || !str(0, "ENDSEC")
        || !str(0, "SECTION") || !str(2, "OBJECTS")
        || !str(0, "DATATABLE") || !str(5, "115") || !str(330, "114")
        || !str(100, "AcDbDataTable") || !integer(70, 2)
        || (malformedProjection == 1 && !integer(70, 3))
        || !integer(90, 4)
        || (malformedProjection == 5 && !integer(90, 4))
        || !integer(91, 2)
        || (malformedProjection == 6 && !integer(91, 2))
        || !str(1, "TABLE_NAME")
        || (malformedProjection == 7 && !str(1, "TABLE_NAME_DUP"))
        || !integer(92, unsupportedColumnType ? 77 : 2)
        || (malformedProjection != 2 && !str(2, "DOUBLE"))
        || (malformedProjection == 3 && !str(2, "DUPLICATE_DOUBLE"))
        || !writer->writeDouble(40, 1.25) || !writer->writeDouble(40, 2.5)
        || !integer(92, 1) || !str(2, "INTEGER")
        || !integer(93, 7) || !integer(93, 8)
        || !integer(92, 6) || !str(2, "TEXT") || !str(3, "alpha")
        || !str(3, "beta") || !integer(92, 1)
        || (malformedProjection != 4 && !str(2, "INTEGER_2"))
        || !integer(93, -3) || !integer(93, 4)
        || (referenceCount >= 1u
            && !str(360, malformedProjection == 9 ? "115"
                         : malformedProjection == 10 ? "114" : "116")))
        return false;
    if (referenceCount >= 2u
        && !str(360, malformedProjection == 8 ? "116" : "117"))
        return false;
    return str(0, "ENDSEC") && str(0, "EOF") && !writer->hasWriteError();
}

bool verify(const DRW_DataTable& table, bool expectHandles,
            bool remapped = false) {
    const std::uint32_t tableHandle = remapped ? 0x215u : 0x115u;
    const std::uint32_t ownerHandle = remapped ? 0x214u : 0x114u;
    const std::uint32_t firstRowHandle = remapped ? 0x216u : 0x116u;
    const std::uint32_t secondRowHandle = remapped ? 0x217u : 0x117u;
    return table.tType == DRW::DATATABLE && table.handle == tableHandle
        && table.parentHandle == ownerHandle && table.flags == 2
        && table.columnCount == 4 && table.rowCount == 2
        && table.tableName == "TABLE_NAME" && table.columns.size() == 4u
        && table.columns[0].type == 2 && table.columns[0].text == "DOUBLE"
        && table.columns[0].rows.size() == 2u
        && table.columns[0].rows[0].dataDouble == 1.25
        && table.columns[0].rows[1].dataDouble == 2.5
        && table.columns[1].type == 1 && table.columns[1].text == "INTEGER"
        && table.columns[1].rows.size() == 2u
        && table.columns[1].rows[0].dataLong == 7
        && table.columns[1].rows[1].dataLong == 8
        && table.columns[2].type == 6 && table.columns[2].text == "TEXT"
        && table.columns[2].rows.size() == 2u
        && table.columns[2].rows[0].dataString == "alpha"
        && table.columns[2].rows[1].dataString == "beta"
        && table.columns[3].type == 1 && table.columns[3].text == "INTEGER_2"
        && table.columns[3].rows.size() == 2u
        && table.columns[3].rows[0].dataLong == -3
        && table.columns[3].rows[1].dataLong == 4
        && table.rowHandles.size() == (expectHandles ? 2u : 1u)
        && table.rowHandles[0] == firstRowHandle
        && (!expectHandles || table.rowHandles[1] == secondRowHandle);
}

bool writeAndRead(const std::filesystem::path& path, bool binary) {
    dxfRW codec(path.string().c_str());
    codec.setHandleRemap({{0x114u, 0x214u}, {0x115u, 0x215u},
                          {0x116u, 0x216u}, {0x117u, 0x217u}});
    Export output(codec);
    output.table.rowHandles.pop_back();
    {
        std::ofstream sentinel(path, std::ios::binary | std::ios::trunc);
        sentinel << "DATATABLE_SENTINEL";
    }
    if (codec.write(&output, DRW::AC1024, binary)) return false;
    {
        std::ifstream input(path, std::ios::binary);
        const std::string value((std::istreambuf_iterator<char>(input)), {});
        if (value != "DATATABLE_SENTINEL") return false;
    }

    // Correct the incomplete row-handle vector and retry on the same codec.
    output.table.rowHandles.push_back(0x117u);
    output.emitted = false;
    if (!codec.write(&output, DRW::AC1024, binary) || !output.emitted)
        return false;
    Reader readback;
    dxfRW reader(path.string().c_str());
    return reader.read(&readback, false) && readback.tables.size() == 1u
        && readback.raw.size() == 1u && readback.xrecords.size() == 2u
        && verify(readback.tables.front(), true, true)
        && readback.xrecords[0].handle == 0x216u
        && readback.xrecords[0].parentHandle == 0x215u
        && readback.xrecords[1].handle == 0x217u
        && readback.xrecords[1].parentHandle == 0x215u;
}

bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

bool readbackMatches(const std::filesystem::path& path, int expectedCloning) {
    Reader readback;
    dxfRW reader(path.string().c_str());
    return reader.read(&readback, false) && readback.tables.size() == 1u
        && readback.raw.size() == 1u && readback.xrecords.size() == 2u
        && verify(readback.tables.front(), true, true)
        && readback.xrecords[0].handle == 0x216u
        && readback.xrecords[0].m_cloning == expectedCloning
        && readback.xrecords[0].parentHandle == 0x215u
        && readback.xrecords[1].handle == 0x217u
        && readback.xrecords[1].m_cloning == expectedCloning
        && readback.xrecords[1].parentHandle == 0x215u;
}

bool sourceUnchanged(const Export& actual, const Export& expected) {
    const auto& a = actual.table;
    const auto& b = expected.table;
    if (a.tType != b.tType || a.handle != b.handle
        || a.parentHandle != b.parentHandle || a.name != b.name
        || a.flags != b.flags || a.reactorHandles != b.reactorHandles
        || a.xDictHandle != b.xDictHandle || !a.extData.empty()
        || !b.extData.empty() || !a.appData.empty() || !b.appData.empty()
        || !a.pendingAppIdResolutions.empty()
        || !b.pendingAppIdResolutions.empty()
        || !a.pendingLayerRefResolutions.empty()
        || !b.pendingLayerRefResolutions.empty()
        || a.extensionDictionaryFlag() != b.extensionDictionaryFlag()
        || a.reactorCount() != b.reactorCount()
        || a.hasDataStorageBinaryData() != b.hasDataStorageBinaryData()
        || a.hasDwgCommonLinkTail() != b.hasDwgCommonLinkTail()
        || a.columnCount != b.columnCount || a.rowCount != b.rowCount
        || a.tableName != b.tableName || a.rowHandles != b.rowHandles
        || a.columns.size() != b.columns.size())
        return false;
    for (std::size_t column = 0; column < a.columns.size(); ++column) {
        const auto& left = a.columns[column];
        const auto& right = b.columns[column];
        if (left.type != right.type || left.text != right.text
            || left.rows.size() != right.rows.size())
            return false;
        for (std::size_t row = 0; row < left.rows.size(); ++row) {
            const auto& leftValue = left.rows[row];
            const auto& rightValue = right.rows[row];
            if (leftValue.dataLong != rightValue.dataLong
                || leftValue.dataDouble != rightValue.dataDouble
                || leftValue.dataString != rightValue.dataString)
                return false;
        }
    }
    return true;
}

bool allocationSweep(const std::filesystem::path& path) {
    const std::string prior = "prior DATATABLE bytes\n";
    constexpr DRW::Version versions[] = {
        DRW::AC1014, DRW::AC1015, DRW::AC1018, DRW::AC1021,
        DRW::AC1024, DRW::AC1027, DRW::AC1032};
    for (const DRW::Version version : versions) {
        const int expectedCloning = version == DRW::AC1014 ? 0 : 1;
        for (const bool binary : {false, true}) {
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(path.string().c_str());
                codec.setHandleRemap({{0x114u, 0x214u}, {0x115u, 0x215u},
                                      {0x116u, 0x216u}, {0x117u, 0x217u}});
                Export exporter(codec);
                exporter.setTargetVersion(version);
                exporter.failAt = 1000000;
                const bool written = codec.write(&exporter, version, binary);
                if (!written || !exporter.emitted
                    || !readbackMatches(path, expectedCloning)
                    || !readBytes(path, expectedBytes)) {
                    std::cerr << "DATATABLE calibration failed: version="
                              << static_cast<int>(version)
                              << " binary=" << binary
                              << " written=" << written
                              << " emitted=" << exporter.emitted << '\n';
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
                dxfRW codec(path.string().c_str());
                codec.setHandleRemap({{0x114u, 0x214u}, {0x115u, 0x215u},
                                      {0x116u, 0x216u}, {0x117u, 0x217u}});
                Export exporter(codec);
                Export expected(codec);
                exporter.setTargetVersion(version);
                expected.setTargetVersion(version);
                exporter.failAt = offset;
                const bool written = codec.write(&exporter, version, binary);
                exporter.failAt = -1;
                std::string current;
                if (written || exporter.emitted || exporter.reached != 1
                    || !readBytes(path, current) || current != prior
                    || !sourceUnchanged(exporter, expected)
                    || exporter.firstChild.handle != 0x116u
                    || exporter.secondChild.handle != 0x117u) {
                    std::cerr << "DATATABLE rollback failed: version="
                              << static_cast<int>(version)
                              << " binary=" << binary
                              << " offset=" << offset
                              << " written=" << written
                              << " emitted=" << exporter.emitted
                              << " reached=" << exporter.reached << '\n';
                    return false;
                }

                const bool retried = codec.write(&exporter, version, binary);
                std::string retryBytes;
                if (!retried || !exporter.emitted
                    || !readBytes(path, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(path, expectedCloning)
                    || !sourceUnchanged(exporter, expected)) {
                    std::cerr << "DATATABLE retry failed: version="
                              << static_cast<int>(version)
                              << " binary=" << binary
                              << " offset=" << offset << '\n';
                    return false;
                }
            }
            std::cout << "DATATABLE DXF version="
                      << static_cast<int>(version) << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
        }
    }
    return true;
}
}

int main() {
    const auto path = std::filesystem::temp_directory_path()
        / ("dxf-datatable-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code ec; std::filesystem::remove(path, ec); }
    } cleanup{path};

    for (const bool binary : {false, true}) {
        dxfRW reader(path.string().c_str());
        if (!writeInput(path, binary)) return 1;
        Reader result;
        if (!reader.read(&result, false) || result.tables.size() != 1u
            || result.raw.size() != 1u || !verify(result.tables.front(), true))
            return 2;

        // Incomplete row references remain available through raw-object
        // preservation but do not publish an incomplete typed graph node.
        if (!writeInput(path, binary, 1u)) return 3;
        Reader incomplete;
        if (!reader.read(&incomplete, false) || !incomplete.tables.empty()
            || incomplete.raw.size() != 1u)
            return 4;

        if (!writeInput(path, binary, 0u)) return 5;
        Reader missingReferences;
        if (!reader.read(&missingReferences, false)
            || !missingReferences.tables.empty()
            || missingReferences.raw.size() != 1u)
            return 6;

        if (!writeInput(path, binary, 2u, true)) return 7;
        Reader unknownColumn;
        if (!reader.read(&unknownColumn, false) || !unknownColumn.tables.empty()
            || unknownColumn.raw.size() != 1u)
            return 8;
        for (const int malformed : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
            if (!writeInput(path, binary, 2u, false, malformed)) return 10;
            Reader malformedProjection;
            if (!reader.read(&malformedProjection, false)
                || !malformedProjection.tables.empty()
                || malformedProjection.raw.size() != 1u)
                return 11;
        }
        if (!writeAndRead(path, binary)) return 9;
    }
    return allocationSweep(path) ? 0 : 12;
}
