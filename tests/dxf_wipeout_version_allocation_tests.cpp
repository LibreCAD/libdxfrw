// SPDX-License-Identifier: GPL-2.0-or-later
// Generated WIPEOUT version-matrix rollback/retry/readback control.
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include "intern/dxfwriter.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>
#include <utility>

namespace {
long allocationsBeforeFailure = -1;
std::size_t allocationAttempts = 0;

void armAllocationFailure(long count) noexcept {
    allocationsBeforeFailure = count;
    allocationAttempts = 0;
}

void disarmAllocationFailure() noexcept { allocationsBeforeFailure = -1; }

struct AllocationScope {
    explicit AllocationScope(long offset) noexcept {
        armAllocationFailure(offset);
    }
    ~AllocationScope() { disarmAllocationFailure(); }
};

DRW_Wipeout makeWipeout() {
    DRW_Wipeout value;
    value.handle = 0xD990u;
    value.basePoint = DRW_Coord{1.25, -2.5, 3.75};
    value.secPoint = DRW_Coord{4.5, 5.25, -6.75};
    value.vVector = DRW_Coord{0.0, 0.125, 0.0};
    value.sizeu = 640.0;
    value.sizev = 480.0;
    value.clip = 1;
    value.brightness = 63;
    value.contrast = 72;
    value.fade = 19;
    value.m_displayProps = 3;
    value.m_clipBoundaryType = 2;
    value.clipMode = false;
    for (unsigned i = 0; i < 64; ++i)
        value.clipPath.emplace_back(static_cast<double>(i),
                                    static_cast<double>((i * 7u) % 31u), 0.0);
    return value;
}

bool sameCoord(const DRW_Coord& a, const DRW_Coord& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool sameWipeout(const DRW_Wipeout& a, const DRW_Wipeout& b) {
    if (!sameCoord(a.basePoint, b.basePoint)
        || !sameCoord(a.secPoint, b.secPoint)
        || !sameCoord(a.vVector, b.vVector)
        || a.sizeu != b.sizeu || a.sizev != b.sizev
        || a.clip != b.clip || a.brightness != b.brightness
        || a.contrast != b.contrast || a.fade != b.fade
        || a.m_displayProps != b.m_displayProps
        || a.m_clipBoundaryType != b.m_clipBoundaryType
        || a.clipMode != b.clipMode || a.clipPath.size() != b.clipPath.size())
        return false;
    for (std::size_t i = 0; i < a.clipPath.size(); ++i) {
        if (!sameCoord(a.clipPath[i], b.clipPath[i]))
            return false;
    }
    return true;
}

struct Capture final : LC3CompatCheck {
    void addWipeout(const DRW_Wipeout* value) override {
        if (value != nullptr) {
            wipeout = *value;
            ++count;
        }
    }
    DRW_Wipeout wipeout;
    unsigned count = 0;
};

struct Export final : LC3CompatCheck {
    explicit Export(dxfRW& codec) : codec(codec), wipeout(makeWipeout()) {}

    void writeEntities() override {
        struct TrackingScope {
            long& reached;
            ~TrackingScope() {
                reached = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
            }
        } trackingScope{reached};
        AllocationScope allocationScope(failAt);
        try {
            accepted = codec.writeWipeout(&wipeout);
        } catch (const std::bad_alloc&) {
            accepted = false;
        }
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    DRW_Wipeout wipeout;
    long failAt = 1000000;
    long reached = 0;
    bool accepted = false;
};

std::string fileBytes(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>{input},
            std::istreambuf_iterator<char>{}};
}

bool readbackMatches(const std::filesystem::path& path,
                     DRW::Version expectedVersion,
                     const DRW_Wipeout& expected) {
    dxfRW codec(path.string().c_str());
    Capture capture;
    return codec.read(&capture, false)
        && codec.getSourceVersion() == expectedVersion
        && capture.count == 1u
        && sameWipeout(capture.wipeout, expected);
}

bool run(const std::filesystem::path& directory) {
    static constexpr std::array<std::pair<DRW::Version, const char*>, 7>
        versions = {{{DRW::AC1014, "AC1014"}, {DRW::AC1015, "AC1015"},
                     {DRW::AC1018, "AC1018"}, {DRW::AC1021, "AC1021"},
                     {DRW::AC1024, "AC1024"}, {DRW::AC1027, "AC1027"},
                     {DRW::AC1032, "AC1032"}}};
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error)
        return false;

    for (const auto& target : versions) {
        for (const bool binary : {false, true}) {
            const auto output = directory / (std::string("wipeout-")
                + target.second + (binary ? "-binary.dxf" : "-ascii.dxf"));
            long allocationCount = 0;
            std::string referenceBytes;
            {
                dxfRW codec(output.string().c_str());
                Export exporter(codec);
                if (!codec.write(&exporter, target.first, binary)
                    || !exporter.accepted || exporter.reached <= 0
                    || !readbackMatches(output, target.first, exporter.wipeout)) {
                    std::cerr << "WIPEOUT clean write failed at "
                              << target.second << '\n';
                    return false;
                }
                allocationCount = exporter.reached;
                referenceBytes = fileBytes(output);
            }
            if (referenceBytes.empty())
                return false;

            const std::string sentinel = "prior WIPEOUT bytes\n";
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream previous(output,
                        std::ios::binary | std::ios::trunc);
                    previous << sentinel;
                    if (!previous)
                        return false;
                }
                dxfRW codec(output.string().c_str());
                Export exporter(codec);
                exporter.failAt = offset;
                const DRW_Wipeout original = exporter.wipeout;
                const bool written = codec.write(&exporter, target.first, binary);
                if (written
                    || exporter.reached != offset + 1
                    || !sameWipeout(exporter.wipeout, original)
                    || fileBytes(output) != sentinel) {
                    std::cerr << "WIPEOUT rollback failed at " << target.second
                              << " offset " << offset << " written=" << written
                              << " callback=" << exporter.accepted
                              << " reached=" << exporter.reached
                              << " caller=" << sameWipeout(exporter.wipeout, original)
                              << " sentinel=" << (fileBytes(output) == sentinel)
                              << '\n';
                    return false;
                }
                exporter.failAt = 1000000;
                if (!codec.write(&exporter, target.first, binary)
                    || !exporter.accepted
                    || fileBytes(output) != referenceBytes
                    || !readbackMatches(output, target.first, exporter.wipeout)) {
                    std::cerr << "WIPEOUT retry failed at " << target.second
                              << " offset " << offset << '\n';
                    return false;
                }
            }
            std::cout << "WIPEOUT " << target.second << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
            std::filesystem::remove(output, error);
            error.clear();
        }
    }
    return true;
}

bool runClipModeVersionBoundary(const std::filesystem::path& directory) {
    for (const bool binary : {false, true}) {
        const auto output = directory / (binary
            ? "wipeout-clip-mode-binary.dxf" : "wipeout-clip-mode-ascii.dxf");
        const auto reference = directory / (binary
            ? "wipeout-clip-mode-reference-binary.dxf"
            : "wipeout-clip-mode-reference-ascii.dxf");
        DRW_Wipeout expected = makeWipeout();
        expected.clipMode = true;
        {
            dxfRW codec(reference.string().c_str());
            Export exporter(codec);
            exporter.wipeout = expected;
            if (!codec.write(&exporter, DRW::AC1024, binary)
                || !exporter.accepted
                || !readbackMatches(reference, DRW::AC1024, expected))
                return false;
        }
        const std::string sentinel = "prior WIPEOUT clip-mode bytes\n";
        {
            std::ofstream previous(output, std::ios::binary | std::ios::trunc);
            previous << sentinel;
            if (!previous)
                return false;
        }
        dxfRW codec(output.string().c_str());
        Export exporter(codec);
        exporter.wipeout = expected;
        const DRW_Wipeout original = exporter.wipeout;
        if (codec.write(&exporter, DRW::AC1021, binary)
            || exporter.accepted || fileBytes(output) != sentinel
            || !sameWipeout(exporter.wipeout, original)) {
            std::cerr << "WIPEOUT AC1021 clipMode loss was not refused\n";
            return false;
        }
        if (!codec.write(&exporter, DRW::AC1024, binary)
            || !exporter.accepted
            || fileBytes(output) != fileBytes(reference)
            || !readbackMatches(output, DRW::AC1024, expected)) {
            std::cerr << "WIPEOUT clipMode same-writer upgrade retry failed\n";
            return false;
        }
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
        std::filesystem::remove(reference, ignored);
    }
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (allocationsBeforeFailure >= 0)
        ++allocationAttempts;
    if (allocationsBeforeFailure == 0) {
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

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: dxf_wipeout_version_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    const std::filesystem::path directory(argv[1]);
    if (!run(directory) || !runClipModeVersionBoundary(directory))
        return 1;
    std::error_code error;
    std::filesystem::remove_all(directory, error);
    return error ? 1 : 0;
}
