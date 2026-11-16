// SPDX-License-Identifier: GPL-2.0-or-later
// The title fixture belongs to the repository; no new drawing is committed.
#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <cstdlib>
#include <iostream>
#include <new>
#include <vector>

namespace {
constexpr std::size_t targetBytes = 2088u;
long remaining = -1;
std::size_t matchingAllocations = 0;
bool fired = false;
void arm(long n) { remaining = n; matchingAllocations = 0; fired = false; }
void disarm() { remaining = -1; }
}
void *operator new(std::size_t n) {
    if (remaining >= 0 && n == targetBytes) {
        ++matchingAllocations;
        if (remaining-- == 0) { remaining = -1; fired = true; throw std::bad_alloc(); }
    }
    if (void *p = std::malloc(n ? n : 1u)) return p;
    throw std::bad_alloc();
}
void *operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }

namespace {
struct Capture final : LC3CompatCheck {
    std::vector<std::uint8_t> recordPreview;
    bool recordSeen = false;
    bool blockSeen = false;
    bool blockMatches = false;
    bool duplicate = false;
    void addBlockRecord(const DRW_Block_Record &record) override {
        if (record.handle != 0x127u) return;
        if (recordSeen) { duplicate = true; return; }
        recordPreview = record.previewData;
        recordSeen = true;
    }
    void addBlock(const DRW_Block &block) override {
        if (block.name != "Title_Block-ISO") return;
        if (blockSeen) { duplicate = true; return; }
        blockSeen = true;
        blockMatches = recordSeen && block.parentHandle == 0x127u
            && block.previewData == recordPreview;
    }
};
bool complete(const Capture &capture) {
    return !capture.duplicate && capture.recordSeen && capture.blockSeen
        && capture.blockMatches && capture.recordPreview.size() == targetBytes;
}
bool read(const char *path, Capture &capture) {
    dwgR reader(path);
    return reader.read(&capture, false) && reader.getVersion() == DRW::AC1024;
}
}
int main(int argc, char **argv) {
    if (argc != 2) return 1;
    Capture baseline;
    if (!read(argv[1], baseline) || !complete(baseline)) {
        std::cerr << "BLOCK preview missing or differs from its source record\n";
        return 1;
    }
    Capture calibration;
    arm(1000000);
    bool ok = false;
    try { ok = read(argv[1], calibration); } catch (...) { disarm(); return 1; }
    const std::size_t count = matchingAllocations;
    disarm();
    if (!ok || !complete(calibration) || count == 0u || count > 100u) return 1;
    for (std::size_t i = 0; i < count; ++i) {
        Capture failed;
        arm(static_cast<long>(i));
        try { (void)read(argv[1], failed); } catch (...) { }
        const bool hit = fired;
        disarm();
        // An observed block must never contain a silently lost preview.
        if (!hit || (failed.blockSeen && !failed.blockMatches)) return 1;
        Capture retry;
        if (!read(argv[1], retry) || !complete(retry)) return 1;
    }
    std::cout << "BLOCK preview delivery PASS: " << targetBytes << " bytes; "
              << count << " allocation failures/retries\n";
    return 0;
}
