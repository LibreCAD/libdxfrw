// SPDX-License-Identifier: GPL-2.0-or-later
// Generated FIELD/FIELDLIST OBJECTS allocation rollback/retry control.
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

DRW_Field makeField() {
    DRW_Field field;
    field.handle = 0xA501u;
    field.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
    field.m_evaluatorId = "AcObjProp";
    field.m_fieldCode = "%<\\AcVar CustomField>%";
    field.m_value.m_dataType = 2;
    field.m_value.m_value.addDouble(140, 23.5);
    field.m_valueString = "23.5";
    field.m_valueStringLength = 4;
    return field;
}

DRW_FieldList makeFieldList() {
    DRW_FieldList fieldList;
    fieldList.handle = 0xA502u;
    fieldList.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
    fieldList.m_fieldHandles = {0xA501u};
    return fieldList;
}

bool fieldStateMatches(const DRW_Field& value, const DRW_Field& expected) {
    return value.tType == expected.tType
        && value.handle == expected.handle
        && value.parentHandle == expected.parentHandle
        && value.name == expected.name && value.flags == expected.flags
        && value.extData.size() == expected.extData.size()
        && value.appData.size() == expected.appData.size()
        && value.reactorHandles == expected.reactorHandles
        && value.xDictHandle == expected.xDictHandle
        && value.m_evaluatorId == expected.m_evaluatorId
        && value.m_fieldCode == expected.m_fieldCode
        && value.m_formatString == expected.m_formatString
        && value.m_evaluationOptionFlags == expected.m_evaluationOptionFlags
        && value.m_filingOptionFlags == expected.m_filingOptionFlags
        && value.m_fieldStateFlags == expected.m_fieldStateFlags
        && value.m_evaluationStatusFlags == expected.m_evaluationStatusFlags
        && value.m_evaluationErrorCode == expected.m_evaluationErrorCode
        && value.m_evaluationErrorMessage == expected.m_evaluationErrorMessage
        && value.m_value.m_formatFlags == expected.m_value.m_formatFlags
        && value.m_value.m_dataType == expected.m_value.m_dataType
        && value.m_value.m_dataSize == expected.m_value.m_dataSize
        && value.m_value.m_unitType == expected.m_value.m_unitType
        && value.m_value.m_value.code() == expected.m_value.m_value.code()
        && value.m_value.m_value.type() == expected.m_value.m_value.type()
        && value.m_value.m_value.type() == DRW_Variant::DOUBLE
        && value.m_value.m_value.d_val() == expected.m_value.m_value.d_val()
        && value.m_value.m_formatString == expected.m_value.m_formatString
        && value.m_value.m_valueString == expected.m_value.m_valueString
        && value.m_value.m_handle == expected.m_value.m_handle
        && value.m_value.m_rawData == expected.m_value.m_rawData
        && value.m_valueString == expected.m_valueString
        && value.m_valueStringLength == expected.m_valueStringLength
        && value.m_childHandles == expected.m_childHandles
        && value.m_objectHandles == expected.m_objectHandles
        && value.m_childValues.size() == expected.m_childValues.size();
}

bool fieldListStateMatches(const DRW_FieldList& value,
                           const DRW_FieldList& expected) {
    return value.tType == expected.tType && value.handle == expected.handle
        && value.parentHandle == expected.parentHandle
        && value.name == expected.name && value.flags == expected.flags
        && value.reactorHandles == expected.reactorHandles
        && value.xDictHandle == expected.xDictHandle
        && value.m_unknown == expected.m_unknown
        && value.m_fieldHandles == expected.m_fieldHandles;
}

enum class FailureTarget { Field, FieldList };

class Export final : public dx_iface {
public:
    Export(dxfRW& codec, FailureTarget target, long failAt)
        : codec(codec), target(target), failAt(failAt), field(makeField()),
          fieldList(makeFieldList()) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &codec;
    }

    void writeObjects() override {
        if (!writeOne(FailureTarget::Field, [&] {
                return codec.writeField(&field);
            }))
            return;
        writeOne(FailureTarget::FieldList, [&] {
            return codec.writeFieldList(&fieldList);
        });
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    FailureTarget target;
    long failAt;
    long reached {0};
    bool accepted {true};
    dx_data storage;
    DRW_Field field;
    DRW_FieldList fieldList;

private:
    template <typename Write>
    bool writeOne(FailureTarget current, Write write) {
        const bool inject = current == target;
        if (inject)
            armAllocationFailure(failAt);
        try {
            accepted = write();
        } catch (const std::bad_alloc&) {
            accepted = false;
        }
        if (inject) {
            reached = static_cast<long>(allocationAttempts);
            disarmAllocationFailure();
        }
        return accepted;
    }
};

class Readback final : public dx_iface {
public:
    Readback() {
        cData = &storage;
        currentBlock = storage.mBlock;
    }
    void addField(const DRW_Field& value) override { fields.push_back(value); }
    void addFieldList(const DRW_FieldList& value) override {
        fieldLists.push_back(value);
    }
    dx_data storage;
    std::vector<DRW_Field> fields;
    std::vector<DRW_FieldList> fieldLists;
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
    const DRW_Field expectedField = makeField();
    const DRW_FieldList expectedFieldList = makeFieldList();
    return codec.read(&reader, false) && reader.fields.size() == 1u
        && reader.fieldLists.size() == 1u
        && fieldStateMatches(reader.fields.front(), expectedField)
        && fieldListStateMatches(reader.fieldLists.front(), expectedFieldList);
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "FIELD/FIELDLIST OBJECTS allocation retry: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const auto output = directory / "field-objects-allocation.dxf";
    const std::string sentinel = "prior FIELD/FIELDLIST destination bytes\n";
    const DRW::Version versions[] = {
        DRW::AC1018, DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032};
    for (const DRW::Version version : versions) {
      for (const bool binary : {false, true}) {
        for (const FailureTarget target :
                {FailureTarget::Field, FailureTarget::FieldList}) {
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                Export exporter(codec, target, 1000000);
                const bool wrote = codec.write(&exporter, version, binary);
                if (!wrote || !exporter.accepted || exporter.reached <= 0
                    || !readbackMatches(output))
                    return fail("calibration write or readback failed");
                allocationCount = exporter.reached;
                if (!readBytes(output, expectedBytes))
                    return fail("could not retain calibration output bytes");
            }

            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                        std::ios::binary | std::ios::trunc);
                    if (!prior || !(prior << sentinel))
                        return fail("could not initialize destination sentinel");
                }
                dxfRW codec(output.string().c_str());
                Export exporter(codec, target, offset);
                const DRW_Field originalField = exporter.field;
                const DRW_FieldList originalFieldList = exporter.fieldList;
                const bool written = codec.write(&exporter, version, binary);
                std::string current;
                if (written || exporter.accepted || exporter.reached != offset + 1
                    || !allocationFailed || !readBytes(output, current)
                    || current != sentinel)
                    return fail("injected failure changed destination or was missed");
                if (!fieldStateMatches(exporter.field, originalField)
                    || !fieldListStateMatches(exporter.fieldList,
                                              originalFieldList))
                    return fail("injected failure changed caller object state");
                exporter.failAt = -1;
                exporter.accepted = true;
                const bool retry = codec.write(&exporter, version, binary);
                std::string retryBytes;
                if (!retry || !exporter.accepted
                    || !readBytes(output, retryBytes)
                    || retryBytes != expectedBytes || !readbackMatches(output))
                    return fail("same-codec retry or typed readback failed");
            }
            std::cout << (target == FailureTarget::Field ? "FIELD" : "FIELDLIST")
                      << ' ' << (binary ? "Binary" : "ASCII")
                      << " version=" << static_cast<int>(version)
                      << ": " << allocationCount
                      << " allocation failures, destination rollback/retry/readback PASS\n";
        }
      }
    }
    std::error_code ignored;
    std::filesystem::remove(output, ignored);
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (failAllocation())
        throw std::bad_alloc();
    if (void* value = std::malloc(size == 0u ? 1u : size))
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
        std::cerr << "usage: field_objects_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
