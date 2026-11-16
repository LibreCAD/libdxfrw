// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include "intern/dwgwriter24.h"
#include "intern/dwgwriter27.h"
#include "intern/dwgobjectframe.h"
#include "intern/drw_textcodec.h"
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <sstream>
#include <unordered_map>

static std::atomic<int> failAfter{-1};
void* operator new(std::size_t n) {
    if (failAfter>=0 && failAfter--==0) throw std::bad_alloc();
    if (void* p=std::malloc(n?n:1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}

class DrwHeaderEncodeTestAccess {
public:
    static bool encode(DRW_Header& h, DRW::Version v, dwgBufferW& data,
                       dwgBufferW& handles, dwgBufferW& strings,
                       std::uint32_t dict, std::uint32_t current) {
        return h.encodeDwgForWriterMaterial(v, &data, &handles, &strings,
            DRW::DwgNamedObjectsDictionaryHandle, 0, nullptr,
            DRW::DwgAcadGroupDictionaryHandle, 0, dict, current);
    }
};

class MaterialSemanticProbe : public DRW_Material {
public:
    using DRW_Material::encodeDwg;
    using DRW_Material::parseDwg;
};

bool ac1027DefaultMaterialReplayRoundtrip(const DRW_Material& source);
bool ac1027DefaultMaterialReplayGraphRoundtrip(
    const std::vector<DRW_Material>& sources,
    const std::filesystem::path& output = {});
bool ac1027DefaultMaterialReplayAllocationSweep(
    const std::vector<DRW_Material>& sources);

class DwgBlockRecordXrefWireTestAccess {
public:
    static void markPlotGraphComplete(dwgWriter15& w) { w.m_defaultPlotStyleGraphEmitted=true; }
    static void failAfterDictionary(dwgWriter15& w) {
        w.setWriteFailurePointForTest(dwgWriter15::kAfterDefaultMaterialDictionary);
    }
    static std::uint64_t generation(const dwgWriter27& w) {return w.m_nextDwgObjectFrameGeneration;}
    static bool cleanLayerFailure(dwgWriter27& w,const std::vector<std::uint8_t>& before,
                                  std::size_t mapSize,std::uint64_t generation) {
        DRW::DwgObjectFrameReceipt r;
        return w.buffer()==before && w.m_objectMap.size()==mapSize
            && w.m_emittedDwgObjectHandles.count(DRW::DwgLayer0Handle)==0u
            && w.m_nextDwgObjectFrameGeneration==generation && w.m_currentHandle==0u
            && !w.getLastDwgObjectFrame(r) && static_cast<dwgWriter15&>(w).objectWriteFailed();
    }
    static bool layerFrame(dwgWriter27& w) {
        DRW_Layer layer; layer.name="0"; layer.setDwgCommonObjectState(0,1,false);
        layer.xrefBlockHandle.code=DRW::DwgHardPointer;
        layer.plotStyleHandle.ref=w.m_defaultPlotStylePlaceholderHandle;
        layer.materialHandle.ref=w.m_defaultMaterialHandles[3];
        if(!w.emitLayerRecord(DRW::DwgLayer0Handle,layer))return false;
        DRW::DwgObjectFrameReceipt r;
        if(!w.getLastDwgObjectFrame(r)||!r.valid||r.objectHandle!=DRW::DwgLayer0Handle||r.handleBitSize!=0u)return false;
        const auto it=std::find_if(w.m_objectMap.begin(),w.m_objectMap.end(),[](const auto& e){return e.first==DRW::DwgLayer0Handle;});
        if(it==w.m_objectMap.end())return false;
        auto& bytes=w.m_buf.data();dwgBuffer buf(bytes.data(),bytes.size());DwgObjectFrame frame;
        if(!frame.readAt(buf,DRW::AC1027,it->second)||frame.objectDataBitSize()!=155u)return false;
        const std::uint32_t refs[]={DRW::DwgLayerControlHandle,0u,w.m_defaultPlotStylePlaceholderHandle,w.m_defaultMaterialHandles[3],DRW::DwgLtypeContinuousHandle,0u};
        const unsigned roles[]={4u,5u,5u,5u,5u,5u}; std::size_t count=0;
        for(const auto& o:r.occurrences)if(o.stream==DRW::DwgObjectHandleStream::Handles){
            if(count>=6u || o.serializedSegment!=DRW::DwgObjectSerializedSegment::Data
               || o.token.code!=roles[count] || o.token.reference!=refs[count] || o.token.startBit<155u)return false;
            ++count;
        }
        return count==6u;
    }
    static bool globalFrame(dwgWriter27& w) {
        w.m_writingDefaultMaterialGraph=true;
        const bool ok=w.emitDefaultDwgMaterial(3);
        w.m_writingDefaultMaterialGraph=false;
        if(!ok) return false;
        DRW::DwgObjectFrameReceipt r;
        const auto handle=w.m_defaultMaterialHandles[3], dict=w.m_defaultMaterialHandles[0];
        if(!w.getLastDwgObjectFrame(r)||!r.valid||r.objectHandle!=handle||r.handleBitSize!=0u) return false;
        const auto it=std::find_if(w.m_objectMap.begin(),w.m_objectMap.end(),[&](const auto& e){return e.first==handle;});
        if(it==w.m_objectMap.end())return false;
        auto& bytes=w.m_buf.data();dwgBuffer buf(bytes.data(),bytes.size());DwgObjectFrame frame;
        if(!frame.readAt(buf,DRW::AC1027,it->second))return false;
        const auto expected=691u+(handle>0xffu?8u:0u);
        if(frame.objectDataBitSize()!=expected || r.dataBitSize!=frame.body().size()*8u) return false;
        DRW_TextCodec codec;codec.setVersion(DRW::AC1027,false);
        auto selectedBytes=frame.body();dwgBuffer original(selectedBytes.data(),selectedBytes.size(),&codec);MaterialSemanticProbe selected;
        if(!selected.parseDwg(DRW::AC1027,&original,frame.bodyBitSize())
            || !selected.hasCompleteDwgSemanticFields() || !selected.hasDwgRawData()
            || selected.dwgRawDataBitSize()!=516u || selected.m_name!="Global"
            || !selected.canReplayAc1027DefaultDwgBody()) {
            std::cerr << "AC1027 exact material replay profile rejected: bits="
                      << selected.dwgRawDataBitSize() << " raw-bytes="
                      << selected.dwgRawData().size() << " name="
                      << selected.m_name << " semantic="
                      << selected.hasCompleteDwgSemanticFields() << '\n';
            return false;
        }
        dwgBufferW replayedBody(&codec), replayedStrings(&codec), replayedHandles(&codec);
        if (!selected.encodeDwg(DRW::AC1027, &replayedBody, &replayedStrings,
                                &replayedHandles)
            || replayedBody.bitCount() != selected.dwgRawDataBitSize()
            || replayedBody.data() != selected.dwgRawData()
            || !replayedStrings.isGood() || !replayedHandles.isGood())
        {
            std::cerr << "AC1027 exact material replay body/string build failed\n";
            return false;
        }
        dwgBuffer replayStringInput(replayedStrings.data().data(),
                                    replayedStrings.data().size(), &codec);
        const std::array<std::string, 7> expectedStrings{{
            "Global", "", "", "", "", "", ""}};
        for (const std::string& expected : expectedStrings)
            if (replayStringInput.getVariableText(DRW::AC1027) != expected)
            {
                std::cerr << "AC1027 exact material replay string order mismatch\n";
                return false;
            }
        std::uint64_t replayStringBit =
            static_cast<std::uint64_t>(replayStringInput.getPosition()) * 8u
            + replayStringInput.getBitPos();
        const std::uint64_t replayStringEnd = replayedStrings.bitCount();
        if (replayStringBit > replayStringEnd
            || replayStringEnd - replayStringBit > 7u)
        {
            std::cerr << "AC1027 exact material replay string stream did not close\n";
            return false;
        }
        while (replayStringBit < replayStringEnd) {
            if (replayStringInput.getBit() != 0u)
                return false;
            ++replayStringBit;
        }
        if (!replayStringInput.isGood())
            return false;
        auto changedMaterial = selected;
        changedMaterial.m_diffuseColorFactor = 0.75;
        dwgBufferW rejectedBody(&codec), rejectedStrings(&codec), rejectedHandles(&codec);
        rejectedBody.putRawChar8(0xA5u);
        rejectedStrings.putRawChar8(0x5Au);
        rejectedHandles.putRawChar8(0x3Cu);
        if (changedMaterial.encodeDwg(DRW::AC1027, &rejectedBody,
                                      &rejectedStrings, &rejectedHandles)
            || rejectedBody.data() != std::vector<std::uint8_t>{0xA5u}
            || rejectedStrings.data() != std::vector<std::uint8_t>{0x5Au}
            || rejectedHandles.data() != std::vector<std::uint8_t>{0x3Cu})
        {
            std::cerr << "AC1027 changed material body did not refuse atomically\n";
            return false;
        }
        MaterialSemanticProbe authoredMaterial;
        dwgBufferW authoredBody(&codec), authoredStrings(&codec), authoredHandles(&codec);
        authoredBody.putRawChar8(0xA5u);
        authoredStrings.putRawChar8(0x5Au);
        authoredHandles.putRawChar8(0x3Cu);
        if (authoredMaterial.encodeDwg(DRW::AC1027, &authoredBody,
                                      &authoredStrings, &authoredHandles)
            || authoredBody.data() != std::vector<std::uint8_t>{0xA5u}
            || authoredStrings.data() != std::vector<std::uint8_t>{0x5Au}
            || authoredHandles.data() != std::vector<std::uint8_t>{0x3Cu})
        {
            std::cerr << "newly authored AC1027 MATERIAL escaped exact-profile refusal\n";
            return false;
        }
        changedMaterial.m_diffuseColorFactor = selected.m_diffuseColorFactor;
        dwgBufferW retryBody(&codec), retryStrings(&codec), retryHandles(&codec);
        if (!changedMaterial.encodeDwg(DRW::AC1027, &retryBody, &retryStrings,
                                       &retryHandles)
            || retryBody.data() != selected.dwgRawData()
            || retryBody.bitCount() != selected.dwgRawDataBitSize()) {
            std::cerr << "AC1027 corrected MATERIAL retry failed\n";
            return false;
        }
        if (!ac1027DefaultMaterialReplayRoundtrip(selected))
            return false;
        // Change one character in the independently bounded detached name;
        // the frame remains parseable, but this is outside the exact default profile.
        const auto bit=expected-17u-118u+10u+7u;
        selectedBytes.at(bit/8u)^=static_cast<std::uint8_t>(1u<<(7u-bit%8u));
        dwgBuffer changed(selectedBytes.data(),selectedBytes.size(),&codec);MaterialSemanticProbe unmatched;
        if(!unmatched.parseDwg(DRW::AC1027,&changed,frame.bodyBitSize())
            || unmatched.hasCompleteDwgSemanticFields() || !unmatched.hasDwgRawData()
            || unmatched.dwgRawDataBitSize()!=516u || unmatched.m_name=="Global")return false;
        std::size_t count=0;
        for(const auto& o:r.occurrences)if(o.stream==DRW::DwgObjectHandleStream::Handles){
            if(o.serializedSegment!=DRW::DwgObjectSerializedSegment::Data || o.token.code!=DRW::DwgSoftPointer || o.token.reference!=dict || o.token.startBit<expected) return false;
            ++count;
        }
        return count==2u;
    }
    static bool tokenConflict(dwgWriter15& w, std::uint32_t owner,
                              std::uint8_t code, std::uint32_t reference) {
        w.m_currentHandle=owner; w.m_objectHandles.reset();
        dwgHandle token; token.code=code; token.ref=reference;
        w.m_objectHandles.putHandle(token);
        return w.rejectDefaultDwgMaterialFrameConflicts();
    }
};

namespace {
struct ThrowingWriter : dwgWriter24 {
    using dwgWriter24::dwgWriter24;
    CompoundWriteCheckpoint checkpointPublicTransaction() const override {
        if (failAdmission) throw std::bad_alloc();
        return dwgWriter24::checkpointPublicTransaction();
    }
    CompoundWriteCheckpoint checkpointCompoundWrite() const override {
        if (failEmission) throw std::bad_alloc();
        return dwgWriter24::checkpointCompoundWrite();
    }
    bool failAdmission = false, failEmission = false;
};
struct MutatingAc1027Writer : dwgWriter27 {
    using dwgWriter27::dwgWriter27;
    int mutation=0;
    void finishObject() override {
        if (mutation && m_writingDefaultMaterialGraph
            && m_currentHandle==m_defaultMaterialHandles[1]) {
            if (mutation==1) m_objectStrings.data()[1] ^= 0x40u;
            if (mutation==2) m_objectBody.data()[6] ^= 0x40u;
            if (mutation==3) m_objectHandles.data()[0] ^= 0x10u;
            if (mutation==4) { mutation=0; throw std::bad_alloc(); }
            mutation=0;
        }
        dwgWriter27::finishObject();
    }
};
bool directAc1027Controls() {
    for (int phase : {0,1,2,3,4,5,6}) {
        DRW_Header h; MutatingAc1027Writer w(nullptr,&h);
        if (!w.stageDefaultDwgPlotStyleGraph() || !w.stageDefaultDwgMaterialGraph()
            || !w.prepareDwgClassManifest() || !w.writeFileHeaderStub()
            || !w.writeDwgHeader() || !w.writeDwgClasses()) return false;
        w.freezeDwgClasses();
        if(phase==2) { if(!DwgBlockRecordXrefWireTestAccess::layerFrame(w)) return false; continue; }
        if(!w.writeDwgObjects()) return false;
        if(phase==1) { if(!DwgBlockRecordXrefWireTestAccess::globalFrame(w)) return false; continue; }
        const auto before=w.checkpointCompoundWrite(); const auto bytes=w.buffer();
        if (phase==0) DwgBlockRecordXrefWireTestAccess::failAfterDictionary(w);
        else w.mutation=phase-2;
        if(w.completeDeferredDwgObjects()||w.buffer()!=bytes)return false;
        const auto after=w.checkpointCompoundWrite();
        if(before.objectMapSize!=after.objectMapSize || before.defaultMaterialHandles!=after.defaultMaterialHandles
           || before.defaultMaterialGraphEmitted!=after.defaultMaterialGraphEmitted
           || before.classInstanceEmittedHandles!=after.classInstanceEmittedHandles
           || before.classInstanceEmissionCounts!=after.classInstanceEmissionCounts)return false;
        if(!w.completeDeferredDwgObjects() || !w.validateDwgNamedObjectDictionaryEntries()
            || !w.validateDwgClassInstanceCounts() || !w.validateDwgClassInstanceFrames())return false;
    }
    return true;
}
struct LayerFaultWriter : dwgWriter27 {
    using dwgWriter27::dwgWriter27;
    int fault=-1; bool triggered=false, escaped=false;
    void finishObject() override {
        if(m_currentHandle!=DRW::DwgLayer0Handle || fault<0) {dwgWriter27::finishObject();return;}
        failAfter=fault;
        try {dwgWriter27::finishObject();} catch(...) {escaped=true;}
        triggered=failAfter<0;failAfter=-1;
    }
};
bool ac1027LayerAllocationSweep() {
    int success=-1;
    for(int i=0;i<500;++i) {
        DRW_Header h;LayerFaultWriter w(nullptr,&h);
        if(!w.stageDefaultDwgPlotStyleGraph() || !w.stageDefaultDwgMaterialGraph()
           || !w.prepareDwgClassManifest() || !w.writeFileHeaderStub()
           || !w.writeDwgHeader() || !w.writeDwgClasses())return false;
        w.freezeDwgClasses(); const auto checkpoint=w.checkpointCompoundWrite();
        const auto before=w.buffer(); const auto generation=DwgBlockRecordXrefWireTestAccess::generation(w);
        w.fault=i;const bool written=DwgBlockRecordXrefWireTestAccess::layerFrame(w);
        if(w.escaped)return false;
        if(!w.triggered) {if(!written)return false;success=i;break;}
        if(written || !DwgBlockRecordXrefWireTestAccess::cleanLayerFailure(w,before,checkpoint.objectMapSize,generation)) {
            std::cerr<<"layer frame allocation "<<i<<"\n";return false;
        }
    }
    return success>0;
}
bool ac1027AllocationSweep() {
    int success=-1;
    for(int i=0;i<1500;++i) {
        DRW_Header h; dwgWriter27 w(nullptr,&h);
        if(!w.stageDefaultDwgPlotStyleGraph() || !w.stageDefaultDwgMaterialGraph()
           || !w.prepareDwgClassManifest() || !w.writeFileHeaderStub()
           || !w.writeDwgHeader() || !w.writeDwgClasses())return false;
        w.freezeDwgClasses(); if(!w.writeDwgObjects())return false;
        // Isolate the material transaction from the separately committed plotstyle transaction.
        DwgBlockRecordXrefWireTestAccess::markPlotGraphComplete(w);
        const auto before=w.checkpointCompoundWrite();const auto bytes=w.buffer();
        failAfter=i;bool written=false;
        try {written=w.completeDeferredDwgObjects();} catch(...) {failAfter=-1;std::cerr<<"escaped allocation "<<i<<"\n";return false;}
        const bool triggered=failAfter<0;failAfter=-1;
        if(!triggered) {if(!written)return false;success=i;break;}
        if(written || w.buffer()!=bytes) {std::cerr<<"bytes after allocation "<<i<<"\n";return false;}
        const auto after=w.checkpointCompoundWrite();
        if(before.objectMapSize!=after.objectMapSize || before.defaultMaterialHandles!=after.defaultMaterialHandles
           || before.defaultMaterialGraphEmitted!=after.defaultMaterialGraphEmitted
           || before.classInstanceEmittedHandles!=after.classInstanceEmittedHandles
           || before.classInstanceEmissionCounts!=after.classInstanceEmissionCounts
           || !w.completeDeferredDwgObjects()) {std::cerr<<"state/retry after allocation "<<i<<"\n";return false;}
    }
    return success>0;
}
bool headerWireControls() {
    for (auto version : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                         DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        DRW_Header header; dwgBufferW data, handles, strings;
        if (!DrwHeaderEncodeTestAccess::encode(header, version, data, handles, strings,
                                               0xD1u, 0xD2u)) return false;
        const auto& tokens=handles.handleOccurrences();
        const auto count=[&tokens](std::uint32_t reference) {
            return std::count_if(tokens.begin(), tokens.end(), [reference](const auto& token) {
                return token.reference==reference && token.code==DRW::DwgHardPointer;
            });
        };
        if (count(0xD1u)!=(version>DRW::AC1015?1:0)
            || count(0xD2u)!=(version>DRW::AC1018?1:0)) return false;
        if (version>DRW::AC1018 && (tokens.size()<5u || tokens[3].reference!=0xD2u
            || tokens[3].code!=DRW::DwgHardPointer)) return false;
        if (version>DRW::AC1015) {
            const auto dict=std::find_if(tokens.begin(),tokens.end(),[](const auto& token) {
                return token.reference==0xD1u;
            });
            if(dict==tokens.end() || dict==tokens.begin() || dict+1==tokens.end()
                || (dict-1)->code!=DRW::DwgHardPointer || (dict-1)->reference!=0u
                || (dict+1)->code!=DRW::DwgHardPointer || (dict+1)->reference!=0u) return false;
        }
    }
    return true;
}
bool directControls() {
    if (!headerWireControls()) return false;
    for (const auto code : {6u, 8u, 10u, 12u, 9u}) {
        DRW_Header relativeHeader;
        ThrowingWriter relative(nullptr, &relativeHeader);
        if (!relative.stageDefaultDwgMaterialGraph()) return false;
        const auto target=relative.defaultDwgMaterialGraphHandles()[2];
        const auto owner=(code==6u||code==10u)?target-1u:target+1u;
        const auto delta=(code==10u||code==12u)?1u:0u;
        if (!DwgBlockRecordXrefWireTestAccess::tokenConflict(
                relative, owner, static_cast<std::uint8_t>(code), delta)) return false;
    }
    DRW_Header header;
    ThrowingWriter writer(nullptr, &header);
    auto initial = writer.checkpointPublicTransaction();
    if (!writer.registerDwgNamedObjectDictionaryEntry("acad_material", 0xB100u)) return false;
    auto high = writer.highWaterHandle();
    if (writer.stageDefaultDwgMaterialGraph() || writer.highWaterHandle() != high) return false;
    writer.rollbackCompoundWrite(initial);
    high = writer.highWaterHandle(); writer.failAdmission = true;
    if (writer.stageDefaultDwgMaterialGraph() || writer.highWaterHandle() != high) return false;
    writer.failAdmission = false;
    if (!writer.stageDefaultDwgMaterialGraph()) return false;
    const auto handles = writer.defaultDwgMaterialGraphHandles();
    for (std::size_t i=0; i<handles.size(); ++i)
        if (handles[i] != high+i) return false;
    if (!writer.prepareDwgClassManifest() || !writer.writeFileHeaderStub()
        || !writer.writeDwgHeader() || !writer.writeDwgClasses()) return false;
    writer.freezeDwgClasses();
    if (!writer.writeDwgObjects()) return false;
    const auto checkpoint = writer.checkpointCompoundWrite();
    const auto bytes = writer.buffer();
    writer.failEmission = true;
    if (writer.completeDeferredDwgObjects() || writer.buffer() != bytes) return false;
    writer.failEmission = false;
    DwgBlockRecordXrefWireTestAccess::failAfterDictionary(writer);
    if (writer.completeDeferredDwgObjects() || writer.buffer() != bytes) return false;
    const auto after = writer.checkpointCompoundWrite();
    if (checkpoint.objectMapSize != after.objectMapSize
        || checkpoint.defaultMaterialHandles != after.defaultMaterialHandles
        || checkpoint.defaultMaterialGraphEmitted != after.defaultMaterialGraphEmitted
        || checkpoint.classInstanceEmittedHandles != after.classInstanceEmittedHandles
        || checkpoint.classInstanceEmissionCounts != after.classInstanceEmissionCounts)
        return false;
    return writer.completeDeferredDwgObjects()
        && writer.validateDwgNamedObjectDictionaryEntries()
        && writer.validateDwgClassInstanceCounts() && writer.validateDwgClassInstanceFrames();
}

enum class Mode { Normal, EarlyRoot, SourceRoot, HeaderInvalid, HeaderGlobal, EarlyMaterialClass, LateMaterialClass,
                  EarlyRawSection, LateRawSection, LateRoot, NodAlias, GroupAlias, DictionaryOwner,
                  DictionaryChild, DictionaryHandle, XrecordOwner, RawObject, CallerMaterial,
                  ReplayMaterial, SuppliedLayer, SuppliedLayerGlobal,
                  ExtraLayerGlobal, ExtraLayerConflictingPlot, LateEntityHandle };
struct Adapter : LC3CompatCheck, DRW_DwgDefaultMaterialProvider, DRW_DwgDefaultPlotStyleProvider {
    explicit Adapter(dwgRW& writer) : output(writer) {}
    bool wantsDwgDefaultMaterialGraph() const override { return enabled; }
    bool dwgDefaultMaterialOpaqueReferencesAbsent(std::uint32_t a, std::uint32_t b,
            std::uint32_t c, std::uint32_t d) const override {
        return opaqueAbsent && a != 0 && b != 0 && c != 0 && d != 0;
    }
    bool wantsDwgDefaultPlotStyleGraph() const override { return plotEnabled; }
    void dwgDefaultPlotStyleGraphPlanned(
            std::uint32_t, std::uint32_t placeholder) override {
        plotStylePlaceholder = placeholder;
    }
    void dwgDefaultMaterialGraphPlanned(std::uint32_t a, std::uint32_t b,
                                        std::uint32_t c, std::uint32_t d) override {
        handles = {a,b,c,d};
        if (mode == Mode::HeaderGlobal && header != nullptr) {
            std::ostringstream value; value << std::hex << d;
            header->addStr("$CMATERIAL", value.str(), 347);
        }
    }
    void writeHeader(DRW_Header& h) override {
        header=&h;
        for (const auto* key : {"$CMATERIAL", "CMATERIAL"}) {
            const auto it=h.vars.find(key);
            if (it!=h.vars.end()) { delete it->second; h.vars.erase(it); }
        }
        if (mode == Mode::HeaderInvalid) h.addStr("$CMATERIAL", "B200", 347);
    }
    void writeDwgClasses() override {
        if (mode == Mode::EarlyRoot || mode == Mode::SourceRoot)
            (void)output.registerDwgNamedObjectDictionaryEntry("acad_material", 0xB100u);
        if (mode == Mode::EarlyMaterialClass) {
            DRW_Material material; material.handle=0xB101u;
            (void)output.registerMaterialObjectClass(&material);
        }
        if (mode == Mode::ReplayMaterial) {
            (void)output.registerDwgNamedObjectDictionaryEntry(
                "ACAD_MATERIAL", 0xB100u);
            const std::size_t count = replayMaterials.empty()
                ? 1u : replayMaterials.size();
            for (std::size_t i = 0; i < count; ++i) {
                DRW_Material material;
                material.handle = static_cast<std::uint32_t>(0xB101u + i);
                (void)output.registerMaterialObjectClass(&material);
            }
        }
        if (mode == Mode::EarlyRawSection) {
            DRW_RawDwgSection section; section.m_name="AcDb:OpaqueProbe";
            section.m_version=version; section.m_data={1,2,3};
            (void)output.writeRawDwgSection(&section);
        }
    }
    void writeLTypes() override {
        if (mode == Mode::LateMaterialClass) {
            DRW_Material material; material.handle=0xB101u;
            rejected=!output.registerMaterialObjectClass(&material);
        }
        if (mode == Mode::LateRawSection) {
            DRW_RawDwgSection section; section.m_name="AcDb:OpaqueProbe";
            section.m_version=version; section.m_data={1,2,3};
            rejected=!output.writeRawDwgSection(&section);
        }
        if (mode == Mode::LateRoot)
            rejected = !output.registerDwgNamedObjectDictionaryEntry("aCaD_mAtErIaL", 0xB100u);
        if (mode == Mode::NodAlias)
            rejected = !output.registerDwgNamedObjectDictionaryEntry("ALIAS", handles[slot]);
        if (mode == Mode::GroupAlias)
            rejected = !output.registerDwgGroupDictionaryEntry("ALIAS", handles[slot]);
    }
    void writeLayers() override {
        if (mode == Mode::SuppliedLayer || mode == Mode::SuppliedLayerGlobal) {
            DRW_Layer layer; layer.name="0";
            layer.materialHandle.ref = mode == Mode::SuppliedLayerGlobal
                ? handles[3] : 0xB200u;
            supplied = output.addLayer(&layer);
        }
        if (mode == Mode::ExtraLayerGlobal || mode == Mode::ExtraLayerConflictingPlot) {
            DRW_Layer layer; layer.name = "Defpoints";
            layer.materialHandle.ref = handles[3];
            if (mode == Mode::ExtraLayerConflictingPlot)
                layer.plotStyleHandle.ref = handles[3];
            supplied = output.addLayer(&layer);
        }
    }
    void writeEntities() override {
        DRW_Line line; line.basePoint={1.,2.,3.}; line.secPoint={4.,5.,6.};
        if (mode == Mode::LateEntityHandle) line.handle=handles[slot];
        (void)output.writeLine(&line);
    }
    void writeObjects() override {
        if (mode == Mode::SourceRoot) {
            DRW_Dictionary source; source.handle=0xB100u;
            source.parentHandle=DRW::DwgNamedObjectsDictionaryHandle;
            (void)output.writeDictionary(&source);
        }
        if (mode == Mode::ReplayMaterial) {
            struct AllocationFaultScope {
                bool enabled;
                bool& triggered;
                ~AllocationFaultScope() {
                    if (enabled) {
                        triggered = failAfter < 0;
                        failAfter = -1;
                    }
                }
            } faultScope{replayAllocationOffset >= 0,
                         replayAllocationTriggered};
            if (faultScope.enabled)
                failAfter = replayAllocationOffset;
            DRW_Dictionary dictionary; dictionary.handle=0xB100u;
            dictionary.parentHandle=DRW::DwgNamedObjectsDictionaryHandle;
            const std::size_t count = replayMaterials.empty()
                ? 1u : replayMaterials.size();
            for (std::size_t i = 0; i < count; ++i) {
                const DRW_Material& source = replayMaterials.empty()
                    ? replayMaterial : replayMaterials[i];
                dictionary.m_entries.push_back({source.m_name,
                    static_cast<std::uint32_t>(0xB101u + i)});
            }
            if (!output.writeDictionary(&dictionary)) { rejected=true; return; }
            for (std::size_t i = 0; i < count; ++i) {
                DRW_Material material = replayMaterials.empty()
                    ? replayMaterial : replayMaterials[i];
                material.handle=static_cast<std::uint32_t>(0xB101u + i);
                material.parentHandle=0xB100u;
                material.reactorHandles={0xB100u};
                const auto xdict = replayHandleMap.find(material.xDictHandle);
                if (xdict != replayHandleMap.end())
                    material.xDictHandle = xdict->second;
                else if (material.xDictHandle != 0u && version == DRW::AC1024) {
                    rejected = true;
                    return;
                }
                if (!output.writeMaterial(&material)) {
                    rejected=true;
                    return;
                }
            }
            for (DRW_Dictionary dictionary : replayExtensionDictionaries) {
                const auto own = replayHandleMap.find(dictionary.handle);
                const auto parent = replayHandleMap.find(dictionary.parentHandle);
                if (own == replayHandleMap.end() || parent == replayHandleMap.end()) {
                    rejected = true;
                    return;
                }
                dictionary.handle = own->second;
                dictionary.parentHandle = parent->second;
                for (auto& entry : dictionary.m_entries) {
                    const auto target = replayHandleMap.find(entry.m_handle);
                    if (target == replayHandleMap.end()) {
                        rejected = true;
                        return;
                    }
                    entry.m_handle = target->second;
                }
                if (!output.writeDictionary(&dictionary)) {
                    rejected = true;
                    return;
                }
            }
            for (DRW_XRecord record : replayXRecords) {
                const auto own = replayHandleMap.find(record.handle);
                const auto parent = replayHandleMap.find(record.parentHandle);
                if (own == replayHandleMap.end() || parent == replayHandleMap.end()) {
                    rejected = true;
                    return;
                }
                record.handle = own->second;
                record.parentHandle = parent->second;
                for (auto& reactor : record.reactorHandles) {
                    const auto target = replayHandleMap.find(reactor);
                    if (target == replayHandleMap.end()) {
                        rejected = true;
                        return;
                    }
                    reactor = target->second;
                }
                if (!output.writeXRecord(&record)) {
                    rejected = true;
                    return;
                }
            }
        }
        if (mode == Mode::DictionaryOwner || mode == Mode::DictionaryChild || mode == Mode::DictionaryHandle) {
            DRW_Dictionary d; d.handle=0xC500; d.parentHandle=DRW::DwgNamedObjectsDictionaryHandle;
            if (mode == Mode::DictionaryOwner) d.parentHandle=handles[slot];
            if (mode == Mode::DictionaryChild) d.m_entries.push_back({"illegal",handles[slot]});
            if (mode == Mode::DictionaryHandle) d.handle=handles[slot];
            rejected = !output.writeDictionary(&d);
        }
        if (mode == Mode::XrecordOwner) {
            DRW_XRecord x; x.handle=0xC501; x.parentHandle=handles[slot];
            rejected = !output.writeXRecord(&x);
        }
        if (mode == Mode::CallerMaterial) {
            DRW_Material m; m.handle=0xC502; rejected=!output.writeMaterial(&m);
        }
        if (mode == Mode::RawObject) {
            DRW_UnsupportedObject raw; rejected=!output.writeRawDwgObject(&raw);
        }
    }
    dwgRW& output; DRW_Header* header=nullptr;
    DRW::Version version=DRW::AC1024;
    bool enabled=true, plotEnabled=true, opaqueAbsent=true, rejected=false, supplied=false;
    Mode mode=Mode::Normal; std::size_t slot=0;
    DRW_Material replayMaterial;
    std::vector<DRW_Material> replayMaterials;
    std::vector<DRW_Dictionary> replayExtensionDictionaries;
    std::vector<DRW_XRecord> replayXRecords;
    std::unordered_map<std::uint32_t, std::uint32_t> replayHandleMap;
    int replayAllocationOffset{-1};
    bool replayAllocationTriggered{false};
    std::array<std::uint32_t,4> handles{};
    std::uint32_t plotStylePlaceholder=0;
};
struct Capture : LC3CompatCheck {
    void addNamedObjectDictionary(const DRW_Dictionary& value) override {
        for (const auto& e:value.m_entries)
            if(e.m_name=="ACAD_MATERIAL" || e.m_name=="acad_material") root=e.m_handle;
    }
    void addDictionary(const DRW_Dictionary& value) override {
        sourceDictionaries.push_back(value);
        if(value.m_entries.size()!=3u) return;
        const std::array<std::string, 3> names{{"ByBlock", "ByLayer", "Global"}};
        std::array<std::uint32_t, 3> handles{};
        for (std::size_t i = 0; i < names.size(); ++i) {
            const auto entry = std::find_if(value.m_entries.begin(),
                value.m_entries.end(), [&names, i](const DRW_Dictionary::Entry& e) {
                    return e.m_name == names[i];
                });
            if (entry == value.m_entries.end()) return;
            handles[i] = entry->m_handle;
        }
        dictionary=value.handle; owner=value.parentHandle; children=handles;
    }
    void addMaterial(const DRW_Material& m) override {
        parsedMaterials.push_back(m);
        const bool r2013Profile = rawVersion == DRW::AC1027;
        const bool autoCad2016Profile = physicalAc1024AutoCadProfile;
        const auto close = [](double left, double right) {
            return std::abs(left - right) <= 1e-9;
        };
        const auto mapTransformMatches = [&](const std::array<double,16>& actual,
                                             bool globalScale) {
            std::array<double,16> expected{{1.0,0.0,0.0,0.0,
                0.0,1.0,0.0,0.0,0.0,0.0,1.0,0.0,
                0.0,0.0,0.0,1.0}};
            if (globalScale) expected[0]=expected[5]=0.0008;
            for (std::size_t index=0; index<actual.size(); ++index)
                if (!close(actual[index],expected[index])) return false;
            return true;
        };
        const bool globalScale=physicalAc1021Profile && m.m_name=="Global";
        const bool physicalFieldsMatch = !physicalAc1021Profile || (
            m.m_ambientColorMethod==0u && close(m.m_ambientColorFactor,1.0)
            && !m.m_hasAmbientColorValue
            && m.m_diffuseColorMethod==0u && close(m.m_diffuseColorFactor,1.0)
            && !m.m_hasDiffuseColorValue
            && m.m_specularColorMethod==0u && close(m.m_specularColorFactor,1.0)
            && !m.m_hasSpecularColorValue
            && close(m.m_diffuseMapBlendFactor,1.0)
            && m.m_diffuseMapSource==1u && m.m_diffuseMapProjection==2u
            && m.m_diffuseMapTiling==1u && m.m_diffuseMapAutoTransform==1u
            && mapTransformMatches(m.m_diffuseMapTransform,globalScale)
            && close(m.m_specularGlossFactor,0.5)
            && close(m.m_specularMapBlendFactor,1.0)
            && m.m_specularMapSource==1u && m.m_specularMapProjection==2u
            && m.m_specularMapTiling==1u && m.m_specularMapAutoTransform==1u
            && mapTransformMatches(m.m_specularMapTransform,false)
            && close(m.m_reflectionMapBlendFactor,1.0)
            && m.m_reflectionMapSource==1u && m.m_reflectionMapProjection==2u
            && m.m_reflectionMapTiling==1u && m.m_reflectionMapAutoTransform==1u
            && mapTransformMatches(m.m_reflectionMapTransform,globalScale)
            && close(m.m_opacityPercent,1.0)
            && close(m.m_opacityMapBlendFactor,1.0)
            && m.m_opacityMapSource==1u && m.m_opacityMapProjection==2u
            && m.m_opacityMapTiling==1u && m.m_opacityMapAutoTransform==1u
            && mapTransformMatches(m.m_opacityMapTransform,globalScale)
            && close(m.m_bumpMapBlendFactor,1.0)
            && m.m_bumpMapSource==1u && m.m_bumpMapProjection==2u
            && m.m_bumpMapTiling==1u && m.m_bumpMapAutoTransform==1u
            && mapTransformMatches(m.m_bumpMapTransform,globalScale)
            && close(m.m_refractionIndex,1.0)
            && close(m.m_refractionMapBlendFactor,1.0)
            && m.m_refractionMapSource==0u && m.m_refractionMapProjection==2u
            && m.m_refractionMapTiling==1u && m.m_refractionMapAutoTransform==1u
            && mapTransformMatches(m.m_refractionMapTransform,false)
            && close(m.m_translucence,0.0)
            && close(m.m_reflectivity,0.0)
            && m.m_illuminationModel==0 && m.m_channelFlags==63);
        if (physicalAc1021Profile && !physicalFieldsMatch) {
            std::cerr << "AC1021 MATERIAL field mismatch: " << m.m_name
                      << " methods=" << unsigned(m.m_ambientColorMethod)
                      << ',' << unsigned(m.m_diffuseColorMethod) << ','
                      << unsigned(m.m_specularColorMethod)
                      << " hasColor=" << m.m_hasAmbientColorValue << ','
                      << m.m_hasDiffuseColorValue << ','
                      << m.m_hasSpecularColorValue
                      << " refractionSource="
                      << unsigned(m.m_refractionMapSource)
                      << " channels=" << m.m_channelFlags << " transforms=";
            for (double value : m.m_diffuseMapTransform)
                std::cerr << value << ',';
            std::cerr << " map-tuples="
                      << m.m_diffuseMapBlendFactor << ',' << unsigned(m.m_diffuseMapSource) << ',' << unsigned(m.m_diffuseMapProjection) << ',' << unsigned(m.m_diffuseMapTiling) << ',' << unsigned(m.m_diffuseMapAutoTransform) << ';'
                      << m.m_specularMapBlendFactor << ',' << unsigned(m.m_specularMapSource) << ',' << unsigned(m.m_specularMapProjection) << ',' << unsigned(m.m_specularMapTiling) << ',' << unsigned(m.m_specularMapAutoTransform) << ';'
                      << m.m_reflectionMapBlendFactor << ',' << unsigned(m.m_reflectionMapSource) << ',' << unsigned(m.m_reflectionMapProjection) << ',' << unsigned(m.m_reflectionMapTiling) << ',' << unsigned(m.m_reflectionMapAutoTransform) << ';'
                      << m.m_opacityMapBlendFactor << ',' << unsigned(m.m_opacityMapSource) << ',' << unsigned(m.m_opacityMapProjection) << ',' << unsigned(m.m_opacityMapTiling) << ',' << unsigned(m.m_opacityMapAutoTransform) << ';'
                      << m.m_bumpMapBlendFactor << ',' << unsigned(m.m_bumpMapSource) << ',' << unsigned(m.m_bumpMapProjection) << ',' << unsigned(m.m_bumpMapTiling) << ',' << unsigned(m.m_bumpMapAutoTransform) << ';'
                      << m.m_refractionMapBlendFactor << ',' << unsigned(m.m_refractionMapSource) << ',' << unsigned(m.m_refractionMapProjection) << ',' << unsigned(m.m_refractionMapTiling) << ',' << unsigned(m.m_refractionMapAutoTransform)
                      << " factors=" << m.m_specularGlossFactor << ',' << m.m_opacityPercent << ',' << m.m_refractionIndex << ',' << m.m_translucence << ',' << m.m_reflectivity << ',' << m.m_illuminationModel
                      << " transforms=" << mapTransformMatches(m.m_diffuseMapTransform,true) << ',' << mapTransformMatches(m.m_specularMapTransform,false) << ',' << mapTransformMatches(m.m_reflectionMapTransform,true) << ',' << mapTransformMatches(m.m_opacityMapTransform,true) << ',' << mapTransformMatches(m.m_bumpMapTransform,true) << ',' << mapTransformMatches(m.m_refractionMapTransform,false) << '\n';
        }
        if (m.parentHandle!=dictionary
            || m.reactorHandles!=std::vector<std::uint32_t>{dictionary}
            || !physicalFieldsMatch
            || (requireSemantic && (!m.hasCompleteDwgSemanticFields()
                || (expectDefaultBody
                    && (m.m_diffuseColorMethod != (r2013Profile ? 0u : 1u)
                        || m.m_hasDiffuseColorValue == r2013Profile
                        || (!r2013Profile
                            && m.m_diffuseColorValue != 0xc2ffffffu)))
                || m.m_channelFlags != (r2013Profile || autoCad2016Profile
                    ? 127 : 63)))
            || (!requireSemantic && m.hasCompleteDwgSemanticFields())
            || (requireRaw && (!m.hasDwgRawData() || m.dwgRawDataVersion()!=rawVersion
                || m.dwgRawDataBitSize() != (r2013Profile ? 516u
                    : autoCad2016Profile ? 1284u : 550u))))
            valid=false;
        materials.push_back({m.m_name,m.handle});
    }
    void addXRecord(const DRW_XRecord& value) override {
        sourceXRecords.push_back(value);
    }
    void addLayer(const DRW_Layer& layer) override {
        if(layer.name=="0") {
            layerMaterial=layer.materialHandle.ref;
            layerPlotStyle=layer.plotStyleHandle.ref;
        }
        if (layer.name == "Defpoints") extraLayerMaterial = layer.materialHandle.ref;
    }
    bool matches(const std::array<std::uint32_t,4>& h, bool supplied,
                 std::uint32_t expectedPlotStyle=0u) const {
        if (!valid || root!=h[0] || dictionary!=h[0] || owner!=DRW::DwgNamedObjectsDictionaryHandle
            || children!=std::array<std::uint32_t,3>{h[1],h[2],h[3]}
            || materials.size()!=3u || layerMaterial!=(supplied?0xB200u:h[3])
            || (expectedPlotStyle!=0u && layerPlotStyle!=expectedPlotStyle)) return false;
        const std::array<std::string,3> names{"ByBlock","ByLayer","Global"};
        for(std::size_t i=0;i<3;++i)
            if(std::find(materials.begin(),materials.end(),std::make_pair(names[i],h[i+1]))==materials.end()) return false;
        return true;
    }
    std::uint32_t root=0,dictionary=0,owner=0,layerMaterial=0,layerPlotStyle=0,extraLayerMaterial=0;
    std::array<std::uint32_t,3> children{};
    std::vector<std::pair<std::string,std::uint32_t>> materials;
    std::vector<DRW_Material> parsedMaterials;
    std::vector<DRW_Dictionary> sourceDictionaries;
    std::vector<DRW_XRecord> sourceXRecords;
    bool valid=true, requireSemantic=true, requireRaw=false;
    bool expectDefaultBody=true;
    bool physicalAc1021Profile=false;
    bool physicalAc1024AutoCadProfile=false;
    DRW::Version rawVersion=DRW::AC1021;
};
bool inspect(const std::string& path,const std::array<std::uint32_t,4>& h,bool supplied,
             DRW::Version version, std::uint32_t expectedPlotStyle=0u) {
    dwgRW reader(path.c_str()); Capture capture;
    // AC1021/AC1024 decode the bounded nonprocedural body; AC1027 additionally
    // requires the exact generated default body/string profile. Retained raw
    // payload is still checked for AC1021 and AC1027.
    capture.requireSemantic = version == DRW::AC1021
        || version == DRW::AC1024 || version == DRW::AC1027;
    capture.requireRaw = version == DRW::AC1021 || version == DRW::AC1027;
    capture.rawVersion = version;
    const bool read = reader.read(&capture, false);
    const bool matched = capture.matches(h, supplied, expectedPlotStyle);
    if (!read || !matched) {
        std::cerr << "inspect read=" << read << " match=" << matched
                  << " root=" << capture.root << " dict=" << capture.dictionary
                  << " layerMaterial=" << capture.layerMaterial
                  << " layerPlotStyle=" << capture.layerPlotStyle
                  << " expectedPlotStyle=" << expectedPlotStyle
                  << " materials=" << capture.materials.size()
                  << " valid=" << capture.valid << '\n';
    }
    return read && matched;
}

bool defaultMaterialReplayGraphRoundtripImpl(
        DRW::Version version,
        const std::vector<DRW_Material>& sources,
        const std::filesystem::path& requestedOutput = {},
        const std::vector<DRW_Dictionary>& extensionDictionaries = {},
        const std::vector<DRW_XRecord>& xrecords = {},
        const std::unordered_map<std::uint32_t, std::uint32_t>& handleMap = {},
        int allocationFailureOffset = -1) {
    const auto replayable = [version](const DRW_Material& source) {
        return version == DRW::AC1024
            ? source.canReplayAc1024AutoCad2016DefaultDwgBody()
            : version == DRW::AC1027
                && source.canReplayAc1027DefaultDwgBody();
    };
    if (sources.empty()
        || !std::all_of(sources.begin(), sources.end(),
            replayable))
        return false;
    const auto nonce = std::chrono::steady_clock::now()
        .time_since_epoch().count();
    const auto path = requestedOutput.empty()
        ? std::filesystem::temp_directory_path()
            / ("libdxfrw-material-replay-" + std::to_string(
                   static_cast<unsigned>(version)) + "-" + std::to_string(nonce)
               + ".dwg")
        : requestedOutput;
    if (!requestedOutput.empty() && std::filesystem::exists(path))
        return false;
    struct Cleanup {
        std::filesystem::path path;
        bool remove{true};
        ~Cleanup() {
            if (remove) {
                std::error_code error;
                std::filesystem::remove(path, error);
            }
        }
    } cleanup{path, requestedOutput.empty()};

    const std::string priorDestination = "prior destination";
    if (allocationFailureOffset >= 0) {
        std::ofstream prior(path, std::ios::binary | std::ios::trunc);
        prior.write(priorDestination.data(),
                    static_cast<std::streamsize>(priorDestination.size()));
        if (!prior)
            return false;
    }

    dwgRW writer(path.string().c_str());
    Adapter adapter(writer);
    adapter.version = version;
    adapter.enabled = false;
    adapter.plotEnabled = false;
    adapter.mode = Mode::ReplayMaterial;
    adapter.replayMaterials = sources;
    adapter.replayExtensionDictionaries = extensionDictionaries;
    adapter.replayXRecords = xrecords;
    adapter.replayHandleMap = handleMap;
    if (!writer.reserveHandle(0xB100u)) {
        return false;
    }
    for (std::size_t i = 0; i < sources.size(); ++i) {
        if (!writer.reserveHandle(static_cast<std::uint32_t>(0xB101u + i)))
            return false;
    }
    for (const auto& mapping : handleMap) {
        if (mapping.second <= 0xB103u)
            continue;
        if (!writer.reserveHandle(mapping.second))
            return false;
    }
    if (allocationFailureOffset >= 0) {
        bool failedWrite = false;
        bool escaped = false;
        adapter.replayAllocationOffset = allocationFailureOffset;
        try {
            failedWrite = !writer.write(&adapter, version, true);
        } catch (...) {
            escaped = true;
        }
        if (escaped || !failedWrite || !adapter.replayAllocationTriggered) {
            std::cerr << "selected MATERIAL injected allocation did not fail transactionally for version "
                      << static_cast<unsigned>(version) << '\n';
            return false;
        }
        std::ifstream prior(path, std::ios::binary);
        const std::string actual(std::istreambuf_iterator<char>(prior), {});
        if (actual != priorDestination) {
            std::cerr << "selected MATERIAL allocation failure changed destination for version "
                      << static_cast<unsigned>(version) << '\n';
            return false;
        }
        adapter.replayAllocationOffset = -1;
    }
    bool written = false;
    try {
        written = writer.write(&adapter, version, true);
    } catch (...) {
        std::cerr << "selected MATERIAL same-writer retry escaped for version "
                  << static_cast<unsigned>(version) << '\n';
        return false;
    }
    if (!written) {
        std::cerr << "selected MATERIAL writer roundtrip could not finalize for version "
                  << static_cast<unsigned>(version) << " error="
                  << writer.getError();
        const auto diagnostic = writer.getLastDiagnostic();
        std::cerr << " diagnostic=" << diagnostic.code << ':'
                  << diagnostic.message << '\n';
        return false;
    }

    class ReplayCapture final : public LC3CompatCheck {
    public:
        ReplayCapture(DRW::Version fileVersion,
                      const std::vector<DRW_Material>& values,
                      const std::vector<DRW_Dictionary>& dictionaries,
                      const std::vector<DRW_XRecord>& records)
            : version(fileVersion), expected(values),
              expectedDictionaries(dictionaries), expectedXRecords(records),
              materialSeen(values.size(), false),
              dictionarySeenByHandle(dictionaries.size(), false),
              xrecordSeenByHandle(records.size(), false) {}
        void addNamedObjectDictionary(const DRW_Dictionary& value) override {
            for (const DRW_Dictionary::Entry& entry : value.m_entries)
                if (entry.m_name == "ACAD_MATERIAL")
                    rootHandle = entry.m_handle;
        }
        void addDictionary(const DRW_Dictionary& value) override {
            if (value.handle == 0xB100u) {
                dictionarySeen = value.parentHandle
                        == DRW::DwgNamedObjectsDictionaryHandle
                    && value.m_entries.size() == expected.size();
                for (std::size_t i = 0; dictionarySeen && i < expected.size(); ++i)
                    dictionarySeen = value.m_entries[i].m_name == expected[i].m_name
                        && value.m_entries[i].m_handle
                            == static_cast<std::uint32_t>(0xB101u + i);
                return;
            }
            for (std::size_t i = 0; i < expectedDictionaries.size(); ++i) {
                const DRW_Dictionary& want = expectedDictionaries[i];
                if (value.handle != want.handle)
                    continue;
                bool matches = value.parentHandle == want.parentHandle
                    && value.cloning == want.cloning
                    && value.hardOwner == want.hardOwner
                    && value.m_entries.size() == want.m_entries.size()
                    && value.reactorHandles == want.reactorHandles;
                for (std::size_t entry = 0; matches && entry < want.m_entries.size(); ++entry)
                    matches = value.m_entries[entry].m_name == want.m_entries[entry].m_name
                        && value.m_entries[entry].m_handle == want.m_entries[entry].m_handle
                        && value.m_entries[entry].m_dxfHandleCode
                            == want.m_entries[entry].m_dxfHandleCode;
                dictionarySeenByHandle[i] = matches;
                return;
            }
        }
        void addXRecord(const DRW_XRecord& value) override {
            for (std::size_t i = 0; i < expectedXRecords.size(); ++i) {
                const DRW_XRecord& want = expectedXRecords[i];
                if (value.handle != want.handle)
                    continue;
                xrecordSeenByHandle[i] = value.parentHandle == want.parentHandle
                    && value.reactorHandles == want.reactorHandles
                    && value.m_cloning == want.m_cloning
                    && value.m_dataEntries.size() == want.m_dataEntries.size();
                for (std::size_t entry = 0;
                     xrecordSeenByHandle[i] && entry < want.m_dataEntries.size(); ++entry) {
                    const DRW_Variant& actual = value.m_dataEntries[entry];
                    const DRW_Variant& expectedEntry = want.m_dataEntries[entry];
                    xrecordSeenByHandle[i] = actual.code() == expectedEntry.code()
                        && actual.type() == expectedEntry.type()
                        && (actual.type() != DRW_Variant::INTEGER
                            || actual.content.i == expectedEntry.content.i);
                }
                return;
            }
        }
        void addMaterial(const DRW_Material& value) override {
            if (value.handle < 0xB101u
                || value.handle >= 0xB101u + expected.size())
                return;
            const std::size_t index = value.handle - 0xB101u;
            const bool profile = version == DRW::AC1024
                ? value.canReplayAc1024AutoCad2016DefaultDwgBody()
                : value.canReplayAc1027DefaultDwgBody();
            const auto& sourceRaw = expected[index].dwgRawData();
            const auto& outputRaw = value.dwgRawData();
            bool sameRawBits = value.dwgRawDataBitSize()
                    == expected[index].dwgRawDataBitSize()
                && sourceRaw.size() * 8u >= value.dwgRawDataBitSize()
                && outputRaw.size() * 8u >= value.dwgRawDataBitSize();
            for (std::uint32_t bit = 0u;
                 sameRawBits && bit < value.dwgRawDataBitSize(); ++bit) {
                const unsigned sourceBit = (sourceRaw[bit / 8u]
                    >> (7u - bit % 8u)) & 1u;
                const unsigned outputBit = (outputRaw[bit / 8u]
                    >> (7u - bit % 8u)) & 1u;
                sameRawBits = sourceBit == outputBit;
            }
            if (!profile)
                std::cerr << "replay readback profile failed: version="
                          << static_cast<unsigned>(version) << " name="
                          << value.m_name << " raw=" << value.dwgRawDataBitSize()
                          << " semantic=" << value.hasCompleteDwgSemanticFields()
                          << " owner=" << value.parentHandle << " react="
                          << value.reactorHandles.size() << '\n';
            materialSeen[index] = value.parentHandle == 0xB100u
                && value.xDictHandle == expected[index].xDictHandle
                && value.reactorHandles == std::vector<std::uint32_t>{0xB100u}
                && value.m_name == expected[index].m_name
                && value.hasCompleteDwgSemanticFields()
                && profile
                && sameRawBits
                && value.m_diffuseMapFileName.empty()
                && value.m_specularMapFileName.empty()
                && value.m_reflectionMapFileName.empty()
                && value.m_opacityMapFileName.empty()
                && value.m_bumpMapFileName.empty()
                && value.m_refractionMapFileName.empty();
        }
        std::uint32_t rootHandle{0u};
        DRW::Version version;
        std::vector<DRW_Material> expected;
        std::vector<DRW_Dictionary> expectedDictionaries;
        std::vector<DRW_XRecord> expectedXRecords;
        std::vector<bool> materialSeen;
        std::vector<bool> dictionarySeenByHandle;
        std::vector<bool> xrecordSeenByHandle;
        bool dictionarySeen{false};
    };
    std::vector<DRW_Dictionary> expectedDictionaries = extensionDictionaries;
    std::vector<DRW_XRecord> expectedXRecords = xrecords;
    for (DRW_Dictionary& dictionary : expectedDictionaries) {
        dictionary.handle = handleMap.at(dictionary.handle);
        dictionary.parentHandle = handleMap.at(dictionary.parentHandle);
        for (auto& entry : dictionary.m_entries)
            entry.m_handle = handleMap.at(entry.m_handle);
    }
    for (DRW_XRecord& record : expectedXRecords) {
        record.handle = handleMap.at(record.handle);
        record.parentHandle = handleMap.at(record.parentHandle);
        for (auto& reactor : record.reactorHandles)
            reactor = handleMap.at(reactor);
    }
    std::vector<DRW_Material> expectedMaterials = sources;
    for (DRW_Material& material : expectedMaterials)
        if (material.xDictHandle != 0u)
            material.xDictHandle = handleMap.at(material.xDictHandle);
    ReplayCapture capture(version, expectedMaterials, expectedDictionaries,
                          expectedXRecords);
    dwgRW reader(path.string().c_str());
    if (!reader.read(&capture, false) || capture.rootHandle != 0xB100u
        || !capture.dictionarySeen
        || std::find(capture.materialSeen.begin(), capture.materialSeen.end(),
                     false) != capture.materialSeen.end()
        || std::find(capture.dictionarySeenByHandle.begin(),
                     capture.dictionarySeenByHandle.end(), false)
            != capture.dictionarySeenByHandle.end()
        || std::find(capture.xrecordSeenByHandle.begin(),
                     capture.xrecordSeenByHandle.end(), false)
            != capture.xrecordSeenByHandle.end()) {
        std::cerr << "selected MATERIAL writer readback mismatch for version "
                  << static_cast<unsigned>(version) << ": root="
                  << capture.rootHandle << " dictionary="
                  << capture.dictionarySeen << " materials="
                  << std::count(capture.materialSeen.begin(),
                                capture.materialSeen.end(), true)
                  << '/' << capture.materialSeen.size() << '\n';
        return false;
    }
    return true;
}
}
bool ac1027DefaultMaterialReplayRoundtrip(const DRW_Material& source) {
    return defaultMaterialReplayGraphRoundtripImpl(DRW::AC1027, {source});
}
bool ac1027DefaultMaterialReplayGraphRoundtrip(
        const std::vector<DRW_Material>& sources,
        const std::filesystem::path& output) {
    return defaultMaterialReplayGraphRoundtripImpl(
        DRW::AC1027, sources, output);
}
bool ac1024AutoCad2016DefaultMaterialReplayGraphRoundtrip(
        const std::vector<DRW_Material>& sources,
        const std::filesystem::path& output,
        const std::vector<DRW_Dictionary>& extensionDictionaries,
        const std::vector<DRW_XRecord>& xrecords,
        const std::unordered_map<std::uint32_t, std::uint32_t>& handleMap) {
    return defaultMaterialReplayGraphRoundtripImpl(
        DRW::AC1024, sources, output, extensionDictionaries, xrecords,
        handleMap, 0);
}

namespace {
struct MaterialBufferState {
    std::vector<std::uint8_t> bytes;
    std::uint8_t bitPos{0};
    bool good{true};
    std::vector<DRW::DwgHandleWriteOccurrence> occurrences;
};

MaterialBufferState materialBufferState(const dwgBufferW& buffer) {
    return {buffer.data(), buffer.bitPos(), buffer.isGood(),
            buffer.handleOccurrences()};
}

bool sameMaterialBufferState(const MaterialBufferState& lhs,
                             const MaterialBufferState& rhs) {
    if (lhs.bytes != rhs.bytes || lhs.bitPos != rhs.bitPos
        || lhs.good != rhs.good || lhs.occurrences.size() != rhs.occurrences.size())
        return false;
    for (std::size_t i = 0; i < lhs.occurrences.size(); ++i) {
        const auto& a = lhs.occurrences[i];
        const auto& b = rhs.occurrences[i];
        if (a.code != b.code || a.reference != b.reference
            || a.startBit != b.startBit || a.endBit != b.endBit)
            return false;
    }
    return true;
}

bool ac1027MaterialDirectEncoderAllocationSweep(
        const std::vector<DRW_Material>& sources) {
    if (sources.empty()) return false;
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1027, false);
    enum class Layout { Separate, SharedStringHandles, BodyString,
                        BodyHandles, AllBody };
    const std::array<Layout, 5> layouts{{Layout::Separate,
        Layout::SharedStringHandles, Layout::BodyString,
        Layout::BodyHandles, Layout::AllBody}};
    std::size_t totalOffsets = 0;
    for (const auto& source : sources) {
        for (const Layout layout : layouts) {
            const auto run = [&](int fault, bool& injected,
                                 std::array<MaterialBufferState, 3>& state,
                                 bool& result, int& allocationsUsed) {
                MaterialSemanticProbe candidate;
                static_cast<DRW_Material&>(candidate) = source;
                dwgBufferW body(&codec), strings(&codec), handles(&codec);
                body.put3Bits(5u);
                strings.put3Bits(2u);
                handles.put3Bits(6u);
                dwgBufferW* stringArg = &strings;
                dwgBufferW* handleArg = &handles;
                if (layout == Layout::SharedStringHandles)
                    handleArg = &strings;
                else if (layout == Layout::BodyString)
                    stringArg = &body;
                else if (layout == Layout::BodyHandles)
                    handleArg = &body;
                else if (layout == Layout::AllBody) {
                    stringArg = &body;
                    handleArg = &body;
                }
                const auto beforeBody = materialBufferState(body);
                const auto beforeStrings = materialBufferState(strings);
                const auto beforeHandles = materialBufferState(handles);
                failAfter = fault;
                result = false;
                try {
                    result = candidate.encodeDwg(DRW::AC1027, &body,
                                                 stringArg, handleArg);
                } catch (...) {
                    allocationsUsed = fault - failAfter.load();
                    failAfter = -1;
                    return false;
                }
                const int remaining = failAfter.load();
                injected = fault >= 0 && remaining < 0;
                allocationsUsed = fault >= 0 ? fault - remaining : 0;
                failAfter = -1;
                state = {{materialBufferState(body),
                          materialBufferState(strings),
                          materialBufferState(handles)}};
                if (!injected) return true;
                if (result || !sameMaterialBufferState(state[0], beforeBody)
                    || !sameMaterialBufferState(state[1], beforeStrings)
                    || !sameMaterialBufferState(state[2], beforeHandles))
                    return false;
                return true;
            };

            bool injected = false;
            bool cleanResult = false;
            std::array<MaterialBufferState, 3> clean{};
            int unusedAllocationCount = 0;
            if (!run(-1, injected, clean, cleanResult, unusedAllocationCount)
                || !cleanResult)
                return false;
            bool calibrationInjected = false;
            bool calibrationResult = false;
            std::array<MaterialBufferState, 3> calibration{};
            constexpr int calibrationLimit = 4096;
            int allocationCount = 0;
            if (!run(calibrationLimit, calibrationInjected, calibration,
                     calibrationResult, allocationCount) || !calibrationResult
                || calibrationInjected)
                return false;
            if (allocationCount <= 0 || allocationCount >= calibrationLimit)
                return false;
            // A non-faulting calibration must be byte-for-byte identical to
            // the clean output, including pre-existing partial-byte state.
            for (std::size_t i = 0; i < clean.size(); ++i)
                if (!sameMaterialBufferState(clean[i], calibration[i]))
                    return false;

            for (int offset = 0; offset < allocationCount; ++offset) {
                // Keep this exact object and these exact streams alive across
                // the failed call and retry.
                MaterialSemanticProbe candidate;
                static_cast<DRW_Material&>(candidate) = source;
                dwgBufferW body(&codec), strings(&codec), handles(&codec);
                body.put3Bits(5u);
                strings.put3Bits(2u);
                handles.put3Bits(6u);
                dwgBufferW* stringArg = &strings;
                dwgBufferW* handleArg = &handles;
                if (layout == Layout::SharedStringHandles)
                    handleArg = &strings;
                else if (layout == Layout::BodyString)
                    stringArg = &body;
                else if (layout == Layout::BodyHandles)
                    handleArg = &body;
                else if (layout == Layout::AllBody) {
                    stringArg = &body;
                    handleArg = &body;
                }
                const std::array<MaterialBufferState, 3> before{{
                    materialBufferState(body), materialBufferState(strings),
                    materialBufferState(handles)}};
                failAfter = offset;
                bool result = false;
                try {
                    result = candidate.encodeDwg(DRW::AC1027, &body,
                                                 stringArg, handleArg);
                } catch (...) {
                    failAfter = -1;
                    return false;
                }
                const int remaining = failAfter.load();
                const bool didInject = remaining < 0;
                const int faultAllocations = offset - remaining;
                failAfter = -1;
                const std::array<MaterialBufferState, 3> failed{{
                    materialBufferState(body), materialBufferState(strings),
                    materialBufferState(handles)}};
                if (!didInject || faultAllocations != offset + 1 || result)
                    return false;
                for (std::size_t i = 0; i < before.size(); ++i)
                    if (!sameMaterialBufferState(before[i], failed[i]))
                        return false;
                if (!candidate.encodeDwg(DRW::AC1027, &body, stringArg,
                                         handleArg))
                    return false;
                const std::array<MaterialBufferState, 3> retried{{
                    materialBufferState(body), materialBufferState(strings),
                    materialBufferState(handles)}};
                for (std::size_t i = 0; i < clean.size(); ++i)
                    if (!sameMaterialBufferState(clean[i], retried[i]))
                        return false;
                ++totalOffsets;
            }
        }
    }
    std::cout << "AC1027 MATERIAL direct encoder rollback/retry PASS; "
              << totalOffsets << " injected allocation offsets across "
              << sources.size() << " source records and 5 stream layouts\n";
    return true;
}
struct CanonicalR2013Adapter final : Adapter, DRW_DwgCanonicalLayerProvider {
    using Adapter::Adapter;
    bool wantsDwgCanonicalLayerNulls(const DRW_Layer&) const override { return true; }
    void writeLayers() override {
        DRW_Layer layer;
        layer.name = fault == 5 ? "unsupported" : "0";
        layer.materialHandle.ref = fault == 4 ? 0xB200u : handles[3];
        layer.plotStyleHandle.ref = fault == 3 ? 0u : plotStylePlaceholder;
        if (fault == 1) layer.unknownHandle.ref = 0xB201u;
        if (fault == 2) layer.unknownHandle.code = DRW::DwgSoftPointer;
        if (fault == 6) layer.xrefBlockHandle.ref = 0xB202u;
        if (fault == 7) layer.unknownHandle.ref64 = std::uint64_t{1} << 40;
        if (fault == 8) layer.unknownHandle.rawCode = DRW::DwgSoftPointer;
        const auto originalUnknown = layer.unknownHandle;
        const auto originalXref = layer.xrefBlockHandle;
        accepted = output.addLayer(&layer);
        callerUnchanged = layer.extensionDictionaryFlag() == 0u
            && layer.unknownHandle.ref == originalUnknown.ref
            && layer.unknownHandle.ref64 == originalUnknown.ref64
            && layer.unknownHandle.code == originalUnknown.code
            && layer.unknownHandle.rawCode == originalUnknown.rawCode
            && layer.xrefBlockHandle.ref == originalXref.ref
            && layer.xrefBlockHandle.code == originalXref.code;
        if (!accepted) output.markDwgWriteFailure();
    }
    int fault = 0;
    bool accepted = false;
    bool callerUnchanged = false;
};

bool canonicalR2013Layer(const std::filesystem::path& path, bool keep) {
    dwgRW output(path.string().c_str());
    CanonicalR2013Adapter adapter(output);
    adapter.version = DRW::AC1027;
    const auto verify = [&]() {
        struct LayerCapture final : LC3CompatCheck {
            void addLayer(const DRW_Layer& value) override {
                if (value.name == "0") { layer = value; ++count; }
            }
            DRW_Layer layer;
            unsigned count = 0u;
        } capture;
        dwgR reader(path.string().c_str());
        return reader.read(&capture, false) && capture.count == 1u
            && capture.layer.extensionDictionaryFlag() == 1u
            && capture.layer.xrefBlockHandle.rawCode == DRW::DwgHardPointer
            && capture.layer.xrefBlockHandle.ref == 0u
            && capture.layer.plotStyleHandle.rawCode == DRW::DwgHardPointer
            && capture.layer.plotStyleHandle.ref == adapter.plotStylePlaceholder
            && capture.layer.materialHandle.rawCode == DRW::DwgHardPointer
            && capture.layer.materialHandle.ref == adapter.handles[3]
            && capture.layer.lTypeH.rawCode == DRW::DwgHardPointer
            && capture.layer.lTypeH.ref != 0u
            && capture.layer.unknownHandle.rawCode == DRW::DwgHardPointer
            && capture.layer.unknownHandle.ref == 0u
            && inspect(path.string(), adapter.handles, false, DRW::AC1027,
                       adapter.plotStylePlaceholder);
    };
    const auto write = [&]() {
        return output.write(&adapter, DRW::AC1027, true);
    };
    if (!write() || !adapter.accepted || !adapter.callerUnchanged || !verify()) {
        std::cerr << "AC1027 canonical layer output/readback failed: "
                  << output.getLastDiagnostic().code << '\n';
        return false;
    }
    if (keep)
        std::filesystem::copy_file(path, path.parent_path() / "canonical-layer-ac1027.dwg");
    for (int fault = 1; fault <= 8; ++fault) {
        { std::ofstream f(path, std::ios::binary | std::ios::trunc); f << "prior destination"; }
        adapter.fault = fault;
        if (write() || adapter.accepted || !adapter.callerUnchanged) return false;
        { std::ifstream f(path, std::ios::binary);
          if (std::string(std::istreambuf_iterator<char>(f), {}) != "prior destination") return false; }
        adapter.fault = 0;
        if (!write() || !adapter.accepted || !adapter.callerUnchanged || !verify()) return false;
    }
    return true;
}
struct CanonicalMultiAdapter final : Adapter, DRW_DwgCanonicalLayerProvider {
    using Adapter::Adapter;
    bool wantsDwgCanonicalLayerNulls(const DRW_Layer&) const override { return true; }
    void writeLayers() override {
        callerUnchanged = true;
        accepted = false;
        for (int index = 0; index < 2; ++index) {
            DRW_Layer layer;
            layer.name = index == 0 ? "0" : (fault == 5 ? "unsupported" : "Defpoints");
            layer.materialHandle.ref = index == 1 && fault == 4 ? 0xB200u : handles[3];
            layer.plotStyleHandle.ref = index == 1 && fault == 3 ? 0u : plotStylePlaceholder;
            if (index == 1 && fault == 1) layer.unknownHandle.ref = 0xB201u;
            if (index == 1 && fault == 2) layer.unknownHandle.code = DRW::DwgSoftPointer;
            if (index == 1 && fault == 6) layer.xrefBlockHandle.ref = 0xB202u;
            if (index == 1 && fault == 7) layer.unknownHandle.ref64 = std::uint64_t{1} << 40;
            if (index == 1 && fault == 8) layer.unknownHandle.rawCode = DRW::DwgSoftPointer;
            if (index == 1 && fault == 9) layer.unknownHandle.code = DRW::DwgHardPointer;
            if (index == 1 && fault == 10) layer.unknownHandle.rawCode = DRW::DwgHardPointer;
            const auto unknown = layer.unknownHandle;
            const auto xref = layer.xrefBlockHandle;
            accepted = output.addLayer(&layer);
            callerUnchanged = callerUnchanged && layer.extensionDictionaryFlag() == 0u
                && layer.unknownHandle.ref == unknown.ref
                && layer.unknownHandle.ref64 == unknown.ref64
                && layer.unknownHandle.code == unknown.code
                && layer.unknownHandle.rawCode == unknown.rawCode
                && layer.xrefBlockHandle.ref == xref.ref
                && layer.xrefBlockHandle.code == xref.code;
            if (!accepted) { output.markDwgWriteFailure(); return; }
        }
    }
    int fault = 0;
    bool accepted = false;
    bool callerUnchanged = false;
};

bool canonicalMultiLayer(const std::filesystem::path& path,
                         DRW::Version version, bool keep) {
    dwgRW output(path.string().c_str());
    CanonicalMultiAdapter adapter(output);
    adapter.version = version;
    const auto verify = [&]() {
        struct LayerCapture final : LC3CompatCheck {
            void addLayer(const DRW_Layer& value) override { layers.push_back(value); }
            std::vector<DRW_Layer> layers;
        } capture;
        dwgR reader(path.string().c_str());
        if (!reader.read(&capture, false) || capture.layers.size() != 2u) return false;
        for (const auto* name : {"0", "Defpoints"}) {
            const auto found = std::find_if(capture.layers.begin(), capture.layers.end(),
                [name](const DRW_Layer& layer) { return layer.name == name; });
            if (found == capture.layers.end() || found->extensionDictionaryFlag() != 1u
                || found->xrefBlockHandle.rawCode != DRW::DwgHardPointer
                || found->xrefBlockHandle.ref != 0u
                || found->plotStyleHandle.rawCode != DRW::DwgHardPointer
                || found->plotStyleHandle.ref != adapter.plotStylePlaceholder
                || found->materialHandle.rawCode != DRW::DwgHardPointer
                || found->materialHandle.ref != adapter.handles[3]
                || found->lTypeH.rawCode != DRW::DwgHardPointer || found->lTypeH.ref == 0u
                || (version == DRW::AC1027
                    && (found->unknownHandle.rawCode != DRW::DwgHardPointer
                        || found->unknownHandle.ref != 0u))) return false;
        }
        return inspect(path.string(), adapter.handles, false, version,
                       adapter.plotStylePlaceholder);
    };
    const auto write = [&]() { return output.write(&adapter, version, true); };
    if (!write() || !adapter.accepted || !adapter.callerUnchanged || !verify()) {
        std::cerr << "canonical two-layer write failed version=" << int(version)
                  << " code=" << output.getLastDiagnostic().code << '\n';
        return false;
    }
    if (keep) {
        const auto name = version == DRW::AC1021 ? "canonical-layers-ac1021.dwg"
                                                  : "canonical-layers-ac1027.dwg";
        std::filesystem::copy_file(path, path.parent_path() / name);
    }
    const int faultCount = version == DRW::AC1021 ? 10 : 8;
    for (int fault = 1; fault <= faultCount; ++fault) {
        { std::ofstream f(path, std::ios::binary | std::ios::trunc); f << "prior destination"; }
        adapter.fault = fault;
        if (write() || adapter.accepted || !adapter.callerUnchanged) return false;
        { std::ifstream f(path, std::ios::binary);
          if (std::string(std::istreambuf_iterator<char>(f), {}) != "prior destination") return false; }
        adapter.fault = 0;
        if (!write() || !adapter.accepted || !adapter.callerUnchanged || !verify()) return false;
    }
    return true;
}
} // namespace

bool ac1027DefaultMaterialReplayAllocationSweep(
        const std::vector<DRW_Material>& sources) {
    if (!ac1027MaterialDirectEncoderAllocationSweep(sources)) return false;
    const auto nonce = std::chrono::steady_clock::now()
        .time_since_epoch().count();
    const std::string sentinel = "prior destination";
    int successfulOffset = -1;
    for (int offset = 0; offset < 1024; ++offset) {
        const auto path = std::filesystem::temp_directory_path()
            / ("libdxfrw-ac1027-material-replay-fault-"
               + std::to_string(nonce) + "-" + std::to_string(offset)
               + ".dwg");
        struct Cleanup {
            std::filesystem::path path;
            ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
        } cleanup{path};
        {
            std::ofstream prior(path, std::ios::binary | std::ios::trunc);
            prior << sentinel;
            if (!prior) return false;
        }
        dwgRW writer(path.string().c_str());
        Adapter adapter(writer);
        adapter.enabled = false;
        adapter.plotEnabled = false;
        adapter.mode = Mode::ReplayMaterial;
        adapter.replayMaterials = sources;
        if (!writer.reserveHandle(0xB100u)) {
            std::cerr << "AC1027 MATERIAL root handle reservation failed\n";
            return false;
        }
        for (std::size_t i = 0; i < sources.size(); ++i)
            if (!writer.reserveHandle(static_cast<std::uint32_t>(0xB101u + i))) {
                std::cerr << "AC1027 MATERIAL child handle reservation failed\n";
                return false;
            }

        bool written = false;
        bool escaped = false;
        try {
            adapter.replayAllocationOffset = offset;
            written = writer.write(&adapter, DRW::AC1027, true);
        } catch (...) {
            escaped = true;
        }
        if (escaped) {
            std::cerr << "AC1027 MATERIAL graph write escaped allocation "
                      << offset << '\n';
            return false;
        }
        if (!adapter.replayAllocationTriggered) {
            if (!written) {
                std::cerr << "AC1027 MATERIAL graph failed without injected allocation at "
                          << offset << '\n';
                return false;
            }
            successfulOffset = offset;
            break;
        }
        if (written) {
            std::cerr << "AC1027 MATERIAL injected allocation " << offset
                      << " did not fail the outer write\n";
            return false;
        }
        {
            std::ifstream prior(path, std::ios::binary);
            const std::string actual(std::istreambuf_iterator<char>(prior), {});
            if (actual != sentinel) {
                std::cerr << "AC1027 MATERIAL allocation " << offset
                          << " changed destination before retry\n";
                return false;
            }
        }
        adapter.replayAllocationOffset = -1;
        if (!writer.write(&adapter, DRW::AC1027, true)) {
            std::cerr << "AC1027 MATERIAL same-writer retry failed at allocation "
                      << offset << '\n';
            return false;
        }
        Capture readback;
        readback.rawVersion = DRW::AC1027;
        readback.requireSemantic = true;
        readback.requireRaw = true;
        dwgRW reader(path.string().c_str());
        if (!reader.read(&readback, false) || !readback.valid
            || readback.root != 0xB100u || readback.dictionary != 0xB100u
            || readback.owner != DRW::DwgNamedObjectsDictionaryHandle
            || readback.parsedMaterials.size() != sources.size()
            || readback.children != std::array<std::uint32_t,3>{{
                0xB101u, 0xB102u, 0xB103u}}) {
            std::cerr << "AC1027 MATERIAL graph allocation retry readback failed at "
                      << offset << '\n';
            return false;
        }
    }
    if (successfulOffset <= 0) {
        std::cerr << "AC1027 MATERIAL allocation sweep did not reach a clean offset\n";
        return false;
    }
    std::cout << "AC1027 MATERIAL source-fed graph rollback/retry PASS; "
              << successfulOffset << " injected allocation offsets\n";
    return true;
}
int main(int argc,char**argv) {
    if (argc == 3
        && std::string(argv[1]) == "--physical-ac1024-autocad-material-replay") {
        dwgRW reader(argv[2]);
        Capture capture;
        capture.requireSemantic = true;
        capture.requireRaw = true;
        capture.expectDefaultBody = false;
        capture.physicalAc1024AutoCadProfile = true;
        capture.rawVersion = DRW::AC1024;
        const std::array<std::uint32_t, 4> handles{{0x10u, 0x19u, 0x11u, 0x21u}};
        if (!reader.read(&capture, false)
            || !capture.matches(handles, false)
            || capture.parsedMaterials.size() != 3u) {
            std::cerr << "physical AutoCAD AC1024 MATERIAL source audit failed: "
                      << "valid=" << capture.valid << " materials="
                      << capture.parsedMaterials.size() << '\n';
            for (const auto& item : capture.parsedMaterials)
                std::cerr << item.m_name << " raw=" << item.dwgRawDataBitSize()
                          << '/' << item.dwgRawData().size() * 8u
                          << " semantic=" << item.hasCompleteDwgSemanticFields()
                          << " owner=" << item.parentHandle << " reactors="
                          << item.reactorHandles.size() << '\n';
            return 1;
        }
        std::vector<DRW_Material> selected;
        const std::array<std::string, 3> names{{"ByBlock", "ByLayer", "Global"}};
        for (const auto& name : names) {
            const auto material = std::find_if(
                capture.parsedMaterials.begin(), capture.parsedMaterials.end(),
                [&name](const DRW_Material& item) { return item.m_name == name; });
            if (material == capture.parsedMaterials.end()
                || !material->canReplayAc1024AutoCad2016DefaultDwgBody()) {
                std::cerr << "AutoCAD AC1024 MATERIAL profile refused for "
                          << name << '\n';
                return 1;
            }
            selected.push_back(*material);
        }
        std::unordered_map<std::uint32_t, std::uint32_t> remap;
        std::vector<DRW_Dictionary> extensionDictionaries;
        std::vector<DRW_XRecord> xrecords;
        std::uint32_t nextExtensionDictionary = 0xB200u;
        std::uint32_t nextXRecord = 0xB210u;
        for (std::size_t i = 0; i < selected.size(); ++i) {
            const DRW_Material& material = selected[i];
            remap.emplace(material.handle,
                          static_cast<std::uint32_t>(0xB101u + i));
            if (material.xDictHandle == 0u
                || !remap.emplace(material.xDictHandle,
                                  nextExtensionDictionary).second) {
                std::cerr << "AutoCAD AC1024 MATERIAL extension dictionary identity is absent or aliased\n";
                return 1;
            }
            const auto dictionary = std::find_if(
                capture.sourceDictionaries.begin(), capture.sourceDictionaries.end(),
                [&material](const DRW_Dictionary& candidate) {
                    return candidate.handle == material.xDictHandle;
                });
            if (dictionary == capture.sourceDictionaries.end()
                || dictionary->m_entries.size() != 6u) {
                std::cerr << "AutoCAD AC1024 MATERIAL tile dictionary is incomplete\n";
                return 1;
            }
            extensionDictionaries.push_back(*dictionary);
            ++nextExtensionDictionary;
            for (const auto& entry : dictionary->m_entries) {
                if (!remap.emplace(entry.m_handle, nextXRecord).second) {
                    std::cerr << "AutoCAD AC1024 MATERIAL XRECORD handle is aliased\n";
                    return 1;
                }
                ++nextXRecord;
            }
        }
        for (const DRW_Dictionary& dictionary : extensionDictionaries) {
            for (const auto& entry : dictionary.m_entries) {
                const auto record = std::find_if(
                    capture.sourceXRecords.begin(), capture.sourceXRecords.end(),
                    [&entry](const DRW_XRecord& candidate) {
                        return candidate.handle == entry.m_handle;
                    });
                if (record == capture.sourceXRecords.end()) {
                    std::cerr << "AutoCAD AC1024 MATERIAL tile XRECORD is missing\n";
                    return 1;
                }
                xrecords.push_back(*record);
            }
        }
        const std::filesystem::path output(argv[2]);
        const auto replayOutput = output.parent_path()
            / (output.stem().string() + "-libdxfrw-replay.dwg");
        if (std::filesystem::exists(replayOutput)
            || !ac1024AutoCad2016DefaultMaterialReplayGraphRoundtrip(
                selected, replayOutput, extensionDictionaries, xrecords,
                remap))
            return 1;
        std::cout << "AutoCAD AC1024 MATERIAL source-fed graph replay/readback PASS: "
                  << replayOutput << '\n';
        return 0;
    }
    if ((argc == 3 || argc == 4)
        && std::string(argv[1]) == "--physical-ac1027-material-dwg-replay") {
        dwgRW reader(argv[2]);
        Capture capture;
        capture.requireSemantic = true;
        capture.requireRaw = true;
        capture.rawVersion = DRW::AC1027;
        if (!reader.read(&capture, false) || !capture.valid
            || capture.root == 0u || capture.dictionary == 0u
            || capture.owner != DRW::DwgNamedObjectsDictionaryHandle
            || capture.parsedMaterials.size() != 3u) {
            std::cerr << "physical AC1027 MATERIAL source audit failed: root="
                      << capture.root << " dictionary=" << capture.dictionary
                      << " materials=" << capture.parsedMaterials.size()
                      << " valid=" << capture.valid << '\n';
            return 1;
        }
        const std::array<std::string, 3> expectedNames{{
            "ByBlock", "ByLayer", "Global"}};
        std::vector<DRW_Material> selectedMaterials;
        for (const auto& name : expectedNames) {
            const auto found = std::find_if(
                capture.parsedMaterials.begin(), capture.parsedMaterials.end(),
                [&name](const DRW_Material& material) {
                    return material.m_name == name;
                });
            if (found == capture.parsedMaterials.end()
                || !found->canReplayAc1027DefaultDwgBody()
                || !ac1027DefaultMaterialReplayRoundtrip(*found)) {
                std::cerr << "physical AC1027 MATERIAL write/readback failed for "
                          << name << '\n';
                return 1;
            }
            selectedMaterials.push_back(*found);
        }
        if (!ac1027DefaultMaterialReplayAllocationSweep(selectedMaterials)) {
            std::cerr << "physical AC1027 MATERIAL allocation rollback/retry failed\n";
            return 1;
        }
        const std::filesystem::path output = argc == 4
            ? std::filesystem::path(argv[3]) : std::filesystem::path{};
        if (!ac1027DefaultMaterialReplayGraphRoundtrip(selectedMaterials, output)) {
            std::cerr << "physical AC1027 MATERIAL graph replay/readback failed\n";
            return 1;
        }
        std::cout << "physical AC1027 MATERIAL selected DWG replay/readback PASS\n";
        return 0;
    }
    if (argc==3 && std::string(argv[1])=="--physical-ac1021-semantic-probe") {
        dwgRW reader(argv[2]);
        Capture capture;
        capture.requireSemantic=true;
        capture.requireRaw=false;
        capture.expectDefaultBody=false;
        capture.physicalAc1021Profile=true;
        const std::array<std::uint32_t,4> handles{{0x6au,0xedu,0xecu,0xeeu}};
        const bool read=reader.read(&capture,false);
        const bool match=capture.matches(handles,false);
        if (!read || !match) {
            std::cerr << "physical AC1021 MATERIAL semantic probe failed: read="
                      << read << " match=" << match << " valid="
                      << capture.valid << " materials="
                      << capture.materials.size() << '\n';
            return 1;
        }
        std::cout << "physical AC1021 MATERIAL semantic probe passed\n";
        return 0;
    }
    const bool keep=argc==2 && std::string(argv[1])=="--keep";
    if(argc>2 || (argc==2 && !keep)) return 1;
    if (!directControls()) { std::cerr << "default material direct controls failed\n"; return 1; }
    if (!directAc1027Controls()) { std::cerr << "AC1027 material controls failed\n"; return 1; }
    if (!ac1027AllocationSweep()) { std::cerr << "AC1027 material allocation sweep failed\n"; return 1; }
    if (!ac1027LayerAllocationSweep()) { std::cerr << "AC1027 layer allocation sweep failed\n"; return 1; }
    const auto dir=std::filesystem::temp_directory_path() /
        ("libdxfrw-material-graph-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!std::filesystem::create_directory(dir)) return 1;
    struct Cleanup {std::filesystem::path p; bool keep; ~Cleanup(){if(!keep){std::error_code e;std::filesystem::remove_all(p,e);}}} cleanup{dir,keep};
    for (const auto version : {DRW::AC1021, DRW::AC1024, DRW::AC1027}) {
        const auto path=(dir/(version == DRW::AC1021 ? "graph-ac1021.dwg" : version == DRW::AC1024 ? "graph-ac1024.dwg" : "graph-ac1027.dwg")).string(); dwgRW output(path.c_str());
        if(!output.reserveHandle(0x30u)||!output.reserveHandle(0x31u)||!output.reserveHandle(0xB200u)) return 1;
        Adapter adapter(output);
        adapter.version=version;
        if (!output.write(&adapter, version, true)) {
            std::cerr << "normal write failed version=" << int(version)
                      << " path=" << path << '\n';
            return 1;
        }
        if (adapter.plotStylePlaceholder == 0u
            || !inspect(path, adapter.handles, false, version,
                        adapter.plotStylePlaceholder)) {
            std::cerr << "normal paired default-graph readback failed version="
                      << int(version) << " plotstyle="
                      << adapter.plotStylePlaceholder << " path=" << path << '\n';
            return 1;
        }
        for(auto h:adapter.handles) if(h==0x30u||h==0x31u||h==0xB200u) return 1;
        adapter.mode=Mode::SuppliedLayer;
        if (version == DRW::AC1027) {
            {std::ofstream f(path,std::ios::binary);f<<"prior destination";}
            if (output.write(&adapter,version,true)) return 1;
            {std::ifstream f(path,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(f),{})!="prior destination")return 1;}
            adapter.mode=Mode::Normal;
            if (!output.write(&adapter,version,true)||!inspect(path,adapter.handles,false,version)) return 1;
        } else if(!output.write(&adapter,version,true)||!adapter.supplied||!inspect(path,adapter.handles,true,version)) {std::cerr<<"supplied layer failed\n";return 1;}
        adapter.mode=Mode::SuppliedLayerGlobal;
        if (!output.write(&adapter, version, true) ||
            !inspect(path, adapter.handles, false, version)) {
            std::cerr << "supplied layer with planned Global material failed\n";
            return 1;
        }
        if (version == DRW::AC1021 || version == DRW::AC1027) {
            // A queued Global reference without the new provider stays refused.
            adapter.mode = Mode::ExtraLayerGlobal;
            { std::ofstream f(path, std::ios::binary | std::ios::trunc); f << "prior destination"; }
            if (output.write(&adapter, version, true)) return 1;
            { std::ifstream f(path, std::ios::binary);
              if (std::string(std::istreambuf_iterator<char>(f), {}) != "prior destination") return 1; }
            adapter.mode = Mode::Normal;
            if (!output.write(&adapter, version, true)
                || !inspect(path, adapter.handles, false, version)) return 1;
            const auto probe = dir / (version == DRW::AC1021
                ? "canonical-multi-r2007-test.dwg" : "canonical-multi-r2013-test.dwg");
            if (!canonicalMultiLayer(probe, version, keep)) {
                std::cerr << "canonical two-layer controls failed version=" << int(version) << '\n';
                return 1;
            }
        }
        if (version == DRW::AC1024) {
            adapter.mode = Mode::ExtraLayerGlobal;
            if (!output.write(&adapter, version, true) || !adapter.supplied) return 1;
            dwgRW extraReader(path.c_str()); Capture extra;
            if (!extraReader.read(&extra, false) || extra.extraLayerMaterial != adapter.handles[3]) return 1;
            if (keep) std::filesystem::copy_file(path, dir / "graph-extra-layer-ac1024.dwg");
            { std::ofstream f(path, std::ios::binary); f << "prior destination"; }
            adapter.mode = Mode::ExtraLayerConflictingPlot;
            if (output.write(&adapter, version, true)) return 1;
            { std::ifstream f(path, std::ios::binary); if (std::string(std::istreambuf_iterator<char>(f), {}) != "prior destination") return 1; }
            adapter.mode = Mode::ExtraLayerGlobal;
            if (!output.write(&adapter, version, true) || !inspect(path, adapter.handles, false, version)) return 1;
        }
        if (version == DRW::AC1027) {
            if (!canonicalR2013Layer(dir / "canonical-r2013-test.dwg", keep)) {
                std::cerr << "AC1027 canonical layer controls failed\n";
                return 1;
            }
            adapter.plotEnabled=false;
            {std::ofstream f(path,std::ios::binary);f<<"prior destination";}
            if (output.write(&adapter,version,true)) return 1;
            {std::ifstream f(path,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(f),{})!="prior destination")return 1;}
            adapter.plotEnabled=true; adapter.mode=Mode::Normal;
            if (!output.write(&adapter,version,true)||!inspect(path,adapter.handles,false,version)) return 1;
        }
        adapter.mode=Mode::HeaderGlobal;
        if(!output.write(&adapter,version,true)||!inspect(path,adapter.handles,false,version))return 1;
        if(keep) std::filesystem::copy_file(path,dir/(version == DRW::AC1021 ? "graph-global-ac1021.dwg"
            : version == DRW::AC1024 ? "graph-global-ac1024.dwg" : "graph-global-ac1027.dwg"));
        for(Mode mode:{Mode::EarlyRoot,Mode::HeaderInvalid,Mode::EarlyMaterialClass,Mode::LateMaterialClass,
             Mode::EarlyRawSection,Mode::LateRawSection,Mode::LateRoot,Mode::NodAlias,Mode::GroupAlias,
             Mode::DictionaryOwner,Mode::DictionaryChild,Mode::DictionaryHandle,Mode::XrecordOwner,
             Mode::RawObject,Mode::CallerMaterial,Mode::LateEntityHandle}) {
            const bool slots=mode==Mode::NodAlias||mode==Mode::GroupAlias||mode==Mode::DictionaryOwner
                ||mode==Mode::DictionaryChild||mode==Mode::DictionaryHandle||mode==Mode::XrecordOwner
                ||mode==Mode::LateEntityHandle;
            for(std::size_t slot=0;slot<(slots?4u:1u);++slot) {
                {std::ofstream f(path,std::ios::binary);f<<"prior destination";}
                adapter.mode=mode;adapter.slot=slot;adapter.rejected=false;
                if(output.write(&adapter,version,true)) {std::cerr<<"collision published mode="<<int(mode)<<" slot="<<slot<<"\n";return 1;}
                std::ifstream f(path,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(f),{})!="prior destination") return 1;
                adapter.mode=Mode::Normal;
                if(!output.write(&adapter,version,true)||!inspect(path,adapter.handles,false,version)) {std::cerr<<"retry failed mode="<<int(mode)<<" slot="<<slot<<"\n";return 1;}
            }
        }
        adapter.mode=Mode::Normal;
        adapter.opaqueAbsent=false;
        {std::ofstream f(path,std::ios::binary);f<<"prior destination";}
        if(output.write(&adapter,version,true)) return 1;
        {std::ifstream f(path,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(f),{})!="prior destination")return 1;}
        adapter.opaqueAbsent=true;
        if(!output.write(&adapter,version,true)||!inspect(path,adapter.handles,false,version))return 1;
        for (const auto unsupported : {DRW::AC1015, DRW::AC1018, DRW::AC1032}) {
            {std::ofstream f(path,std::ios::binary);f<<"prior destination";}
            if(output.write(&adapter,unsupported,true)) return 1;
            std::ifstream f(path,std::ios::binary);
            if(std::string(std::istreambuf_iterator<char>(f),{})!="prior destination")return 1;
        }
        adapter.enabled=false;
        if(!output.write(&adapter,version,true))return 1;
        dwgRW reader(path.c_str());Capture capture;
        if(!reader.read(&capture,false)||capture.root!=0||!capture.materials.empty())return 1;
        adapter.mode=Mode::SourceRoot;
        if(!output.write(&adapter,version,true))return 1;
        {dwgRW sourceReader(path.c_str()); Capture source;
         if(!sourceReader.read(&source,false)||source.root!=0xB100u||!source.materials.empty())return 1;}
        adapter.mode=Mode::Normal; adapter.enabled=true;
        if(!output.write(&adapter,version,true)||!inspect(path,adapter.handles,false,version))return 1;
        if(keep)std::cout<<"OUTPUT "<<path<<'\n';
    }
    std::cout<<"DWG default material opt-in PASS\n";return 0;
}
