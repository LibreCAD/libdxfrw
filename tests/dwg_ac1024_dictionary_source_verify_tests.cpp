// SPDX-License-Identifier: GPL-2.0-or-later
// Source-frame correlation only; no graph or output admission is inferred.
#include "lc3_compat_check.cpp"
#include "libdwgr.h"

#include <array>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <map>
#include <new>
#include <string>

namespace {
long allocationsBeforeFailure = -1;
std::size_t allocationAttempts = 0u;
bool allocationTriggered = false;

void armAllocationFailure(long offset) noexcept {
    allocationsBeforeFailure = offset;
    allocationAttempts = 0u;
    allocationTriggered = false;
}

void disarmAllocationFailure() noexcept { allocationsBeforeFailure = -1; }
}

void *operator new(std::size_t size) {
    if (allocationsBeforeFailure >= 0) {
        ++allocationAttempts;
        if (allocationsBeforeFailure == 0) {
            allocationsBeforeFailure = -1;
            allocationTriggered = true;
            throw std::bad_alloc();
        }
        --allocationsBeforeFailure;
    }
    if (void *result = std::malloc(size == 0u ? 1u : size)) return result;
    throw std::bad_alloc();
}
void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void *value) noexcept { std::free(value); }
void operator delete[](void *value) noexcept { std::free(value); }
void operator delete(void *value, std::size_t) noexcept { std::free(value); }
void operator delete[](void *value, std::size_t) noexcept { std::free(value); }

namespace {
constexpr std::array<std::uint32_t, 3> targets{{0x22D3u, 0x21CDu, 0x2120u}};

bool selected(std::uint32_t handle) {
    for (const auto target : targets)
        if (target == handle) return true;
    return false;
}

struct Capture final : LC3CompatCheck {
    std::map<std::uint32_t, DRW_Dictionary> typed;
    std::map<std::uint32_t, DRW_UnsupportedObject> raw;
    std::map<std::uint32_t, DRW_DwgFramePublication> publications;
    std::map<std::uint32_t, DRW_DwgDictionaryMembership> memberships;
    bool duplicate = false;

    void addDictionary(const DRW_Dictionary &value) override {
        if (selected(value.handle) && !typed.emplace(value.handle, value).second)
            duplicate = true;
    }
    void addUnsupportedObject(const DRW_UnsupportedObject &value) override {
        if (selected(value.m_handle) && !raw.emplace(value.m_handle, value).second)
            duplicate = true;
    }
    void addDwgFramePublication(const DRW_DwgFramePublication &value) override {
        if (selected(value.m_handle)
            && !publications.emplace(value.m_handle, value).second)
            duplicate = true;
    }
    void addDwgDictionaryMembership(const DRW_DwgDictionaryMembership &value) override {
        if (selected(value.m_dictionaryHandle)
            && !memberships.emplace(value.m_dictionaryHandle, value).second)
            duplicate = true;
    }
};

bool check(bool value, const char *message, std::uint32_t handle) {
    if (!value) std::cerr << "AC1024 DICTIONARY source verification: "
                          << message << " handle=" << std::hex << handle
                          << std::dec << '\n';
    return value;
}

bool run(const std::string &path) {
    Capture source;
    dwgR reader(path.c_str());
    if (!reader.read(&source, false) || reader.getVersion() != DRW::AC1024
        || source.duplicate || source.typed.size() != targets.size()
        || source.raw.size() != targets.size()
        || source.publications.size() != targets.size()
        || source.memberships.size() != targets.size()) {
        std::cerr << "AC1024 DICTIONARY source callbacks were not complete\n";
        return false;
    }
    std::size_t totalAllocationOffsets = 0u;
    for (const auto handle : targets) {
        const auto &typed = source.typed.at(handle);
        const auto &raw = source.raw.at(handle);
        const auto &publication = source.publications.at(handle);
        const auto &membership = source.memberships.at(handle);
        const auto valid = [&]() {
            return typed.matchesAc1024DwgFrame(raw, publication, membership);
        };
        if (!check(valid(), "matching source was refused", handle)) return false;

        armAllocationFailure(1000000);
        const bool calibration = valid();
        const std::size_t offsets = allocationAttempts;
        disarmAllocationFailure();
        if (!check(calibration && offsets > 0u, "allocation calibration failed", handle))
            return false;
        totalAllocationOffsets += offsets;
        for (std::size_t offset = 0; offset < offsets; ++offset) {
            armAllocationFailure(static_cast<long>(offset));
            const bool accepted = valid();
            const bool triggered = allocationTriggered;
            disarmAllocationFailure();
            if (!check(triggered && !accepted, "allocation failure was accepted", handle)
                || !check(valid(), "retry after allocation failure failed", handle))
                return false;
        }

        auto changedTyped = typed;
        changedTyped.cloning ^= 1;
        if (!check(!changedTyped.matchesAc1024DwgFrame(raw, publication, membership),
                   "changed cloning accepted", handle)) return false;
        changedTyped = typed;
        if (changedTyped.m_entries.empty()) return false;
        changedTyped.m_entries.front().m_name += "_changed";
        if (!check(!changedTyped.matchesAc1024DwgFrame(raw, publication, membership),
                   "changed entry accepted", handle)) return false;
        changedTyped = typed;
        changedTyped.parentHandle ^= 1;
        if (!check(!changedTyped.matchesAc1024DwgFrame(raw, publication, membership),
                   "changed owner accepted", handle)) return false;

        auto changedRaw = raw;
        changedRaw.m_rawBytes.front() ^= 0x80u;
        if (!check(!typed.matchesAc1024DwgFrame(changedRaw, publication, membership),
                   "changed object type accepted", handle)) return false;
        changedRaw = raw;
        changedRaw.m_objectSize -= 1u;
        if (!check(!typed.matchesAc1024DwgFrame(changedRaw, publication, membership),
                   "truncated object declaration accepted", handle)) return false;
        changedRaw = raw;
        changedRaw.m_rawBytes.pop_back();
        --changedRaw.m_objectSize;
        if (!check(!typed.matchesAc1024DwgFrame(changedRaw, publication, membership),
                   "truncated frame accepted", handle)) return false;
        changedRaw = raw;
        changedRaw.m_version = DRW::AC1027;
        if (!check(!typed.matchesAc1024DwgFrame(changedRaw, publication, membership),
                   "wrong source version accepted", handle)) return false;
        changedRaw = raw;
        changedRaw.m_hasDataStorage = true;
        if (!check(!typed.matchesAc1024DwgFrame(changedRaw, publication, membership),
                   "unmodeled data storage accepted", handle)) return false;
        changedRaw = raw;
        changedRaw.m_xDictFlag = 0u;
        if (!check(!typed.matchesAc1024DwgFrame(changedRaw, publication, membership),
                   "extension dictionary profile accepted", handle)) return false;
        changedRaw = raw;
        changedRaw.m_commonLinkEvidence = DRW_DwgCommonLinkEvidence::Unknown;
        if (!check(!typed.matchesAc1024DwgFrame(changedRaw, publication, membership),
                   "unvalidated common links accepted", handle)) return false;
        if (handle == targets.front()) {
            changedRaw = raw;
            changedRaw.m_rawBytes.resize(4u * 1024u * 1024u + 1u);
            changedRaw.m_objectSize = static_cast<std::uint32_t>(changedRaw.m_rawBytes.size());
            if (!check(!typed.matchesAc1024DwgFrame(changedRaw, publication, membership),
                       "oversized frame accepted", handle)) return false;
        }

        auto changedPublication = publication;
        ++changedPublication.m_sourceOffset;
        if (!check(!typed.matchesAc1024DwgFrame(raw, changedPublication, membership),
                   "changed publication location accepted", handle)) return false;
        changedPublication = publication;
        changedPublication.m_carrier = DRW_DwgFramePublication::Carrier::Typed;
        if (!check(!typed.matchesAc1024DwgFrame(raw, changedPublication, membership),
                   "missing raw publication accepted", handle)) return false;

        auto changedMembership = membership;
        changedMembership.m_complete = false;
        if (!check(!typed.matchesAc1024DwgFrame(raw, publication, changedMembership),
                   "incomplete membership accepted", handle)) return false;
        changedMembership = membership;
        changedMembership.m_entries.front().m_handle ^= 1u;
        if (!check(!typed.matchesAc1024DwgFrame(raw, publication, changedMembership),
                   "changed membership accepted", handle)) return false;
        changedMembership = membership;
        ++changedMembership.m_sourceMapOrdinal;
        if (!check(!typed.matchesAc1024DwgFrame(raw, publication, changedMembership),
                   "different source ordinal accepted", handle)) return false;
        if (!check(valid(), "source was changed by failed checks", handle)) return false;
    }
    std::cout << "AC1024 DICTIONARY source verification: PASS (3 physical records, "
              << totalAllocationOffsets << " allocation failure/retry controls)\n";
    return true;
}
} // namespace

int main(int argc, char **argv) {
    return argc == 2 && run(argv[1]) ? 0 : 1;
}
