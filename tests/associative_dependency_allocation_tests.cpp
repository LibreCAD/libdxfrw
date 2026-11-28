// SPDX-License-Identifier: GPL-2.0-or-later
// Generated associative-dependency OBJECTS allocation rollback and retry.
#include "lc3_compat_check.cpp"
#include "dx_iface.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <new>
#include <string>

namespace {
long allocationsBeforeFailure = -1;
std::size_t failedAllocations = 0;

enum class DependencyProfile { Base, Value, IntegerValue, Geometry };

struct AllocationFailure {
    explicit AllocationFailure(long count) { allocationsBeforeFailure = count; }
    ~AllocationFailure() { allocationsBeforeFailure = -1; }
};

DRW_AssociativeObject makeDependency(DependencyProfile profile) {
    const char* recordName = (profile == DependencyProfile::Value
                              || profile == DependencyProfile::IntegerValue)
        ? "ACDBASSOCVALUEDEPENDENCY"
        : profile == DependencyProfile::Geometry
            ? "ACDBASSOCGEOMDEPENDENCY" : "ACDBASSOCDEPENDENCY";
    DRW_AssociativeObject dependency(recordName);
    dependency.handle = 0xE901u;
    dependency.m_classVersion = 3u;
    dependency.m_status = -7;
    dependency.m_dependencyFieldsParsed = true;
    dependency.m_dependencyWireComplete = true;
    dependency.m_dependencyIsRead = true;
    dependency.m_dependencyIsAttachedToObject = true;
    dependency.m_dependencyOrder = -2;
    dependency.m_dependencyNamePresent = true;
    dependency.m_dependencyName = "allocation-retry";
    dependency.m_dependencyBodyId = -9;
    if (profile == DependencyProfile::Base)
        dependency.m_dependencyBodyHandle = 0xE902u;
    if (profile == DependencyProfile::Value
        || profile == DependencyProfile::IntegerValue) {
        dependency.m_valueDependencyFieldsParsed = true;
        dependency.m_valueDependencyClassVersion = 0;
        dependency.m_valueDependencyName = "VALUE_DEPENDENCY";
        dependency.m_valueDependencyValueCode =
            profile == DependencyProfile::IntegerValue ? 90 : 40;
        dependency.m_valueDependencyValueKind =
            profile == DependencyProfile::IntegerValue
                ? DRW_AssociativeObject::EvalVariantKind::Int32
                : DRW_AssociativeObject::EvalVariantKind::Real;
        dependency.m_valueDependencyValueReal =
            profile == DependencyProfile::IntegerValue ? 0.0 : 12.5;
        dependency.m_valueDependencyValueInt32 =
            profile == DependencyProfile::IntegerValue ? 17 : 0;
    } else if (profile == DependencyProfile::Geometry) {
        dependency.m_geomDependencyFieldsParsed = true;
        dependency.m_geomClassVersion = 0;
        dependency.m_geomEnabled = true;
        dependency.m_geomPersistentSubentityClassName =
            "AcDbAssocSingleEdgePersSubentId";
        dependency.m_geomDependentOnCompoundObject = true;
    }
    return dependency;
}

DRW_AssociativeObject makeDimensionDependencyBody() {
    DRW_AssociativeObject body("ASSOCDIMDEPENDENCYBODY");
    body.handle = 0xE902u;
    body.parentHandle = 0xE901u;
    body.name = "BOLTHOLE";
    body.m_dependencyBodyVersion = 1u;
    body.m_dimBaseVersion = 1u;
    body.m_classVersion = 1u;
    body.m_dimensionDependencyBodyParsed = true;
    DRW_AssociativePrefixStatus status;
    status.m_kind =
        DRW_AssociativePrefixStatus::Kind::AcDbAssocDependencyBody;
    status.m_bitSize = 30u;
    status.m_classVersion = 1u;
    status.m_status = DRW_AssociativePrefixStatus::ParseStatus::Complete;
    status.m_decodedValueCount = 4u;
    body.m_prefixStatuses.push_back(status);
    return body;
}

class AssociativeDependencyExport final : public LC3CompatCheck {
public:
    explicit AssociativeDependencyExport(dxfRW& output,
                                         DependencyProfile profile)
        : output(output), profile(profile) {
        dependency = makeDependency(profile);
        if (profile == DependencyProfile::Base)
            body = makeDimensionDependencyBody();
    }

    dxfRW& output;
    DRW_AssociativeObject dependency;
    DRW_AssociativeObject body;
    DependencyProfile profile;
    long failAt = -1;
    long reached = 0;
    bool emitted = false;

    void writeObjects() override {
        const auto emit = [this]() {
            emitted = output.writeAssociativeDependency(&dependency);
            if (emitted && profile == DependencyProfile::Base)
                emitted = output.writeAssociativeDependencyBody(&body);
        };
        if (failAt < 0) {
            emit();
            return;
        }
        const auto before = failedAllocations;
        try {
            AllocationFailure failure(failAt);
            emit();
            reached = failedAllocations != before
                ? static_cast<long>(failedAllocations - before)
                : failAt - allocationsBeforeFailure;
        } catch (const std::bad_alloc&) {
            reached = failedAllocations != before
                ? static_cast<long>(failedAllocations - before)
                : failAt - allocationsBeforeFailure;
            throw;
        }
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

bool write(AssociativeDependencyExport& exporter, dxfRW& codec, bool binary) {
    return codec.write(&exporter, DRW::AC1024, binary);
}

bool verify(const std::filesystem::path& path, DependencyProfile profile) {
    dx_data data;
    dx_iface reader;
    if (!reader.fileImport(path.string(), &data, false)
        || data.associativeObjects.size()
            != (profile == DependencyProfile::Base ? 2u : 1u))
        return false;
    const auto dependencyIt = std::find_if(
        data.associativeObjects.begin(), data.associativeObjects.end(),
        [](const DRW_AssociativeObject& object) {
            return object.handle == 0xE901u;
        });
    if (dependencyIt == data.associativeObjects.end())
        return false;
    const DRW_AssociativeObject& actual = *dependencyIt;
    const bool dependencyMatches = actual.m_recordName ==
        ((profile == DependencyProfile::Value
          || profile == DependencyProfile::IntegerValue)
            ? "ACDBASSOCVALUEDEPENDENCY"
            : profile == DependencyProfile::Geometry
                ? "ACDBASSOCGEOMDEPENDENCY" : "ACDBASSOCDEPENDENCY")
        && actual.m_dependencyFieldsParsed
        && actual.m_dependencyWireComplete
        && actual.m_classVersion == 3u && actual.m_status == -7
        && actual.m_dependencyIsRead
        && actual.m_dependencyIsAttachedToObject
        && actual.m_dependencyOrder == -2
        && actual.m_dependencyNamePresent
        && actual.m_dependencyName == "allocation-retry"
        && actual.m_dependencyBodyId == -9
        && actual.m_dependencyBodyHandle
            == (profile == DependencyProfile::Base ? 0xE902u : 0u)
        && ((profile != DependencyProfile::Value
             && profile != DependencyProfile::IntegerValue)
            || (actual.m_valueDependencyFieldsParsed
                && actual.m_valueDependencyClassVersion == 0
                && actual.m_valueDependencyName == "VALUE_DEPENDENCY"
                && actual.m_valueDependencyValueCode
                    == (profile == DependencyProfile::IntegerValue ? 90 : 40)
                && actual.m_valueDependencyValueKind
                    == (profile == DependencyProfile::IntegerValue
                        ? DRW_AssociativeObject::EvalVariantKind::Int32
                        : DRW_AssociativeObject::EvalVariantKind::Real)
                && (profile == DependencyProfile::IntegerValue
                    ? actual.m_valueDependencyValueInt32 == 17
                    : actual.m_valueDependencyValueReal == 12.5)))
        && (profile != DependencyProfile::Geometry
            || (actual.m_geomDependencyFieldsParsed
                && actual.m_geomClassVersion == 0
                && actual.m_geomEnabled
                && actual.m_geomPersistentSubentityClassName
                    == "AcDbAssocSingleEdgePersSubentId"
                && actual.m_geomDependentOnCompoundObject));
    if (!dependencyMatches || profile != DependencyProfile::Base)
        return dependencyMatches;
    const auto bodyIt = std::find_if(
        data.associativeObjects.begin(), data.associativeObjects.end(),
        [](const DRW_AssociativeObject& object) {
            return object.handle == 0xE902u;
        });
    return bodyIt != data.associativeObjects.end()
        && bodyIt->m_recordName == "ASSOCDIMDEPENDENCYBODY"
        && bodyIt->parentHandle == 0xE901u
        && bodyIt->name == "BOLTHOLE"
        && bodyIt->m_dimensionDependencyBodyParsed
        && bodyIt->m_dependencyBodyVersion == 1u
        && bodyIt->m_dimBaseVersion == 1u
        && bodyIt->m_classVersion == 1u;
}

bool sourceUnchanged(const DRW_AssociativeObject& actual,
                     const DRW_AssociativeObject& expected) {
    return actual.handle == expected.handle
        && actual.parentHandle == expected.parentHandle
        && actual.reactorHandles == expected.reactorHandles
        && actual.m_recordName == expected.m_recordName
        && actual.m_dependencyFieldsParsed
            == expected.m_dependencyFieldsParsed
        && actual.m_dependencyWireComplete
            == expected.m_dependencyWireComplete
        && actual.m_dependencyName == expected.m_dependencyName
        && actual.m_dependencyBodyId == expected.m_dependencyBodyId
        && actual.m_dependencyBodyHandle
            == expected.m_dependencyBodyHandle
        && actual.m_valueDependencyFieldsParsed
            == expected.m_valueDependencyFieldsParsed
        && actual.m_valueDependencyClassVersion
            == expected.m_valueDependencyClassVersion
        && actual.m_valueDependencyName == expected.m_valueDependencyName
        && actual.m_valueDependencyValueCode
            == expected.m_valueDependencyValueCode
        && actual.m_valueDependencyValueKind
            == expected.m_valueDependencyValueKind
        && actual.m_valueDependencyValueReal
            == expected.m_valueDependencyValueReal
        && actual.m_geomDependencyFieldsParsed
            == expected.m_geomDependencyFieldsParsed
        && actual.m_geomClassVersion == expected.m_geomClassVersion
        && actual.m_geomEnabled == expected.m_geomEnabled
        && actual.m_geomPersistentSubentityClassName
            == expected.m_geomPersistentSubentityClassName
        && actual.m_geomPersistentSubentityFieldsParsed
            == expected.m_geomPersistentSubentityFieldsParsed
        && actual.m_geomDependentOnCompoundObject
            == expected.m_geomDependentOnCompoundObject;
}

bool bodyUnchanged(const DRW_AssociativeObject& actual,
                   const DRW_AssociativeObject& expected) {
    return actual.handle == expected.handle
        && actual.parentHandle == expected.parentHandle
        && actual.m_recordName == expected.m_recordName
        && actual.name == expected.name
        && actual.m_dimensionDependencyBodyParsed
            == expected.m_dimensionDependencyBodyParsed
        && actual.m_dependencyBodyVersion == expected.m_dependencyBodyVersion
        && actual.m_dimBaseVersion == expected.m_dimBaseVersion
        && actual.m_classVersion == expected.m_classVersion
        && actual.m_prefixStatuses.size()
            == expected.m_prefixStatuses.size()
        && (actual.m_prefixStatuses.empty()
            || (actual.m_prefixStatuses.front().m_kind
                    == expected.m_prefixStatuses.front().m_kind
                && actual.m_prefixStatuses.front().m_bitSize
                    == expected.m_prefixStatuses.front().m_bitSize
                && actual.m_prefixStatuses.front().m_classVersion
                    == expected.m_prefixStatuses.front().m_classVersion
                && actual.m_prefixStatuses.front().m_status
                    == expected.m_prefixStatuses.front().m_status
                && actual.m_prefixStatuses.front().m_decodedValueCount
                    == expected.m_prefixStatuses.front().m_decodedValueCount));
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "associative dependency OBJECTS allocation retry: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const auto output = directory / "associative-dependency-objects.dxf";
    const std::string sentinel = "prior associative dependency bytes\n";
    dxfRW codec(output.string().c_str());
    for (const bool binary : {false, true}) {
        for (const DependencyProfile profile : {
                 DependencyProfile::Base, DependencyProfile::Value,
                 DependencyProfile::IntegerValue,
                 DependencyProfile::Geometry}) {
            AssociativeDependencyExport exporter(codec, profile);
            AssociativeDependencyExport expected(codec, profile);
            if (!write(exporter, codec, binary)
                || !verify(output, profile))
                return fail("generated baseline did not read back typed dependency fields");
            exporter.failAt = 1000000;
            if (!write(exporter, codec, binary)
                || !verify(output, profile))
                return fail("allocation calibration write/readback failed");
            const long allocationCount = exporter.reached;
            if (allocationCount <= 0)
                return fail("calibration observed no callback allocations");
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                                        std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior)
                        return fail("could not initialize destination sentinel");
                }
                exporter.failAt = offset;
                exporter.reached = 0;
                exporter.emitted = false;
                const DRW_AssociativeObject sourceBefore = exporter.dependency;
                const DRW_AssociativeObject bodyBefore = exporter.body;
                const bool wrote = write(exporter, codec, binary);
                exporter.failAt = -1;
                std::string current;
                if (wrote || exporter.reached < 1
                    || !readAll(output, current) || current != sentinel)
                    return fail("injected allocation published bytes or was missed");
                if (!sourceUnchanged(exporter.dependency, sourceBefore)
                    || !sourceUnchanged(exporter.dependency,
                                        expected.dependency)
                    || !bodyUnchanged(exporter.body, bodyBefore)
                    || !bodyUnchanged(exporter.body, expected.body))
                    return fail("injected failure changed the caller object");
                if (!write(exporter, codec, binary)
                    || !verify(output, profile))
                    return fail("same-codec retry failed typed readback");
                if (!sourceUnchanged(exporter.dependency,
                                     expected.dependency)
                    || !bodyUnchanged(exporter.body, expected.body))
                    return fail("retry changed the caller object");
            }
            std::cout << "AC1024 "
                      << (profile == DependencyProfile::Value ? "real value"
                          : profile == DependencyProfile::IntegerValue
                              ? "integer value"
                          : profile == DependencyProfile::Geometry ? "geometry"
                          : "base")
                      << " associative dependency OBJECTS "
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
        }
    }
    std::error_code ignored;
    std::filesystem::remove(output, ignored);
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (allocationsBeforeFailure == 0) {
        ++failedAllocations;
        throw std::bad_alloc();
    }
    if (allocationsBeforeFailure > 0)
        --allocationsBeforeFailure;
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
        std::cerr << "usage: associative_dependency_allocation_tests <scratch-dir>\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
