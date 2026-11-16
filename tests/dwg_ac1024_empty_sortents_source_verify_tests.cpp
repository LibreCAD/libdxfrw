// SPDX-License-Identifier: GPL-2.0-or-later
// Correlation only: these controls do not admit output or prove graph closure.
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
long beforeFailure = -1;
std::size_t attempts = 0u;
bool triggered = false;
void arm(long count) noexcept { beforeFailure = count; attempts = 0u; triggered = false; }
void disarm() noexcept { beforeFailure = -1; }
}
void *operator new(std::size_t size) {
    if (beforeFailure >= 0) {
        ++attempts;
        if (beforeFailure-- == 0) { beforeFailure = -1; triggered = true; throw std::bad_alloc(); }
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
constexpr std::array<std::uint32_t, 4> targets{{0xF3u, 0x2121u, 0x21D3u, 0x22D4u}};
bool selected(std::uint32_t handle) {
    for (const auto target : targets) if (target == handle) return true;
    return false;
}
struct Capture final : LC3CompatCheck {
    std::map<std::uint32_t, DRW_SortEntsTable> typed;
    std::map<std::uint32_t, DRW_UnsupportedObject> raw;
    std::map<std::uint32_t, DRW_DwgFramePublication> publications;
    std::map<std::uint32_t, DRW_DwgSortEntsMembership> memberships;
    bool duplicate = false;
    void addSortEntsTable(const DRW_SortEntsTable &value) override {
        if (selected(value.handle) && !typed.emplace(value.handle, value).second) duplicate = true;
    }
    void addUnsupportedObject(const DRW_UnsupportedObject &value) override {
        if (selected(value.m_handle) && !raw.emplace(value.m_handle, value).second) duplicate = true;
    }
    void addDwgFramePublication(const DRW_DwgFramePublication &value) override {
        if (selected(value.m_handle) && !publications.emplace(value.m_handle, value).second) duplicate = true;
    }
    void addDwgSortEntsMembership(const DRW_DwgSortEntsMembership &value) override {
        if (selected(value.m_tableHandle) && !memberships.emplace(value.m_tableHandle, value).second) duplicate = true;
    }
};
bool check(bool condition, const char *message, std::uint32_t handle) {
    if (!condition) std::cerr << "AC1024 empty SORTENTSTABLE: " << message
                              << " handle=" << std::hex << handle << std::dec << '\n';
    return condition;
}
bool run(const std::string &path) {
    Capture c;
    dwgR reader(path.c_str());
    if (!reader.read(&c, false) || reader.getVersion() != DRW::AC1024
        || c.duplicate || c.typed.size() != targets.size() || c.raw.size() != targets.size()
        || c.publications.size() != targets.size() || c.memberships.size() != targets.size())
        return check(false, "source callbacks incomplete", 0u);
    std::size_t totalOffsets = 0u;
    // Owner spans are diagnostic positions in the decompressed source body.
    constexpr std::array<std::size_t, 4> ownerEndBits{{82u, 106u, 114u, 106u}};
    std::size_t targetIndex = 0u;
    for (auto handle : targets) {
        const auto &t = c.typed.at(handle);
        const auto &r = c.raw.at(handle);
        const auto &p = c.publications.at(handle);
        const auto &m = c.memberships.at(handle);
        const auto valid = [&] { return t.matchesAc1024EmptyDwgFrame(r, p, m); };
        if (!check(valid(), "matching frame refused", handle)) return false;
        arm(1000000); const bool calibration = valid(); const auto count = attempts; disarm();
        if (!check(calibration && count > 0u, "allocation calibration failed", handle)) return false;
        totalOffsets += count;
        for (std::size_t i = 0u; i < count; ++i) {
            arm(static_cast<long>(i)); const bool ok = valid(); const bool hit = triggered; disarm();
            if (!check(hit && !ok, "allocation failure accepted", handle)
                || !check(valid(), "retry failed", handle)) return false;
        }
        auto altered = t; altered.m_blockOwnerHandle ^= 1u;
        if (!check(!altered.matchesAc1024EmptyDwgFrame(r, p, m), "altered block owner accepted", handle)) return false;
        altered = t; altered.reactorHandles[0] ^= 1u;
        if (!check(!altered.matchesAc1024EmptyDwgFrame(r, p, m), "altered reactor accepted", handle)) return false;
        altered = t; altered.m_entityHandles.push_back(0x10u);
        if (!check(!altered.matchesAc1024EmptyDwgFrame(r, p, m), "nonempty typed entries accepted", handle)) return false;
        auto raw = r;
        const std::size_t ownerBit = ownerEndBits[targetIndex++] - 1u;
        if (!check(ownerBit / 8u < raw.m_rawBytes.size(), "owner span outside frame", handle)) return false;
        raw.m_rawBytes[ownerBit / 8u] ^= static_cast<std::uint8_t>(0x80u >> (ownerBit % 8u));
        if (!check(!t.matchesAc1024EmptyDwgFrame(raw, p, m), "altered wire owner accepted", handle)) return false;
        raw = r; raw.m_rawBytes.front() ^= 0x80u;
        if (!check(!t.matchesAc1024EmptyDwgFrame(raw, p, m), "altered encoded type accepted", handle)) return false;
        raw = r; raw.m_rawBytes.pop_back(); --raw.m_objectSize;
        if (!check(!t.matchesAc1024EmptyDwgFrame(raw, p, m), "truncated frame accepted", handle)) return false;
        raw = r; raw.m_reactorHandles[0] ^= 1u;
        if (!check(!t.matchesAc1024EmptyDwgFrame(raw, p, m), "wrong reactor accepted", handle)) return false;
        raw = r; raw.m_version = DRW::AC1027;
        if (!check(!t.matchesAc1024EmptyDwgFrame(raw, p, m), "wrong version accepted", handle)) return false;
        raw = r; raw.m_className += "Wrong";
        if (!check(!t.matchesAc1024EmptyDwgFrame(raw, p, m), "wrong class accepted", handle)) return false;
        auto publication = p; ++publication.m_sourceMapOrdinal;
        if (!check(!t.matchesAc1024EmptyDwgFrame(r, publication, m), "wrong source ordinal accepted", handle)) return false;
        publication = p; publication.m_carrier = DRW_DwgFramePublication::Carrier::Typed;
        if (!check(!t.matchesAc1024EmptyDwgFrame(r, publication, m), "missing raw carrier accepted", handle)) return false;
        auto membership = m; ++membership.m_blockOwnerHandle;
        if (!check(!t.matchesAc1024EmptyDwgFrame(r, p, membership), "wrong membership owner accepted", handle)) return false;
        membership = m; membership.m_entries.push_back({1u, 2u, 0u, false});
        if (!check(!t.matchesAc1024EmptyDwgFrame(r, p, membership), "nonempty membership accepted", handle)) return false;
        membership = m; membership.m_classStreamOrdinal.reset();
        if (!check(!t.matchesAc1024EmptyDwgFrame(r, p, membership), "missing class ordinal accepted", handle)) return false;
        if (!check(valid(), "failed controls changed the source", handle)) return false;
    }
    std::cout << "AC1024 empty SORTENTSTABLE source verification: PASS (4 physical records, "
              << totalOffsets << " allocation failure/retry controls)\n";
    return true;
}
}
int main(int argc, char **argv) { return argc == 2 && run(argv[1]) ? 0 : 1; }
