// SPDX-License-Identifier: GPL-2.0-or-later
// Explicit default DWG plot-style graph: no checked-in drawing fixtures.
#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include "intern/dwg_fixed_handles.h"
#include "intern/dwgwriter15.h"
#include "intern/dwgwriter21.h"
#include "intern/dwgwriter24.h"
#include "intern/dwgwriter27.h"
#include "intern/dwgwriter32.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>
#include <utility>
#include <vector>

class DrwObjectEncodeTestAccess {
public:
    static bool encodePlaceholder(DRW_AcDbPlaceholder& placeholder,
                                  DRW::Version version, dwgBufferW& data,
                                  dwgBufferW& handles, bool typedLegacy) {
        return placeholder.encodeDwgForWriter(version, &data, &handles, typedLegacy);
    }
    static bool encodeDefaultDictionary(DRW_DictionaryWithDefault& dictionary,
                                        DRW::Version version, dwgBufferW& data,
                                        dwgBufferW& strings, dwgBufferW& handles,
                                        bool typedLegacy) {
        return dictionary.encodeDwgForWriter(version, &data, &strings, &handles,
                                            typedLegacy);
    }
};

class DwgBlockRecordXrefWireTestAccess {
public:
    static void failAfterGraphDictionary(dwgWriter15& writer) {
        writer.setWriteFailurePointForTest(dwgWriter15::kAfterDefaultPlotStyleDictionary);
    }
};

namespace {
class ThrowingCheckpointWriter final : public dwgWriter15 {
public:
    using dwgWriter15::dwgWriter15;
    CompoundWriteCheckpoint checkpointPublicTransaction() const override {
        if (failAdmissionCheckpoint) throw std::bad_alloc();
        return dwgWriter15::checkpointPublicTransaction();
    }
    CompoundWriteCheckpoint checkpointCompoundWrite() const override {
        if (failEmissionCheckpoint) throw std::bad_alloc();
        return dwgWriter15::checkpointCompoundWrite();
    }
    mutable bool failAdmissionCheckpoint = false;
    mutable bool failEmissionCheckpoint = false;
};

bool defaultDictionaryNullRoles() {
    for (const auto version : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                               DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        for (const bool synthetic : {false, true}) {
            DRW_DictionaryWithDefault child;
            child.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
            child.setDwgCommonObjectState(0, 0, false);
            child.m_entries = {{"Normal", 0x31u}};
            child.m_defaultEntryHandle = 0x31u;
            dwgBufferW body, strings, handles;
            if (!DrwObjectEncodeTestAccess::encodeDefaultDictionary(
                    child, version, body, strings, handles, synthetic))
                return false;
            const bool typed = version == DRW::AC1015 && synthetic;
            const std::vector<std::uint8_t> expected{
                0x41u, 0x0cu, static_cast<std::uint8_t>(typed ? 0x30u : 0x00u),
                0x21u, 0x31u, 0x51u, 0x31u};
            if (handles.data() != expected) return false;
            DRW_AcDbPlaceholder placeholder;
            placeholder.parentHandle = 0x30u;
            dwgBufferW placeholderBody, placeholderHandles;
            if (!DrwObjectEncodeTestAccess::encodePlaceholder(
                    placeholder, version, placeholderBody, placeholderHandles, synthetic)
                || placeholderHandles.data() != std::vector<std::uint8_t>(
                    {0x41u, 0x30u, static_cast<std::uint8_t>(typed ? 0x30u : 0x00u)}))
                return false;
            placeholder.xDictHandle = 0x71u;
            dwgBufferW nonNullBody, nonNullHandles;
            if (!DrwObjectEncodeTestAccess::encodePlaceholder(
                    placeholder, version, nonNullBody, nonNullHandles, synthetic)
                || nonNullHandles.data() != std::vector<std::uint8_t>(
                    {0x41u, 0x30u, 0x31u, 0x71u}))
                return false;
        }
    }
    return true;
}

template <typename Writer>
bool modernHeaderControl(bool extended) {
    DRW_Header header;
    Writer writer(nullptr, &header);
    if (!writer.stageDefaultDwgPlotStyleGraph()) return false;
    const auto stagedHighWater = writer.highWaterHandle();
    if (writer.stageDefaultDwgPlotStyleGraph()
        || writer.highWaterHandle() != stagedHighWater) return false;
    std::uint32_t dictionary = 0, placeholder = 0;
    if (!writer.defaultDwgPlotStyleGraphHandles(dictionary, placeholder)
        || !writer.prepareDwgClassManifest() || !writer.writeFileHeaderStub()) return false;
    const std::size_t start = writer.buffer().size();
    if (!writer.writeDwgHeader()) return false;
    const auto& bytes = writer.buffer();
    dwgBuffer stream(const_cast<std::uint8_t*>(bytes.data()), bytes.size());
    const std::size_t sizeWord = start + (extended ? 24u : 20u);
    if (!stream.setPosition(sizeWord)) return false;
    const std::uint64_t handleBit = sizeWord * 8u + stream.getRawLong32();
    if (!stream.isGood() || !stream.setPosition(handleBit / 8u)) return false;
    stream.setBitPos(handleBit & 7u);
    for (unsigned i = 0; i < 29u && stream.isGood(); ++i) (void)stream.getHandle();
    const auto group = stream.getHandle();
    const auto mline = stream.getHandle();
    const auto nod = stream.getHandle();
    const auto layouts = stream.getHandle();
    const auto plotsettings = stream.getHandle();
    const auto plotstyle = stream.getHandle();
    return stream.isGood() && group.code == DRW::DwgHardPointer
        && group.ref == DRW::DwgAcadGroupDictionaryHandle
        && mline.code == DRW::DwgHardPointer && mline.ref == 0u
        && nod.code == DRW::DwgHardOwnership && nod.ref == DRW::DwgNamedObjectsDictionaryHandle
        && layouts.code == DRW::DwgHardPointer && layouts.ref == 0u
        && plotsettings.code == DRW::DwgHardPointer && plotsettings.ref == 0u
        && plotstyle.code == DRW::DwgHardPointer && plotstyle.ref == dictionary;
}

bool classOrdinalCollisionControl() {
    DRW_Header header;
    dwgWriter15 writer(nullptr, &header);
    DRW_UnsupportedObject other;
    other.m_isCustomClass = true;
    other.m_objectType = DRW_DictionaryWithDefault::kDwgClassNum;
    other.m_className = "AcDbOtherClass";
    other.m_recordName = "OTHER_CLASS";
    if (!writer.registerRawObjectClass(other) || !writer.stageDefaultDwgPlotStyleGraph())
        return false;
    std::uint32_t dictionary = 0, placeholder = 0;
    std::uint16_t actualClass = 0;
    return writer.defaultDwgPlotStyleGraphHandles(dictionary, placeholder)
        && writer.getDwgClassInstanceClass(dictionary, actualClass)
        && actualClass >= 500u && actualClass != DRW_DictionaryWithDefault::kDwgClassNum
        && writer.prepareDwgClassManifest();
}

bool directRollbackControls() {
    // Even an atypical low-level caller that emitted HEADER without preparing
    // CLASSES cannot stage the graph after the reference was serialized.
    DRW_Header lateHeader;
    dwgWriter15 late(nullptr, &lateHeader);
    if (!late.writeFileHeaderStub() || !late.writeDwgHeader()) return false;
    const auto lateBefore = late.highWaterHandle();
    if (late.stageDefaultDwgPlotStyleGraph() || late.highWaterHandle() != lateBefore)
        return false;
    DRW_Header header;
    ThrowingCheckpointWriter writer(nullptr, &header);
    const auto noConflict = writer.checkpointPublicTransaction();
    if (!writer.registerDwgNamedObjectDictionaryEntry("acad_plotstylename", 0xB100u))
        return false;
    const auto beforeFailure = writer.highWaterHandle();
    if (writer.stageDefaultDwgPlotStyleGraph()
        || writer.highWaterHandle() != beforeFailure) return false;
    writer.rollbackCompoundWrite(noConflict);
    const auto afterRollback = writer.highWaterHandle();
    writer.failAdmissionCheckpoint = true;
    if (writer.stageDefaultDwgPlotStyleGraph()
        || writer.highWaterHandle() != afterRollback) return false;
    writer.failAdmissionCheckpoint = false;
    if (!writer.stageDefaultDwgPlotStyleGraph()) return false;
    std::uint32_t dictionary = 0, placeholder = 0;
    if (!writer.defaultDwgPlotStyleGraphHandles(dictionary, placeholder)
        || dictionary != afterRollback || placeholder != afterRollback + 1u)
        return false;
    if (!writer.prepareDwgClassManifest() || !writer.writeFileHeaderStub()
        || !writer.writeDwgHeader() || !writer.writeDwgClasses()) return false;
    writer.freezeDwgClasses();
    if (!writer.writeDwgObjects()) return false;
    const auto beforeBytes = writer.buffer();
    const auto before = writer.checkpointCompoundWrite();
    writer.failEmissionCheckpoint = true;
    if (writer.completeDeferredDwgObjects() || writer.buffer() != beforeBytes)
        return false;
    writer.failEmissionCheckpoint = false;
    DwgBlockRecordXrefWireTestAccess::failAfterGraphDictionary(writer);
    if (writer.completeDeferredDwgObjects() || writer.buffer() != beforeBytes)
        return false;
    const auto after = writer.checkpointCompoundWrite();
    if (before.objectMapSize != after.objectMapSize
        || before.rawClassInstanceHandles != after.rawClassInstanceHandles
        || before.classInstanceHandleOwners != after.classInstanceHandleOwners
        || before.classInstanceEmittedHandles != after.classInstanceEmittedHandles
        || before.classInstanceEmissionCounts != after.classInstanceEmissionCounts
        || before.defaultPlotStyleGraphEmitted != after.defaultPlotStyleGraphEmitted
        || before.defaultPlotStyleDictionaryHandle != after.defaultPlotStyleDictionaryHandle
        || before.defaultPlotStylePlaceholderHandle != after.defaultPlotStylePlaceholderHandle)
        return false;
    return writer.completeDeferredDwgObjects()
        && writer.validateDwgClassInstanceCounts()
        && writer.validateDwgClassInstanceFrames()
        && writer.validateDwgNamedObjectDictionaryEntries();
}

enum class Scenario { Normal, EarlyRoot, LateRoot, LateAlias, LateGroupAlias,
                      LateHandle, LateEntityHandle, NestedDictionaryRoot,
                      NestedDictionaryPlaceholder, NestedDefaultAlias,
                      NestedDefaultImpersonation, EarlyOtherRoot, SuppliedLayer };
struct Adapter : LC3CompatCheck, DRW_DwgDefaultPlotStyleProvider {
    explicit Adapter(dwgRW& destination) : output(destination) {}
    bool wantsDwgDefaultPlotStyleGraph() const override { return enabled; }
    void dwgDefaultPlotStyleGraphPlanned(std::uint32_t dictionary,
                                         std::uint32_t placeholder) override {
        plannedDictionary = dictionary;
        plannedPlaceholder = placeholder;
    }
    void writeDwgClasses() override {
        if (scenario == Scenario::EarlyRoot)
            (void)output.registerDwgNamedObjectDictionaryEntry("acad_plotstylename", 0xB100u);
        if (scenario == Scenario::EarlyOtherRoot)
            earlyOtherRegistered = output.registerDwgNamedObjectDictionaryEntry("EARLY_OTHER", 0xB202u);
    }
    void writeLTypes() override {
        if (scenario == Scenario::LateRoot)
            lateRootRejected = !output.registerDwgNamedObjectDictionaryEntry(
                "aCaD_PlotStyleName", 0xB100u);
        if (scenario == Scenario::LateAlias)
            lateAliasRejected = !output.registerDwgNamedObjectDictionaryEntry(
                "ILLEGAL_ALIAS", plannedPlaceholder);
        if (scenario == Scenario::LateGroupAlias)
            lateAliasRejected = !output.registerDwgGroupDictionaryEntry(
                "ILLEGAL_GROUP_ALIAS", plannedPlaceholder);
    }
    void writeLayers() override {
        if (scenario == Scenario::SuppliedLayer) {
            DRW_Layer layer;
            layer.name = "0";
            layer.plotStyleHandle.ref = 0xB200u;
            layerAdded = output.addLayer(&layer);
        }
    }
    void writeEntities() override {
        DRW_Line line;
        if (scenario == Scenario::LateEntityHandle)
            line.handle = plannedPlaceholder;
        line.basePoint = {1., 2., 3.};
        line.secPoint = {4., 5., 6.};
        lineWritten = output.writeLine(&line);
    }
    void writeObjects() override {
        if (scenario == Scenario::NestedDictionaryRoot
            || scenario == Scenario::NestedDictionaryPlaceholder) {
            DRW_Dictionary dictionary;
            dictionary.handle = 0xB300u;
            dictionary.m_entries.push_back({"ALIAS", scenario == Scenario::NestedDictionaryRoot
                ? plannedDictionary : plannedPlaceholder});
            nestedAliasRejected = !output.writeDictionary(&dictionary);
        }
        if (scenario == Scenario::NestedDefaultAlias
            || scenario == Scenario::NestedDefaultImpersonation) {
            DRW_DictionaryWithDefault dictionary;
            dictionary.handle = scenario == Scenario::NestedDefaultImpersonation
                ? plannedDictionary : 0xB300u;
            dictionary.m_defaultEntryHandle = plannedPlaceholder;
            nestedAliasRejected = !output.writeDictionaryWithDefault(&dictionary);
        }
        if (scenario == Scenario::LateHandle || scenario == Scenario::SuppliedLayer
            || scenario == Scenario::EarlyOtherRoot) {
            DRW_AcDbPlaceholder placeholder;
            placeholder.handle = scenario == Scenario::LateHandle
                ? plannedPlaceholder : scenario == Scenario::EarlyOtherRoot ? 0xB202u : 0xB200u;
            if (scenario == Scenario::EarlyOtherRoot)
                placeholder.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
            placeholderWritten = output.writeAcDbPlaceholder(&placeholder);
        }
    }
    dwgRW& output;
    Scenario scenario = Scenario::Normal;
    std::uint32_t plannedDictionary = 0, plannedPlaceholder = 0;
    bool enabled = true, lateRootRejected = false, lateAliasRejected = false, layerAdded = false;
    bool nestedAliasRejected = false;
    bool lineWritten = false, placeholderWritten = false, earlyOtherRegistered = false;
};
// Ordinary consumers do not implement the new optional provider interface.
struct LegacyAdapter : LC3CompatCheck {
    explicit LegacyAdapter(dwgRW& destination) : output(destination) {}
    void writeEntities() override {
        DRW_Line line;
        line.basePoint = {1., 2., 3.}; line.secPoint = {4., 5., 6.};
        (void)output.writeLine(&line);
    }
    dwgRW& output;
};
struct Capture : LC3CompatCheck {
    void addNamedObjectDictionary(const DRW_Dictionary& value) override {
        for (const auto& entry : value.m_entries)
            if (entry.m_name == "ACAD_PLOTSTYLENAME") {
                ++roots; dictionary = entry.m_handle;
            } else if (entry.m_name == "EARLY_OTHER" && entry.m_handle == 0xB202u) {
                sawEarlyOther = true;
            }
    }
    void addDictionaryWithDefault(const DRW_DictionaryWithDefault& value) override {
        if (value.m_entries.size() == 1u && value.m_entries[0].m_name == "Normal"
            && value.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
            && value.m_defaultEntryHandle == value.m_entries[0].m_handle) {
            graphDictionary = value.handle;
            placeholder = value.m_defaultEntryHandle;
            dictionaryReactors = value.reactorHandles;
        }
    }
    void addAcDbPlaceholder(const DRW_AcDbPlaceholder& value) override {
        placeholders.push_back({value.handle, value.parentHandle, value.reactorHandles});
    }
    void addLayer(const DRW_Layer& value) override {
        if (value.name == "0") { ++layerCount; layerPlotStyle = value.plotStyleHandle.ref; }
    }
    void addLine(const DRW_Line& value) override {
        if (value.basePoint.x == 1. && value.secPoint.z == 6.) ++lines;
    }
    int roots = 0, layerCount = 0, lines = 0;
    bool sawEarlyOther = false;
    std::uint32_t dictionary = 0, graphDictionary = 0, placeholder = 0, layerPlotStyle = 0;
    bool hasGraph(bool suppliedLayer, DRW::Version version) const {
        if (roots != 1 || dictionary == 0 || dictionary == 0x30u || dictionary == 0x31u
            || graphDictionary != dictionary || placeholder == 0 || placeholder == dictionary
            || placeholder == 0x30u || placeholder == 0x31u || layerCount != 1 || lines != 1
            || layerPlotStyle != (suppliedLayer ? 0xB200u : placeholder)) return false;
        const bool hasPersistentReactorProfile = version >= DRW::AC1015
            && version <= DRW::AC1024;
        const std::vector<std::uint32_t> expectedDictionaryReactors =
            hasPersistentReactorProfile
                ? std::vector<std::uint32_t>{DRW::DwgNamedObjectsDictionaryHandle}
                : std::vector<std::uint32_t>{};
        const std::vector<std::uint32_t> expectedPlaceholderReactors =
            hasPersistentReactorProfile
                ? std::vector<std::uint32_t>{dictionary}
                : std::vector<std::uint32_t>{};
        const auto emittedPlaceholder = std::find_if(
            placeholders.begin(), placeholders.end(), [this](const PlaceholderCapture& value) {
                return value.handle == placeholder && value.parent == dictionary;
            });
        return dictionaryReactors == expectedDictionaryReactors
            && emittedPlaceholder != placeholders.end()
            && emittedPlaceholder->reactors == expectedPlaceholderReactors;
    }
    struct PlaceholderCapture {
        std::uint32_t handle;
        std::uint32_t parent;
        std::vector<std::uint32_t> reactors;
    };
    std::vector<std::uint32_t> dictionaryReactors;
    std::vector<PlaceholderCapture> placeholders;
};
bool inspect(const std::string& name, DRW::Version version, bool enabled,
             bool suppliedLayer = false,
             bool earlyOther = false) {
    dwgRW reader(name.c_str()); Capture capture;
    if (!reader.read(&capture, false)) return false;
    if (enabled) {
        const bool graph = capture.hasGraph(suppliedLayer, version);
        if (!graph) {
            std::cerr << "capture version=" << version << " roots=" << capture.roots
                      << " dict=" << capture.dictionary << " graphDict="
                      << capture.graphDictionary << " placeholder=" << capture.placeholder
                      << " dictReactors=" << capture.dictionaryReactors.size()
                      << " placeholderCount=" << capture.placeholders.size()
                      << " layer=" << capture.layerCount << " line=" << capture.lines << '\n';
        }
        return graph && (!earlyOther || (capture.sawEarlyOther
                            && capture.dictionary != 0xB202u
                            && capture.placeholder != 0xB202u));
    }
    return capture.roots == 0 && capture.layerCount == 1
        && capture.layerPlotStyle == 0 && capture.lines == 1;
}
bool runVersion(const std::filesystem::path& path, DRW::Version version) {
    const std::string name = path.string();
    dwgRW output(name.c_str());
    if (!output.reserveHandle(0x30u) || !output.reserveHandle(0x31u)
        || !output.reserveHandle(0xB200u)) return false;
    LegacyAdapter legacy(output);
    if (!output.write(&legacy, version, true) || !inspect(name, version, false)) {
        std::cerr << "legacy consumer failed for " << version << '\n'; return false;
    }
    Adapter adapter(output);
    adapter.enabled = false;
    if (!output.write(&adapter, version, true) || !inspect(name, version, false)
        || adapter.plannedDictionary != 0 || adapter.plannedPlaceholder != 0) {
        std::cerr << "default behavior failed for " << version << '\n'; return false;
    }
    adapter.enabled = true;
    const bool optInWrote = output.write(&adapter, version, true);
    const bool optInReadback = optInWrote && inspect(name, version, true);
    if (!optInWrote || !optInReadback
        || adapter.plannedDictionary == 0x30u || adapter.plannedDictionary == 0x31u
        || adapter.plannedDictionary == 0xB200u || adapter.plannedPlaceholder == 0x30u
        || adapter.plannedPlaceholder == 0x31u || adapter.plannedPlaceholder == 0xB200u) {
        std::cerr << "opt-in graph failed for " << version << " write=" << optInWrote
                  << " readback=" << optInReadback << " handles="
                  << adapter.plannedDictionary << ',' << adapter.plannedPlaceholder << '\n';
        return false;
    }
    adapter.scenario = Scenario::SuppliedLayer;
    if (!output.write(&adapter, version, true) || !adapter.layerAdded
        || !adapter.placeholderWritten || !inspect(name, version, true, true)) {
        std::cerr << "caller layer override failed for " << version << '\n'; return false;
    }
    adapter.scenario = Scenario::EarlyOtherRoot;
    if (!output.write(&adapter, version, true) || !adapter.earlyOtherRegistered
        || !adapter.placeholderWritten || !inspect(name, version, true, false, true)) {
        std::cerr << "caller identity reservation failed for " << version << '\n'; return false;
    }
    for (const Scenario fault : {Scenario::EarlyRoot, Scenario::LateRoot,
                                 Scenario::LateAlias, Scenario::LateGroupAlias,
                                 Scenario::LateHandle, Scenario::LateEntityHandle,
                                 Scenario::NestedDictionaryRoot,
                                 Scenario::NestedDictionaryPlaceholder,
                                 Scenario::NestedDefaultAlias,
                                 Scenario::NestedDefaultImpersonation}) {
        { std::ofstream out(path, std::ios::binary); out << "prior destination"; }
        adapter.scenario = fault;
        adapter.lateRootRejected = false;
        adapter.lateAliasRejected = false;
        adapter.nestedAliasRejected = false;
        adapter.placeholderWritten = false;
        adapter.lineWritten = false;
        if (output.write(&adapter, version, true)) {
            std::cerr << "collision unexpectedly wrote for " << version
                      << ", scenario=" << static_cast<int>(fault) << '\n'; return false;
        }
        { std::ifstream in(path, std::ios::binary);
          const std::string contents((std::istreambuf_iterator<char>(in)), {});
          if (contents != "prior destination") return false; }
        if (fault == Scenario::LateRoot && !adapter.lateRootRejected) return false;
        if ((fault == Scenario::LateAlias || fault == Scenario::LateGroupAlias)
            && !adapter.lateAliasRejected) return false;
        if (fault == Scenario::LateHandle && !adapter.placeholderWritten) return false;
        if (fault == Scenario::LateEntityHandle && !adapter.lineWritten) return false;
        if ((fault == Scenario::NestedDictionaryRoot
             || fault == Scenario::NestedDictionaryPlaceholder
             || fault == Scenario::NestedDefaultAlias
             || fault == Scenario::NestedDefaultImpersonation) && !adapter.nestedAliasRejected)
            return false;
        adapter.scenario = Scenario::Normal;
        if (!output.write(&adapter, version, true) || !inspect(name, version, true)) {
            std::cerr << "same-writer retry failed for " << version << '\n'; return false;
        }
    }
    return true;
}
} // namespace

int main(int argc, char** argv) {
    const bool keep = argc == 2 && std::string(argv[1]) == "--keep";
    if (argc > 2 || (argc == 2 && !keep)) return 2;
    if (!defaultDictionaryNullRoles()
        || !directRollbackControls() || !classOrdinalCollisionControl()
        || !modernHeaderControl<dwgWriter21>(false)
        || !modernHeaderControl<dwgWriter24>(false)
        || !modernHeaderControl<dwgWriter27>(true)
        || !modernHeaderControl<dwgWriter32>(true)) {
        std::cerr << "direct graph rollback, class or HEADER controls failed\n";
        return 1;
    }
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto dir = std::filesystem::temp_directory_path()
        / ("libdxfrw-default-plotstyle-" + std::to_string(nonce));
    if (!std::filesystem::create_directory(dir)) return 2;
    struct Cleanup { std::filesystem::path path; bool keep; ~Cleanup() {
        if (!keep) { std::error_code error; std::filesystem::remove_all(path,error); }
    }} cleanup{dir, keep};
    for (const DRW::Version version : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                                       DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        if (!runVersion(dir / ("probe-" + std::to_string(version) + ".dwg"), version))
            return 1;
    }
    if (keep) std::cout << "OUTPUT " << dir.string() << '\n';
    std::cout << "DWG default plotstyle opt-in PASS\n";
    return 0;
}
