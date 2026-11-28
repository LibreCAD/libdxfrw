// SPDX-License-Identifier: GPL-2.0-or-later
// Generated EVALUATIONGRAPH DXF OBJECTS allocation rollback/retry control.
#include "dx_iface.h"

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

DRW_EvaluationGraph makeGraph() {
    DRW_EvaluationGraph graph;
    graph.handle = 0xD7E1u;
    graph.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
    graph.m_value96 = 96;
    graph.m_value97 = 97;
    DRW_EvaluationGraphNode first;
    first.m_index = 1;
    first.m_flags = 2;
    first.m_nextNodeIndex = 2;
    first.m_data1 = 11;
    first.m_data2 = 12;
    first.m_data3 = 13;
    first.m_data4 = 14;
    graph.m_nodes.push_back(first);
    DRW_EvaluationGraphNode second;
    second.m_index = 2;
    second.m_flags = 3;
    second.m_nextNodeIndex = 0;
    second.m_data1 = 21;
    second.m_data2 = 22;
    second.m_data3 = 23;
    second.m_data4 = 24;
    graph.m_nodes.push_back(second);
    DRW_EvaluationGraphEdge edge;
    edge.m_value92 = 92;
    edge.m_value93 = 93;
    edge.m_value94 = 94;
    edge.m_value91a = 91;
    edge.m_value91b = 910;
    edge.m_value92a = 921;
    edge.m_value92b = 922;
    edge.m_value92c = 923;
    edge.m_value92d = 924;
    edge.m_value92e = 925;
    graph.m_edges.push_back(edge);
    return graph;
}

bool sourceMatches(const DRW_EvaluationGraph& actual,
                   const DRW_EvaluationGraph& expected) {
    if (actual.handle != expected.handle
        || actual.parentHandle != expected.parentHandle
        || actual.m_value96 != expected.m_value96
        || actual.m_value97 != expected.m_value97
        || actual.m_nodes.size() != expected.m_nodes.size()
        || actual.m_edges.size() != expected.m_edges.size())
        return false;
    for (std::size_t i = 0; i < actual.m_nodes.size(); ++i) {
        const auto& a = actual.m_nodes[i];
        const auto& e = expected.m_nodes[i];
        if (a.m_index != e.m_index || a.m_flags != e.m_flags
            || a.m_nextNodeIndex != e.m_nextNodeIndex
            || a.m_expressionHandle != e.m_expressionHandle
            || a.m_data1 != e.m_data1 || a.m_data2 != e.m_data2
            || a.m_data3 != e.m_data3 || a.m_data4 != e.m_data4)
            return false;
    }
    for (std::size_t i = 0; i < actual.m_edges.size(); ++i) {
        const auto& a = actual.m_edges[i];
        const auto& e = expected.m_edges[i];
        if (a.m_value92 != e.m_value92 || a.m_value93 != e.m_value93
            || a.m_value94 != e.m_value94 || a.m_value91a != e.m_value91a
            || a.m_value91b != e.m_value91b || a.m_value92a != e.m_value92a
            || a.m_value92b != e.m_value92b || a.m_value92c != e.m_value92c
            || a.m_value92d != e.m_value92d || a.m_value92e != e.m_value92e)
            return false;
    }
    return true;
}

class Export final : public dx_iface {
public:
    explicit Export(dxfRW& codec) : codec(codec), graph(makeGraph()) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &codec;
    }

    void writeObjects() override {
        if (failAt < 0) {
            accepted = codec.writeEvaluationGraph(
                &graph, "ACAD_EVALUATION_GRAPH");
            return;
        }
        armAllocationFailure(failAt);
        try {
            accepted = codec.writeEvaluationGraph(
                &graph, "ACAD_EVALUATION_GRAPH");
        } catch (const std::bad_alloc&) {
            accepted = false;
        }
        reached = static_cast<long>(allocationAttempts);
        disarmAllocationFailure();
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    dx_data storage;
    DRW_EvaluationGraph graph;
    long failAt = -1;
    long reached = 0;
    bool accepted = false;
};

class Readback final : public dx_iface {
public:
    Readback() {
        cData = &storage;
        currentBlock = storage.mBlock;
    }

    void addEvaluationGraph(const DRW_EvaluationGraph& value) override {
        graphs.push_back(value);
    }
    dx_data storage;
    std::vector<DRW_EvaluationGraph> graphs;
};

bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

bool readbackMatches(const std::filesystem::path& path) {
    dxfRW codec(path.string().c_str());
    Readback reader;
    const bool read = codec.read(&reader, false);
    return read && reader.graphs.size() == 1u
        && sourceMatches(reader.graphs.front(), makeGraph());
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "EVALUATIONGRAPH OBJECTS allocation retry: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    const std::string sentinel = "prior EVALUATIONGRAPH destination bytes\n";
    for (const DRW::Version version :
         {DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
      const auto output = directory
          / ("evaluation-graph-objects-"
             + std::to_string(static_cast<int>(version)) + ".dxf");
      for (const bool binary : {false, true}) {
        long allocationCount = 0;
        std::string expectedBytes;
        {
            dxfRW codec(output.string().c_str());
            Export exporter(codec);
            exporter.failAt = 1000000;
            const bool wrote = codec.write(&exporter, version, binary);
            if (!wrote || !exporter.accepted || !readbackMatches(output))
                return fail("allocation calibration output did not read back");
            allocationCount = exporter.reached;
            if (!readBytes(output, expectedBytes))
                return fail("could not retain calibration output bytes");
        }
        if (allocationCount <= 0)
            return fail("calibration observed no callback allocations");

        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream prior(output, std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior)
                    return fail("could not initialize destination sentinel");
            }
            dxfRW codec(output.string().c_str());
            Export exporter(codec);
            const DRW_EvaluationGraph source = exporter.graph;
            exporter.failAt = offset;
            exporter.reached = 0;
            exporter.accepted = false;
            const bool written = codec.write(&exporter, version, binary);
            exporter.failAt = -1;
            std::string current;
            if (written || exporter.accepted || exporter.reached != offset + 1
                || !allocationFailed
                || !readBytes(output, current) || current != sentinel)
                return fail("injected failure changed destination or missed allocation");
            if (!sourceMatches(exporter.graph, source))
                return fail("injected failure changed caller EVALUATIONGRAPH state");
            const bool retry = codec.write(&exporter, version, binary);
            std::string retryBytes;
            if (!retry || !exporter.accepted || !readBytes(output, retryBytes)
                || retryBytes != expectedBytes || !readbackMatches(output)
                || !sourceMatches(exporter.graph, source))
                return fail("same-codec retry failed exact bytes or typed readback");
        }
        std::cout << "EVALUATIONGRAPH OBJECTS "
                  << static_cast<int>(version) << ' '
                  << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", rollback/retry/readback=PASS\n";
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
      }
    }
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (failAllocation())
        throw std::bad_alloc();
    if (void* value = std::malloc(size == 0 ? 1u : size))
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
        std::cerr << "usage: evaluation_graph_objects_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
