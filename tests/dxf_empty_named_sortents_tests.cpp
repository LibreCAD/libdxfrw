// SPDX-License-Identifier: GPL-2.0-or-later
// Locally authored empty named-block SORTENTSTABLE graph and failure controls.
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>
#include <vector>
namespace {
long failAfter = -1;
std::size_t allocationAttempts = 0u;
bool allocationTriggered = false;
void arm(long n) noexcept { failAfter = n; allocationAttempts = 0u; allocationTriggered = false; }
void disarm() noexcept { failAfter = -1; }
}
void* operator new(std::size_t size) {
    if (failAfter >= 0) { ++allocationAttempts; if (failAfter-- == 0) {
        failAfter = -1; allocationTriggered = true; throw std::bad_alloc();
    }}
    if (void* p = std::malloc(size == 0u ? 1u : size)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
namespace {
constexpr std::uint32_t blockHandle = 0xA003u, dictHandle = 0xB003u;
constexpr std::uint32_t sortHandle = 0xB100u, extraHandle = 0xB200u;
struct Author final : LC3CompatCheck, DRW_DxfEmptyNamedSortEntsProvider {
    explicit Author(dxfRW& codec) : w(codec) {}
    dxfRW& w;
    bool ok = true, skipBody = false, nonempty = false, hit = false, enabled = true;
    bool allowDxfEmptyNamedSortEnts() const noexcept override { return enabled; }
    long failure = -1;
    std::size_t allocations = 0u;
    void writeBlockRecords() override { ok = w.writeBlockRecord("Named") && ok; }
    void writeBlocks() override {
        if (skipBody) return;
        DRW_Block block; block.name = "Named"; block.handle = 0xA020u;
        block.parentHandle = blockHandle;
        ok = w.writeBlock(&block) && ok;
        ok = w.writeBlockEnd() && ok;
    }
    void writeObjects() override {
        DRW_XRecord extra; extra.handle = extraHandle; extra.parentHandle = dictHandle;
        ok = w.writeXRecord(&extra) && ok;
        DRW_SortEntsTable table; table.handle = sortHandle;
        table.parentHandle = dictHandle; table.m_blockOwnerHandle = blockHandle;
        if (nonempty) { table.m_entityHandles.push_back(0xA100u); table.m_sortHandles.push_back(1u); }
        if (failure >= 0) arm(failure);
        bool written = false;
        try { written = w.writeSortEntsTable(&table); } catch (...) { written = false; }
        if (failure >= 0) { hit = allocationTriggered; allocations = allocationAttempts; disarm(); }
        ok = written && ok;
    }
    bool finalizeDxfWrite() override { return ok; }
};
// An existing consumer without the separate provider keeps the strict policy.
struct LegacyAuthor final : LC3CompatCheck {
    explicit LegacyAuthor(dxfRW& codec) : delegate(codec) {}
    Author delegate;
    void writeBlockRecords() override { delegate.writeBlockRecords(); }
    void writeBlocks() override { delegate.writeBlocks(); }
    void writeObjects() override { delegate.writeObjects(); }
    bool finalizeDxfWrite() override { return delegate.ok; }
};
struct Reader final : LC3CompatCheck {
    std::vector<DRW_Block_Record> blocks;
    std::vector<DRW_Dictionary> dictionaries;
    std::vector<DRW_SortEntsTable> tables;
    std::vector<DRW_XRecord> extras;
    void addBlockRecord(const DRW_Block_Record& v) override { blocks.push_back(v); }
    void addDictionary(const DRW_Dictionary& v) override { dictionaries.push_back(v); }
    void addSortEntsTable(const DRW_SortEntsTable& v) override { tables.push_back(v); }
    void addXRecord(const DRW_XRecord& v) override { extras.push_back(v); }
};
DRW_Block_Record makeBlock() {
    DRW_Block_Record value; value.name = "Named"; value.handle = blockHandle;
    value.xDictHandle = dictHandle; return value;
}
DRW_Dictionary makeDictionary() {
    DRW_Dictionary value; value.handle = dictHandle; value.parentHandle = blockHandle;
    DRW_Dictionary::Entry a; a.m_name = "ACAD_SORTENTS"; a.m_handle = sortHandle;
    DRW_Dictionary::Entry b; b.m_name = "OTHER"; b.m_handle = extraHandle;
    value.m_entries = {a, b}; return value;
}
bool check(bool condition, const char* message) {
    if (!condition) std::cerr << "named empty SORTENTSTABLE: " << message << '\n';
    return condition;
}
std::string bytes(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
bool seed(const std::filesystem::path& path) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc); stream << "unchanged destination\n";
    return static_cast<bool>(stream);
}
bool readback(const std::filesystem::path& path) {
    dxfRW codec(path.string().c_str()); Reader reader;
    if (!codec.read(&reader, false) || reader.tables.size() != 1u || reader.extras.size() != 1u)
        return false;
    const DRW_Block_Record* block = nullptr;
    for (const auto& value : reader.blocks) if (value.name == "Named") block = &value;
    if (!block || block->xDictHandle == 0u) return false;
    const DRW_Dictionary* dict = nullptr;
    for (const auto& value : reader.dictionaries) if (value.handle == block->xDictHandle) dict = &value;
    if (!dict || dict->parentHandle != block->handle || dict->m_entries.size() != 2u)
        return false;
    const auto& table = reader.tables.front(); const auto& extra = reader.extras.front();
    if (table.parentHandle != dict->handle || table.m_blockOwnerHandle != block->handle
        || !table.m_entityHandles.empty() || !table.m_sortHandles.empty()
        || extra.parentHandle != dict->handle) return false;
    bool sawSort = false, sawExtra = false;
    for (const auto& entry : dict->m_entries) {
        if (entry.m_name == "ACAD_SORTENTS" && entry.m_handle == table.handle) sawSort = true;
        if (entry.m_name == "OTHER" && entry.m_handle == extra.handle) sawExtra = true;
    }
    return sawSort && sawExtra;
}
bool run(const std::filesystem::path& path, bool binary) {
    constexpr long calibrate = 1000000;
    dxfRW baseline(path.string().c_str()); baseline.setSourceBlockRecords({makeBlock()}, true);
    baseline.setNamedDictObjects({makeDictionary()}); Author author(baseline); author.failure = calibrate;
    if (!check(baseline.write(&author, DRW::AC1024, binary) && author.ok && !author.hit
               && author.allocations > 0u && readback(path), "baseline or semantic readback failed")) return false;
    const auto expected = bytes(path); const auto count = author.allocations;
    for (std::size_t i = 0u; i < count; ++i) {
        if (!seed(path)) return false;
        dxfRW codec(path.string().c_str()); codec.setSourceBlockRecords({makeBlock()}, true);
        codec.setNamedDictObjects({makeDictionary()}); Author candidate(codec); candidate.failure = static_cast<long>(i);
        const bool first = codec.write(&candidate, DRW::AC1024, binary);
        if (!check(!first && candidate.hit && bytes(path) == "unchanged destination\n", "allocation failure changed destination")) return false;
        candidate.failure = -1; candidate.ok = true;
        if (!check(codec.write(&candidate, DRW::AC1024, binary) && candidate.ok
                   && bytes(path) == expected && readback(path), "same-writer allocation retry failed")) return false;
    }
    for (unsigned mode = 0; mode < 10u; ++mode) {
        if (!seed(path)) return false;
        dxfRW codec(path.string().c_str());
        auto block = makeBlock(); auto dict = makeDictionary();
        if (mode == 0u) block.xDictHandle = 0u;
        if (mode == 1u) dict.m_entries[0].m_handle = 0xB300u;
        if (mode == 2u) dict.parentHandle = 0xA004u;
        if (mode == 3u) dict.m_entries.erase(dict.m_entries.begin());
        if (mode == 8u) dict.m_entries.push_back(dict.m_entries.front());
        if (mode == 9u) codec.setSourceBlockRecords({block, block}, true);
        else codec.setSourceBlockRecords({block}, mode != 7u);
        codec.setNamedDictObjects({dict});
        Author candidate(codec); candidate.skipBody = mode == 4u; candidate.nonempty = mode == 5u; candidate.enabled = mode != 6u;
        if (!check(!codec.write(&candidate, DRW::AC1024, binary)
                   && bytes(path) == "unchanged destination\n", "invalid graph was accepted")) return false;
        codec.setSourceBlockRecords({makeBlock()}, true); codec.setNamedDictObjects({makeDictionary()});
        candidate.ok = true; candidate.skipBody = false; candidate.nonempty = false; candidate.enabled = true;
        if (!check(codec.write(&candidate, DRW::AC1024, binary) && candidate.ok
                   && readback(path), "invalid graph retry failed")) return false;
    }
    if (!seed(path)) return false;
    dxfRW legacyCodec(path.string().c_str());
    legacyCodec.setSourceBlockRecords({makeBlock()}, true);
    legacyCodec.setNamedDictObjects({makeDictionary()});
    LegacyAuthor legacy(legacyCodec);
    if (!check(!legacyCodec.write(&legacy, DRW::AC1024, binary)
               && bytes(path) == "unchanged destination\n", "legacy consumer bypassed default refusal")) return false;
    Author optIn(legacyCodec);
    if (!check(legacyCodec.write(&optIn, DRW::AC1024, binary) && optIn.ok
               && readback(path), "legacy-to-opt-in retry failed")) return false;
    std::cout << (binary ? "binary" : "ASCII") << ": " << count
              << " allocation failures and eleven invalid or default-policy retries passed\n";
    return true;
}
}
int main(int argc, char **argv) {
    if (argc > 2 || (argc == 2 && std::string(argv[1]) != "--keep")) return 2;
    const bool keep = argc == 2;
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto dir = std::filesystem::temp_directory_path()
        / ("libdxfrw-empty-named-sortents-generated-" + std::to_string(nonce));
    std::error_code error; std::filesystem::create_directories(dir, error);
    if (error) return 1;
    struct Cleanup {
        std::filesystem::path path; bool keep;
        ~Cleanup() { if (!keep) { std::error_code e; std::filesystem::remove_all(path, e); } }
    } cleanup{dir, keep};
    if (!run(dir / "ascii.dxf", false) || !run(dir / "binary.dxf", true)) return 1;
    if (keep) std::cout << "generated files: " << dir.string() << '\n';
    return 0;
}
