// SPDX-License-Identifier: GPL-2.0-or-later
// Isolated fail-once allocation sweep for the generated TABLE extension facade.
#define LIBDXFRW_TABLE_FACADE_ALLOC_SWEEP 1
#define main libdxfrw_unused_roundtrip_main
#include "dwg_local_roundtrip_tests.cpp"
#undef main

#include <cstdlib>
#include <new>

namespace {
bool allocationProbeEnabled = false;
bool allocationTracking = false;
long allocationFailureOffset = -1;
std::size_t allocationAttempts = 0u;
bool allocationFailed = false;
int allocationProbeStage = 0;
bool allocationTrackingStack[8]{};
std::size_t allocationTrackingDepth = 0u;
void tableFacadeAllocationBegin(int stage) {
    if (allocationTrackingDepth < sizeof(allocationTrackingStack)
                                         / sizeof(allocationTrackingStack[0])) {
        allocationTrackingStack[allocationTrackingDepth++] = allocationTracking;
    }
    if (allocationProbeEnabled && allocationProbeStage == stage)
        allocationTracking = true;
}
void tableFacadeAllocationEnd() {
    allocationTracking = allocationTrackingDepth == 0u
        ? false : allocationTrackingStack[--allocationTrackingDepth];
}
}

void* operator new(std::size_t size) {
    if (allocationTracking) {
        ++allocationAttempts;
        if (allocationFailureOffset == 0) {
            allocationFailureOffset = -1; // rollback and retry can allocate
            allocationFailed = true;
            throw std::bad_alloc();
        }
        if (allocationFailureOffset > 0)
            --allocationFailureOffset;
    }
    if (void* p = std::malloc(size == 0u ? 1u : size))
        return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

int main() {
    const auto path = std::filesystem::temp_directory_path()
        / "libdxfrw-table-extension-allocation-sweep.dwg";
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code ignored; std::filesystem::remove(path, ignored); }
    } cleanup{path};
    const std::string sentinel = "TABLE extension allocation rollback\n";
    auto seed = [&]() {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out << sentinel;
        return static_cast<bool>(out);
    };
    auto unchanged = [&]() {
        std::ifstream in(path, std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>()) == sentinel;
    };
    dx_data calibrationData;
    dwgRW calibration(path.string().c_str());
    TableStyleNodWholeFileAdapter baseline(&calibration, &calibrationData,
                                            true, false, false);
    if (!calibration.reserveHandle(baseline.kExtensionHandle)) return 1;
    baseline.setExtensionMode(6);
    allocationProbeEnabled = true;
    allocationAttempts = 0u;
    const bool calibrationOk = calibration.write(&baseline, DRW::AC1018, true);
    allocationProbeEnabled = false;
    const std::size_t count = allocationAttempts;
    if (!calibrationOk || !baseline.ready || count == 0u) {
        std::cerr << "TABLE extension allocation calibration failed\n";
        return 1;
    }
    for (std::size_t offset = 0u; offset < count; ++offset) {
        dx_data data;
        dwgRW writer(path.string().c_str());
        TableStyleNodWholeFileAdapter producer(&writer, &data, true, false, false);
        if (!writer.reserveHandle(producer.kExtensionHandle) || !seed()) return 1;
        producer.setExtensionMode(6);
        allocationAttempts = 0u;
        allocationFailed = false;
        allocationFailureOffset = static_cast<long>(offset);
        allocationProbeEnabled = true;
        const bool wrote = writer.write(&producer, DRW::AC1018, true);
        allocationProbeEnabled = false;
        allocationTracking = false;
        if (!allocationFailed || wrote || !unchanged()
            || producer.table.handle != 0xA720u
            || producer.table.blockRecH.ref != 0u
            || producer.table.getDwgClassNum() != 0u) {
            std::cerr << "TABLE extension allocation rollback failed offset="
                      << offset << " fired=" << allocationFailed
                      << " wrote=" << wrote << '\n';
            return 1;
        }
        allocationFailureOffset = -1;
        if (!writer.write(&producer, DRW::AC1018, true) || !producer.ready) {
            std::cerr << "TABLE extension allocation retry failed offset="
                      << offset << '\n';
            return 1;
        }
        dx_data readback;
        dx_iface reader;
        reader.cData = &readback;
        dwgRW input(path.string().c_str());
        if (!input.read(&reader, false) || readback.tables.size() != 1u
            || readback.tables.front().xDictHandle != producer.kExtensionHandle) {
            std::cerr << "TABLE extension allocation readback failed offset="
                      << offset << '\n';
            return 1;
        }
        const auto priorBlock = std::find_if(readback.blocks.cbegin(),
            readback.blocks.cend(), [](const dx_ifaceBlock* block) {
                return block != nullptr && block->name == "*T1";
            });
        if (priorBlock == readback.blocks.cend()
            || (*priorBlock)->ent.size() != 15u) {
            std::cerr << "TABLE extension prior block readback failed offset="
                      << offset << '\n';
            return 1;
        }
    }

    // The direct TABLE facade sweep above isolates writeTable(). This second
    // sweep injects into the complete DWG operation so allocations in class,
    // table, *T block, dictionary, section assembly, and final publication
    // participate in the same rollback/retry contract.
    const auto configureWholeTable = [](TableStyleNodWholeFileAdapter& adapter) {
        adapter.setOmitTableStyleRoot(false);
        adapter.useParsedBlockRecordMetadata(true);
        adapter.useParsedProxyCache(true);
        adapter.setExtensionMode(0);
    };
    const auto fileBytes = [&path]() {
        std::ifstream in(path, std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());
    };
    const auto prepareWholeWriter = [&path, &configureWholeTable](
            dx_data& data, dwgRW& writer,
            TableStyleNodWholeFileAdapter& adapter) {
        adapter.cData = &data;
        adapter.currentBlock = data.mBlock;
        configureWholeTable(adapter);
        return writer.getError() == DRW::BAD_NONE;
    };
    dx_data wholeCalibrationData;
    dwgRW wholeCalibration(path.string().c_str());
    TableStyleNodWholeFileAdapter wholeCalibrationAdapter(
        &wholeCalibration, &wholeCalibrationData, true, false, false);
    if (!prepareWholeWriter(wholeCalibrationData, wholeCalibration,
                            wholeCalibrationAdapter))
        return 1;
    allocationProbeStage = 2;
    allocationProbeEnabled = true;
    allocationAttempts = 0u;
    allocationFailed = false;
    allocationFailureOffset = -1;
    tableFacadeAllocationBegin(2);
    const bool wholeCalibrationWrote =
        wholeCalibration.write(&wholeCalibrationAdapter, DRW::AC1018, true);
    tableFacadeAllocationEnd();
    allocationProbeEnabled = false;
    const std::size_t wholeAllocationCount = allocationAttempts;
    const std::string cleanWholeFile = fileBytes();
    if (!wholeCalibrationWrote || !wholeCalibrationAdapter.ready
        || wholeAllocationCount == 0u || cleanWholeFile.empty()) {
        std::cerr << "TABLE whole-file allocation calibration failed"
                  << " wrote=" << wholeCalibrationWrote
                  << " ready=" << wholeCalibrationAdapter.ready
                  << " allocations=" << wholeAllocationCount << '\n';
        return 1;
    }
    std::vector<std::size_t> wholeFailureOffsets{
        0u, 1u, wholeAllocationCount / 4u, wholeAllocationCount / 2u,
        (wholeAllocationCount * 3u) / 4u, wholeAllocationCount - 1u};
    std::sort(wholeFailureOffsets.begin(), wholeFailureOffsets.end());
    wholeFailureOffsets.erase(
        std::unique(wholeFailureOffsets.begin(), wholeFailureOffsets.end()),
        wholeFailureOffsets.end());
    std::size_t wholeFailures = 0u;
    for (const std::size_t offset : wholeFailureOffsets) {
        dx_data data;
        dwgRW writer(path.string().c_str());
        TableStyleNodWholeFileAdapter producer(&writer, &data, true, false,
                                               false);
        if (!prepareWholeWriter(data, writer, producer)) return 1;
        if (!seed()) return 1;
        allocationProbeStage = 2;
        allocationAttempts = 0u;
        allocationFailed = false;
        allocationFailureOffset = static_cast<long>(offset);
        allocationProbeEnabled = true;
        tableFacadeAllocationBegin(2);
        const bool wrote = writer.write(&producer, DRW::AC1018, true);
        tableFacadeAllocationEnd();
        allocationProbeEnabled = false;
        allocationTracking = false;
        const bool sourceRolledBack = producer.table.handle == 0xA720u
            && producer.table.parentHandle == DRW::NoHandle
            && producer.table.getDwgClassNum() == 0u
            && producer.table.blockRecH.ref == 0u
            && ((producer.table.proxyGraphics.empty()
                    && producer.table.numProxyGraph == 0)
                || (producer.table.proxyGraphics == generatedTableProxyCache()
                    && producer.table.numProxyGraph
                        == static_cast<int>(generatedTableProxyCache().size())))
            && producer.table.m_semanticContentComplete;
        if (!allocationFailed || wrote || !unchanged() || !sourceRolledBack) {
            std::cerr << "TABLE whole-file allocation rollback failed offset="
                      << offset << " fired=" << allocationFailed
                      << " wrote=" << wrote
                      << " state=" << sourceRolledBack
                      << " ready=" << producer.ready
                      << " handle=0x" << std::hex << producer.table.handle
                      << " class=" << producer.table.getDwgClassNum()
                      << " parent=0x" << producer.table.parentHandle
                      << " block=0x" << producer.table.blockRecH.ref
                      << std::dec
                      << " proxy=" << producer.table.proxyGraphics.size()
                      << '/' << producer.table.numProxyGraph
                      << " complete="
                      << producer.table.m_semanticContentComplete << '\n';
            return 1;
        }
        ++wholeFailures;
        allocationFailureOffset = -1;
        if (!writer.write(&producer, DRW::AC1018, true)
            || !producer.ready || fileBytes() != cleanWholeFile) {
            std::cerr << "TABLE whole-file same-writer retry failed offset="
                      << offset << '\n';
            return 1;
        }
        dx_data readback;
        dx_iface reader;
        reader.cData = &readback;
        reader.currentBlock = readback.mBlock;
        dwgRW input(path.string().c_str());
        const bool decoded = input.read(&reader, false);
        const DRW_Table* table = readback.tables.size() == 1u
            ? &readback.tables.front() : nullptr;
        const auto tableBlock = std::find_if(
            readback.blocks.begin(), readback.blocks.end(),
            [](const dx_ifaceBlock* block) {
                return block != nullptr && block->name == "*T1";
            });
        const bool graphReadBack = decoded && table != nullptr
            && table->name == "*T1" && table->m_content.m_rows.size() == 3u
            && table->m_content.m_columns.size() == 3u
            && table->m_content.m_rows[2].m_cells[2].m_contents.size() == 1u
            && table->m_content.m_rows[2].m_cells[2].m_contents.front().m_text
                == "6"
            && tableBlock != readback.blocks.end()
            && (*tableBlock)->ent.size() == 15u
            && readback.hasNamedObjectDictionary
            && std::any_of(readback.tableStyles.begin(),
                           readback.tableStyles.end(),
                [](const DRW_TableStyle& style) {
                    return style.handle
                            == TableStyleNodWholeFileAdapter::kStyleHandle
                        && style.m_name == "Standard";
                });
        if (!graphReadBack) {
            std::cerr << "TABLE whole-file retry typed readback failed offset="
                      << offset << " decoded=" << decoded
                      << " tables=" << readback.tables.size()
                      << " blocks=" << readback.blocks.size()
                      << " styles=" << readback.tableStyles.size() << '\n';
            return 1;
        }
    }
    if (wholeFailures != wholeFailureOffsets.size()) return 1;
    std::cout << "TABLE whole-file allocation failures=" << wholeFailures
              << '/' << wholeAllocationCount
              << " sampled offsets rollback/retry: PASS\n";

    for (const int mode : {20, 21, 22, 23}) {
        dx_data data;
        dwgRW writer(path.string().c_str());
        TableStyleNodWholeFileAdapter producer(&writer, &data, true, false, false);
        if (!writer.reserveHandle(producer.kExtensionHandle) || !seed()) return 1;
        producer.setExtensionMode(mode);
        allocationProbeStage = 1;
        allocationAttempts = 0u;
        allocationFailed = false;
        allocationFailureOffset = 0;
        allocationProbeEnabled = true;
        const bool wrote = writer.write(&producer, DRW::AC1018, true);
        allocationProbeEnabled = false;
        allocationTracking = false;
        if (!allocationFailed || !producer.caughtException()
            || ((mode == 20 || mode == 22) && (wrote || !unchanged()))
            || ((mode == 21 || mode == 23) && (!wrote || !producer.ready))) {
            std::cerr << "caught duplicate allocation failure mismatch mode="
                      << mode << " fired=" << allocationFailed
                      << " caught=" << producer.caughtException()
                      << " wrote=" << wrote << '\n';
            return 1;
        }
        if (mode == 20 || mode == 22) {
            allocationFailureOffset = -1;
            producer.setExtensionMode(6);
            if (!writer.write(&producer, DRW::AC1018, true) || !producer.ready) {
                std::cerr << "caught duplicate retry failed\n";
                return 1;
            }
        }
    }
    std::cout << "Caught fixed/generic allocation exceptions: "
                 "refusal, explicit rollback and retry PASS\n";
    std::cout << "TABLE extension facade allocation failures=" << count
              << " rollback/retry/readback: PASS\n";
    return 0;
}
