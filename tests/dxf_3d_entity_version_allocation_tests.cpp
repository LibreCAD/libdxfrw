// SPDX-License-Identifier: GPL-2.0-or-later
// Generated DXF 3DFACE and 3DLINE writer version transaction controls.
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

enum class Profile { Face, Line };

DRW_3Dface makeFace() {
    DRW_3Dface face;
    face.handle = 0xD926u;
    face.basePoint = DRW_Coord{1.0, 2.0, 3.0};
    face.secPoint = DRW_Coord{4.0, 5.0, 6.0};
    face.thirdPoint = DRW_Coord{7.0, 8.0, 9.0};
    face.fourPoint = DRW_Coord{10.0, 11.0, 12.0};
    face.invisibleflag = DRW_3Dface::SecodEdge | DRW_3Dface::FourthEdge;
    return face;
}

DRW_3DLine makeLine() {
    DRW_3DLine line;
    line.handle = 0xD92Au;
    line.basePoint = DRW_Coord{1.25, -2.5, 3.75};
    line.secPoint = DRW_Coord{-4.5, 5.25, -6.75};
    line.extPoint = DRW_Coord{0.0, 1.0, 0.0};
    line.thickness = 2.5;
    return line;
}

bool sameFace(const DRW_3Dface& a, const DRW_3Dface& b) {
    return a.handle == b.handle && a.basePoint.x == b.basePoint.x
        && a.basePoint.y == b.basePoint.y && a.basePoint.z == b.basePoint.z
        && a.secPoint.x == b.secPoint.x && a.secPoint.y == b.secPoint.y
        && a.secPoint.z == b.secPoint.z
        && a.thirdPoint.x == b.thirdPoint.x
        && a.thirdPoint.y == b.thirdPoint.y
        && a.thirdPoint.z == b.thirdPoint.z
        && a.fourPoint.x == b.fourPoint.x && a.fourPoint.y == b.fourPoint.y
        && a.fourPoint.z == b.fourPoint.z
        && a.invisibleflag == b.invisibleflag;
}

bool sameLine(const DRW_3DLine& a, const DRW_3DLine& b) {
    return a.handle == b.handle && a.basePoint.x == b.basePoint.x
        && a.basePoint.y == b.basePoint.y && a.basePoint.z == b.basePoint.z
        && a.secPoint.x == b.secPoint.x && a.secPoint.y == b.secPoint.y
        && a.secPoint.z == b.secPoint.z
        && a.extPoint.x == b.extPoint.x && a.extPoint.y == b.extPoint.y
        && a.extPoint.z == b.extPoint.z && a.thickness == b.thickness;
}

class Export final : public dx_iface {
public:
    Export(dxfRW& codec, Profile profile) : codec(codec), profile(profile) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &codec;
        face = makeFace();
        line = makeLine();
    }

    void writeEntities() override {
        struct TrackingScope {
            long& reached;
            ~TrackingScope() {
                reached = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
            }
        } trackingScope{reached};
        armAllocationFailure(failAt);
        accepted = profile == Profile::Face
            ? codec.write3dface(&face) : codec.write3DLine(&line);
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    Profile profile;
    dx_data storage;
    DRW_3Dface face;
    DRW_3DLine line;
    long failAt = -1;
    long reached = 0;
    bool accepted = false;
};

bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

bool readbackMatches(const std::filesystem::path& path, Profile profile,
                     DRW::Version expectedVersion) {
    dx_data data;
    dx_iface reader;
    if (!reader.fileImport(path.string(), &data, false)
        || data.sourceVersion != expectedVersion || data.mBlock == nullptr
        || data.mBlock->ent.size() != 1u) {
        std::cerr << "3D entity typed readback failed, profile="
                  << (profile == Profile::Face ? "3DFACE" : "3DLINE")
                  << "\n";
        return false;
    }
    const DRW_Entity* entity = data.mBlock->ent.front();
    if (profile == Profile::Face) {
        const auto* face = dynamic_cast<const DRW_3Dface*>(entity);
        const DRW_3Dface expected = makeFace();
        return face != nullptr && face->eType == DRW::E3DFACE
            && face->basePoint.x == expected.basePoint.x
            && face->basePoint.y == expected.basePoint.y
            && face->basePoint.z == expected.basePoint.z
            && face->secPoint.x == expected.secPoint.x
            && face->secPoint.y == expected.secPoint.y
            && face->secPoint.z == expected.secPoint.z
            && face->thirdPoint.x == expected.thirdPoint.x
            && face->thirdPoint.y == expected.thirdPoint.y
            && face->thirdPoint.z == expected.thirdPoint.z
            && face->fourPoint.x == expected.fourPoint.x
            && face->fourPoint.y == expected.fourPoint.y
            && face->fourPoint.z == expected.fourPoint.z
            && face->invisibleflag == expected.invisibleflag;
    }
    const auto* line = dynamic_cast<const DRW_3DLine*>(entity);
    const DRW_3DLine expected = makeLine();
    return line != nullptr && line->eType == DRW::THREEDLINE
        && line->basePoint.x == expected.basePoint.x
        && line->basePoint.y == expected.basePoint.y
        && line->basePoint.z == expected.basePoint.z
        && line->secPoint.x == expected.secPoint.x
        && line->secPoint.y == expected.secPoint.y
        && line->secPoint.z == expected.secPoint.z
        && line->extPoint.x == expected.extPoint.x
        && line->extPoint.y == expected.extPoint.y
        && line->extPoint.z == expected.extPoint.z
        && line->thickness == expected.thickness;
}

bool runProfile(const std::filesystem::path& directory, Profile profile) {
    const auto fail = [profile](const char* message) {
        std::cerr << (profile == Profile::Face ? "3DFACE" : "3DLINE")
                  << " transaction matrix: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    const std::array<std::pair<DRW::Version, const char*>, 7> faceVersions = {{
        {DRW::AC1014, "AC1014"}, {DRW::AC1015, "AC1015"},
        {DRW::AC1018, "AC1018"}, {DRW::AC1021, "AC1021"},
        {DRW::AC1024, "AC1024"}, {DRW::AC1027, "AC1027"},
        {DRW::AC1032, "AC1032"}}};
    const std::array<std::pair<DRW::Version, const char*>, 6> lineVersions = {{
        {DRW::AC1015, "AC1015"}, {DRW::AC1018, "AC1018"},
        {DRW::AC1021, "AC1021"}, {DRW::AC1024, "AC1024"},
        {DRW::AC1027, "AC1027"}, {DRW::AC1032, "AC1032"}}};
    const auto runVersion = [&](DRW::Version target, const char* versionName) {
        for (const bool binary : {false, true}) {
            const std::string stem = profile == Profile::Face
                ? "3dface-" : "3dline-";
            const auto output = directory
                / (stem + versionName + (binary ? "-binary.dxf" : "-ascii.dxf"));
            std::string expectedBytes;
            long allocationCount = 0;
            {
                dxfRW codec(output.string().c_str());
                Export exporter(codec, profile);
                exporter.failAt = 1000000;
                const bool written = codec.write(&exporter, target, binary);
                if (!written || !exporter.accepted
                    || !readBytes(output, expectedBytes))
                    return fail("clean reference write failed");
                allocationCount = exporter.reached;
            }
            if (allocationCount <= 0)
                return fail("writer reported no allocation callbacks");

            const std::string sentinel = "prior 3D entity destination\n";
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                                        std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior)
                        return fail("could not write destination sentinel");
                }
                dxfRW codec(output.string().c_str());
                Export exporter(codec, profile);
                const DRW_3Dface originalFace = exporter.face;
                const DRW_3DLine originalLine = exporter.line;
                exporter.failAt = offset;
                const bool written = codec.write(&exporter, target, binary);
                const bool callerPreserved = profile == Profile::Face
                    ? sameFace(exporter.face, originalFace)
                    : sameLine(exporter.line, originalLine);
                if (!allocationFailed || written
                    || exporter.reached != offset + 1 || !callerPreserved) {
                    std::cerr << "offset=" << offset
                              << " attempts=" << exporter.reached
                              << " injected=" << allocationFailed
                              << " written=" << written
                              << " accepted=" << exporter.accepted
                              << " callerPreserved=" << callerPreserved
                              << '\n';
                    return fail("allocation failure did not roll back caller state");
                }
                std::string afterFailure;
                if (!readBytes(output, afterFailure) || afterFailure != sentinel)
                    return fail("allocation failure replaced prior destination");

                exporter.failAt = -1;
                const bool retried = codec.write(&exporter, target, binary);
                std::string retryBytes;
                if (!retried || !exporter.accepted
                    || !readBytes(output, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(output, profile, target)
                    || (profile == Profile::Face
                        ? !sameFace(exporter.face, originalFace)
                        : !sameLine(exporter.line, originalLine)))
                    return fail("same-codec retry or typed readback failed");
            }
            std::cout << (profile == Profile::Face ? "3DFACE " : "3DLINE ")
                      << versionName << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
        return true;
    };

    if (profile == Profile::Face) {
        for (const auto& version : faceVersions)
            if (!runVersion(version.first, version.second))
                return false;
    } else {
        for (const auto& version : lineVersions)
            if (!runVersion(version.first, version.second))
                return false;
    }
    return true;
}

bool run(const std::filesystem::path& directory) {
    return runProfile(directory, Profile::Face)
        && runProfile(directory, Profile::Line);
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
        std::cerr << "usage: dxf_3d_entity_version_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
