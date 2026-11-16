// SPDX-License-Identifier: GPL-2.0-or-later
// Generated VIEW table-record allocation rollback/retry control.
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

void disarmAllocationFailure() noexcept {
    allocationsBeforeFailure = -1;
}

DRW_View makeView(DRW::Version version, bool modernExtras = false,
                  bool camera = false, bool resourceReferences = false) {
    DRW_View view;
    view.handle = 0xD720u;
    view.parentHandle = 6u;
    view.name = "Generated allocation retry VIEW";
    view.size = DRW_Coord(240.0, 180.0, 0.0);
    view.center = DRW_Coord(3.5, -4.25, 0.0);
    view.viewDirectionFromTarget = DRW_Coord(0.0, 0.0, 1.0);
    view.targetPoint = DRW_Coord(5.0, 6.0, 7.0);
    view.lensLen = 50.0;
    view.frontClippingPlaneOffset = 0.25;
    view.backClippingPlaneOffset = 100.0;
    view.twistAngle = 12.5;
    view.viewMode = 3;
    if (version > DRW::AC1014) {
        view.renderMode = 2;
        view.hasUCS = true;
        view.ucsOrigin = DRW_Coord(1.0, 2.0, 3.0);
        view.ucsXAxis = DRW_Coord(1.0, 0.0, 0.0);
        view.ucsYAxis = DRW_Coord(0.0, 1.0, 0.0);
        view.ucsOrthoType = 4;
        view.ucsElevation = 8.5;
    }
    view.cameraPlottable = camera;
    if (version > DRW::AC1018 || modernExtras) {
        view.m_useDefaultLights = false;
        view.m_defaultLightingType = 1;
        view.m_brightness = 0.75;
        view.m_contrast = 0.4;
        view.m_ambientColor = 7;
    }
    if (resourceReferences) {
        view.m_backgroundHandle = 0xD721u;
        view.m_visualStyleHandle = 0xD722u;
    }
    view.extData.push_back(new DRW_Variant(1001, std::string("ACAD")));
    view.extData.push_back(new DRW_Variant(
        1000, std::string("view-allocation-retry")));
    return view;
}

bool sameView(const DRW_View& actual, DRW::Version version,
              bool readback = false, bool modernExtras = false,
              bool camera = false, bool resourceReferences = false) {
    const DRW_View expected = makeView(
        version, modernExtras, camera, resourceReferences);
    return actual.tType == DRW::VIEW
        && (readback ? actual.handle != 0u : actual.handle == expected.handle)
        && actual.parentHandle == expected.parentHandle
        && actual.name == expected.name
        && actual.size.x == expected.size.x
        && actual.size.y == expected.size.y
        && actual.center.x == expected.center.x
        && actual.center.y == expected.center.y
        && actual.viewDirectionFromTarget.x
            == expected.viewDirectionFromTarget.x
        && actual.viewDirectionFromTarget.y
            == expected.viewDirectionFromTarget.y
        && actual.viewDirectionFromTarget.z
            == expected.viewDirectionFromTarget.z
        && actual.targetPoint.x == expected.targetPoint.x
        && actual.targetPoint.y == expected.targetPoint.y
        && actual.targetPoint.z == expected.targetPoint.z
        && actual.lensLen == expected.lensLen
        && actual.frontClippingPlaneOffset
            == expected.frontClippingPlaneOffset
        && actual.backClippingPlaneOffset
            == expected.backClippingPlaneOffset
        && actual.twistAngle == expected.twistAngle
        && actual.viewMode == expected.viewMode
        && actual.renderMode == expected.renderMode
        && actual.hasUCS == expected.hasUCS
        && actual.cameraPlottable == expected.cameraPlottable
        && actual.m_useDefaultLights == expected.m_useDefaultLights
        && actual.m_defaultLightingType == expected.m_defaultLightingType
        && actual.m_brightness == expected.m_brightness
        && actual.m_contrast == expected.m_contrast
        && actual.m_ambientColor == expected.m_ambientColor
        && actual.m_backgroundHandle == expected.m_backgroundHandle
        && actual.m_visualStyleHandle == expected.m_visualStyleHandle
        && actual.ucsOrigin.x == expected.ucsOrigin.x
        && actual.ucsOrigin.y == expected.ucsOrigin.y
        && actual.ucsOrigin.z == expected.ucsOrigin.z
        && actual.ucsXAxis.x == expected.ucsXAxis.x
        && actual.ucsXAxis.y == expected.ucsXAxis.y
        && actual.ucsXAxis.z == expected.ucsXAxis.z
        && actual.ucsYAxis.x == expected.ucsYAxis.x
        && actual.ucsYAxis.y == expected.ucsYAxis.y
        && actual.ucsYAxis.z == expected.ucsYAxis.z
        && actual.ucsOrthoType == expected.ucsOrthoType
        && actual.ucsElevation == expected.ucsElevation
        && actual.m_useDefaultLights == expected.m_useDefaultLights
        && actual.m_defaultLightingType == expected.m_defaultLightingType
        && actual.m_brightness == expected.m_brightness
        && actual.m_contrast == expected.m_contrast
        && actual.m_ambientColor == expected.m_ambientColor
        && actual.extData.size() == expected.extData.size()
        && actual.extData[0]
        && expected.extData[0]
        && actual.extData[0]->code() == expected.extData[0]->code()
        && actual.extData[0]->content.s
        && expected.extData[0]->content.s
        && *actual.extData[0]->content.s == *expected.extData[0]->content.s
        && actual.extData[1]
        && expected.extData[1]
        && actual.extData[1]->code() == expected.extData[1]->code()
        && actual.extData[1]->content.s
        && expected.extData[1]->content.s
        && *actual.extData[1]->content.s == *expected.extData[1]->content.s;
}

class Export final : public LC3CompatCheck {
public:
    Export(dxfRW& codec, DRW::Version version,
           bool modernExtras = false, bool camera = false,
           bool resourceReferences = false)
        : codec(codec), version(version), modernExtras(modernExtras),
          camera(camera), resourceReferences(resourceReferences),
          view(makeView(version, modernExtras, camera, resourceReferences)) {
        if (resourceReferences)
            codec.setResolvedBackgroundHandles({view.m_backgroundHandle});
    }

    void writeViews() override {
        if (failAt < 0) {
            accepted = codec.writeView(&view);
            return;
        }
        armAllocationFailure(failAt);
        try {
            accepted = codec.writeView(&view);
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
    bool modernExtras;
    bool camera;
    bool resourceReferences;
    DRW_View view;
    long failAt = -1;
    long reached = 0;
    bool accepted = false;
};

class Reader final : public LC3CompatCheck {
public:
    void addView(const DRW_View& value) override { views.push_back(value); }
    std::vector<DRW_View> views;
};

const char* versionCode(DRW::Version version);

bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
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

bool readbackMatches(const std::filesystem::path& path,
                     DRW::Version version, bool modernExtras = false,
                     bool camera = false, bool resourceReferences = false) {
    dxfRW codec(path.string().c_str());
    Reader reader;
    if (!codec.read(&reader, false) || reader.views.size() != 1u)
        return false;
    if (!sameView(reader.views.front(), version, true, modernExtras,
                  camera, resourceReferences)) {
        const DRW_View& view = reader.views.front();
        const DRW_View expected = makeView(
            version, modernExtras, camera, resourceReferences);
        std::cerr << "VIEW readback mismatch: version=" << versionCode(version)
                  << " handle=" << view.handle << '/' << expected.handle
                  << " owner=" << view.parentHandle << '/' << expected.parentHandle
                  << " name=" << view.name << '/' << expected.name
                  << " geometry=" << view.size.x << ',' << view.size.y << '/'
                  << expected.size.x << ',' << expected.size.y
                  << " mode=" << view.viewMode << '/' << expected.viewMode
                  << " render=" << view.renderMode << '/' << expected.renderMode
                  << " ucs=" << view.hasUCS << '/' << expected.hasUCS
                  << " ortho=" << view.ucsOrthoType << '/'
                  << expected.ucsOrthoType
                  << " lighting=" << view.m_useDefaultLights << ','
                  << static_cast<unsigned int>(view.m_defaultLightingType)
                  << ',' << view.m_brightness << ',' << view.m_contrast << ','
                  << view.m_ambientColor << " / " << expected.m_useDefaultLights
                  << ',' << static_cast<unsigned int>(expected.m_defaultLightingType)
                  << ',' << expected.m_brightness << ',' << expected.m_contrast
                  << ',' << expected.m_ambientColor
                  << " eed=" << view.extData.size() << '/'
                  << expected.extData.size() << '\n';
        return false;
    }
    return true;
}

bool allocationSweep(const std::filesystem::path& directory) {
    const auto path = directory / "view-table-allocation.dxf";
    const std::string prior = "prior VIEW destination bytes\n";
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
                const bool sourceMatches = sameView(exporter.view, version);
                const bool readback = readbackMatches(path, version);
                if (!written || !exporter.accepted || !sourceMatches
                    || !readback || !readBytes(path, expectedBytes)) {
                    std::cerr << "clean VIEW write failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " written=" << written
                              << " accepted=" << exporter.accepted
                              << " source=" << sourceMatches
                              << " readback=" << readback << '\n';
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
                Export exporter(codec, version);
                exporter.failAt = offset;
                const bool written = codec.write(&exporter, version, binary);
                exporter.failAt = -1;
                std::string current;
                if (written || exporter.accepted
                    || exporter.reached != offset + 1
                    || !allocationFailed
                    || !readBytes(path, current) || current != prior
                    || !sameView(exporter.view, version)) {
                    std::cerr << "VIEW allocation rollback failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " offset=" << offset << " written=" << written
                              << " accepted=" << exporter.accepted
                              << " reached=" << exporter.reached
                              << " failed=" << allocationFailed << '\n';
                    return false;
                }

                const bool retried = codec.write(&exporter, version, binary);
                std::string retryBytes;
                if (!retried || !exporter.accepted
                    || !sameView(exporter.view, version)
                    || !readBytes(path, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(path, version)) {
                    std::cerr << "VIEW retry failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " offset=" << offset << '\n';
                    return false;
                }
            }
            std::cout << "VIEW " << versionCode(version)
                      << (binary ? " Binary" : " ASCII")
                      << " table-writer allocation offsets="
                      << allocationCount << '\n';
        }
    }
    return true;
}

bool refuseLostViewStateThenRetry(
        const std::filesystem::path& directory, const char* label,
        DRW::Version refusedVersion, DRW::Version supportedVersion,
        bool modernExtras, bool camera, bool resourceReferences) {
    const std::string sentinel = "prior VIEW destination bytes\n";
    for (const bool binary : {false, true}) {
        const std::string stem = std::string("view-loss-") + label
            + (binary ? "-binary" : "-ascii");
        const auto output = directory / (stem + ".dxf");
        const auto expectedPath = directory / (stem + "-expected.dxf");
        {
            dxfRW expectedCodec(expectedPath.string().c_str());
            Export expectedExporter(expectedCodec, supportedVersion,
                modernExtras, camera, resourceReferences);
            if (!expectedCodec.write(&expectedExporter, supportedVersion, binary)
                || !expectedExporter.accepted
                || !readbackMatches(expectedPath, supportedVersion,
                    modernExtras, camera, resourceReferences))
                return false;
        }
        std::string expectedBytes;
        if (!readBytes(expectedPath, expectedBytes))
            return false;
        {
            std::ofstream prior(output, std::ios::binary | std::ios::trunc);
            prior << sentinel;
            if (!prior)
                return false;
        }
        dxfRW codec(output.string().c_str());
        Export exporter(codec, supportedVersion, modernExtras,
                        camera, resourceReferences);
        const DRW_View original(exporter.view);
        const bool refused = !codec.write(&exporter, refusedVersion, binary)
            && !exporter.accepted
            && sameView(exporter.view, supportedVersion, false,
                        modernExtras, camera, resourceReferences);
        std::string afterRefusal;
        if (!refused || !readBytes(output, afterRefusal)
            || afterRefusal != sentinel || !sameView(exporter.view,
                supportedVersion, false, modernExtras, camera,
                resourceReferences))
            return false;
        const bool retried = codec.write(&exporter, supportedVersion, binary)
            && exporter.accepted
            && sameView(exporter.view, supportedVersion, false,
                        modernExtras, camera, resourceReferences);
        std::string retryBytes;
        if (!retried || exporter.view.handle != original.handle
            || !readBytes(output, retryBytes) || retryBytes != expectedBytes
            || !readbackMatches(output, supportedVersion, modernExtras,
                                camera, resourceReferences))
            return false;
    }
    return true;
}

bool lossAvoidanceControls(const std::filesystem::path& directory) {
    return refuseLostViewStateThenRetry(
               directory, "camera-plottable", DRW::AC1018, DRW::AC1021,
               false, true, false)
        && refuseLostViewStateThenRetry(
               directory, "lighting", DRW::AC1018, DRW::AC1021,
               true, false, false)
        && refuseLostViewStateThenRetry(
               directory, "resource-refs", DRW::AC1018, DRW::AC1021,
               false, false, true);
}

// R12/R14 VIEW records cannot retain the R2000 render-mode and associated
// UCS fields in a native drawing. Refuse both profiles before downgrading.
bool viewLegacyLossGuard(const std::filesystem::path& directory) {
    const auto path = directory / "view-legacy-loss.dxf";
    const std::string prior = "previous destination bytes\n";
    constexpr std::array<DRW::Version, 2> legacy{{DRW::AC1009, DRW::AC1014}};
    for (const auto target : legacy) {
      for (const bool ucs : {false, true}) {
        for (const bool binary : {false, true}) {
            dxfRW codec(path.string().c_str());
            Export exporter(codec, DRW::AC1015);
            exporter.view.hasUCS = ucs;
            if (ucs) exporter.view.renderMode = 0u;
            const DRW_View original(exporter.view);
            {
                std::ofstream output(path, std::ios::binary | std::ios::trunc);
                output << prior;
                if (!output) return false;
            }
            const bool wrote = codec.write(&exporter, target, binary);
            std::string current;
            if (wrote || exporter.accepted || exporter.view.hasUCS != ucs
                || exporter.view.renderMode != original.renderMode
                || exporter.view.handle != original.handle
                || exporter.view.ucsElevation != original.ucsElevation
                || !readBytes(path, current) || current != prior) {
                std::cerr << "nondefault VIEW state lost on legacy export: version="
                          << static_cast<int>(target) << " ucs=" << ucs
                          << " binary=" << binary << '\n';
                return false;
            }
            if (!codec.write(&exporter, DRW::AC1015, binary) || !exporter.accepted
                || exporter.view.handle != original.handle
                || exporter.view.hasUCS != ucs
                || exporter.view.renderMode != original.renderMode
                || exporter.view.ucsElevation != original.ucsElevation)
                return false;
            Reader modern;
            dxfRW modernReader(path.string().c_str());
            if (!modernReader.read(&modern, false) || modern.views.size() != 1u
                || modern.views.front().renderMode != original.renderMode
                || modern.views.front().hasUCS != ucs
                || (ucs && modern.views.front().ucsElevation != original.ucsElevation))
                return false;
            exporter.view.renderMode = 0u;
            exporter.view.hasUCS = false;
            if (!codec.write(&exporter, target, binary) || !exporter.accepted)
                return false;
            Reader legacyRead;
            dxfRW legacyReader(path.string().c_str());
            if (!legacyReader.read(&legacyRead, false) || legacyRead.views.size() != 1u
                || legacyRead.views.front().renderMode != 0u
                || legacyRead.views.front().hasUCS)
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
        / ("view-table-allocation-"
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
    return viewLegacyLossGuard(directory) && allocationSweep(directory)
        && lossAvoidanceControls(directory)
        ? 0 : 2;
}
