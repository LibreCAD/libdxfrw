// SPDX-License-Identifier: GPL-2.0-or-later
// Generated APPID table-record allocation rollback/retry control.
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

DRW_AppId makeAppId() {
    DRW_AppId app;
    app.handle = 0xD750u;
    app.name = "GENERATED_ALLOCATION_RETRY_APP";
    app.flags = 0x10;
    return app;
}

bool sameAppId(const DRW_AppId& a, const DRW_AppId& b, bool readback) {
    return a.tType == DRW::APPID
        && (readback ? a.handle != 0u : a.handle == b.handle)
        && a.name == b.name && a.flags == b.flags
        && a.unknown71 == b.unknown71;
}

class Export final : public LC3CompatCheck {
public:
    explicit Export(dxfRW& codec) : codec(codec), app(makeAppId()) {}
    void writeHeader(DRW_Header&) override {}
    void writeAppId() override {
        if (failAt < 0) {
            accepted = codec.writeAppId(&app);
            return;
        }
        armAllocationFailure(failAt);
        try {
            accepted = codec.writeAppId(&app);
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
    DRW_AppId app;
    long failAt = -1;
    long reached = 0;
    bool accepted = false;
};

class Reader final : public LC3CompatCheck {
public:
    void addAppId(const DRW_AppId& value) override {
        if (value.name == "GENERATED_ALLOCATION_RETRY_APP")
            appIds.push_back(value);
    }
    std::vector<DRW_AppId> appIds;
};

class CanonicalExport final : public LC3CompatCheck {
public:
    explicit CanonicalExport(dxfRW& codec) : codec(codec) {
        app.name = "ACAD";
        app.handle = 0xD751u;
    }
    void writeHeader(DRW_Header&) override {}
    void writeAppId() override { accepted = codec.writeAppId(&app); }
    bool finalizeDxfWrite() override { return accepted; }
    dxfRW& codec;
    DRW_AppId app;
    bool accepted = false;
};

class CanonicalReader final : public LC3CompatCheck {
public:
    void addAppId(const DRW_AppId& app) override {
        if (app.name == "ACAD") {
            ++count;
            flags = app.flags;
        }
    }
    unsigned count = 0;
    int flags = -1;
};

const char* versionCode(DRW::Version version) {
    switch (version) {
    case DRW::AC1009: return "AC1009";
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
    const DRW_AppId expected = makeAppId();
    return codec.read(&reader, false) && reader.appIds.size() == 1u
        && sameAppId(reader.appIds.front(), expected, true);
}

bool canonicalMetadataRefusalRetry(const std::filesystem::path& directory) {
    const auto path = directory / "canonical-appid.dxf";
    const auto cleanPath = directory / "canonical-appid-clean.dxf";
    const std::string prior = "prior canonical APPID destination\n";
    for (const auto version : {DRW::AC1009, DRW::AC1014, DRW::AC1032}) {
        for (const bool binary : {false, true}) {
            dxfRW cleanCodec(cleanPath.string().c_str());
            CanonicalExport clean(cleanCodec);
            if (!cleanCodec.write(&clean, version, binary) || !clean.accepted)
                return false;
            std::string expected;
            if (!readBytes(cleanPath, expected)) return false;
            for (unsigned fault = 0; fault != 8; ++fault) {
                { std::ofstream output(path, std::ios::binary | std::ios::trunc); output << prior; }
                dxfRW codec(path.string().c_str());
                CanonicalExport exporter(codec);
                if (fault == 0) exporter.app.flags = 16;
                if (fault == 1) exporter.app.appData.push_back({
                    {102, std::string("{CANONICAL_META")}, {1, std::string("value")},
                    {102, std::string("}")}});
                if (fault == 2) {
                    exporter.app.extData.push_back(new DRW_Variant(1001, std::string("ACAD")));
                    exporter.app.extData.push_back(new DRW_Variant(1000, std::string("value")));
                }
                if (fault == 3) exporter.app.reactorHandles.push_back(0x123u);
                if (fault == 4) exporter.app.xDictHandle = 0x123u;
                if (fault == 5) exporter.app.pendingAppIdResolutions.push_back({});
                if (fault == 6) exporter.app.pendingLayerRefResolutions.push_back({});
                if (fault == 7) exporter.app.setDwgCommonObjectState(1, 0u, false);
                const auto beforeFlags = exporter.app.flags;
                const auto beforeAppData = exporter.app.appData.size();
                const auto beforeExtData = exporter.app.extData.size();
                const auto beforeReactors = exporter.app.reactorHandles.size();
                const auto beforeXDict = exporter.app.xDictHandle;
                const auto beforePendingApp = exporter.app.pendingAppIdResolutions.size();
                const auto beforePendingLayer = exporter.app.pendingLayerRefResolutions.size();
                const auto beforeReactorCount = exporter.app.reactorCount();
                std::string current;
                if (codec.write(&exporter, version, binary) || exporter.accepted
                    || !readBytes(path, current) || current != prior
                    || exporter.app.flags != beforeFlags || exporter.app.appData.size() != beforeAppData
                    || exporter.app.extData.size() != beforeExtData
                    || exporter.app.reactorHandles.size() != beforeReactors
                    || exporter.app.xDictHandle != beforeXDict
                    || exporter.app.pendingAppIdResolutions.size() != beforePendingApp
                    || exporter.app.pendingLayerRefResolutions.size() != beforePendingLayer
                    || exporter.app.reactorCount() != beforeReactorCount) {
                    std::cerr << "canonical APPID metadata was not refused: version="
                              << versionCode(version) << " binary=" << binary << " fault=" << fault << '\n';
                    return false;
                }
                exporter.app = clean.app;
                if (!codec.write(&exporter, version, binary) || !exporter.accepted
                    || !readBytes(path, current) || current != expected) {
                    std::cerr << "canonical APPID retry failed\n";
                    return false;
                }
                dxfRW readerCodec(path.string().c_str());
                CanonicalReader reader;
                if (!readerCodec.read(&reader, false) || reader.count != 1u || reader.flags != 0)
                    return false;
            }
        }
    }
    return true;
}

bool allocationSweep(const std::filesystem::path& directory) {
    const auto path = directory / "appid-table-allocation.dxf";
    const std::string prior = "prior APPID destination bytes\n";
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
                const DRW_AppId expected = makeAppId();
                if (!written || !exporter.accepted
                    || !sameAppId(exporter.app, expected, false)
                    || !readbackMatches(path)
                    || !readBytes(path, expectedBytes)) {
                    std::cerr << "clean APPID write failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " written=" << written
                              << " accepted=" << exporter.accepted
                              << " error=" << codec.getError() << '\n';
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
                const DRW_AppId expected = makeAppId();
                if (written || exporter.accepted
                    || exporter.reached != offset + 1
                    || !allocationFailed || !readBytes(path, current)
                    || current != prior
                    || !sameAppId(exporter.app, expected, false)) {
                    std::cerr << "APPID rollback failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " offset=" << offset
                              << " reached=" << exporter.reached
                              << " failed=" << allocationFailed << '\n';
                    return false;
                }
                const bool retried = codec.write(&exporter, version, binary);
                std::string retryBytes;
                if (!retried || !exporter.accepted
                    || !sameAppId(exporter.app, expected, false)
                    || !readBytes(path, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(path)) {
                    std::cerr << "APPID retry failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " offset=" << offset << '\n';
                    return false;
                }
            }
            std::cout << "APPID " << versionCode(version)
                      << (binary ? " Binary" : " ASCII")
                      << " table-writer allocation offsets="
                      << allocationCount << '\n';
        }
    }
    return true;
}

bool unknownGroup71RefusalRetry(const std::filesystem::path& directory) {
    const auto path = directory / "appid-unknown-71.dxf";
    const auto cleanPath = directory / "appid-unknown-71-clean.dxf";
    const std::string prior = "prior APPID destination bytes\n";
    constexpr std::array<DRW::Version, 2> versions{{
        DRW::AC1014, DRW::AC1032}};
    for (const DRW::Version version : versions) {
        for (const bool binary : {false, true}) {
            dxfRW cleanCodec(cleanPath.string().c_str());
            Export cleanExporter(cleanCodec);
            if (!cleanCodec.write(&cleanExporter, version, binary)
                || !cleanExporter.accepted)
                return false;
            std::string expectedBytes;
            if (!readBytes(cleanPath, expectedBytes))
                return false;

            {
                std::ofstream output(path,
                                     std::ios::binary | std::ios::trunc);
                output << prior;
                if (!output)
                    return false;
            }
            dxfRW codec(path.string().c_str());
            Export exporter(codec);
            exporter.app.unknown71 = 2u;
            if (codec.write(&exporter, version, binary)
                || exporter.accepted || exporter.app.unknown71 != 2u) {
                std::cerr << "APPID group-71 refusal failed: version="
                          << versionCode(version) << " binary=" << binary
                          << '\n';
                return false;
            }
            std::string current;
            if (!readBytes(path, current) || current != prior)
                return false;

            exporter.app.unknown71 = 0u;
            if (!codec.write(&exporter, version, binary)
                || !exporter.accepted || !readBytes(path, current)
                || current != expectedBytes || !readbackMatches(path)) {
                std::cerr << "APPID group-71 retry failed: version="
                          << versionCode(version) << " binary=" << binary
                          << '\n';
                return false;
            }
        }
    }
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
        / ("appid-table-allocation-"
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
    return canonicalMetadataRefusalRetry(directory) && allocationSweep(directory)
        && unknownGroup71RefusalRetry(directory) ? 0 : 2;
}
