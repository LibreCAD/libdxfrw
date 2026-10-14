#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "drw_base.h"
#include "intern/drw_dbg.h"
#include "intern/dwgbuffer.h"
#include "libdwgr.h"
#include "libdxfrw.h"
#include "dwg2dxf/dx_iface.h"

namespace {
struct Checks {
    int failures{0};
    void expect(bool value, const char* label) {
        if (!value) {
            ++failures;
            std::cerr << "FAIL: " << label << '\n';
        }
    }
};

class SilentPrinter final : public DRW::DebugPrinter {};

void sequentialAndEager(Checks& checks) {
    dxfRW first("");
    first.setDebug(DRW::DebugLevel::Debug);
    checks.expect(DRW_dbg::getInstance()->isDebugEnabled(), "global debug enabled");
    dxfRW second("");
    checks.expect(!DRW_dbg::getInstance()->isDebugEnabled(), "DXF constructor retains global reset");
    first.setDebug(DRW::DebugLevel::Debug);
    dwgRW third("");
    checks.expect(!DRW_dbg::getInstance()->isDebugEnabled(), "DWG constructor retains global reset");
    DRW::setCustomDebugPrinter(new SilentPrinter);
    for (auto level : {DRW_dbg::Level::None, DRW_dbg::Level::Debug}) {
        DRW_dbg::getInstance()->setLevel(level);
        int sideEffects = 0;
        const auto next = [&sideEffects] { return ++sideEffects; };
        DRW_DBG(next());
        DRW_DBGH(next());
        DRW_DBGB(next());
        DRW_DBGHL(next(), next(), next());
        DRW_DBGPT(next(), next(), next());
        checks.expect(sideEffects == 9, "all debug arguments remain eager");
        std::uint8_t bytes[]{0x34, 0x12, 0x78, 0x56};
        dwgBuffer buffer(bytes, sizeof(bytes));
        DRW_DBG(buffer.getRawShort16());
        checks.expect(buffer.getPosition() == 2 && buffer.getBitPos() == 0
                          && buffer.getRawShort16() == 0x5678,
                      "debug argument retains exact DWG cursor advancement");
    }
    DRW_dbg::getInstance()->setLevel(DRW_dbg::Level::None);
    DRW::setCustomDebugPrinter(nullptr);
}

struct BlockingState {
    std::mutex mutex;
    std::condition_variable changed;
    unsigned entered{0};
    bool release{false};
    std::atomic<unsigned> destroyed{0};
    std::atomic<unsigned> callbackReentries{0};
};

class BlockingPrinter final : public DRW::DebugPrinter {
public:
    explicit BlockingPrinter(BlockingState& state) : state_(state) {}
    ~BlockingPrinter() override {
        // Destruction may call back into the debug service. No service lock
        // may be held while the owned printer's destructor runs.
        (void)DRW_dbg::getInstance()->getLevel();
        ++state_.destroyed;
    }
    void printS(const std::string&) override {
        BlockingState& state = state_;
        (void)DRW_dbg::getInstance()->getLevel();
        ++state.callbackReentries;
        std::unique_lock<std::mutex> lock(state.mutex);
        ++state.entered;
        state.changed.notify_all();
        state.changed.wait(lock, [&state] { return state.release; });
    }
private:
    BlockingState& state_;
};

void replacementAndOverlap(Checks& checks) {
    BlockingState state;
    DRW::setCustomDebugPrinter(new BlockingPrinter(state));
    DRW_dbg::getInstance()->setLevel(DRW_dbg::Level::Debug);
    std::thread first([] { DRW_DBG("one"); });
    std::thread second([] { DRW_DBG("two"); });
    bool overlapped = false;
    {
        std::unique_lock<std::mutex> lock(state.mutex);
        overlapped = state.changed.wait_for(lock, std::chrono::seconds(2),
                                           [&state] { return state.entered == 2; });
    }
    checks.expect(overlapped, "custom printer callbacks may overlap and reenter getters");
    DRW::setCustomDebugPrinter(nullptr);
    checks.expect(state.destroyed == 0, "replacement pins a printer with active callbacks");
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.release = true;
    }
    state.changed.notify_all();
    first.join();
    second.join();
    checks.expect(state.destroyed == 1 && state.callbackReentries == 2,
                  "retired printer is destroyed once outside service locks");
    DRW_dbg::getInstance()->setLevel(DRW_dbg::Level::None);
}

class ReplacingPrinter final : public DRW::DebugPrinter {
public:
    explicit ReplacingPrinter(std::atomic<unsigned>& destroyed) : destroyed_(destroyed) {}
    ~ReplacingPrinter() override {
        DRW_dbg::getInstance()->setLevel(DRW_dbg::Level::None);
        ++destroyed_;
    }
    void printS(const std::string&) override {
        DRW::setCustomDebugPrinter(new SilentPrinter);
        DRW_dbg::getInstance()->setLevel(DRW_dbg::Level::None);
    }
private:
    std::atomic<unsigned>& destroyed_;
};

class ThrowingPrinter final : public DRW::DebugPrinter {
public:
    void printS(const std::string&) override { throw std::runtime_error("debug sentinel"); }
};

void reentryAndExceptions(Checks& checks) {
    std::atomic<unsigned> destroyed{0};
    DRW::setCustomDebugPrinter(new ReplacingPrinter(destroyed));
    DRW_dbg::getInstance()->setLevel(DRW_dbg::Level::Debug);
    DRW_DBG("replace from callback");
    checks.expect(destroyed == 1, "callback replacement and destructor setters do not deadlock");
    DRW::setCustomDebugPrinter(new ThrowingPrinter);
    DRW_dbg::getInstance()->setLevel(DRW_dbg::Level::Debug);
    bool propagated = false;
    try { DRW_DBG("throw"); }
    catch (const std::runtime_error&) { propagated = true; }
    checks.expect(propagated, "custom callback exceptions retain propagation");
    DRW::setCustomDebugPrinter(nullptr);
    DRW_dbg::getInstance()->setLevel(DRW_dbg::Level::None);
}

void builtInFormatting(Checks& checks) {
    DRW::setCustomDebugPrinter(nullptr);
    DRW_dbg::getInstance()->setLevel(DRW_dbg::Level::Debug);
    std::ostringstream capture;
    std::streambuf* previous = std::cerr.rdbuf(capture.rdbuf());
    const auto flags = std::cerr.flags();
    const auto precision = std::cerr.precision();
    const auto fill = std::cerr.fill();
    const auto width = std::cerr.width();
    DRW_DBGH(15);
    DRW_DBG(" ");
    DRW_DBGB(12);
    DRW_DBG(" ");
    DRW_DBG(1.25);
    DRW_DBG(" ");
    DRW_DBGHL(1, 2, 15);
    DRW_DBG(" ");
    DRW_DBGPT(1., 2., 3.);
    const bool unchanged = std::cerr.flags() == flags && std::cerr.precision() == precision
        && std::cerr.fill() == fill && std::cerr.width() == width;
    std::cerr.rdbuf(previous);
    std::cerr.flags(flags);
    std::cerr.precision(precision);
    std::cerr.fill(fill);
    std::cerr.width(width);
    checks.expect(unchanged, "built-in printer does not mutate caller stream formatting");
    checks.expect(capture.str() == "0x0f 00000012 1.250000 1.2.0x0f x: 1.000000, y: 2.000000, z: 3.000000",
                  "built-in numeric fragment conventions retained");
    DRW_dbg::getInstance()->setLevel(DRW_dbg::Level::None);
}

void concurrentState(Checks& checks) {
    std::atomic<bool> start{false};
    std::atomic<unsigned> iterations{0};
    std::vector<std::thread> workers;
    for (unsigned worker = 0; worker < 4; ++worker) {
        workers.emplace_back([&start, &iterations, worker] {
            while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
            for (unsigned i = 0; i < 300; ++i) {
                dxfRW dxf("");
                dwgRW dwg("");
                DRW::setCustomDebugPrinter(new SilentPrinter);
                dxf.setDebug((i + worker) % 2 ? DRW::DebugLevel::None : DRW::DebugLevel::Debug);
                DRW_DBG("parallel state");
                (void)DRW_dbg::getInstance()->getLevel();
                (void)DRW_dbg::getInstance()->isDebugEnabled();
                DRW::setCustomDebugPrinter(nullptr);
                ++iterations;
            }
        });
    }
    start.store(true, std::memory_order_release);
    for (auto& worker : workers) worker.join();
    checks.expect(iterations == 1200, "independent concurrent constructors and setters complete");
    DRW_dbg::getInstance()->setLevel(DRW_dbg::Level::None);
    DRW::setCustomDebugPrinter(nullptr);
}

class Transcript final : public dx_iface {
public:
    Transcript() { cData = &data_; currentBlock = data_.mBlock; }
    void addHeader(const DRW_Header* value) override {
        events << "header;";
        dx_iface::addHeader(value);
    }
    void addBlock(const DRW_Block& value) override {
        events << "block:" << value.handle << ':' << value.name << ';';
        dx_iface::addBlock(value);
    }
    void addLine(const DRW_Line& value) override {
        events << std::setprecision(17) << "line:" << value.handle << ':' << value.basePoint.x
               << ',' << value.basePoint.y << ',' << value.basePoint.z << ':' << value.secPoint.x
               << ',' << value.secPoint.y << ',' << value.secPoint.z << ';';
        dx_iface::addLine(value);
    }
    void addLayer(const DRW_Layer& value) override {
        events << "layer:" << value.handle << ':' << value.name << ';';
        dx_iface::addLayer(value);
    }
    void addDwgFramePublication(const DRW_DwgFramePublication& value) override {
        events << "frame:" << value.m_handle << ':' << value.m_sourceOffset << ':'
               << value.m_sourceMapOrdinal << ':' << value.m_encodedType << ':'
               << value.m_resolvedType << ':' << static_cast<int>(value.m_carrier) << ';';
    }
    void addDwgFrameCoverageReport(const DRW_DwgFrameCoverageReport& value) override {
        events << "coverage:" << static_cast<int>(value.m_status) << ':'
               << value.m_complete << ':' << value.m_entries.size() << ';';
        for (const auto& entry : value.m_entries)
            events << entry.m_handle << ':' << entry.m_sourceOffset << ':'
                   << entry.m_sourceMapOrdinal << ':' << static_cast<int>(entry.m_disposition)
                   << ':' << static_cast<int>(entry.m_reason) << ':'
                   << static_cast<unsigned>(entry.m_publicationCount) << ';';
    }
    std::ostringstream events;
private:
    dx_data data_;
};

void dwgTranscript(Checks& checks) {
    const std::string filename = std::string(LIBDXFRW_DWG_FIXTURE_DIR) + "/ordinary_enc_AC1015.dwg";
    std::string first;
    for (auto level : {DRW::DebugLevel::None, DRW::DebugLevel::Debug}) {
        Transcript transcript;
        dwgRW reader(filename.c_str());
        DRW::setCustomDebugPrinter(new SilentPrinter);
        reader.setDebug(level);
        const bool ok = reader.read(&transcript, false);
        checks.expect(ok && !transcript.events.str().empty(), "tracked DWG reads in both debug states");
        transcript.events << "stats:" << reader.getEntityParseFailures() << ':'
                          << reader.getObjectParseFailures() << ':' << static_cast<int>(reader.getError());
        if (level == DRW::DebugLevel::None) first = transcript.events.str();
        else checks.expect(first == transcript.events.str(), "debug on/off callback and frame transcripts match");
    }
    DRW_dbg::getInstance()->setLevel(DRW_dbg::Level::None);
    DRW::setCustomDebugPrinter(nullptr);
}
} // namespace

int main(int argc, char** argv) {
    Checks checks;
    if (argc == 2 && std::string(argv[1]) == "--concurrent-only") {
        concurrentState(checks);
    } else if (argc == 2 && std::string(argv[1]) == "--format-only") {
        builtInFormatting(checks);
    } else {
        sequentialAndEager(checks);
        replacementAndOverlap(checks);
        reentryAndExceptions(checks);
        builtInFormatting(checks);
        concurrentState(checks);
        dwgTranscript(checks);
    }
    return checks.failures ? 1 : 0;
}
