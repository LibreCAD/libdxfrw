// SPDX-License-Identifier: GPL-2.0-or-later
// Provisional source-body observation; this is not graph/output authorization.
#include "drw_objects.h"
#ifndef LIBDXFRW_HAS_AC1024_LAYER_CONTROL_SOURCE
#error The source data header must advertise its optional capture interface
#endif
#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include "intern/dwgbuffer.h"
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <new>
#include <optional>
#include <string>

namespace {
long allocationsBeforeFailure = -1;
std::size_t allocationAttempts = 0u;
bool allocationTriggered = false;
void arm(long count) noexcept { allocationsBeforeFailure = count; allocationAttempts = 0u; allocationTriggered = false; }
void disarm() noexcept { allocationsBeforeFailure = -1; }
}
void *operator new(std::size_t size) {
    if (allocationsBeforeFailure >= 0) {
        ++allocationAttempts;
        if (allocationsBeforeFailure-- == 0) {
            allocationsBeforeFailure = -1; allocationTriggered = true; throw std::bad_alloc();
        }
    }
    if (void *value = std::malloc(size == 0u ? 1u : size)) return value;
    throw std::bad_alloc();
}
void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void *value) noexcept { std::free(value); }
void operator delete[](void *value) noexcept { std::free(value); }
void operator delete(void *value, std::size_t) noexcept { std::free(value); }
void operator delete[](void *value, std::size_t) noexcept { std::free(value); }
namespace {
struct Capture final : LC3CompatCheck, DRW_DwgAc1024LayerControlSourceHandler {
    std::optional<DRW_DwgAc1024LayerControlSource> source;
    std::optional<DRW_DwgFramePublication> publication;
    DRW_DwgFrameCoverageReport coverage;
    bool failed = false;
    bool rejectCapture = false;
    bool captureRejected = false;
    void addDwgAc1024LayerControlSource(const DRW_DwgAc1024LayerControlSource &value) noexcept override {
        try {
            if (rejectCapture) throw std::bad_alloc();
            if (source) failed = true; else source = value;
        } catch (...) { captureRejected = true; }
    }
    void addDwgFramePublication(const DRW_DwgFramePublication &value) override {
        if (value.m_handle == 0x10u) { if (publication) failed = true; else publication = value; }
    }
    void addDwgFrameCoverageReport(const DRW_DwgFrameCoverageReport &value) override { coverage = value; }
};
bool require(bool value, const char *what) {
    if (!value) std::cerr << "AC1024 layer-control source: " << what << '\n';
    return value;
}
bool run(const std::string &sourcePath, const std::string &otherPath) {
    Capture c;
    dwgR reader(sourcePath.c_str());
    if (!require(reader.read(&c, false), "title fixture read failed")
        || !require(!c.failed && c.source.has_value() && c.publication.has_value(), "missing or duplicate callback"))
        return false;
    const auto &s = *c.source;
    const auto &r = s.m_raw;
    const auto &p = *c.publication;
    if (!require(r.m_version == DRW::AC1024 && r.m_handle == 0x10u
                 && r.m_objectType == DRW::DwgLayerControlObjectType
                 && r.m_xDictFlag == 0u && r.m_xDictHandle == 0x107u
                 && !r.m_rawBytes.empty() && r.m_rawBytes.size() == r.m_objectSize,
                 "source identity changed")
        || !require(p.m_handle == r.m_handle && p.m_version == r.m_version
                    && p.m_sourceOffset == s.m_sourceOffset
                    && p.m_sourceMapOrdinal == s.m_sourceMapOrdinal
                    && p.m_sourceOffsetSpace == s.m_sourceOffsetSpace
                    && p.m_carrier == DRW_DwgFramePublication::Carrier::Control
                    && p.m_xDictHandle == r.m_xDictHandle,
                    "source/publication mismatch")
        || !require(c.coverage.m_complete && c.coverage.m_status == DRW_DwgFrameCoverageStatus::FinalizedComplete,
                    "source frame coverage incomplete")) return false;
    const auto valid = [&] { return s.matchesAc1024LayerControlFrame(p); };
    if (!require(valid(), "matching control source was refused")) return false;
    arm(1000000); const bool calibrated = valid(); const auto count = allocationAttempts; disarm();
    if (!require(calibrated && count > 0u, "allocation calibration failed")) return false;
    for (std::size_t i = 0u; i < count; ++i) {
        arm(static_cast<long>(i)); const bool accepted = valid(); const bool hit = allocationTriggered; disarm();
        if (!require(hit && !accepted && valid(), "allocation failure/retry failed")) return false;
    }
    auto changed = s;
    const std::uint64_t handleStart = static_cast<std::uint64_t>(r.m_rawBytes.size()) * 8u - r.m_bodyBitSize;
    for (const std::uint64_t bit : {handleStart, handleStart + 8u, handleStart + 32u}) {
        changed = s;
        changed.m_raw.m_rawBytes[bit / 8u] ^= static_cast<std::uint8_t>(0x80u >> (bit % 8u));
        if (!require(!changed.matchesAc1024LayerControlFrame(p), "changed wire role accepted")) return false;
    }
    changed = s; changed.m_raw.m_rawBytes.pop_back(); --changed.m_raw.m_objectSize;
    if (!require(!changed.matchesAc1024LayerControlFrame(p), "truncated control frame accepted")) return false;
    changed = s; changed.m_raw.m_version = DRW::AC1027;
    if (!require(!changed.matchesAc1024LayerControlFrame(p), "wrong source version accepted")) return false;
    changed = s; changed.m_raw.m_xDictHandle ^= 1u;
    if (!require(!changed.matchesAc1024LayerControlFrame(p), "wrong dictionary accepted")) return false;
    changed = s; changed.m_raw.m_parentHandle = 1u;
    if (!require(!changed.matchesAc1024LayerControlFrame(p), "invented raw owner accepted")) return false;
    changed = s; changed.m_raw.m_commonHandleDataValidated = true;
    if (!require(!changed.matchesAc1024LayerControlFrame(p), "invented raw evidence accepted")) return false;
    auto changedPublication = p; ++changedPublication.m_sourceMapOrdinal;
    if (!require(!s.matchesAc1024LayerControlFrame(changedPublication), "wrong publication accepted")) return false;
    changedPublication = p; changedPublication.m_controlHandles.front() ^= 1u;
    if (!require(!s.matchesAc1024LayerControlFrame(changedPublication), "wrong member accepted")) return false;
    if (!require(valid(), "failed controls changed source")) return false;
    std::cout << count << " verifier allocation failure/retry controls passed\n";
    dwgBuffer body(r.m_rawBytes.data(), r.m_rawBytes.size());
    if (!require(body.getObjType(DRW::AC1024) == DRW::DwgLayerControlObjectType && body.isGood(),
                 "wire type mismatch")) return false;
    const std::uint64_t totalBits = static_cast<std::uint64_t>(r.m_rawBytes.size()) * 8u;
    if (!require(r.m_bodyBitSize < totalBits, "bad handle stream size")) return false;
    const auto start = totalBits - r.m_bodyBitSize;
    if (!require(body.setPosition(start >> 3u), "bad handle stream offset")) return false;
    body.setBitPos(static_cast<std::uint8_t>(start & 7u));
    const dwgHandle owner = body.getHandle();
    const dwgHandle dictionary = body.getHandle();
    const dwgHandle layer = body.getHandle();
    if (!require(body.isGood() && owner.code == DRW::DwgSoftPointer && owner.ref == 0u
                 && dictionary.code == DRW::DwgHardOwnership && dictionary.ref == 0x107u
                 && layer.code == DRW::DwgSoftOwnership && layer.ref == 0x4u,
                 "wire owner/dictionary/layer roles differ from the independently decoded source")) return false;
    Capture other;
    dwgR oldReader(otherPath.c_str());
    if (!require(oldReader.read(&other, false) && !other.failed && !other.source.has_value(),
                 "non-AC1024 input published selected evidence")) return false;
    Capture retry;
    retry.rejectCapture = true;
    dwgR firstAttempt(sourcePath.c_str());
    if (!require(firstAttempt.read(&retry, false) && retry.captureRejected
                 && !retry.source.has_value() && !retry.failed,
                 "consumer evidence allocation failure changed the read")) return false;
    retry.rejectCapture = false;
    retry.captureRejected = false;
    retry.publication.reset();
    retry.coverage = {};
    dwgR secondAttempt(sourcePath.c_str());
    if (!require(secondAttempt.read(&retry, false) && retry.source.has_value()
                 && !retry.captureRejected && !retry.failed,
                 "consumer evidence retry failed")) return false;
    LC3CompatCheck legacy;
    dwgR oldConsumer(sourcePath.c_str());
    if (!require(oldConsumer.read(&legacy, false), "consumer without optional handler failed")) return false;
    std::cout << "AC1024 layer-control source: PASS (physical owner/xdictionary/layer roles; old consumer, version and evidence failure/retry controls)\n";
    return true;
}
}
int main(int argc, char **argv) { return argc == 3 && run(argv[1], argv[2]) ? 0 : 1; }
