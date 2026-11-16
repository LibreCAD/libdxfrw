// SPDX-License-Identifier: GPL-2.0-or-later
// Generated VPORT table-record allocation rollback/retry control.
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

DRW_Vport makeVport(DRW::Version version) {
    DRW_Vport port;
    port.handle = 0xD710u;
    port.parentHandle = 8u;
    port.name = "*ACTIVE";
    port.lowerLeft = DRW_Coord(-1.0, -2.0, 0.0);
    port.UpperRight = DRW_Coord(3.0, 4.0, 0.0);
    port.center = DRW_Coord(5.0, 6.0, 0.0);
    port.snapBase = DRW_Coord(0.25, 0.5, 0.0);
    port.snapSpacing = DRW_Coord(0.75, 1.25, 0.0);
    port.gridSpacing = DRW_Coord(2.0, 3.0, 0.0);
    port.viewDir = DRW_Coord(0.0, 0.0, 1.0);
    port.viewTarget = DRW_Coord(7.0, 8.0, 9.0);
    port.height = 90.0;
    port.ratio = 1.5;
    port.lensHeight = 45.0;
    port.frontClip = 0.125;
    port.backClip = 250.0;
    port.snapAngle = 5.0;
    port.twistAngle = 12.5;
    port.viewMode = 3;
    port.circleZoom = 80;
    port.fastZoom = 1;
    port.ucsIcon = 3;
    port.snap = 1;
    port.grid = 1;
    port.snapStyle = 1;
    port.snapIsopair = 2;
    if (version > DRW::AC1014) {
        port.renderMode = 2;
        port.ucsPerVP = true;
        port.ucsOrigin = DRW_Coord(1.0, 2.0, 3.0);
        port.ucsXAxis = DRW_Coord(1.0, 0.0, 0.0);
        port.ucsYAxis = DRW_Coord(0.0, 1.0, 0.0);
        port.ucsElevation = 4.5;
        port.ucsOrthoType = 3;
    }
    if (version > DRW::AC1018) {
        port.gridBehavior = 3;
        port.gridMajorLines = 4;
        port.useDefaultLighting = false;
        port.defaultLightingType = 1;
        port.brightness = 0.65;
        port.contrast = 0.35;
        port.ambientColor = 7;
        port.hasDxfShadePlotMode = true;
        port.shadePlotMode = 2;
    }
    port.extData.push_back(new DRW_Variant(1001, std::string("ACAD")));
    port.extData.push_back(new DRW_Variant(
        1000, std::string("vport-allocation-retry")));
    return port;
}

bool sameVport(const DRW_Vport& a, const DRW_Vport& b, bool readback) {
    return (readback ? a.handle != 0u : a.handle == b.handle)
        && a.tType == DRW::VPORT && a.parentHandle == b.parentHandle
        && a.name == b.name && a.flags == b.flags
        && a.lowerLeft.x == b.lowerLeft.x && a.lowerLeft.y == b.lowerLeft.y
        && a.UpperRight.x == b.UpperRight.x
        && a.UpperRight.y == b.UpperRight.y
        && a.center.x == b.center.x && a.center.y == b.center.y
        && a.snapBase.x == b.snapBase.x && a.snapBase.y == b.snapBase.y
        && a.snapSpacing.x == b.snapSpacing.x
        && a.snapSpacing.y == b.snapSpacing.y
        && a.gridSpacing.x == b.gridSpacing.x
        && a.gridSpacing.y == b.gridSpacing.y
        && a.viewDir.x == b.viewDir.x && a.viewDir.y == b.viewDir.y
        && a.viewDir.z == b.viewDir.z
        && a.viewTarget.x == b.viewTarget.x
        && a.viewTarget.y == b.viewTarget.y
        && a.viewTarget.z == b.viewTarget.z
        && a.height == b.height && a.ratio == b.ratio
        && a.lensHeight == b.lensHeight && a.frontClip == b.frontClip
        && a.backClip == b.backClip && a.snapAngle == b.snapAngle
        && a.twistAngle == b.twistAngle && a.viewMode == b.viewMode
        && a.circleZoom == b.circleZoom && a.fastZoom == b.fastZoom
        && a.ucsIcon == b.ucsIcon && a.snap == b.snap && a.grid == b.grid
        && a.snapStyle == b.snapStyle && a.snapIsopair == b.snapIsopair
        && a.renderMode == b.renderMode && a.ucsPerVP == b.ucsPerVP
        && a.ucsOrigin.x == b.ucsOrigin.x && a.ucsOrigin.y == b.ucsOrigin.y
        && a.ucsOrigin.z == b.ucsOrigin.z
        && a.ucsXAxis.x == b.ucsXAxis.x && a.ucsXAxis.y == b.ucsXAxis.y
        && a.ucsXAxis.z == b.ucsXAxis.z
        && a.ucsYAxis.x == b.ucsYAxis.x && a.ucsYAxis.y == b.ucsYAxis.y
        && a.ucsYAxis.z == b.ucsYAxis.z
        && a.ucsElevation == b.ucsElevation
        && a.ucsOrthoType == b.ucsOrthoType
        && a.gridBehavior == b.gridBehavior
        && a.gridMajorLines == b.gridMajorLines
        && a.useDefaultLighting == b.useDefaultLighting
        && a.defaultLightingType == b.defaultLightingType
        && a.brightness == b.brightness && a.contrast == b.contrast
        && a.ambientColor == b.ambientColor
        && a.ambientColorRgb == b.ambientColorRgb
        && a.ambientColorName == b.ambientColorName
        && a.shadePlotMode == b.shadePlotMode
        && a.hasDxfShadePlotMode == b.hasDxfShadePlotMode
        && a.shadePlotHandle == b.shadePlotHandle
        && a.hasDxfShadePlotHandle == b.hasDxfShadePlotHandle
        && a.visualStyleHandle == b.visualStyleHandle
        && a.m_sunHandle == b.m_sunHandle
        && a.backgroundHandle == b.backgroundHandle
        && a.namedUcsHandle == b.namedUcsHandle
        && a.baseUcsHandle == b.baseUcsHandle
        && a.xrefBlockHandle.ref == b.xrefBlockHandle.ref
        && a.reactorHandles == b.reactorHandles
        && a.xDictHandle == b.xDictHandle
        && a.extData.size() == b.extData.size()
        && a.extData[0] && b.extData[0]
        && a.extData[0]->code() == b.extData[0]->code()
        && a.extData[0]->content.s && b.extData[0]->content.s
        && *a.extData[0]->content.s == *b.extData[0]->content.s
        && a.extData[1] && b.extData[1]
        && a.extData[1]->code() == b.extData[1]->code()
        && a.extData[1]->content.s && b.extData[1]->content.s
        && *a.extData[1]->content.s == *b.extData[1]->content.s;
}

class Export final : public LC3CompatCheck {
public:
    Export(dxfRW& codec, DRW::Version version)
        : codec(codec), version(version), vport(makeVport(version)) {}

    void writeVports() override {
        if (failAt < 0) {
            accepted = codec.writeVport(&vport);
            return;
        }
        armAllocationFailure(failAt);
        try {
            accepted = codec.writeVport(&vport);
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
    DRW::Version version;
    DRW_Vport vport;
    long failAt = -1;
    long reached = 0;
    bool accepted = false;
};

class Reader final : public LC3CompatCheck {
public:
    void addVport(const DRW_Vport& value) override { vports.push_back(value); }
    std::vector<DRW_Vport> vports;
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
    dxfRW codec(path.string().c_str());
    Reader reader;
    if (!codec.read(&reader, false) || reader.vports.size() != 1u)
        return false;
    const DRW_Vport expected = makeVport(version);
    return sameVport(reader.vports.front(), expected, true);
}

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

bool viewportNamesAndRetry(const std::filesystem::path& directory) {
    class NamedExport final : public LC3CompatCheck {
    public:
        explicit NamedExport(dxfRW& output) : codec(output) {}
        void writeHeader(DRW_Header&) override {}
        void writeVports() override {
            accepted = true;
            for (auto& port : ports)
                accepted = codec.writeVport(&port) && accepted;
        }
        bool finalizeDxfWrite() override { return accepted; }
        dxfRW& codec;
        std::vector<DRW_Vport> ports;
        bool accepted = false;
    };
    const auto path = directory / "viewport-names.dxf";
    const std::string sentinel = "prior viewport output\n";
    for (const auto version : {DRW::AC1009, DRW::AC1014, DRW::AC1032}) {
        for (const bool binary : {false, true}) {
            for (int mode = 0; mode != 3; ++mode) {
                dxfRW codec(path.string().c_str());
                NamedExport output(codec);
                const auto add = [&output](const std::string& name, double center) {
                    DRW_Vport port;
                    port.name = name;
                    port.center.x = center;
                    output.ports.push_back(port);
                };
                if (mode == 0) {
                    add("NAMED_CONFIG", 12.0);
                    add("*Active", 34.0);
                    add("*ACTIVE", 56.0);
                    add("NAMED_CONFIG", 78.0);
                } else if (mode == 1) {
                    add("NAMED_CONFIG", 12.0);
                } else {
                    add("", 90.0);
                }
                if (mode == 0) {
                    { std::ofstream file(path, std::ios::binary | std::ios::trunc); file << sentinel; }
                    output.ports[1].shadePlotMode = 99;
                    std::string bytes;
                    if (codec.write(&output, version, binary)
                        || !readBytes(path, bytes) || bytes != sentinel
                        || output.ports[0].name != "NAMED_CONFIG"
                        || output.ports[1].name != "*Active"
                        || output.ports[1].shadePlotMode != 99)
                        return false;
                    output.ports[1].shadePlotMode = 0;
                }
                if (!codec.write(&output, version, binary) || !output.accepted)
                    return false;
                if (mode == 0) {
                    const auto cleanPath = directory / "viewport-names-clean.dxf";
                    dxfRW cleanCodec(cleanPath.string().c_str());
                    NamedExport clean(cleanCodec);
                    clean.ports = output.ports;
                    std::string actualBytes, cleanBytes;
                    if (!cleanCodec.write(&clean, version, binary) || !clean.accepted
                        || !readBytes(path, actualBytes)
                        || !readBytes(cleanPath, cleanBytes)
                        || actualBytes != cleanBytes)
                        return false;
                }
                dxfRW input(path.string().c_str());
                Reader reader;
                if (!input.read(&reader, false)) return false;
                if (mode == 0) {
                    if (reader.vports.size() != 4u
                        || reader.vports[0].name != "NAMED_CONFIG"
                        || reader.vports[1].name != "*ACTIVE"
                        || reader.vports[2].name != "*ACTIVE"
                        || reader.vports[3].name != "NAMED_CONFIG"
                        || reader.vports[0].center.x != 12.0
                        || reader.vports[1].center.x != 34.0
                        || reader.vports[2].center.x != 56.0
                        || reader.vports[3].center.x != 78.0)
                        return false;
                } else if (mode == 1) {
                    if (reader.vports.size() != 2u
                        || reader.vports[0].name != "NAMED_CONFIG"
                        || reader.vports[0].center.x != 12.0
                        || reader.vports[1].name != "*ACTIVE")
                        return false;
                } else if (reader.vports.size() != 1u
                           || reader.vports[0].name != "*ACTIVE"
                           || reader.vports[0].center.x != 90.0) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool allocationSweep(const std::filesystem::path& directory) {
    const auto path = directory / "vport-table-allocation.dxf";
    const std::string prior = "prior VPORT destination bytes\n";
    constexpr std::array<DRW::Version, 7> versions{{
        DRW::AC1014, DRW::AC1015, DRW::AC1018, DRW::AC1021,
        DRW::AC1024, DRW::AC1027, DRW::AC1032}};
    for (const DRW::Version version : versions) {
        for (const bool binary : {false, true}) {
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(path.string().c_str());
                Export exporter(codec, version);
                exporter.failAt = 1000000;
                const bool written = codec.write(&exporter, version, binary);
                if (!written || !exporter.accepted
                    || !sameVport(exporter.vport,
                                  makeVport(version), false)
                    || !readbackMatches(path, version)
                    || !readBytes(path, expectedBytes))
                    return false;
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
                Export exporter(codec, version);
                exporter.failAt = offset;
                const bool written = codec.write(&exporter, version, binary);
                exporter.failAt = -1;
                std::string current;
                if (written || exporter.accepted
                    || exporter.reached != offset + 1 || !allocationFailed
                    || !readBytes(path, current) || current != prior
                    || !sameVport(exporter.vport,
                                  makeVport(version), false))
                    return false;

                const bool retried = codec.write(&exporter, version, binary);
                std::string retryBytes;
                if (!retried || !exporter.accepted
                    || !sameVport(exporter.vport,
                                  makeVport(version), false)
                    || !readBytes(path, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(path, version))
                    return false;
            }
            std::cout << "VPORT " << versionCode(version)
                      << (binary ? " Binary" : " ASCII")
                      << " table-writer allocation offsets="
                      << allocationCount << '\n';
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
        / ("vport-table-allocation-"
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
    return viewportNamesAndRetry(directory) && allocationSweep(directory) ? 0 : 2;
}
