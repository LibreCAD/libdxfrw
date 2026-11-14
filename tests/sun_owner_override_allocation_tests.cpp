// SPDX-License-Identifier: GPL-2.0-or-later
// Generated retained SUN raw-owner overload allocation rollback/retry control.
#include "dx_iface.h"
#include "intern/dxfwriter.h"

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

const char* versionTag(DRW::Version version) {
    switch (version) {
    case DRW::AC1021: return "AC1021";
    case DRW::AC1024: return "AC1024";
    case DRW::AC1027: return "AC1027";
    case DRW::AC1032: return "AC1032";
    default: return "";
    }
}

bool writeInput(const std::filesystem::path& path, DRW::Version version) {
    const char* const versionString = versionTag(version);
    if (*versionString == '\0')
        return false;
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output)
        return false;
    dxfWriterAscii writer(&output);
    writer.setVersion(versionString, true);
    const auto text = [&writer](int code, const char* value) {
        return writer.writeString(code, value);
    };
    const bool written = text(0, "SECTION") && text(2, "HEADER")
        && text(9, "$ACADVER") && text(1, versionString)
        && text(0, "ENDSEC") && text(0, "SECTION")
        && text(2, "TABLES") && text(0, "TABLE") && text(2, "VPORT")
        && text(5, "8") && text(330, "0") && text(100, "AcDbSymbolTable")
        && writer.writeInt16(70, 1) && text(0, "VPORT")
        && text(5, "721") && text(330, "8")
        && text(100, "AcDbSymbolTableRecord")
        && text(100, "AcDbViewportTableRecord") && text(2, "*ACTIVE")
        && writer.writeInt16(70, 0) && text(361, "706")
        && text(0, "ENDTAB") && text(0, "ENDSEC")
        && text(0, "SECTION") && text(2, "OBJECTS")
        && text(0, "SUN") && text(5, "706") && text(330, "721")
        && text(100, "AcDbObject") && text(100, "AcDbSun")
        && writer.writeInt32(90, 2) && writer.writeBool(290, true)
        && writer.writeInt16(63, 4) && writer.writeDouble(40, 1.25)
        && writer.writeBool(291, true) && writer.writeInt32(91, 2451545)
        && writer.writeInt32(92, 43200000) && writer.writeBool(292, false)
        && writer.writeInt16(70, 1) && writer.writeInt16(71, 1024)
        && writer.writeInt16(280, 2) && text(0, "ENDSEC")
        && text(0, "EOF");
    return written && !writer.hasWriteError() && output.good();
}

bool rawMatches(const DRW_RawDxfObject& actual,
                const DRW_RawDxfObject& expected) {
    if (actual.name != expected.name || actual.handle != expected.handle
        || actual.parentHandle != expected.parentHandle
        || actual.groups.size() != expected.groups.size())
        return false;
    for (std::size_t index = 0; index < actual.groups.size(); ++index) {
        const DRW_Variant& left = actual.groups[index];
        const DRW_Variant& right = expected.groups[index];
        if (left.code() != right.code() || left.type() != right.type())
            return false;
        if (left.type() == DRW_Variant::STRING) {
            if (left.c_str() == nullptr || right.c_str() == nullptr
                || std::string(left.c_str()) != right.c_str())
                return false;
        } else if (left.type() == DRW_Variant::INTEGER) {
            if (left.i_val() != right.i_val())
                return false;
        } else if (left.type() == DRW_Variant::DOUBLE) {
            if (left.d_val() != right.d_val())
                return false;
        } else {
            return false;
        }
    }
    return true;
}

class RawSunExport final : public dx_iface {
public:
    explicit RawSunExport(DRW_RawDxfObject raw) : sun(std::move(raw)) {}

    void writeObjects() override {
        struct TrackingScope {
            ~TrackingScope() { disarmAllocationFailure(); }
        } trackingScope;
        armAllocationFailure(failAt);
        emitted = dxfW->writeRawDxfObject(&sun, sun.parentHandle);
        reached = static_cast<long>(allocationAttempts);
    }

    bool finalizeDxfWrite() override { return emitted; }

    DRW_RawDxfObject sun;
    long failAt = -1;
    long reached = 0;
    bool emitted = false;
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
    dx_iface reader;
    if (!reader.fileImport(path.string(), &data, false)
        || data.sourceVersion != version
        || data.VPorts.size() != 1u || data.suns.size() != 1u
        || data.rawSunObjects.size() != 1u)
        return false;
    const std::uint32_t owner = data.VPorts.front().handle;
    return data.VPorts.front().name == "*ACTIVE"
        && data.VPorts.front().m_sunHandle == data.suns.front().handle
        && data.suns.front().parentHandle == owner
        && data.rawSunObjects.front().handle == data.suns.front().handle
        && data.rawSunObjects.front().parentHandle == owner
        && data.rawSunObjects.front().name == "SUN";
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "raw SUN owner-override allocation retry: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    const std::string sentinel = "previous SUN owner-override destination\n";
    for (const DRW::Version version : {DRW::AC1021, DRW::AC1024,
                                       DRW::AC1027, DRW::AC1032}) {
      const auto input = directory
          / (std::string("sun-owner-override-input-")
             + std::to_string(static_cast<int>(version)) + ".dxf");
      if (!writeInput(input, version))
          return fail("could not generate the versioned VPORT/SUN source");
      dx_data source;
      dx_iface importer;
      if (!importer.fileImport(input.string(), &source, false)
          || source.sourceVersion != version
          || source.VPorts.size() != 1u || source.suns.size() != 1u
          || source.rawSunObjects.size() != 1u
          || source.rawSunObjects.front().handle != 0x706u
          || source.rawSunObjects.front().parentHandle != 0x721u)
          return fail("generated source did not import the retained SUN graph");

      const DRW_RawDxfObject expectedSource = source.rawSunObjects.front();
      for (const bool binary : {false, true}) {
        const auto output = directory / (std::string("sun-owner-override-")
            + std::to_string(static_cast<int>(version))
            + (binary ? "-binary.dxf" : "-ascii.dxf"));
        long allocationCount = 0;
        std::string expectedBytes;
        {
            RawSunExport exporter(expectedSource);
            exporter.failAt = 1000000;
            const bool wrote = exporter.fileExport(
                output.string(), version, binary, &source, false);
            if (!wrote || !exporter.emitted
                || !readbackMatches(output, version)
                || !rawMatches(exporter.sun, expectedSource))
                return fail("calibration output/readback failed");
            allocationCount = exporter.reached;
            if (!readBytes(output, expectedBytes) || allocationCount <= 0)
                return fail("could not retain calibration bytes/count");
        }

        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream prior(output,
                                    std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior)
                    return fail("could not initialize destination sentinel");
            }
            RawSunExport exporter(expectedSource);
            exporter.failAt = offset;
            const bool wrote = exporter.fileExport(
                output.string(), version, binary, &source, false);
            if (!allocationFailed || wrote || exporter.emitted
                || allocationAttempts
                    != static_cast<std::size_t>(offset + 1u)
                || !rawMatches(exporter.sun, expectedSource))
                return fail(
                    "injected allocation changed caller state or succeeded");
            std::string afterFailure;
            if (!readBytes(output, afterFailure) || afterFailure != sentinel)
                return fail("injected allocation replaced destination");

            exporter.failAt = -1;
            const bool retried = exporter.fileExport(
                output.string(), version, binary, &source, false);
            std::string retryBytes;
            if (!retried || !exporter.emitted
                || !readBytes(output, retryBytes)
                || retryBytes != expectedBytes
                || !readbackMatches(output, version)
                || !rawMatches(exporter.sun, expectedSource))
                return fail("same-adapter retry failed exact output/readback");
        }

        std::cout << "raw SUN owner override version="
                  << static_cast<int>(version) << ' '
                  << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", rollback/retry/typed-owner-readback=PASS\n";
      }
    }
    std::error_code ignored;
    std::filesystem::remove_all(directory, ignored);
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
        std::cerr << "usage: sun_owner_override_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
