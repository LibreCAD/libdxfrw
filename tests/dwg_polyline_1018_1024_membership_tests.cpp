// SPDX-License-Identifier: GPL-2.0-or-later
#include "libdwgr.h"
#include "drw_header.h"
#include "intern/dwgwriter21.h"
#include "intern/dwgwriter18.h"
#include "intern/dwgwriter27.h"
#include "intern/dwgwriter32.h"
#include "intern/dwgwriter24.h"
#include "lc3_compat_check.cpp"
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <new>

void drwThrowNextAc1021PolylineForTest();
void drwThrowNextAc1021PolylineLateForTest();

class DrwEntityEncodeTestAccess {
public:
    static void seed(DRW_Entity& value) {
        value.dwgEedLayerWriteRefs.push_back({7u, 0x1234u, 0x123456789ABCDEF0ULL, true});
        value.dwgEedAppIdWriteRefs.push_back({9u, 0x2345u, 0xFEDCBA9876543210ULL, true});
    }
    static bool preserved(const DRW_Entity& value) {
        return value.dwgEedLayerWriteRefs.size() == 1u
            && value.dwgEedAppIdWriteRefs.size() == 1u
            && value.dwgEedLayerWriteRefs[0].indexInExtData == 7u
            && value.dwgEedLayerWriteRefs[0].handleRef == 0x1234u
            && value.dwgEedLayerWriteRefs[0].rawHandleRef == 0x123456789ABCDEF0ULL
            && value.dwgEedLayerWriteRefs[0].useRawHandleRef
            && value.dwgEedAppIdWriteRefs[0].indexInExtData == 9u
            && value.dwgEedAppIdWriteRefs[0].handleRef == 0x2345u
            && value.dwgEedAppIdWriteRefs[0].rawHandleRef == 0xFEDCBA9876543210ULL
            && value.dwgEedAppIdWriteRefs[0].useRawHandleRef;
    }
};

class DwgBlockRecordXrefWireTestAccess {
public:
    static bool exceptionRollback(DRW::Version version, bool late) {
        DRW_Header header;
        dwgRW facade(nullptr);
        std::unique_ptr<dwgWriter15> writer;
        if (version == DRW::AC1018) writer = std::make_unique<dwgWriter18>(nullptr, &header);
        else writer = std::make_unique<dwgWriter24>(nullptr, &header);
        auto* observed = writer.get();
        facade.writer = std::move(writer);
        facade.version = version;
        DRW_Line line;
        line.basePoint = {1., 2., 3.}; line.secPoint = {4., 5., 6.};
        if (!facade.writeLine(&line)) return false;
        const auto beforeBytes = observed->m_buf.size();
        const auto beforeMap = observed->m_objectMap.size();
        const auto beforeOwned = observed->m_modelSpaceEntityHandles;
        const auto beforeCount = facade.m_successfulEntityWrites;
        DRW_Polyline poly;
        poly.addVertex(DRW_Vertex(1., 2., 0., 0.));
        poly.addVertex(DRW_Vertex(4., 5., 0., 0.));
        const auto parentOwner = poly.parentHandle;
        const auto parentSpace = poly.space;
        const auto seqend = poly.dwgSeqEndHandle();
        const auto childOwner = poly.vertlist[0]->parentHandle;
        const auto childSpace = poly.vertlist[0]->space;
        const auto childSubtype = poly.vertlist[0]->dwgSubtype();
        DrwEntityEncodeTestAccess::seed(poly);
        for (auto& v : poly.vertlist) DrwEntityEncodeTestAccess::seed(*v);
        const auto token = facade.beginDwgWriteTransaction(false, true, false);
        if (!token) return false;
        if (late) drwThrowNextAc1021PolylineLateForTest();
        else drwThrowNextAc1021PolylineForTest();
        bool caught = false;
        try { (void)facade.writePolyline(&poly); }
        catch (const std::bad_alloc&) { caught = true; }
        DRW::DwgCompoundEntityWriteReceipt receipt;
        if (!caught || !facade.m_requiredWriteFailure
            || facade.m_successfulEntityWrites != beforeCount
            || facade.getLastDwgCompoundEntityWriteReceipt(receipt)
            || observed->m_buf.size() != beforeBytes
            || observed->m_objectMap.size() != beforeMap
            || observed->m_modelSpaceEntityHandles != beforeOwned
            || observed->m_ac1021PolylineChildPermit != nullptr
            || poly.handle != 0u || poly.vertlist[0]->handle != 0u
            || poly.vertlist[1]->handle != 0u
            || poly.parentHandle != parentOwner || poly.space != parentSpace
            || poly.dwgSeqEndHandle() != seqend
            || poly.vertlist[0]->parentHandle != childOwner
            || poly.vertlist[1]->parentHandle != childOwner
            || poly.vertlist[0]->space != childSpace || poly.vertlist[1]->space != childSpace
            || poly.vertlist[0]->dwgSubtype() != childSubtype
            || poly.vertlist[1]->dwgSubtype() != childSubtype
            || !DrwEntityEncodeTestAccess::preserved(poly)
            || !DrwEntityEncodeTestAccess::preserved(*poly.vertlist[0])
            || !DrwEntityEncodeTestAccess::preserved(*poly.vertlist[1]))
            return false;
        if (!facade.rollbackDwgWriteTransaction(token)
            || observed->m_buf.size() != beforeBytes
            || observed->m_modelSpaceEntityHandles != beforeOwned)
            return false;
        // The facade's required-failure latch remains set for this document,
        // but the restored writer and caller state permit a direct retry.
        return facade.writePolyline(&poly)
            && facade.getLastDwgCompoundEntityWriteReceipt(receipt)
            && receipt.valid && receipt.frames.size() == 4u;
    }

    static bool collision(DRW::Version version) {
        DRW_Header header; dwgRW facade(nullptr); std::unique_ptr<dwgWriter15> writer;
        if (version == DRW::AC1018) writer=std::make_unique<dwgWriter18>(nullptr,&header);
        else writer=std::make_unique<dwgWriter24>(nullptr,&header);
        auto* observed=writer.get(); facade.writer=std::move(writer); facade.version=version;
        DRW_Polyline poly; poly.handle=0x451u;
        poly.addVertex(DRW_Vertex(0.,0.,0.,0.)); poly.addVertex(DRW_Vertex(5.,0.,0.,0.));
        poly.vertlist[0]->handle=0x501u; poly.vertlist[1]->handle=0x501u;
        const auto bytes=observed->m_buf.size(); const auto count=observed->m_objectMap.size();
        if (facade.writePolyline(&poly) || observed->m_buf.size()!=bytes
            || observed->m_objectMap.size()!=count || !observed->m_modelSpaceEntityHandles.empty()
            || poly.handle!=0x451u || poly.vertlist[0]->handle!=0x501u
            || poly.vertlist[1]->handle!=0x501u || observed->m_ac1021PolylineChildPermit) return false;
        poly.vertlist[1]->handle=0x502u;
        return facade.writePolyline(&poly) && observed->m_modelSpaceEntityHandles.size()==1u;
    }

    static bool checkScope(DRW::Version version, bool paper, bool named) {
        DRW_Header header; dwgRW facade(nullptr); std::unique_ptr<dwgWriter15> writer;
        if (version == DRW::AC1018) writer=std::make_unique<dwgWriter18>(nullptr,&header);
        else if (version == DRW::AC1024) writer=std::make_unique<dwgWriter24>(nullptr,&header);
        else if (version == DRW::AC1027) writer=std::make_unique<dwgWriter27>(nullptr,&header);
        else writer=std::make_unique<dwgWriter32>(nullptr,&header);
        auto* observed=writer.get(); facade.writer=std::move(writer); facade.version=version;
        std::uint32_t block=0;
        if (named) { block=observed->defineBlock("OTHER_SCOPE",DRW_Coord(0.,0.,0.)); if (!block || !observed->beginBlockContent(block)) return false; }
        DRW_Polyline poly; if (paper) poly.space=DRW::PaperSpace;
        poly.addVertex(DRW_Vertex(0.,0.,0.,0.)); poly.addVertex(DRW_Vertex(5.,0.,0.,0.));
        if (!facade.writePolyline(&poly) || observed->m_ac1021PolylineChildPermit) return false;
        const auto& entities=named ? observed->findUserBlock(block)->entityHandles : (paper ? observed->m_paperSpaceEntityHandles : observed->m_modelSpaceEntityHandles);
        const auto expected = (version == DRW::AC1018 || version == DRW::AC1024)
            && (paper || named) ? 1u : 4u;
        return entities.size()==expected && entities.front()==poly.handle;
    }

    static bool run(DRW::Version version, bool is3d, bool paper, bool explicitModel,
                    int otherPolylineFlags = 0) {
        DRW_Header header;
        dwgRW facade(nullptr);
        std::unique_ptr<dwgWriter15> writer;
        if (version == DRW::AC1018)
            writer = std::make_unique<dwgWriter18>(nullptr, &header);
        else if (version == DRW::AC1021)
            writer = std::make_unique<dwgWriter21>(nullptr, &header);
        else
            writer = std::make_unique<dwgWriter24>(nullptr, &header);
        auto* observed = writer.get();
        facade.writer = std::move(writer);
        facade.version = version;
        DRW_Line line;
        line.space = paper ? DRW::PaperSpace : DRW::ModelSpace;
        line.basePoint = {1., 2., 3.}; line.secPoint = {4., 5., 6.};
        if (!facade.writeLine(&line)) return false;
        const auto beforeBytes = observed->m_buf.size();
        const auto beforeMap = observed->m_objectMap.size();
        const auto& blockEntities = paper ? observed->m_paperSpaceEntityHandles
                                          : observed->m_modelSpaceEntityHandles;
        const auto beforeEntities = blockEntities;
        DRW_Polyline poly;
        poly.space = line.space;
        if (explicitModel) poly.parentHandle = DRW::DwgModelSpaceBlockRecordHandle;
        const auto originalParent = poly.parentHandle;
        poly.flags = otherPolylineFlags ? otherPolylineFlags : (is3d ? 8 : 0);
        DRW_Vertex a, b;
        a.basePoint = {1., 2., is3d ? 3. : 0.};
        b.basePoint = {4., 5., is3d ? 6. : 0.};
        if (is3d) a.flags = b.flags = 32;
        poly.addVertex(a); poly.addVertex(b);
        facade.m_failNextCompoundSeqEndForTest = true;
        if (facade.writePolyline(&poly) || observed->m_buf.size() != beforeBytes
            || observed->m_objectMap.size() != beforeMap
            || blockEntities != beforeEntities
            || poly.handle != 0u || poly.vertlist[0]->handle != 0u
            || poly.vertlist[1]->handle != 0u
            || poly.parentHandle != originalParent
            || observed->m_ac1021PolylineChildPermit != nullptr)
            return false;
        if (!facade.writePolyline(&poly)) return false;
        const auto& owned = blockEntities;
        const std::size_t expected = (version == DRW::AC1018 || version == DRW::AC1024 || version == DRW::AC1021)
            && otherPolylineFlags == 0 ? 2u : 5u;
        if (owned.size() != expected || owned[0] != line.handle
            || owned[1] != poly.handle || observed->m_objectMap.size() != beforeMap + 4u
            || observed->m_ac1021PolylineChildPermit != nullptr)
            return false;
        DRW::DwgCompoundEntityWriteReceipt receipt;
        if (!facade.getLastDwgCompoundEntityWriteReceipt(receipt)
            || !receipt.valid || receipt.frames.size() != 4u
            || receipt.frames[0].frame.objectHandle != poly.handle
            || receipt.frames[1].frame.objectHandle != poly.vertlist[0]->handle
            || receipt.frames[2].frame.objectHandle != poly.vertlist[1]->handle
            || receipt.frames[3].role != DRW::DwgCompoundEntityFrameRole::SeqEnd)
            return false;
        if (!facade.rollbackLastDwgObjectWrite()
            || observed->m_buf.size() != beforeBytes
            || observed->m_objectMap.size() != beforeMap
            || blockEntities != beforeEntities
            || poly.handle != 0u || poly.vertlist[0]->handle != 0u
            || poly.vertlist[1]->handle != 0u || poly.parentHandle != originalParent)
            return false;
        if (!facade.writePolyline(&poly)) return false;
        return blockEntities.size() == expected;
    }
};

class ExceptionAdapter final : public LC3CompatCheck {
public:
    explicit ExceptionAdapter(dwgRW& out) : output(out) {}
    void writeEntities() override {
        DRW_Polyline poly;
        if (threeD) poly.flags = 8;
        poly.addVertex(DRW_Vertex(1., 2., 0., 0.));
        poly.addVertex(DRW_Vertex(3., 4., threeD ? 1. : 0., 0.));
        const auto parentOwner = poly.parentHandle;
        const auto parentSpace = poly.space;
        const auto seqend = poly.dwgSeqEndHandle();
        const auto childOwner = poly.vertlist[0]->parentHandle;
        const auto childSpace = poly.vertlist[0]->space;
        const auto childSubtype = poly.vertlist[0]->dwgSubtype();
        DrwEntityEncodeTestAccess::seed(poly);
        for (auto& v : poly.vertlist) DrwEntityEncodeTestAccess::seed(*v);
        if (!inject) {
            ok = output.writePolyline(&poly);
            return;
        }
        if (late) drwThrowNextAc1021PolylineLateForTest();
        else drwThrowNextAc1021PolylineForTest();
        try { (void)output.writePolyline(&poly); }
        catch (const std::bad_alloc&) { caught = true; }
        DRW::DwgCompoundEntityWriteReceipt receipt;
        restored = poly.handle == 0u && poly.vertlist[0]->handle == 0u
            && poly.vertlist[1]->handle == 0u
            && poly.parentHandle == parentOwner && poly.space == parentSpace
            && poly.dwgSeqEndHandle() == seqend
            && poly.vertlist[0]->parentHandle == childOwner
            && poly.vertlist[1]->parentHandle == childOwner
            && poly.vertlist[0]->space == childSpace && poly.vertlist[1]->space == childSpace
            && poly.vertlist[0]->dwgSubtype() == childSubtype
            && poly.vertlist[1]->dwgSubtype() == childSubtype
            && DrwEntityEncodeTestAccess::preserved(poly)
            && DrwEntityEncodeTestAccess::preserved(*poly.vertlist[0])
            && DrwEntityEncodeTestAccess::preserved(*poly.vertlist[1])
            && !output.getLastDwgCompoundEntityWriteReceipt(receipt);
    }
    // A caller that catches and ignores the exception must not publish a
    // document missing its required POLYLINE.
    bool finalizeDwgWrite() override { return true; }
    dwgRW& output;
    bool inject {true}, late {false}, threeD {false};
    bool caught {false}, restored {false}, ok {false};
};

class GroupAdapter final : public LC3CompatCheck {
public:
    explicit GroupAdapter(dwgRW& out) : output(out) {}
    void writeDwgClasses() override {
        registered = output.registerDwgGroupDictionaryEntry("PARENT_OR_CHILD", 0x900u);
    }
    void writeEntities() override {
        DRW_Polyline poly;
        poly.handle = 0x451u;
        if (threeD) poly.flags = 8;
        poly.addVertex(DRW_Vertex(1., 2., 0., 0.));
        poly.addVertex(DRW_Vertex(3., 4., 0., 0.));
        if (threeD) for (auto& vertex : poly.vertlist) vertex->flags = 32;
        if (reciprocal) poly.vertlist[0]->reactorHandles.push_back(0x900u);
        poly.vertlist[0]->handle = 0x501u;
        poly.vertlist[1]->handle = 0x502u;
        wrotePolyline = output.writePolyline(&poly);
    }
    void writeObjects() override {
        DRW_Group group;
        group.handle = 0x900u;
        group.parentHandle = DRW::DwgAcadGroupDictionaryHandle;
        group.name = "PARENT_OR_CHILD";
        group.m_entityHandles = {targetChild ? 0x501u : 0x451u};
        // Deliberately ignore this callback result in finalizeDwgWrite().
        wroteGroup = output.writeGroup(&group);
    }
    bool finalizeDwgWrite() override { return true; }
    dwgRW& output;
    bool targetChild {true}, registered {false}, wrotePolyline {false}, wroteGroup {false}, threeD {false}, reciprocal {false};
};

class GroupReader final : public LC3CompatCheck {
public:
    void addGroup(const DRW_Group& group) override {
        if (group.handle == 0x900u
            && group.parentHandle == DRW::DwgAcadGroupDictionaryHandle
            && group.m_entityHandles == std::vector<std::uint32_t>{0x451u})
            parentGroup = true;
    }
    bool parentGroup {false};
};

int main() {
    for (const auto version : {DRW::AC1018, DRW::AC1024}) for (const bool late : {false, true}) {
        if (!DwgBlockRecordXrefWireTestAccess::exceptionRollback(version, late)) {
            std::cerr << "exception rollback failed, late=" << late << '\n';
            return 1;
        }
    }
    for (const auto version : {DRW::AC1018, DRW::AC1021, DRW::AC1024}) {
        for (const bool is3d : {false, true}) {
          for (const bool paper : {false, true}) {
          for (const bool explicitModel : {false, true}) {
            if (paper && explicitModel) continue;
            if (!DwgBlockRecordXrefWireTestAccess::run(version, is3d, paper, explicitModel)) {
                std::cerr << "POLYLINE block ownership/rollback failed for "
                          << static_cast<int>(version) << ", 3D=" << is3d
                          << ", paper=" << paper << ", explicitModel=" << explicitModel << '\n';
                return 1;
            }
          }
          }
        }
    }
    for (const int flags : {16, 64}) {
        if (!DwgBlockRecordXrefWireTestAccess::run(DRW::AC1024, false, false, false, flags)) {
            std::cerr << "unchanged mesh/polyface ownership failed, flags=" << flags << '\n';
            return 1;
        }
    }
    for (auto version : {DRW::AC1018,DRW::AC1024})
        if (!DwgBlockRecordXrefWireTestAccess::collision(version)) { std::cerr << "collision rollback failed " << int(version) << "\n"; return 1; }
    for (auto version : {DRW::AC1018,DRW::AC1024}) for (int scope : {1,2})
        if (!DwgBlockRecordXrefWireTestAccess::checkScope(version,scope==1,scope==2)) { std::cerr << "scope membership failed " << int(version) << " " << scope << "\n"; return 1; }
    for (auto version : {DRW::AC1027,DRW::AC1032})
        if (!DwgBlockRecordXrefWireTestAccess::checkScope(version,false,false)) { std::cerr << "unchanged version failed " << int(version) << "\n"; return 1; }
    const auto path = std::filesystem::temp_directory_path()
        / ("polyline-modern-membership-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dwg");
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::error_code e; std::filesystem::remove(path,e); } } cleanup{path};
    dwgRW output(path.string().c_str());
    for (auto version : {DRW::AC1018, DRW::AC1024}) {
        ExceptionAdapter adapter(output);
        for (bool late : {false,true}) for (bool threeD : {false,true}) {
            { std::ofstream f(path,std::ios::binary|std::ios::trunc); f << "sentinel"; }
            adapter.inject=true; adapter.late=late; adapter.threeD=threeD;
            adapter.caught=adapter.restored=adapter.ok=false;
            if (output.write(&adapter,version,true) || !adapter.caught || !adapter.restored) { std::cerr << "exception write failed " << int(version) << "\n"; return 1; }
            std::ifstream f(path,std::ios::binary); if (std::string(std::istreambuf_iterator<char>(f),{}) != "sentinel") return 1; f.close();
            adapter.inject=false;
            if (!output.write(&adapter,version,true) || !adapter.ok) { std::cerr << "retry failed " << int(version) << "\n"; return 1; }
        }
        for (bool threeD : {false,true}) for (bool reciprocal : {false,true}) {
        GroupAdapter groups(output); groups.threeD = threeD; groups.reciprocal = reciprocal;
        { std::ofstream f(path,std::ios::binary|std::ios::trunc); f << "sentinel"; }
        if (output.write(&groups,version,true) || !groups.registered || !groups.wrotePolyline || groups.wroteGroup) { std::cerr << "child GROUP did not refuse " << int(version) << "\n"; return 1; }
        std::ifstream f(path,std::ios::binary); if (std::string(std::istreambuf_iterator<char>(f),{}) != "sentinel") return 1; f.close();
        groups.targetChild=false;
        if (!output.write(&groups,version,true) || !groups.wroteGroup) { std::cerr << "parent GROUP retry failed " << int(version) << "\n"; return 1; }
        dwgRW input(path.string().c_str()); GroupReader reader;
        if (!input.read(&reader,false) || !reader.parentGroup) { std::cerr << "parent GROUP read failed " << int(version) << "\n"; return 1; }
        }
    }
    return 0;
}
