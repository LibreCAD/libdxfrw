// SPDX-License-Identifier: GPL-2.0-or-later
// Generated associative dependency-graph OBJECTS allocation rollback/retry.
#include "lc3_compat_check.cpp"
#include "dx_iface.h"

#include <algorithm>
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

DRW_AssociativeObject makeNetwork() {
    DRW_AssociativeObject network("ACDBASSOCNETWORK");
    network.handle = 0xE901u;
    network.parentHandle = 0xE900u;
    network.reactorHandles = {0xE900u};
    network.m_classVersion = 1u;
    network.m_geometryStatus = 0;
    network.m_owningNetworkHandle = 0u;
    network.m_actionBodyHandle = 0u;
    network.m_actionIndex = 0;
    network.m_maxDependencyIndex = 0;
    network.m_networkVersion = 0;
    network.m_networkActionIndex = 0;
    network.m_networkFieldsParsed = true;
    network.m_actions = {{false, 0xE902u}, {false, 0xE903u}};

    DRW_AssociativePrefixStatus action;
    action.m_kind = DRW_AssociativePrefixStatus::Kind::AcDbAssocAction;
    action.m_status = DRW_AssociativePrefixStatus::ParseStatus::Complete;
    action.m_classVersion = 1u;
    action.m_decodedHandleCount = 2u;
    DRW_AssociativePrefixStatus networkStatus;
    networkStatus.m_kind =
        DRW_AssociativePrefixStatus::Kind::AcDbAssocNetwork;
    networkStatus.m_status =
        DRW_AssociativePrefixStatus::ParseStatus::Complete;
    networkStatus.m_classVersion = 1u;
    networkStatus.m_decodedHandleCount = 2u;
    networkStatus.m_decodedCountValue = 2;
    network.m_prefixStatuses = {action, networkStatus};
    return network;
}

DRW_AssociativeObject makeVariable() {
    DRW_AssociativeObject variable("ACDBASSOCVARIABLE");
    variable.handle = 0xE902u;
    variable.parentHandle = 0xE900u;
    variable.reactorHandles = {0xE900u};
    variable.m_classVersion = 1u;
    variable.m_geometryStatus = 0;
    variable.m_owningNetworkHandle = 0xE901u;
    variable.m_actionBodyHandle = 0u;
    variable.m_actionIndex = 6;
    variable.m_maxDependencyIndex = 0;
    variable.m_associativeVariableFieldsParsed = true;
    variable.m_associativeVariableWireComplete = true;
    variable.m_variableOwnedDependenciesParsed = true;
    variable.m_variableClassVersion = 2;
    variable.m_variableName = "BOLTHOLE";
    variable.m_variableExpression = "1";
    variable.m_variableEvaluator = "AcDbCalc:1.0";
    variable.m_variableValueParsed = true;
    variable.m_variableValueCode = 40;
    variable.m_variableValueKind =
        DRW_AssociativeObject::EvalVariantKind::Real;
    variable.m_variableValueReal = 3.5;

    DRW_AssociativePrefixStatus action;
    action.m_kind = DRW_AssociativePrefixStatus::Kind::AcDbAssocAction;
    action.m_status = DRW_AssociativePrefixStatus::ParseStatus::Complete;
    action.m_classVersion = 1u;
    action.m_decodedHandleCount = 2u;
    DRW_AssociativePrefixStatus body;
    body.m_kind = DRW_AssociativePrefixStatus::Kind::AcDbAssocVariable;
    body.m_status = DRW_AssociativePrefixStatus::ParseStatus::Complete;
    body.m_classVersion = 2u;
    body.m_decodedValueCount = 10u;
    variable.m_prefixStatuses = {action, body};
    return variable;
}

DRW_AssociativeObject makeTargetVariable() {
    DRW_AssociativeObject variable = makeVariable();
    variable.handle = 0xE903u;
    variable.parentHandle = 0xE901u;
    variable.reactorHandles = {0xE904u};
    variable.m_variableName = "TARGET_VALUE";
    variable.m_variableExpression = "17";
    variable.m_variableDescription = "dependency target";
    variable.m_variableValueReal = 17.0;
    return variable;
}

DRW_AssociativeObject makeValueDependency() {
    DRW_AssociativeObject dependency("ACDBASSOCVALUEDEPENDENCY");
    dependency.handle = 0xE904u;
    dependency.parentHandle = 0xE902u;
    dependency.m_classVersion = 1u;
    dependency.m_dependencyFieldsParsed = true;
    dependency.m_dependencyWireComplete = true;
    dependency.m_dependencyIsRead = true;
    dependency.m_dependencyIsAttachedToObject = true;
    dependency.m_dependencyIsDelegatingToOwningAction = true;
    dependency.m_dependencyHandle = 0xE903u;
    dependency.m_dependencyBodyId = 2;
    dependency.m_valueDependencyFieldsParsed = true;
    dependency.m_valueDependencyClassVersion = 0;
    dependency.m_valueDependencyValueCode = 90;
    dependency.m_valueDependencyValueKind =
        DRW_AssociativeObject::EvalVariantKind::Int32;
    dependency.m_valueDependencyValueInt32 = 17;
    return dependency;
}

class AssociativeNetworkExport final : public LC3CompatCheck {
public:
    explicit AssociativeNetworkExport(dxfRW& output) : output(output) {
        dictionary.handle = 0xE900u;
        DRW_Dictionary::Entry entry;
        entry.m_name = "ACAD_ASSOCNETWORK";
        entry.m_handle = 0xE901u;
        dictionary.m_entries.push_back(std::move(entry));
        network = makeNetwork();
        variable = makeVariable();
        targetVariable = makeTargetVariable();
        dependency = makeValueDependency();
    }

    dxfRW& output;
    DRW_Dictionary dictionary;
    DRW_AssociativeObject network;
    DRW_AssociativeObject variable;
    DRW_AssociativeObject targetVariable;
    DRW_AssociativeObject dependency;
    long failAt = -1;
    long reached = 0;
    bool emitted = false;

    void writeObjects() override {
        const auto emit = [this]() {
            emitted = output.writeDictionary(&dictionary)
                && output.writeAssociativeNetwork(&network)
                && output.writeAssociativeVariable(&variable)
                && output.writeAssociativeVariable(&targetVariable)
                && output.writeAssociativeDependency(&dependency);
        };
        if (failAt < 0) {
            emit();
            return;
        }
        armAllocationFailure(failAt);
        try {
            emit();
        } catch (const std::bad_alloc&) {
            reached = static_cast<long>(allocationAttempts);
            disarmAllocationFailure();
            throw;
        }
        reached = static_cast<long>(allocationAttempts);
        disarmAllocationFailure();
    }

    bool finalizeDxfWrite() override { return emitted; }
};

bool readAll(const std::filesystem::path& path, std::string& value) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    value.assign(std::istreambuf_iterator<char>(input),
                 std::istreambuf_iterator<char>());
    return !input.bad();
}

bool write(AssociativeNetworkExport& exporter, dxfRW& codec, bool binary) {
    codec.setRootDictEntries({{"ACAD_ASSOCNETWORK", "E900"}});
    return codec.write(&exporter, DRW::AC1024, binary);
}

bool verify(const std::filesystem::path& path) {
    dx_data data;
    dx_iface reader;
    const bool imported = reader.fileImport(path.string(), &data, false);
    if (!imported || !data.hasNamedObjectDictionary
        || data.associativeObjects.size() != 4u)
        return false;
    const bool rooted = std::any_of(
        data.namedObjectDictionary.m_entries.begin(),
        data.namedObjectDictionary.m_entries.end(),
        [](const DRW_Dictionary::Entry& entry) {
            return entry.m_name == "ACAD_ASSOCNETWORK"
                && entry.m_handle == 0xE900u;
        });
    const auto dictionary = std::find_if(data.dictionaries.begin(),
        data.dictionaries.end(), [](const DRW_Dictionary& value) {
            return value.handle == 0xE900u;
        });
    if (!rooted || dictionary == data.dictionaries.end()
        || dictionary->parentHandle != data.namedObjectDictionary.handle
        || dictionary->m_entries.size() != 1u
        || dictionary->m_entries.front().m_name != "ACAD_ASSOCNETWORK"
        || dictionary->m_entries.front().m_handle != 0xE901u)
        return false;
    const auto network = std::find_if(data.associativeObjects.begin(),
        data.associativeObjects.end(), [](const DRW_AssociativeObject& value) {
            return value.handle == 0xE901u
                && value.m_recordName == "ACDBASSOCNETWORK";
        });
    const auto variable = std::find_if(data.associativeObjects.begin(),
        data.associativeObjects.end(), [](const DRW_AssociativeObject& value) {
            return value.handle == 0xE902u
                && value.m_recordName == "ACDBASSOCVARIABLE";
        });
    const auto targetVariable = std::find_if(
        data.associativeObjects.begin(), data.associativeObjects.end(),
        [](const DRW_AssociativeObject& value) {
            return value.handle == 0xE903u
                && value.m_recordName == "ACDBASSOCVARIABLE";
        });
    const auto dependency = std::find_if(data.associativeObjects.begin(),
        data.associativeObjects.end(), [](const DRW_AssociativeObject& value) {
            return value.handle == 0xE904u
                && value.m_recordName == "ACDBASSOCVALUEDEPENDENCY";
        });
    return network != data.associativeObjects.end()
        && network->parentHandle == 0xE900u
        && network->reactorHandles == std::vector<std::uint32_t>{0xE900u}
        && network->m_classVersion == 1u && network->m_networkVersion == 0
        && network->m_networkFieldsParsed
        && network->m_actions.size() == 2u
        && !network->m_actions.front().m_isOwned
        && network->m_actions.front().m_handle == 0xE902u
        && !network->m_actions.back().m_isOwned
        && network->m_actions.back().m_handle == 0xE903u
        && network->m_owningNetworkHandle == 0u
        && variable != data.associativeObjects.end()
        && variable->parentHandle == 0xE900u
        && variable->reactorHandles
            == std::vector<std::uint32_t>{0xE900u}
        && variable->m_owningNetworkHandle == network->handle
        && variable->m_associativeVariableFieldsParsed
        && variable->m_variableClassVersion == 2
        && variable->m_variableName == "BOLTHOLE"
        && variable->m_variableExpression == "1"
        && variable->m_variableEvaluator == "AcDbCalc:1.0"
        && variable->m_variableValueCode == 40
        && variable->m_variableValueKind
            == DRW_AssociativeObject::EvalVariantKind::Real
        && variable->m_variableValueReal == 3.5
        && targetVariable != data.associativeObjects.end()
        && targetVariable->parentHandle == network->handle
        && targetVariable->m_owningNetworkHandle == network->handle
        && targetVariable->reactorHandles
            == std::vector<std::uint32_t>{0xE904u}
        && targetVariable->m_variableName == "TARGET_VALUE"
        && targetVariable->m_variableExpression == "17"
        && dependency != data.associativeObjects.end()
        && dependency->parentHandle == variable->handle
        && dependency->m_dependencyWireComplete
        && dependency->m_dependencyHandle == targetVariable->handle
        && dependency->m_valueDependencyFieldsParsed
        && dependency->m_valueDependencyValueCode == 90
        && dependency->m_valueDependencyValueKind
            == DRW_AssociativeObject::EvalVariantKind::Int32
        && dependency->m_valueDependencyValueInt32 == 17;
}

bool sourceUnchanged(const AssociativeNetworkExport& actual,
                     const AssociativeNetworkExport& expected) {
    const auto& a = actual.network;
    const auto& b = expected.network;
    const auto& av = actual.variable;
    const auto& bv = expected.variable;
    const auto& at = actual.targetVariable;
    const auto& bt = expected.targetVariable;
    const auto& ad = actual.dependency;
    const auto& bd = expected.dependency;
    if (actual.dictionary.handle != expected.dictionary.handle
        || actual.dictionary.m_entries.size() != 1u
        || actual.dictionary.m_entries.front().m_name
            != expected.dictionary.m_entries.front().m_name
        || actual.dictionary.m_entries.front().m_handle
            != expected.dictionary.m_entries.front().m_handle
        || a.handle != b.handle || a.parentHandle != b.parentHandle
        || a.reactorHandles != b.reactorHandles
        || a.m_owningNetworkHandle != b.m_owningNetworkHandle
        || a.m_classVersion != b.m_classVersion
        || a.m_networkVersion != b.m_networkVersion
        || a.m_actions.size() != b.m_actions.size()
        || a.m_prefixStatuses.size() != b.m_prefixStatuses.size()
        || av.handle != bv.handle || av.parentHandle != bv.parentHandle
        || av.reactorHandles != bv.reactorHandles
        || av.m_owningNetworkHandle != bv.m_owningNetworkHandle
        || av.m_associativeVariableFieldsParsed
            != bv.m_associativeVariableFieldsParsed
        || av.m_associativeVariableWireComplete
            != bv.m_associativeVariableWireComplete
        || av.m_variableName != bv.m_variableName
        || av.m_variableExpression != bv.m_variableExpression
        || av.m_variableEvaluator != bv.m_variableEvaluator
        || av.m_variableValueCode != bv.m_variableValueCode
        || av.m_variableValueReal != bv.m_variableValueReal
        || av.m_prefixStatuses.size() != bv.m_prefixStatuses.size()
        || at.handle != bt.handle || at.parentHandle != bt.parentHandle
        || at.reactorHandles != bt.reactorHandles
        || at.m_owningNetworkHandle != bt.m_owningNetworkHandle
        || at.m_variableName != bt.m_variableName
        || at.m_variableExpression != bt.m_variableExpression
        || at.m_variableValueCode != bt.m_variableValueCode
        || at.m_variableValueReal != bt.m_variableValueReal
        || at.m_prefixStatuses.size() != bt.m_prefixStatuses.size()
        || ad.handle != bd.handle || ad.parentHandle != bd.parentHandle
        || ad.reactorHandles != bd.reactorHandles
        || ad.m_dependencyHandle != bd.m_dependencyHandle
        || ad.m_dependencyFieldsParsed != bd.m_dependencyFieldsParsed
        || ad.m_dependencyWireComplete != bd.m_dependencyWireComplete
        || ad.m_valueDependencyFieldsParsed
            != bd.m_valueDependencyFieldsParsed
        || ad.m_valueDependencyValueCode != bd.m_valueDependencyValueCode
        || ad.m_valueDependencyValueKind != bd.m_valueDependencyValueKind
        || ad.m_valueDependencyValueInt32
            != bd.m_valueDependencyValueInt32)
        return false;
    for (std::size_t index = 0u; index < a.m_actions.size(); ++index)
        if (a.m_actions[index].m_isOwned != b.m_actions[index].m_isOwned
            || a.m_actions[index].m_handle != b.m_actions[index].m_handle)
            return false;
    for (std::size_t index = 0u; index < a.m_prefixStatuses.size(); ++index) {
        const auto& left = a.m_prefixStatuses[index];
        const auto& right = b.m_prefixStatuses[index];
        if (left.m_kind != right.m_kind || left.m_status != right.m_status
            || left.m_classVersion != right.m_classVersion
            || left.m_decodedHandleCount != right.m_decodedHandleCount
            || left.m_decodedCountValue != right.m_decodedCountValue)
            return false;
    }
    for (std::size_t index = 0u; index < av.m_prefixStatuses.size(); ++index) {
        const auto& left = av.m_prefixStatuses[index];
        const auto& right = bv.m_prefixStatuses[index];
        if (left.m_kind != right.m_kind || left.m_status != right.m_status
            || left.m_classVersion != right.m_classVersion
            || left.m_decodedHandleCount != right.m_decodedHandleCount
            || left.m_decodedValueCount != right.m_decodedValueCount
            || left.m_decodedCountValue != right.m_decodedCountValue)
            return false;
    }
    return true;
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "associative-network OBJECTS allocation retry: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const auto output = directory / "associative-network-objects.dxf";
    const std::string sentinel = "prior associative-network destination\n";
    dxfRW codec(output.string().c_str());
    AssociativeNetworkExport exporter(codec);
    AssociativeNetworkExport expected(codec);

    for (const bool binary : {false, true}) {
        if (!write(exporter, codec, binary))
            return fail("warm-up export was refused");
        exporter.failAt = 1000000;
        if (!write(exporter, codec, binary))
            return fail("allocation calibration export was refused");
        if (!verify(output))
            return fail("calibration output did not read back typed network");
        const long allocationCount = exporter.reached;
        if (allocationCount <= 0)
            return fail("calibration observed no callback allocations");
        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream prior(output, std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior)
                    return fail("could not initialize destination sentinel");
            }
            exporter.failAt = offset;
            exporter.reached = 0;
            exporter.emitted = false;
            const bool wrote = write(exporter, codec, binary);
            exporter.failAt = -1;
            std::string current;
            if (wrote || exporter.reached != offset + 1 || !allocationFailed
                || !readAll(output, current)
                || current != sentinel)
                return fail("injected failure changed destination or was missed");
            if (!sourceUnchanged(exporter, expected))
                return fail("injected failure changed caller graph");
            if (!write(exporter, codec, binary) || !verify(output))
                return fail("same-writer retry failed typed network readback");
            if (!sourceUnchanged(exporter, expected))
                return fail("same-writer retry changed caller graph");
        }
        std::cout << "associative-network OBJECTS "
                  << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", rollback/retry/readback=PASS\n";
    }
    std::error_code ignored;
    std::filesystem::remove(output, ignored);
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (failAllocation())
        throw std::bad_alloc();
    if (void* value = std::malloc(size == 0 ? 1 : size))
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
        std::cerr << "usage: associative_network_objects_allocation_tests <scratch-dir>\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
