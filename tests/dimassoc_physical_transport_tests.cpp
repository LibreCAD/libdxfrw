#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <new>
#include <sstream>
#include <string>
#include <vector>

#include "drw_base.h"
#include "drw_header.h"
#define private public
#include "libdxfrw.h"
#include "dx_data.h"
#undef private
#include "dx_iface.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"
#include "libdwgr.h"
#include "lc3_compat_check.cpp"

namespace {

long allocationsBeforeFailure = -1;
std::size_t failedAllocations = 0;

struct AllocationFailure {
    explicit AllocationFailure(long count) { allocationsBeforeFailure = count; }
    ~AllocationFailure() { allocationsBeforeFailure = -1; }
};

class DimensionAssociationProbe final : public DRW_DimensionAssociation {
public:
    using DRW_DimensionAssociation::parseCode;

    std::uint8_t extensionDictionaryFlag() const noexcept {
        return xDictFlag;
    }
    std::int32_t reactorCount() const noexcept { return numReactors; }
};

class DimensionAssociationCapture final : public dx_iface {
public:
    void attachData(dx_data& data) {
        cData = &data;
        currentBlock = data.mBlock;
        data.sourceFormat = dx_source_format::DWG;
        data.importGeneration = 1u;
    }

    void addDimensionAssociation(
            const DRW_DimensionAssociation& association) override {
        DimensionAssociationProbe copy;
        static_cast<DRW_DimensionAssociation&>(copy) = association;
        associations.push_back(std::move(copy));
    }

    std::vector<DimensionAssociationProbe> associations;
};

class DimensionAssociationAuthor final : public dx_iface {
public:
    explicit DimensionAssociationAuthor(dwgRW& output) : output(output) {
        cData = &storage;
        currentBlock = storage.mBlock;
        association.handle = 0xA300u;
        association.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        association.reactorHandles = {DRW::DwgNamedObjectsDictionaryHandle};
        association.m_dimensionHandle = 0x30u;
        association.m_associativityFlags = 0x07u;
        association.m_isTransSpace = true;
        association.m_rotatedDimensionType = 4u;

        DRW_DimensionAssociationOsnapRef point;
        point.m_className = "AcDbOsnapPointRef";
        point.m_hasObjectOsnapType = true;
        point.m_objectOsnapType = 1u;
        point.m_objectHandles = {0x40u, 0x44u};
        point.m_objectHandle = point.m_objectHandles.front();
        point.m_hasSubentType = true;
        point.m_subentType = 2;
        point.m_hasGsMarker = true;
        point.m_gsMarker = 9;
        point.m_hasNearParameter = true;
        point.m_nearParameter = 1.0;
        point.m_hasOsnapPoint = true;
        point.m_osnapPoint = {1.25, -2.5, 2.0e50};
        point.m_hasLastPointRef = true;
        point.m_lastPointRef = 0;
        association.m_osnapRefs.push_back(point);

        DRW_DimensionAssociationOsnapRef intersection;
        intersection.m_className = "AcDbOsnapPointRef";
        intersection.m_hasObjectOsnapType = true;
        intersection.m_objectOsnapType = 6u;
        intersection.m_objectHandle = 0x41u;
        intersection.m_hasSubentType = true;
        intersection.m_subentType = 3;
        intersection.m_hasGsMarker = true;
        intersection.m_gsMarker = 10;
        intersection.m_hasNearParameter = true;
        intersection.m_nearParameter = 0.04914376136317;
        intersection.m_hasOsnapPoint = true;
        intersection.m_osnapPoint = {-3.0, 4.5, 6.0};
        intersection.m_hasIntersectionObjectHandle = true;
        intersection.m_intersectionObjectHandle = 0x50u;
        intersection.m_hasIntersectionSubentType = true;
        intersection.m_intersectionSubentType = 5;
        intersection.m_hasIntersectionGsMarker = true;
        intersection.m_intersectionGsMarker = 0;
        intersection.m_hasIntersectionXrefObjectHandleString = true;
        intersection.m_intersectionXrefObjectHandleString =
            "xref-intersection-6";
        intersection.m_hasLastPointRef = true;
        intersection.m_lastPointRef = 0;
        association.m_osnapRefs.push_back(intersection);

        DRW_DimensionAssociationOsnapRef xref;
        xref.m_className = "AcDbOsnapPointRef";
        xref.m_hasObjectOsnapType = true;
        xref.m_objectOsnapType = 13u;
        xref.m_objectHandle = 0x42u;
        xref.m_hasSubentType = true;
        xref.m_subentType = 4;
        xref.m_hasGsMarker = true;
        xref.m_gsMarker = 12;
        xref.m_hasXrefObjectHandleString = true;
        xref.m_xrefObjectHandleString = "xref-main-13";
        xref.m_hasNearParameter = true;
        xref.m_nearParameter = 2.0;
        xref.m_hasOsnapPoint = true;
        xref.m_osnapPoint = {7.0, 8.0, 9.0};
        xref.m_hasLastPointRef = true;
        xref.m_lastPointRef = 0;
        association.m_osnapRefs.push_back(xref);
    }

    void writeHeader(DRW_Header& header) override { header.vars.clear(); }
    void writeDwgClasses() override {
        classRegistered = output.registerDimensionAssociationObjectClass(
            &association);
    }
    void writeObjects() override {
        objectWritten = output.writeDimensionAssociation(&association);
    }
    bool finalizeDwgWrite() override {
        return classRegistered && objectWritten;
    }
    bool finalizeDwgWriteStructure() override { return true; }

    dwgRW& output;
    dx_data storage;
    DRW_DimensionAssociation association;
    bool classRegistered = false;
    bool objectWritten = false;
};

class DimensionAssociationDxfExport final : public LC3CompatCheck {
public:
    DimensionAssociationDxfExport(dxfRW& output,
                                  const DimensionAssociationProbe& source)
        : output(output), source(source) {}

    dxfRW& output;
    DimensionAssociationProbe source;
    long failAt = -1;
    long reached = 0;
    bool emitted = false;

    void writeObjects() override {
        auto emit = [this]() {
            emitted = output.writeDimensionAssociation(&source);
        };
        if (failAt < 0) {
            emit();
            return;
        }

        const std::size_t failuresBefore = failedAllocations;
        try {
            AllocationFailure failure(failAt);
            emit();
            reached = failAt - allocationsBeforeFailure;
        } catch (...) {
            reached = static_cast<long>(failedAllocations - failuresBefore);
            throw;
        }
    }
};

bool sameVariant(const DRW_Variant& left, const DRW_Variant& right) {
    if (left.type() != right.type() || left.code() != right.code())
        return false;
    switch (left.type()) {
    case DRW_Variant::STRING:
        return std::string(left.c_str()) == std::string(right.c_str());
    case DRW_Variant::INTEGER:
        return left.i_val() == right.i_val();
    case DRW_Variant::INTEGER64:
        return left.i64_val() == right.i64_val();
    case DRW_Variant::DOUBLE:
        return std::abs(left.d_val() - right.d_val())
            <= 1e-12 * std::max(1.0, std::abs(left.d_val()));
    case DRW_Variant::COORD: {
        const DRW_Coord* a = left.coord();
        const DRW_Coord* b = right.coord();
        return a != nullptr && b != nullptr && a->x == b->x
            && a->y == b->y && a->z == b->z;
    }
    case DRW_Variant::BINARY:
        return *left.binary() == *right.binary();
    case DRW_Variant::INVALID:
        return true;
    }
    return false;
}

bool sameExtendedData(const std::vector<DRW_Variant*>& left,
                      const std::vector<DRW_Variant*>& right) {
    if (left.size() != right.size())
        return false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (left[i] == nullptr || right[i] == nullptr
            || !sameVariant(*left[i], *right[i]))
            return false;
    }
    return true;
}

bool isReactorEnvelope(const std::list<DRW_Variant>& group) {
    return !group.empty() && group.front().code() == 102
        && group.front().type() == DRW_Variant::STRING
        && std::string(group.front().c_str()) == "ACAD_REACTORS";
}

bool sameAppData(const std::list<std::list<DRW_Variant>>& left,
                 const std::list<std::list<DRW_Variant>>& right) {
    std::vector<const std::list<DRW_Variant>*> leftGroups;
    std::vector<const std::list<DRW_Variant>*> rightGroups;
    for (const auto& group : left) {
        if (!isReactorEnvelope(group))
            leftGroups.push_back(&group);
    }
    for (const auto& group : right) {
        if (!isReactorEnvelope(group))
            rightGroups.push_back(&group);
    }
    if (leftGroups.size() != rightGroups.size())
        return false;
    for (std::size_t groupIndex = 0; groupIndex < leftGroups.size();
         ++groupIndex) {
        const auto& leftGroup = *leftGroups[groupIndex];
        const auto& rightGroup = *rightGroups[groupIndex];
        if (leftGroup.size() != rightGroup.size())
            return false;
        auto leftValue = leftGroup.begin();
        auto rightValue = rightGroup.begin();
        for (; leftValue != leftGroup.end(); ++leftValue, ++rightValue) {
            if (!sameVariant(*leftValue, *rightValue))
                return false;
        }
    }
    return true;
}

std::vector<std::uint32_t> mainObjectHandles(
        const DRW_DimensionAssociationOsnapRef& ref) {
    if (!ref.m_objectHandles.empty())
        return ref.m_objectHandles;
    return ref.m_objectHandle != 0
        ? std::vector<std::uint32_t>{ref.m_objectHandle}
        : std::vector<std::uint32_t>{};
}

bool sameRef(const DRW_DimensionAssociationOsnapRef& source,
             const DRW_DimensionAssociationOsnapRef& output,
             const std::map<std::uint32_t, std::uint32_t>& remap) {
    const auto mapped = [&remap](std::uint32_t handle) {
        if (handle == 0)
            return std::uint32_t{0};
        const auto found = remap.find(handle);
        return found == remap.end() ? handle : found->second;
    };
    const std::vector<std::uint32_t> sourceHandles =
        mainObjectHandles(source);
    const std::vector<std::uint32_t> outputHandles =
        mainObjectHandles(output);
    if (sourceHandles.size() != outputHandles.size())
        return false;
    for (std::size_t i = 0; i < sourceHandles.size(); ++i) {
        if (mapped(sourceHandles[i]) != outputHandles[i])
            return false;
    }
    return source.m_className == output.m_className
        && source.m_objectOsnapType == output.m_objectOsnapType
        && source.m_hasObjectOsnapType == output.m_hasObjectOsnapType
        && mapped(source.m_objectHandle) == output.m_objectHandle
        && source.m_hasSubentType == output.m_hasSubentType
        && source.m_subentType == output.m_subentType
        && source.m_hasGsMarker == output.m_hasGsMarker
        && source.m_gsMarker == output.m_gsMarker
        && source.m_hasNearParameter == output.m_hasNearParameter
        && (!source.m_hasNearParameter
            || std::abs(source.m_nearParameter - output.m_nearParameter)
                <= 1e-12 * std::max(1.0, std::abs(source.m_nearParameter)))
        && source.m_hasOsnapPoint == output.m_hasOsnapPoint
        && (!source.m_hasOsnapPoint
            || (source.m_osnapPoint.x == output.m_osnapPoint.x
                && source.m_osnapPoint.y == output.m_osnapPoint.y
                && source.m_osnapPoint.z == output.m_osnapPoint.z))
        && source.m_hasLastPointRef == output.m_hasLastPointRef
        && source.m_lastPointRef == output.m_lastPointRef
        && source.m_hasIntersectionObjectHandle
            == output.m_hasIntersectionObjectHandle
        && mapped(source.m_intersectionObjectHandle)
            == output.m_intersectionObjectHandle
        && source.m_hasIntersectionSubentType
            == output.m_hasIntersectionSubentType
        && source.m_intersectionSubentType
            == output.m_intersectionSubentType
        && source.m_hasIntersectionGsMarker
            == output.m_hasIntersectionGsMarker
        && source.m_intersectionGsMarker == output.m_intersectionGsMarker
        && source.m_hasXrefObjectHandleString
            == output.m_hasXrefObjectHandleString
        && source.m_xrefObjectHandleString == output.m_xrefObjectHandleString
        && source.m_hasIntersectionXrefObjectHandleString
            == output.m_hasIntersectionXrefObjectHandleString
        && source.m_intersectionXrefObjectHandleString
            == output.m_intersectionXrefObjectHandleString;
}

bool sameAssociation(const DimensionAssociationProbe& source,
                     const DimensionAssociationProbe& output,
                     const std::map<std::uint32_t, std::uint32_t>& remap) {
    const auto mapped = [&remap](std::uint32_t handle) {
        if (handle == 0)
            return std::uint32_t{0};
        const auto found = remap.find(handle);
        return found == remap.end() ? handle : found->second;
    };
    if (mapped(source.handle) != output.handle
        || mapped(source.parentHandle) != output.parentHandle
        || source.name != output.name
        || source.flags != output.flags
        || mapped(source.xDictHandle) != output.xDictHandle
        || source.reactorHandles.size() != output.reactorHandles.size()
        || !sameExtendedData(source.extData, output.extData)
        || !sameAppData(source.appData, output.appData)
        || source.m_dimensionHandle == 0
        || mapped(source.m_dimensionHandle) != output.m_dimensionHandle
        || source.m_associativityFlags != output.m_associativityFlags
        || source.m_isTransSpace != output.m_isTransSpace
        || source.m_rotatedDimensionType != output.m_rotatedDimensionType
        || source.m_osnapRefs.size() != output.m_osnapRefs.size()
        || output.m_hasUnrepresentableDetail)
        return false;
    for (std::size_t i = 0; i < source.reactorHandles.size(); ++i) {
        if (mapped(source.reactorHandles[i]) != output.reactorHandles[i])
            return false;
    }
    for (std::size_t i = 0; i < source.m_osnapRefs.size(); ++i) {
        if (!sameRef(source.m_osnapRefs[i], output.m_osnapRefs[i], remap))
            return false;
    }
    return true;
}

bool callerAssociationUnchanged(const DimensionAssociationProbe& actual,
                                const DimensionAssociationProbe& expected) {
    const std::map<std::uint32_t, std::uint32_t> noRemap;
    return sameAssociation(actual, expected, noRemap)
        && actual.extensionDictionaryFlag()
            == expected.extensionDictionaryFlag()
        && actual.reactorCount() == expected.reactorCount();
}

std::map<std::uint32_t, std::uint32_t> makeHandleRemap(
        const DRW_DimensionAssociation& association) {
    std::map<std::uint32_t, std::uint32_t> result;
    std::uint32_t next = 0xE0000001u;
    const auto add = [&result, &next](std::uint32_t handle) {
        if (handle == 0 || handle == DRW::NoHandle
            || result.count(handle) != 0)
            return;
        result.emplace(handle, next++);
    };
    add(association.handle);
    add(association.parentHandle);
    add(association.xDictHandle);
    add(association.m_dimensionHandle);
    for (const std::uint32_t reactor : association.reactorHandles)
        add(reactor);
    for (const auto& ref : association.m_osnapRefs) {
        for (std::uint32_t handle : mainObjectHandles(ref))
            add(handle);
        if (ref.m_hasIntersectionObjectHandle)
            add(ref.m_intersectionObjectHandle);
    }
    return result;
}

bool emitAndReadback(const DimensionAssociationProbe& source,
                     DRW::Version version, bool binary) {
    const auto remap = makeHandleRemap(source);
    std::ostringstream bytes;
    dxfRW writer("");
    writer.version = version;
    writer.binFile = binary;
    writer.setHandleRemap(remap);
    if (binary)
        writer.writer = std::make_unique<dxfWriterBinary>(&bytes);
    else
        writer.writer = std::make_unique<dxfWriterAscii>(&bytes);

    auto mutableSource = source;
    if (!writer.writeDimensionAssociation(&mutableSource)
        || !writer.writer->writeString(0, "WIRE_END")
        || writer.writer->hasWriteError()) {
        std::cerr << "write-failed format=" << (binary ? "binary" : "ascii")
                  << " handle=" << std::hex << source.handle << std::dec
                  << '\n';
        return false;
    }
    const DRW_WritingContext* writeContext = writer.getWritingContext();
    if (writeContext == nullptr
        || writeContext->sourceHandleToWrittenObjectMap.count(source.handle)
            != 1u
        || writeContext->sourceHandleToWrittenObjectMap.at(source.handle)
            != writer.remapHandle(source.handle)) {
        std::cerr << "DIMASSOC source identity was not registered handle="
                  << std::hex << source.handle << std::dec << '\n';
        return false;
    }

    std::istringstream input(bytes.str());
    std::unique_ptr<dxfReader> reader;
    if (binary)
        reader = std::make_unique<dxfReaderBinary>(&input);
    else
        reader = std::make_unique<dxfReaderAscii>(&input);
    DimensionAssociationProbe output;
    int code = 0;
    if (!reader->readRec(&code) || code != 0
        || reader->getString() != "DIMASSOC")
        return false;
    bool parsed = true;
    bool ended = false;
    while (reader->readRec(&code)) {
        if (code == 0) {
            ended = reader->getString() == "WIRE_END";
            break;
        }
        if (!output.parseCode(code, reader)) {
            parsed = false;
            break;
        }
    }
    if (!parsed || !ended) {
        std::cerr << "parse-failed format=" << (binary ? "binary" : "ascii")
                  << " handle=" << std::hex << source.handle << std::dec
                  << " ended=" << ended << '\n';
        return false;
    }
    if (!sameAssociation(source, output, remap)) {
        std::cerr << "compare-failed format=" << (binary ? "binary" : "ascii")
                  << " handle=" << std::hex << source.handle << std::dec
                  << " srcParent=" << source.parentHandle
                  << " outParent=" << output.parentHandle
                  << " srcName=" << source.name
                  << " outName=" << output.name
                  << " srcFlags=" << source.flags
                  << " outFlags=" << output.flags
                  << " srcXDict=" << source.xDictHandle
                  << " outXDict=" << output.xDictHandle
                  << " srcXDictFlag=" << static_cast<int>(
                         source.extensionDictionaryFlag())
                  << " outXDictFlag=" << static_cast<int>(
                         output.extensionDictionaryFlag())
                  << " srcNumReactors=" << source.reactorCount()
                  << " outNumReactors=" << output.reactorCount()
                  << " srcDim=" << source.m_dimensionHandle
                  << " outDim=" << output.m_dimensionHandle
                  << " srcReactors=" << source.reactorHandles.size()
                  << " outReactors=" << output.reactorHandles.size()
                  << " srcXData=" << source.extData.size()
                  << " outXData=" << output.extData.size()
                  << " srcAppData=" << source.appData.size()
                  << " outAppData=" << output.appData.size()
                  << " srcRefs=" << source.m_osnapRefs.size()
                  << " outRefs=" << output.m_osnapRefs.size()
                  << " unrepresentable="
                  << output.m_hasUnrepresentableDetail << '\n';
        return false;
    }
    return true;
}

bool allocationRollbackRetry(DRW::Version version, bool binary) {
    dwgRW unusedDwgWriter("");
    DimensionAssociationAuthor authored(unusedDwgWriter);
    DimensionAssociationProbe source;
    static_cast<DRW_DimensionAssociation&>(source) = authored.association;
    const auto remap = makeHandleRemap(source);
    const std::string sentinel = "DIMASSOC-RECORD-SENTINEL";

    const auto outputPath = std::filesystem::temp_directory_path()
        / ("libdxfrw-dimassoc-allocation-retry-"
           + std::to_string(static_cast<int>(version))
           + (binary ? "-binary.dxf" : "-ascii.dxf"));
    const std::string outputPathString = outputPath.string();
    const auto writeSentinel = [&]() {
        std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
        output.write(sentinel.data(),
                     static_cast<std::streamsize>(sentinel.size()));
        return output.good();
    };
    const auto destinationIsSentinel = [&]() {
        std::ifstream input(outputPath, std::ios::binary);
        const std::string value((std::istreambuf_iterator<char>(input)),
                                std::istreambuf_iterator<char>());
        return input.good() || input.eof() ? value == sentinel : false;
    };
    const auto readbackIsExact = [&]() {
        dx_data data;
        DimensionAssociationCapture capture;
        const bool imported = capture.fileImport(outputPathString, &data,
                                                  false);
        return imported && capture.associations.size() == 1u
            && sameAssociation(source, capture.associations.front(), remap);
    };

    if (!writeSentinel()) {
        std::cerr << "could not seed DIMASSOC destination sentinel\n";
        return false;
    }
    dxfRW calibrationCodec(outputPathString.c_str());
    calibrationCodec.setHandleRemap(remap);
    DimensionAssociationDxfExport calibration(calibrationCodec, source);
    calibration.failAt = 1000000;
    const bool calibrated = calibrationCodec.write(
        &calibration, version, binary);
    const long allocationCount = calibration.reached;
    if (!calibrated || !calibration.emitted || allocationCount <= 0) {
        std::remove(outputPathString.c_str());
        std::cerr << "DIMASSOC allocation calibration failed; allocations="
                  << allocationCount << '\n';
        return false;
    }
    if (!callerAssociationUnchanged(calibration.source, source)) {
        std::remove(outputPathString.c_str());
        std::cerr << "DIMASSOC clean write changed the caller record\n";
        return false;
    }
    std::string cleanBytes;
    {
        std::ifstream input(outputPath, std::ios::binary);
        cleanBytes.assign(std::istreambuf_iterator<char>(input),
                          std::istreambuf_iterator<char>());
        if (!input.good() && !input.eof()) {
            std::remove(outputPathString.c_str());
            std::cerr << "could not read clean DIMASSOC allocation output\n";
            return false;
        }
    }

    std::size_t failures = 0;
    for (long offset = 0; offset < allocationCount; ++offset) {
        if (!writeSentinel()) {
            std::remove(outputPathString.c_str());
            return false;
        }
        dxfRW codec(outputPathString.c_str());
        codec.setHandleRemap(remap);
        DimensionAssociationDxfExport exporter(codec, source);
        exporter.failAt = offset;
        bool wrote = false;
        const std::size_t before = failedAllocations;
        try {
            wrote = codec.write(&exporter, version, binary);
        } catch (const std::bad_alloc&) {
            allocationsBeforeFailure = -1;
        }
        allocationsBeforeFailure = -1;
        if (wrote || failedAllocations == before
            || !destinationIsSentinel()
            || !callerAssociationUnchanged(exporter.source, source)
            || codec.getWritingContext() == nullptr
            || codec.getWritingContext()->sourceHandleToWrittenObjectMap
                    .count(source.handle) != 0u) {
            std::remove(outputPathString.c_str());
            std::cerr << "DIMASSOC " << (binary ? "binary" : "ASCII")
                      << " allocation offset=" << offset
                      << " wrote=" << wrote
                      << " failedAllocations=" << failedAllocations - before
                      << " destinationPreserved="
                      << destinationIsSentinel()
                      << " callerPreserved="
                      << callerAssociationUnchanged(exporter.source, source)
                      << " identityRolledBack="
                      << (codec.getWritingContext() != nullptr
                          && codec.getWritingContext()
                              ->sourceHandleToWrittenObjectMap.count(
                                  source.handle) == 0u)
                      << '\n';
            return false;
        }
        exporter.failAt = -1;
        exporter.emitted = false;
        if (!codec.write(&exporter, version, binary)
            || !exporter.emitted
            || !callerAssociationUnchanged(exporter.source, source)
            || !readbackIsExact()
            || codec.getWritingContext() == nullptr
            || codec.getWritingContext()->sourceHandleToWrittenObjectMap
                    .count(source.handle) != 1u
            || codec.getWritingContext()->sourceHandleToWrittenObjectMap.at(
                   source.handle) != codec.remapHandle(source.handle)) {
            std::remove(outputPathString.c_str());
            std::cerr << "DIMASSOC corrected retry/readback failed at offset "
                      << offset << '\n';
            return false;
        }
        std::string retryBytes;
        {
            std::ifstream input(outputPath, std::ios::binary);
            retryBytes.assign(std::istreambuf_iterator<char>(input),
                              std::istreambuf_iterator<char>());
            if ((!input.good() && !input.eof()) || retryBytes != cleanBytes) {
                std::remove(outputPathString.c_str());
                std::cerr << "DIMASSOC retry bytes differ from clean output at offset "
                          << offset << '\n';
                return false;
            }
        }
        ++failures;
    }
    if (failures != static_cast<std::size_t>(allocationCount))
        return false;
    std::remove(outputPathString.c_str());
    std::cout << "DIMASSOC target=" << static_cast<int>(version) << ' '
              << (binary ? "binary" : "ASCII")
              << ": record-allocations=" << allocationCount
              << ", whole-file destination rollback and corrected same-writer"
                 " readback retries="
              << failures << '\n';
    return true;
}

bool hasResolvedEntityReferences(
        const DRW_DimensionAssociation& association,
        const std::map<std::uint32_t, DRW::ETYPE>& entities) {
    const auto dimension = entities.find(association.m_dimensionHandle);
    const bool dimensionType = dimension != entities.end()
        && (dimension->second == DRW::DIMENSION
            || dimension->second == DRW::DIMALIGNED
            || dimension->second == DRW::DIMLINEAR
            || dimension->second == DRW::DIMRADIAL
            || dimension->second == DRW::DIMDIAMETRIC
            || dimension->second == DRW::DIMANGULAR
            || dimension->second == DRW::DIMANGULAR3P
            || dimension->second == DRW::DIMORDINATE
            || dimension->second == DRW::DIMARC
            || dimension->second == DRW::LEADER
            || dimension->second == DRW::MLEADER);
    if (association.m_dimensionHandle == 0 || !dimensionType)
        return false;
    for (const auto& ref : association.m_osnapRefs) {
        const std::vector<std::uint32_t> handles = mainObjectHandles(ref);
        if (handles.empty() || ref.m_hasXrefObjectHandleString)
            return false;
        for (std::uint32_t handle : handles) {
            if (handle == 0 || entities.count(handle) == 0)
                return false;
        }
        if (ref.m_hasIntersectionObjectHandle
            && (entities.count(ref.m_intersectionObjectHandle) == 0
                || ref.m_hasIntersectionXrefObjectHandleString))
            return false;
    }
    return true;
}

bool runPhysicalSource(const std::string& path) {
    dx_data data;
    DimensionAssociationCapture capture;
    capture.attachData(data);
    dwgRW reader(path.c_str());
    const bool read = reader.read(&capture, false);
    if (!read) {
        std::cerr << "DWG read failed: error=" << reader.getError() << '\n';
        return false;
    }
    if (!data.dwgProxyResourceOrderCaptured
        || data.dwgProxyResourceOrderCaptureFailed
        || data.dwgProxyLayerNameOrder != reader.getLayerNameOrder()
        || data.dwgProxyLinetypeNameOrder != reader.getLtypeNameOrder()) {
        std::cerr << "DWG proxy resource order was not retained exactly"
                  << " captured=" << data.dwgProxyResourceOrderCaptured
                  << " failed=" << data.dwgProxyResourceOrderCaptureFailed
                  << " sourceFormat=" << static_cast<int>(data.sourceFormat)
                  << " generation=" << data.importGeneration
                  << " layers=" << data.dwgProxyLayerNameOrder.size() << '/'
                  << reader.getLayerNameOrder().size()
                  << " linetypes=" << data.dwgProxyLinetypeNameOrder.size()
                  << '/' << reader.getLtypeNameOrder().size() << '\n';
        return false;
    }

    std::map<std::uint32_t, DRW::ETYPE> entities;
    const auto collectBlock = [&entities](const dx_ifaceBlock* block) {
        if (block == nullptr)
            return;
        for (const DRW_Entity* entity : block->ent) {
            if (entity != nullptr && entity->handle != 0
                && entity->handle != DRW::NoHandle)
                entities.emplace(entity->handle, entity->eType);
        }
    };
    collectBlock(data.mBlock);
    for (const dx_ifaceBlock* block : data.blocks)
        collectBlock(block);

    std::size_t closed = 0;
    std::size_t ascii = 0;
    std::size_t binary = 0;
    std::size_t refused = 0;
    std::map<int, std::size_t> dimensionTargetTypes;
    for (const DimensionAssociationProbe& association
         : capture.associations) {
        const auto dimension = entities.find(association.m_dimensionHandle);
        if (dimension != entities.end())
            ++dimensionTargetTypes[static_cast<int>(dimension->second)];
        const bool targetsResolved =
            hasResolvedEntityReferences(association, entities);
        if (association.m_hasUnrepresentableDetail || !targetsResolved) {
            ++refused;
            std::cerr << "excluded handle=" << std::hex << association.handle
                      << " dimension=" << association.m_dimensionHandle
                      << " dimensionTargetFound="
                      << (dimension != entities.end())
                      << " dimensionTargetType="
                      << (dimension == entities.end()
                              ? -1 : static_cast<int>(dimension->second))
                      << " refs=" << std::dec
                      << association.m_osnapRefs.size()
                      << " unrepresentable="
                      << association.m_hasUnrepresentableDetail << '\n';
            for (const auto& ref : association.m_osnapRefs) {
                const std::vector<std::uint32_t> handles =
                    mainObjectHandles(ref);
                std::cerr << "  main=" << std::hex;
                for (std::uint32_t handle : handles)
                    std::cerr << handle << ',';
                std::cerr << " mainFound=";
                for (std::uint32_t handle : handles)
                    std::cerr << entities.count(handle) << ',';
                std::cerr
                          << " xref=" << ref.m_hasXrefObjectHandleString
                          << " intersection="
                          << ref.m_intersectionObjectHandle
                          << " intersectionFound="
                          << entities.count(ref.m_intersectionObjectHandle)
                          << " intersectionXref="
                          << ref.m_hasIntersectionXrefObjectHandleString
                          << std::dec << " class=" << ref.m_className
                          << " osnapType="
                          << static_cast<unsigned>(ref.m_objectOsnapType)
                          << " hasSubent=" << ref.m_hasSubentType
                          << " subent=" << ref.m_subentType
                          << " hasGs=" << ref.m_hasGsMarker
                          << " gs=" << ref.m_gsMarker
                          << " hasNear=" << ref.m_hasNearParameter
                          << " near=" << ref.m_nearParameter
                          << " hasPoint=" << ref.m_hasOsnapPoint
                          << " hasLast=" << ref.m_hasLastPointRef
                          << " last=" << ref.m_lastPointRef
                          << " hasInterSubent="
                          << ref.m_hasIntersectionSubentType
                          << " interSubent=" << ref.m_intersectionSubentType
                          << " hasInterGs="
                          << ref.m_hasIntersectionGsMarker
                          << " interGs=" << ref.m_intersectionGsMarker
                          << " hasMainXref="
                          << ref.m_hasXrefObjectHandleString
                          << " mainXrefPath="
                          << ref.m_xrefObjectHandleString
                          << " hasInterXref="
                          << ref.m_hasIntersectionXrefObjectHandleString
                          << " interXrefPath="
                          << ref.m_intersectionXrefObjectHandleString
                          << std::dec << '\n';
            }
            continue;
        }
        ++closed;
        if (emitAndReadback(association, reader.getVersion(), false))
            ++ascii;
        if (emitAndReadback(association, reader.getVersion(), true))
            ++binary;
    }

    std::cout << "sourceVersion=" << reader.getVersion()
              << " entityParseFailures="
              << reader.getReadObservations().entityParseFailures
              << " objectParseFailures="
              << reader.getReadObservations().objectParseFailures
              << " callbacks=" << capture.associations.size()
              << " typedEntityHandles=" << entities.size()
              << " resolvedRepresentable=" << closed
              << " refusedOrUnresolved=" << refused
              << " asciiReadback=" << ascii
              << " binaryReadback=" << binary
              << " dimensionTargetTypes=";
    for (const auto& [type, count] : dimensionTargetTypes)
        std::cout << type << ':' << count << ',';
    std::cout << '\n';
    return closed != 0 && ascii == closed && binary == closed;
}

bool runStaleGenerationOutput(const std::string& outputPath) {
    dx_data data;
    data.sourceFormat = dx_source_format::DXF;
    data.sourceVersion = DRW::AC1024;
    data.importGeneration = 2u;
    DRW_DimensionAssociation stale;
    stale.handle = 0x900u;
    data.dwgDimensionAssociations.emplace(
        dx_data::DimensionAssociationKey{1u, stale.handle}, stale);
    data.dimensionAssociationsRetainedRecords = 1u;

    dx_iface exporter;
    if (!exporter.fileExport(outputPath, DRW::AC1024, false, &data, false)) {
        std::cerr << "stale DIMASSOC generation poisoned current DXF export\n";
        return false;
    }
    std::ifstream input(outputPath, std::ios::binary);
    const std::string contents((std::istreambuf_iterator<char>(input)),
                               std::istreambuf_iterator<char>());
    std::remove(outputPath.c_str());
    if (contents.empty()
        || contents.find("0\nDIMASSOC\n") != std::string::npos) {
        std::cerr << "stale DIMASSOC generation was emitted or output is empty\n";
        return false;
    }
    return true;
}

bool runAdapterClosureRetryRoundtrip(const std::string& outputPath,
                                     bool binary) {
    dx_data data;
    data.sourceFormat = dx_source_format::DWG;
    data.sourceVersion = DRW::AC1024;
    data.importGeneration = 1u;
    data.dwgReadOutcome.attempted = true;
    data.dwgReadOutcome.operationSucceeded = true;
    data.dwgReadOutcome.sourceVersion = DRW::AC1024;
    data.hasNamedObjectDictionary = true;
    data.namedObjectDictionary.handle = 0x60u;
    DRW_Dictionary associationOwner;
    associationOwner.handle = 0x61u;
    associationOwner.parentHandle = data.namedObjectDictionary.handle;
    data.namedObjectDictionary.m_entries.push_back(
        {"ACAD_DIMASSOC", associationOwner.handle});
    data.dictionaries.push_back(associationOwner);

    auto* line = new DRW_Line;
    line->handle = 0x40u;
    line->basePoint = DRW_Coord(0.0, 0.0, 0.0);
    line->secPoint = DRW_Coord(10.0, 0.0, 0.0);
    data.mBlock->ent.push_back(line);
    auto* dimension = new DRW_DimAligned;
    dimension->handle = 0x30u;
    dimension->setName("*D1");
    dimension->setStyle("Standard");
    dimension->setDimPoint(DRW_Coord(5.0, 2.0, 0.0));
    dimension->setTextPoint(DRW_Coord(5.0, 3.0, 0.0));
    dimension->setDef1Point(DRW_Coord(0.0, 0.0, 0.0));
    dimension->setDef2Point(DRW_Coord(10.0, 0.0, 0.0));
    data.mBlock->ent.push_back(dimension);

    DRW_DimensionAssociation association;
    association.handle = 0x500u;
    association.parentHandle = associationOwner.handle;
    association.reactorHandles = {associationOwner.handle};
    association.xDictHandle = associationOwner.handle;
    association.setDwgCommonObjectState(1, 1u, false);
    association.m_dimensionHandle = dimension->handle;
    association.m_associativityFlags = 1u;
    association.m_osnapRefs.resize(1u);
    DRW_DimensionAssociationOsnapRef& ref =
        association.m_osnapRefs.front();
    ref.m_className = "AcDbOsnapPointRef";
    ref.m_objectOsnapType = 1u;
    ref.m_hasObjectOsnapType = true;
    ref.m_objectHandle = 0x41u; // invalid until the correction below
    ref.m_objectHandles = {0x41u};
    ref.m_hasSubentType = true;
    ref.m_subentType = 1;
    ref.m_hasGsMarker = true;
    ref.m_gsMarker = 0;
    ref.m_hasNearParameter = true;
    ref.m_nearParameter = 0.5;
    ref.m_hasOsnapPoint = true;
    ref.m_osnapPoint = DRW_Coord(5.0, 0.0, 0.0);
    ref.m_hasLastPointRef = true;
    ref.m_lastPointRef = 0;
    data.dwgDimensionAssociations.emplace(
        dx_data::DimensionAssociationKey{data.importGeneration,
                                         association.handle},
        association);
    data.dimensionAssociationsRetainedRecords = 1u;

    const dx_data::StandardDwgFrameKey key{
        data.importGeneration, association.handle};
    DRW_UnsupportedObject rawFrame;
    rawFrame.m_objectType = 1;
    rawFrame.m_handle = association.handle;
    rawFrame.m_version = DRW::AC1024;
    rawFrame.m_objectSize = 1u;
    rawFrame.m_rawBytes = {0u};
    data.m_opaqueDwgSourceFrames.emplace(key, std::move(rawFrame));
    data.m_opaqueDwgSourceFrameIdentities.insert(key);
    data.dwgReadOutcome.opaqueSourceFramesCaptured = 1u;

    dx_iface exporter;
    std::remove(outputPath.c_str());
    const std::string sentinel = "prior destination must survive refusal\n";
    {
        std::ofstream output(outputPath, std::ios::binary);
        output << sentinel;
    }
    auto& capturedAssociation =
        data.dwgDimensionAssociations.begin()->second;
    const auto refusalPreservesDestination = [&]() {
        if (exporter.fileExport(outputPath, DRW::AC1024, binary, &data,
                                false))
            return false;
        std::ifstream input(outputPath, std::ios::binary);
        const std::string retained(
            (std::istreambuf_iterator<char>(input)),
            std::istreambuf_iterator<char>());
        return retained == sentinel;
    };
    const auto rejectsInvalidCommonLink = [&](auto invalidate) {
        const DRW_DimensionAssociation saved = capturedAssociation;
        invalidate(capturedAssociation);
        const bool refused = refusalPreservesDestination();
        capturedAssociation = saved;
        return refused;
    };
    if (!rejectsInvalidCommonLink([](DRW_DimensionAssociation& value) {
            value.parentHandle = 0x999u;
        })
        || !rejectsInvalidCommonLink([](DRW_DimensionAssociation& value) {
            value.parentHandle = 0x40u; // emitted LINE, not DICTIONARY
        })
        || !rejectsInvalidCommonLink([](DRW_DimensionAssociation& value) {
            value.reactorHandles = {0x999u};
            value.setDwgCommonObjectState(1, 1u, false);
        })
        || !rejectsInvalidCommonLink([](DRW_DimensionAssociation& value) {
            value.setDwgCommonObjectState(0, 1u, false);
        })
        || !rejectsInvalidCommonLink([](DRW_DimensionAssociation& value) {
            value.xDictHandle = 0x999u;
        })) {
        std::remove(outputPath.c_str());
        std::cerr << "adapter accepted an unclosed DIMASSOC common link or "
                     "changed the prior destination\n";
        return false;
    }
    if (exporter.fileExport(outputPath, DRW::AC1024, binary, &data, false)) {
        std::remove(outputPath.c_str());
        std::cerr << "adapter accepted an absent DIMASSOC entity target\n";
        return false;
    }
    {
        std::ifstream input(outputPath, std::ios::binary);
        const std::string retained((std::istreambuf_iterator<char>(input)),
                                   std::istreambuf_iterator<char>());
        if (retained != sentinel) {
            std::remove(outputPath.c_str());
            std::cerr << "DIMASSOC refusal changed the prior destination\n";
            return false;
        }
    }

    data.dwgDimensionAssociations.begin()->second.m_osnapRefs.front()
        .m_objectHandle = line->handle;
    data.dwgDimensionAssociations.begin()->second.m_osnapRefs.front()
        .m_objectHandles = {line->handle};
    if (!exporter.fileExport(outputPath, DRW::AC1024, binary, &data, false)) {
        std::cerr << "adapter same-instance retry rejected the corrected graph\n";
        return false;
    }

    dx_data readbackData;
    DimensionAssociationCapture readback;
    const bool imported = readback.fileImport(outputPath, &readbackData, false);
    std::map<std::uint32_t, DRW::ETYPE> outputEntities;
    for (const DRW_Entity* entity : readbackData.mBlock->ent) {
        if (entity != nullptr)
            outputEntities.emplace(entity->handle, entity->eType);
    }
    const auto dimensionType = [&outputEntities](std::uint32_t handle) {
        const auto found = outputEntities.find(handle);
        return found != outputEntities.end()
            && (found->second == DRW::DIMENSION
                || found->second == DRW::DIMALIGNED
                || found->second == DRW::DIMLINEAR
                || found->second == DRW::DIMRADIAL
                || found->second == DRW::DIMDIAMETRIC
                || found->second == DRW::DIMANGULAR
                || found->second == DRW::DIMANGULAR3P
                || found->second == DRW::DIMORDINATE
                || found->second == DRW::DIMARC
                || found->second == DRW::LEADER
                || found->second == DRW::MLEADER);
    };
    const bool graphReadback = imported && readback.associations.size() == 1u
        && readback.associations.front().m_dimensionHandle != 0u
        && dimensionType(readback.associations.front().m_dimensionHandle)
        && readback.associations.front().reactorHandles.size() == 1u
        && readback.associations.front().reactorHandles.front()
            == readback.associations.front().parentHandle
        && readback.associations.front().xDictHandle
            == readback.associations.front().parentHandle
        && std::any_of(
            readbackData.dictionaries.begin(), readbackData.dictionaries.end(),
            [&readback](const DRW_Dictionary& dictionary) {
                return dictionary.handle
                    == readback.associations.front().parentHandle;
            })
        && readback.associations.front().m_osnapRefs.size() == 1u
        && readback.associations.front().m_osnapRefs.front().m_objectHandle
            != 0u
        && outputEntities.count(
               readback.associations.front().m_osnapRefs.front().m_objectHandle)
            == 1u
        && readback.associations.front().m_osnapRefs.front().m_hasOsnapPoint
        && readback.associations.front().m_osnapRefs.front().m_osnapPoint.x
            == 5.0;
    if (!graphReadback) {
        std::remove(outputPath.c_str());
        std::cerr << "adapter DIMASSOC success path failed typed readback"
                  << " imported=" << imported
                  << " associations=" << readback.associations.size()
                  << " dictionaries=" << readbackData.dictionaries.size();
        if (!readback.associations.empty()) {
            const DRW_DimensionAssociation& result =
                readback.associations.front();
            std::cerr << " owner=" << result.parentHandle
                      << " reactors=" << result.reactorCount() << '/'
                      << result.reactorHandles.size()
                      << " xdict=" << result.xDictHandle;
        }
        std::cerr << '\n';
        return false;
    }
    std::remove(outputPath.c_str());
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
    if (argc == 3 && std::string(argv[1]) == "--stale-generation-output")
        return runStaleGenerationOutput(argv[2]) ? 0 : 1;
    if (argc == 3 && std::string(argv[1]) == "--adapter-closure-retry-ascii")
        return runAdapterClosureRetryRoundtrip(argv[2], false) ? 0 : 1;
    if (argc == 3 && std::string(argv[1]) == "--adapter-closure-retry-binary")
        return runAdapterClosureRetryRoundtrip(argv[2], true) ? 0 : 1;
    if (argc == 2 && std::string(argv[1]) == "--allocation-rollback-retry") {
        for (const DRW::Version version :
             {DRW::AC1018, DRW::AC1021, DRW::AC1024, DRW::AC1027,
              DRW::AC1032}) {
            if (!allocationRollbackRetry(version, false)
                || !allocationRollbackRetry(version, true))
                return 1;
        }
        return 0;
    }
    if (argc == 3 && std::string(argv[1]) == "--generate-dimassoc") {
        dwgRW writer(argv[2]);
        DimensionAssociationAuthor author(writer);
        return writer.write(&author, DRW::AC1024, false) ? 0 : 1;
    }
    if (argc != 2) {
        std::cerr << "usage: libdxfrw_dimassoc_physical_transport_tests <source.dwg>\n"
                  << "   or: libdxfrw_dimassoc_physical_transport_tests --generate-dimassoc <scratch.dwg>\n";
        return 2;
    }
    return runPhysicalSource(argv[1]) ? 0 : 1;
}
