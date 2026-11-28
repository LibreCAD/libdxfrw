// SPDX-License-Identifier: GPL-2.0-or-later
// Generated typed PROXY_ENTITY object-ID allocation rollback/retry control.
#include "dx_iface.h"

#include <array>
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

DRW_ProxyEntity makeProxy(bool includeObjectIds = true) {
    DRW_ProxyEntity proxy;
    proxy.handle = 0xD7C0u;
    proxy.m_hasProxyClassId = true;
    proxy.m_proxyClassId = 500;
    proxy.m_hasProxyCarrierId = true;
    proxy.m_proxyCarrierId = DRW_ProxyEntity::kDwgType;
    proxy.m_proxySubclass = "AcDbProxyEntity";
    proxy.m_hasProxyDrawingFormat = true;
    proxy.m_proxyDrawingFormat = 0x00001024u;
    proxy.m_hasFromDxf = true;
    proxy.m_fromDxf = false;
    proxy.m_entityData = {0x31u, 0x32u, 0x33u};
    proxy.m_hasEntityDataBitSize = true;
    proxy.m_entityDataBitSize = 19u;
    if (includeObjectIds) {
        for (std::uint8_t index = 0; index < 8u; ++index) {
            DRW_ProxyObjectIdRef reference;
            reference.m_handleCode = 4u;
            reference.m_dxfCode = 330;
            reference.m_handle = 0xD7D0u + index;
            reference.m_rawHandle = reference.m_handle;
            proxy.m_objectIdRefs.push_back(reference);
        }
    }
    return proxy;
}

bool proxyMatches(const DRW_ProxyEntity& actual,
                  const DRW_ProxyEntity& expected,
                  bool emittedReferences) {
    if ((!emittedReferences && actual.handle != expected.handle)
        || (emittedReferences && actual.handle == 0u)
        || actual.m_entityData != expected.m_entityData
        || actual.m_entityDataBitSize != expected.m_entityDataBitSize
        || actual.m_objectIdRefs.size() != expected.m_objectIdRefs.size())
        return false;
    for (std::size_t index = 0; index < expected.m_objectIdRefs.size(); ++index) {
        const auto& a = actual.m_objectIdRefs[index];
        const auto& e = expected.m_objectIdRefs[index];
        const std::uint32_t expectedHandle = emittedReferences
            ? 0xE100u + static_cast<std::uint32_t>(index) : e.m_handle;
        if (a.m_handleCode != 4u || a.m_dxfCode != 330
            || a.m_handle != expectedHandle || a.m_rawHandle != expectedHandle)
            return false;
    }
    return true;
}

DRW_Class makeProxyClass() {
    DRW_Class proxyClass;
    proxyClass.recName = "TEST_PROXY_RECORD";
    proxyClass.className = "AcDbTestProxyEntity";
    proxyClass.appName = "TEST_APPLICATION";
    proxyClass.proxyFlag = 0x401;
    proxyClass.entityFlag = 1;
    proxyClass.instanceCount = 1;
    proxyClass.wasaProxyFlag = 1;
    return proxyClass;
}

class Export final : public dx_iface {
public:
    explicit Export(dxfRW& codec, bool explicitResolver = true,
                    bool includeObjectIds = true,
                    bool configuredResolver = false)
        : codec(codec), proxy(makeProxy(includeObjectIds)),
          useExplicitResolver(explicitResolver) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &codec;
        if (configuredResolver) {
            codec.setReferenceResolver([](std::uint32_t source) {
                return source >= 0xD7D0u && source < 0xD7D8u
                    ? 0xE100u + (source - 0xD7D0u) : 0u;
            });
        }
    }

    void writeEntities() override {
        struct TrackingScope {
            ~TrackingScope() { disarmAllocationFailure(); }
        } trackingScope;
        armAllocationFailure(failAt);
        accepted = useExplicitResolver
            ? codec.writeProxyEntity(&proxy, 500u,
                [](std::uint32_t source) {
                    return 0xE100u + (source - 0xD7D0u);
                })
            : codec.writeProxyEntity(&proxy, 500u);
        reached = static_cast<long>(allocationAttempts);
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    dx_data storage;
    DRW_ProxyEntity proxy;
    bool useExplicitResolver;
    long failAt = -1;
    long reached = 0;
    bool accepted = false;
};

class Readback final : public dx_iface {
public:
    Readback() {
        cData = &storage;
        currentBlock = storage.mBlock;
    }

    void addProxyEntity(const DRW_ProxyEntity& value) override {
        proxies.push_back(value);
    }

    dx_data storage;
    std::vector<DRW_ProxyEntity> proxies;
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
                     bool includeObjectIds, DRW::Version expectedVersion) {
    Readback reader;
    const bool read = reader.fileImport(path.string(), &reader.storage, false);
    if (!read || reader.storage.sourceVersion != expectedVersion
        || reader.proxies.size() != 1u) {
        std::cerr << "PROXY_ENTITY readback read=" << read
                  << " version=" << static_cast<int>(reader.storage.sourceVersion)
                  << " records=" << reader.proxies.size() << '\n';
        return false;
    }
    if (!proxyMatches(reader.proxies.front(), makeProxy(includeObjectIds),
                      true)) {
        std::cerr << "PROXY_ENTITY typed readback field mismatch handle="
                  << reader.proxies.front().handle
                  << " refs=" << reader.proxies.front().m_objectIdRefs.size()
                  << '\n';
        return false;
    }
    return true;
}

bool runProfile(const std::filesystem::path& directory,
                bool explicitResolver, bool includeObjectIds,
                bool configuredResolver,
                const char* outputName, const char* profileName) {
    const auto fail = [profileName](const char* message) {
        std::cerr << "PROXY_ENTITY " << profileName << " allocation retry: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    const std::string sentinel = "prior PROXY_ENTITY destination bytes\n";
    const std::array<std::pair<DRW::Version, const char*>, 5> versions = {{
        {DRW::AC1018, "AC1018"}, {DRW::AC1021, "AC1021"},
        {DRW::AC1024, "AC1024"}, {DRW::AC1027, "AC1027"},
        {DRW::AC1032, "AC1032"}}};
    for (const auto& version : versions) {
      for (const bool binary : {false, true}) {
        const auto output = directory
            / (std::string(outputName) + "-" + version.second
               + (binary ? "-binary.dxf" : "-ascii.dxf"));
        long allocationCount = 0;
        std::string expectedBytes;
        {
            dxfRW codec(output.string().c_str());
            codec.setDxfClasses(std::vector<DRW_Class>{makeProxyClass()});
            Export exporter(codec, explicitResolver, includeObjectIds,
                            configuredResolver);
            exporter.failAt = 1000000;
            const bool wrote = codec.write(&exporter, version.first, binary);
            if (!wrote || !exporter.accepted
                || !readbackMatches(output, includeObjectIds, version.first))
                return fail("calibration output did not read back");
            allocationCount = exporter.reached;
            if (!readBytes(output, expectedBytes))
                return fail("could not retain calibration bytes");
        }
        if (allocationCount <= 0)
            return fail("calibration observed no callback allocations");

        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream prior(output,
                                   std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior)
                    return fail("could not initialize destination sentinel");
            }
            dxfRW codec(output.string().c_str());
            codec.setDxfClasses(std::vector<DRW_Class>{makeProxyClass()});
            Export exporter(codec, explicitResolver, includeObjectIds,
                            configuredResolver);
            const DRW_ProxyEntity source = exporter.proxy;
            exporter.failAt = offset;
            const bool wrote = codec.write(&exporter, version.first, binary);
            if (!allocationFailed || wrote || exporter.accepted
                || allocationAttempts != static_cast<std::size_t>(offset + 1)
                || !proxyMatches(exporter.proxy, source, false)) {
                std::cerr << "PROXY_ENTITY offset=" << offset
                          << " attempts=" << allocationAttempts
                          << " fired=" << allocationFailed
                          << " wrote=" << wrote
                          << " callback=" << exporter.accepted << '\n';
                return fail("injected failure changed caller state or succeeded");
            }
            std::string afterFailure;
            if (!readBytes(output, afterFailure) || afterFailure != sentinel)
                return fail("injected failure replaced destination");

            exporter.failAt = -1;
            const bool retried = codec.write(&exporter, version.first, binary);
            std::string retryBytes;
            if (!retried || !exporter.accepted
                || !readBytes(output, retryBytes)
                || retryBytes != expectedBytes
                || !readbackMatches(output, includeObjectIds, version.first)
                || !proxyMatches(exporter.proxy, source, false))
                return fail("same-codec retry failed exact bytes/readback");
        }
        std::cout << "PROXY_ENTITY " << profileName << ' '
                  << version.second << ' '
                  << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", rollback/retry/readback=PASS\n";
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
      }
    }
    return true;
}

bool run(const std::filesystem::path& directory) {
    return runProfile(directory, true, true, false,
                      "proxy-entity-object-ids",
                      "object-ID")
        && runProfile(directory, false, false, false,
                      "proxy-entity-default-overload",
                      "two-argument overload without object IDs")
        && runProfile(directory, false, true, true,
                      "proxy-entity-default-overload-object-ids",
                      "two-argument overload with configured resolver");
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
        std::cerr << "usage: proxy_entity_object_id_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
