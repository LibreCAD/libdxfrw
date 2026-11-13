// SPDX-License-Identifier: GPL-2.0-or-later
// Generated DATALINK OBJECTS allocation-failure rollback and retry control.
#include "lc3_compat_check.cpp"
#include "dx_iface.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace {
long allocationsBeforeFailure = -1;
std::size_t failedAllocations = 0;

struct AllocationFailure {
    explicit AllocationFailure(long count) { allocationsBeforeFailure = count; }
    ~AllocationFailure() { allocationsBeforeFailure = -1; }
};

class DataLinkExport final : public LC3CompatCheck {
public:
    static constexpr std::uint32_t kTargetCount = 16u;

    explicit DataLinkExport(dxfRW& output) : output(output) {
        dictionary.handle = 0xE000u;
        DRW_Dictionary::Entry entry;
        entry.m_name = "MainLink";
        entry.m_handle = 0xE001u;
        dictionary.m_entries.push_back(std::move(entry));

        mainLink.handle = 0xE001u;
        mainLink.parentHandle = dictionary.handle;
        mainLink.m_dataAdapter = "EXCEL";
        mainLink.m_description = "generated DATALINK with custom references";
        mainLink.m_tooltip = "rollback control";
        mainLink.m_connectionString = "inert://generated-control.xlsx";
        mainLink.m_option = 7;
        mainLink.m_updateOption = 511;
        mainLink.m_value92 = -3;
        mainLink.m_year = 2026;
        mainLink.m_month = 11;
        mainLink.m_day = 8;
        mainLink.m_hour = 14;
        mainLink.m_minute = 30;
        mainLink.m_seconds = 12;
        mainLink.m_milliseconds = 345;
        mainLink.m_pathOption = 3;
        mainLink.m_value93 = 19;
        mainLink.m_updateStatus = "ready";
        mainLink.m_customDataCount = static_cast<std::int32_t>(kTargetCount);
        for (std::uint32_t index = 0; index < kTargetCount; ++index) {
            const std::uint32_t handle = 0xE002u + index;
            mainLink.m_customData.push_back(
                {handle, "sheet target " + std::to_string(index)});
            DRW_DataLink target;
            target.handle = handle;
            target.parentHandle = dictionary.handle;
            target.m_dataAdapter = "EXCEL";
            target.m_description = "target " + std::to_string(index);
            targets.push_back(std::move(target));
        }
    }

    dxfRW& output;
    DRW_Dictionary dictionary;
    DRW_DataLink mainLink;
    std::vector<DRW_DataLink> targets;
    long failAt = -1;
    long reached = 0;
    bool emitted = false;

    void writeObjects() override {
        const auto emit = [this]() {
            emitted = output.writeDictionary(&dictionary)
                && output.writeDataLink(&mainLink);
            for (DRW_DataLink& target : targets)
                emitted = emitted && output.writeDataLink(&target);
        };
        if (failAt < 0) {
            emit();
            return;
        }
        const auto before = failedAllocations;
        try {
            AllocationFailure failure(failAt);
            emit();
            reached = failedAllocations != before
                ? static_cast<long>(failedAllocations - before)
                : failAt - allocationsBeforeFailure;
        } catch (const std::bad_alloc&) {
            reached = failedAllocations != before
                ? static_cast<long>(failedAllocations - before)
                : failAt - allocationsBeforeFailure;
            throw;
        }
    }

    bool finalizeDxfWrite() override { return emitted; }
};

bool readAll(const std::filesystem::path& path, std::string& value) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    value.assign(std::istreambuf_iterator<char>(input),
                 std::istreambuf_iterator<char>());
    return !input.bad();
}

bool write(DataLinkExport& exporter, dxfRW& codec, DRW::Version version,
           bool binary) {
    codec.setRootDictEntries({{"ACAD_DATALINK", "E000"}});
    return codec.write(&exporter, version, binary);
}

bool verifyGraph(const std::filesystem::path& path) {
    dx_data data;
    dx_iface reader;
    if (!reader.fileImport(path.string(), &data, false)
        || !data.hasNamedObjectDictionary
        || data.dataLinks.size() != DataLinkExport::kTargetCount + 1u)
        return false;
    bool rooted = false;
    for (const auto& entry : data.namedObjectDictionary.m_entries)
        rooted = rooted || (entry.m_name == "ACAD_DATALINK"
                            && entry.m_handle == 0xE000u);
    const auto dictionary = std::find_if(data.dictionaries.begin(),
        data.dictionaries.end(), [](const DRW_Dictionary& value) {
            return value.handle == 0xE000u;
        });
    if (!rooted || dictionary == data.dictionaries.end()
        || dictionary->parentHandle != data.namedObjectDictionary.handle
        || dictionary->m_entries.size() != 1u
        || dictionary->m_entries.front().m_name != "MainLink"
        || dictionary->m_entries.front().m_handle != 0xE001u)
        return false;
    const auto link = [&data](std::uint32_t handle) -> const DRW_DataLink* {
        const auto found = std::find_if(data.dataLinks.begin(), data.dataLinks.end(),
            [handle](const DRW_DataLink& value) {
                return value.handle == handle;
            });
        return found == data.dataLinks.end() ? nullptr : &*found;
    };
    const DRW_DataLink* main = link(0xE001u);
    if (main == nullptr)
        return false;
    if (main->parentHandle != 0xE000u
        || main->m_dataAdapter != "EXCEL"
        || main->m_description != "generated DATALINK with custom references"
        || main->m_tooltip != "rollback control"
        || main->m_connectionString != "inert://generated-control.xlsx"
        || main->m_option != 7 || main->m_updateOption != 511
        || main->m_value92 != -3 || main->m_year != 2026
        || main->m_month != 11 || main->m_day != 8
        || main->m_hour != 14 || main->m_minute != 30
        || main->m_seconds != 12 || main->m_milliseconds != 345
        || main->m_pathOption != 3 || main->m_value93 != 19
        || main->m_updateStatus != "ready"
        || main->m_customDataCount
            != static_cast<std::int32_t>(DataLinkExport::kTargetCount)
        || main->m_customData.size() != DataLinkExport::kTargetCount)
        return false;
    for (std::uint32_t index = 0; index < DataLinkExport::kTargetCount; ++index) {
        const DRW_DataLink* target = link(0xE002u + index);
        if (target == nullptr || target->parentHandle != 0xE000u
            || target->m_description != "target " + std::to_string(index)
            || main->m_customData[index].m_targetHandle != 0xE002u + index
            || main->m_customData[index].m_text
                != "sheet target " + std::to_string(index))
            return false;
    }
    return true;
}

bool sourceUnchanged(const DataLinkExport& actual,
                     const DataLinkExport& expected) {
    const auto sameLink = [](const DRW_DataLink& a, const DRW_DataLink& b) {
        if (a.handle != b.handle || a.parentHandle != b.parentHandle
            || a.m_dataAdapter != b.m_dataAdapter
            || a.m_description != b.m_description || a.m_tooltip != b.m_tooltip
            || a.m_connectionString != b.m_connectionString
            || a.m_option != b.m_option || a.m_updateOption != b.m_updateOption
            || a.m_value92 != b.m_value92 || a.m_year != b.m_year
            || a.m_month != b.m_month || a.m_day != b.m_day
            || a.m_hour != b.m_hour || a.m_minute != b.m_minute
            || a.m_seconds != b.m_seconds || a.m_milliseconds != b.m_milliseconds
            || a.m_pathOption != b.m_pathOption || a.m_value93 != b.m_value93
            || a.m_updateStatus != b.m_updateStatus
            || a.m_customDataCount != b.m_customDataCount
            || a.m_customData.size() != b.m_customData.size())
            return false;
        for (std::size_t i = 0; i < a.m_customData.size(); ++i)
            if (a.m_customData[i].m_targetHandle
                    != b.m_customData[i].m_targetHandle
                || a.m_customData[i].m_text != b.m_customData[i].m_text)
                return false;
        return true;
    };
    if (!(actual.dictionary.handle == expected.dictionary.handle
        && actual.dictionary.m_entries.size() == expected.dictionary.m_entries.size()
        && actual.dictionary.m_entries.front().m_name
            == expected.dictionary.m_entries.front().m_name
        && actual.dictionary.m_entries.front().m_handle
            == expected.dictionary.m_entries.front().m_handle
        && sameLink(actual.mainLink, expected.mainLink)
        && actual.targets.size() == expected.targets.size()))
        return false;
    for (std::size_t index = 0; index < actual.targets.size(); ++index)
        if (!sameLink(actual.targets[index], expected.targets[index]))
            return false;
    return true;
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "DATALINK OBJECTS allocation retry: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const std::string sentinel = "prior DATALINK destination bytes\n";
    for (const DRW::Version version : {
             DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        const auto output = directory
            / (std::string("datalink-objects-")
               + std::to_string(static_cast<int>(version)) + ".dxf");
        dxfRW codec(output.string().c_str());
        DataLinkExport exporter(codec);
        DataLinkExport expected(codec);
        for (const bool binary : {false, true}) {
            if (!write(exporter, codec, version, binary))
                return fail("warm-up export was refused");
            exporter.failAt = 1000000;
            if (!write(exporter, codec, version, binary))
                return fail("allocation calibration export was refused");
            if (!verifyGraph(output))
                return fail("allocation calibration output did not read back graph");
            std::string cleanBytes;
            if (!readAll(output, cleanBytes))
                return fail("could not read allocation calibration bytes");
            const long allocationCount = exporter.reached;
            if (allocationCount <= 0)
                return fail("calibration observed no callback allocations");
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output, std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior)
                        return fail("could not initialize destination sentinel");
                }
                exporter.failAt = offset;
                exporter.reached = 0;
                exporter.emitted = false;
                const bool wrote = write(exporter, codec, version, binary);
                exporter.failAt = -1;
                std::string current;
                if (wrote || exporter.reached < 1
                    || !readAll(output, current) || current != sentinel)
                    return fail("injected failure changed destination or was missed");
                if (!sourceUnchanged(exporter, expected))
                    return fail("injected failure changed the caller graph");
                if (!write(exporter, codec, version, binary)
                    || !verifyGraph(output))
                    return fail("same-writer retry failed typed graph readback");
                std::string retryBytes;
                if (!readAll(output, retryBytes) || retryBytes != cleanBytes)
                    return fail("same-writer retry differs from clean output");
                if (!sourceUnchanged(exporter, expected))
                    return fail("same-writer retry changed the caller graph");
            }
            std::cout << "DATALINK OBJECTS version="
                      << static_cast<int>(version) << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
        }
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
    }
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (allocationsBeforeFailure == 0) {
        ++failedAllocations;
        throw std::bad_alloc();
    }
    if (allocationsBeforeFailure > 0)
        --allocationsBeforeFailure;
    if (void* value = std::malloc(size == 0 ? 1 : size))
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
        std::cerr << "usage: datalink_objects_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    const auto before = failedAllocations;
    if (!run(argv[1]))
        return 1;
    if (failedAllocations == before) {
        std::cerr << "allocation fault injector did not reach operator new\n";
        return 1;
    }
    return 0;
}
