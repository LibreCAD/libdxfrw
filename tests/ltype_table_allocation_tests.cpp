// SPDX-License-Identifier: GPL-2.0-or-later
// Generated LTYPE table-record allocation rollback/retry control.
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>
#include <vector>

namespace {
long allocationsBeforeFailure = -1;
std::size_t allocationAttempts = 0u;
bool allocationFailed = false;

void armAllocationFailure(long offset) noexcept {
    allocationsBeforeFailure = offset;
    allocationAttempts = 0u;
    allocationFailed = false;
}
void disarmAllocationFailure() noexcept { allocationsBeforeFailure = -1; }

DRW_LType makeLType() {
    DRW_LType lineType;
    lineType.handle = 0xD730u;
    lineType.name = "Generated allocation retry LTYPE";
    lineType.desc = "Generated dash profile";
    lineType.alignment = 'A';
    lineType.path = {10.0, -5.0};
    return lineType;
}

bool sameCaller(const DRW_LType& actual) {
    const DRW_LType expected = makeLType();
    return actual.tType == DRW::LTYPE
        && actual.handle == expected.handle
        && actual.name == expected.name
        && actual.desc == expected.desc
        && actual.alignment == expected.alignment
        && actual.path == expected.path
        && actual.size == expected.size
        && actual.length == expected.length
        && actual.segments.empty();
}

bool sameReadback(const DRW_LType& actual) {
    return actual.tType == DRW::LTYPE && actual.handle != 0u
        && actual.name == "Generated allocation retry LTYPE"
        && actual.desc == "Generated dash profile"
        && actual.alignment == 'A'
        && actual.path == std::vector<double>{10.0, -5.0}
        && actual.size == 2 && actual.length == 15.0;
}

class Export final : public LC3CompatCheck {
public:
    explicit Export(dxfRW& codec) : codec(codec), lineType(makeLType()) {}
    void writeHeader(DRW_Header&) override {}
    void writeLTypes() override {
        if (failAt < 0) {
            accepted = codec.writeLineType(&lineType);
            return;
        }
        armAllocationFailure(failAt);
        try {
            accepted = codec.writeLineType(&lineType);
        } catch (const std::bad_alloc&) {
            accepted = false;
        }
        reached = static_cast<long>(allocationAttempts);
        disarmAllocationFailure();
        if (!accepted)
            (void)codec.writeRawDxfObject(nullptr);
    }
    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    DRW_LType lineType;
    long failAt = -1;
    long reached = 0;
    bool accepted = false;
};

class EditedExport final : public LC3CompatCheck {
public:
    EditedExport(dxfRW& codec, const DRW_LType& lineType)
        : codec(codec), lineType(lineType) {}
    void writeHeader(DRW_Header&) override {}
    void writeLTypes() override {
        accepted = codec.writeLineType(&lineType);
    }
    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    DRW_LType lineType;
    bool accepted = false;
};

class Reader final : public LC3CompatCheck {
public:
    void addLType(const DRW_LType& value) override {
        if (value.name == "Generated allocation retry LTYPE")
            lineTypes.push_back(value);
    }
    std::vector<DRW_LType> lineTypes;
};

const char* versionCode(DRW::Version version) {
    switch (version) {
    case DRW::AC1014: return "AC1014";
    case DRW::AC1015: return "AC1015";
    case DRW::AC1018: return "AC1018";
    case DRW::AC1021: return "AC1021";
    case DRW::AC1024: return "AC1024";
    case DRW::AC1027: return "AC1027";
    case DRW::AC1032: return "AC1032";
    default: return "unknown";
    }
}

bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

bool readbackMatches(const std::filesystem::path& path) {
    dxfRW codec(path.string().c_str());
    Reader reader;
    return codec.read(&reader, false) && reader.lineTypes.size() == 1u
        && sameReadback(reader.lineTypes.front());
}

bool allocationSweep(const std::filesystem::path& directory) {
    const auto path = directory / "ltype-table-allocation.dxf";
    const std::string prior = "prior LTYPE destination bytes\n";
    constexpr std::array<DRW::Version, 7> versions{{
        DRW::AC1014, DRW::AC1015, DRW::AC1018, DRW::AC1021,
        DRW::AC1024, DRW::AC1027, DRW::AC1032}};
    for (const DRW::Version version : versions) {
        for (const bool binary : {false, true}) {
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(path.string().c_str());
                Export exporter(codec);
                exporter.failAt = 1000000;
                const bool written = codec.write(&exporter, version, binary);
                if (!written || !exporter.accepted
                    || !sameCaller(exporter.lineType)
                    || !readbackMatches(path)
                    || !readBytes(path, expectedBytes)) {
                    std::cerr << "clean LTYPE write failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " written=" << written
                              << " accepted=" << exporter.accepted << '\n';
                    return false;
                }
                allocationCount = exporter.reached;
            }
            if (allocationCount <= 0)
                return false;
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream output(path,
                                         std::ios::binary | std::ios::trunc);
                    output << prior;
                    if (!output)
                        return false;
                }
                dxfRW codec(path.string().c_str());
                Export exporter(codec);
                exporter.failAt = offset;
                const bool written = codec.write(&exporter, version, binary);
                exporter.failAt = -1;
                std::string current;
                if (written || exporter.accepted
                    || exporter.reached != offset + 1
                    || !allocationFailed || !readBytes(path, current)
                    || current != prior || !sameCaller(exporter.lineType)) {
                    std::cerr << "LTYPE rollback failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " offset=" << offset
                              << " reached=" << exporter.reached
                              << " failed=" << allocationFailed << '\n';
                    return false;
                }
                const bool retried = codec.write(&exporter, version, binary);
                std::string retryBytes;
                if (!retried || !exporter.accepted
                    || !sameCaller(exporter.lineType)
                    || !readBytes(path, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(path)) {
                    std::cerr << "LTYPE retry failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " offset=" << offset << '\n';
                    return false;
                }
            }
            std::cout << "LTYPE " << versionCode(version)
                      << (binary ? " Binary" : " ASCII")
                      << " table-writer allocation offsets="
                      << allocationCount << '\n';
        }
    }
    return true;
}

bool editedImportedRecordRoundTrip(const std::filesystem::path& directory) {
    const auto source = directory / "ltype-edit-source.dxf";
    const auto output = directory / "ltype-edit-output.dxf";
    {
        dxfRW codec(source.string().c_str());
        Export exporter(codec);
        if (!codec.write(&exporter, DRW::AC1027, false) || !exporter.accepted)
            return false;
    }

    dxfRW sourceReader(source.string().c_str());
    Reader imported;
    if (!sourceReader.read(&imported, false) || imported.lineTypes.size() != 1u)
        return false;
    DRW_LType edited = imported.lineTypes.front();
    edited.desc = "Edited after import";
    edited.path = {3.0, -1.0};

    for (const bool binary : {false, true}) {
        dxfRW codec(output.string().c_str());
        EditedExport exporter(codec, edited);
        if (!codec.write(&exporter, DRW::AC1027, binary) || !exporter.accepted)
            return false;
        dxfRW outputReader(output.string().c_str());
        Reader readback;
        if (!outputReader.read(&readback, false)
            || readback.lineTypes.size() != 1u)
            return false;
        const DRW_LType& actual = readback.lineTypes.front();
        if (actual.name != edited.name || actual.desc != edited.desc
            || actual.path != std::vector<double>{3.0, -1.0}
            || actual.size != 2 || actual.length != 4.0) {
            std::cerr << "edited imported LTYPE fields were not re-emitted: "
                         "binary=" << binary << '\n';
            return false;
        }
    }
    std::cout << "edited imported LTYPE description and dash pattern "
                 "readback passed\n";
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (allocationsBeforeFailure == 0) {
        ++allocationAttempts;
        allocationsBeforeFailure = -1;
        allocationFailed = true;
        throw std::bad_alloc();
    }
    if (allocationsBeforeFailure > 0) {
        --allocationsBeforeFailure;
        ++allocationAttempts;
    }
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
        / ("ltype-table-allocation-"
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
    return allocationSweep(directory)
            && editedImportedRecordRoundTrip(directory)
        ? 0 : 2;
}
