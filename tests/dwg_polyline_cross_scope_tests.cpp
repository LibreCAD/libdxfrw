// SPDX-License-Identifier: GPL-2.0-or-later
#include "libdwgr.h"
#include "drw_header.h"
#include "intern/dwgwriter21.h"
#include "intern/dwgwriter18.h"
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
    static bool exceptionRollback(DRW::Version version, bool late, bool paper) {
        DRW_Header header;
        dwgRW facade(nullptr);
        std::unique_ptr<dwgWriter15> writer;
        if (version == DRW::AC1018) writer = std::make_unique<dwgWriter18>(nullptr, &header);
        else writer = std::make_unique<dwgWriter24>(nullptr, &header);
        auto* observed = writer.get();
        facade.writer = std::move(writer);
        facade.version = version;
        DRW_Line line;
        line.space = paper ? DRW::PaperSpace : DRW::ModelSpace;
        line.basePoint = {1., 2., 3.}; line.secPoint = {4., 5., 6.};
        if (!facade.writeLine(&line)) return false;
        const auto beforeBytes = observed->m_buf.size();
        const auto beforeMap = observed->m_objectMap.size();
        const auto owned = [&]() -> const std::vector<std::uint32_t>& {
            return paper ? observed->m_paperSpaceEntityHandles : observed->m_modelSpaceEntityHandles;
        };
        const auto beforeOwned = owned();
        const auto beforeCount = facade.m_successfulEntityWrites;
        DRW_Polyline poly;
        poly.space = line.space;
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
            || owned() != beforeOwned
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
            || owned() != beforeOwned)
            return false;
        // The facade's required-failure latch remains set for this document,
        // but the restored writer and caller state permit a direct retry.
        return facade.writePolyline(&poly)
            && facade.getLastDwgCompoundEntityWriteReceipt(receipt)
            && receipt.valid && receipt.frames.size() == 4u;
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
        if (explicitModel) poly.parentHandle = paper ? DRW::DwgPaperSpaceBlockRecordHandle
                                                    : DRW::DwgModelSpaceBlockRecordHandle;
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
        const std::size_t expected = (version == DRW::AC1021 || version == DRW::AC1018 || version == DRW::AC1024)
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

    static bool unchangedUnsupportedScope(DRW::Version version, int kind) {
        DRW_Header header;
        dwgRW facade(nullptr);
        std::unique_ptr<dwgWriter15> writer;
        if (version == DRW::AC1018) writer = std::make_unique<dwgWriter18>(nullptr, &header);
        else writer = std::make_unique<dwgWriter24>(nullptr, &header);
        auto* observed = writer.get();
        facade.writer = std::move(writer);
        facade.version = version;
        std::uint32_t block = 0u;
        if (kind == 1) {
            block = observed->defineBlock("UNSUPPORTED_SCOPE", DRW_Coord(0., 0., 0.));
        } else if (kind == 2) {
            DRW_Block_Record layout;
            layout.name = "*Paper_Space123";
            layout.layoutHandle = 0x123u;
            block = observed->defineBlock(layout);
        }
        if (kind != 0) {
            if (!block || !observed->beginBlockContent(block)) return false;
        }
        DRW_Polyline poly;
        if (kind != 2) poly.space = DRW::PaperSpace;
        if (kind == 0) poly.parentHandle = DRW::DwgModelSpaceBlockRecordHandle;
        poly.addVertex(DRW_Vertex(0., 0., 0., 0.));
        poly.addVertex(DRW_Vertex(5., 0., 0., 0.));
        if (!facade.writePolyline(&poly)) return false;
        const auto owned = kind != 0 ? observed->findUserBlock(block)->entityHandles
                                     : observed->m_paperSpaceEntityHandles;
        return owned.size() == 4u && owned.front() == poly.handle
            && observed->m_ac1021PolylineChildPermit == nullptr;
    }

    static bool runNamedBlock(DRW::Version version, bool is3d, bool explicitOwner, int fault) {
        DRW_Header header;
        dwgRW facade(nullptr);
        std::unique_ptr<dwgWriter15> writer;
        if (version == DRW::AC1018) writer = std::make_unique<dwgWriter18>(nullptr, &header);
        else writer = std::make_unique<dwgWriter24>(nullptr, &header);
        auto* observed = writer.get();
        facade.writer = std::move(writer);
        facade.version = version;
        const auto block = observed->defineBlock("OWNER_TEST", DRW_Coord(0., 0., 0.));
        if (!block || !observed->beginBlockContent(block)) return false;
        DRW_Line first;
        first.basePoint = {-3., -1., 0.}; first.secPoint = {-2., -1., 0.};
        if (!facade.writeLine(&first)) return false;
        const auto beforeBytes = observed->m_buf.size();
        const auto beforeMap = observed->m_objectMap.size();
        const auto beforeOwned = observed->findUserBlock(block)->entityHandles;
        DRW_Polyline poly;
        if (explicitOwner) poly.parentHandle = block;
        const auto parentOwner = poly.parentHandle;
        const auto parentSpace = poly.space;
        poly.flags = is3d ? 8 : 0;
        poly.addVertex(DRW_Vertex(0., 0., 0., 0.));
        poly.addVertex(DRW_Vertex(5., 0., is3d ? 1. : 0., 0.));
        if (is3d) for (auto& v : poly.vertlist) v->flags = 32;
        const auto childOwner = poly.vertlist[0]->parentHandle;
        const auto childSpace = poly.vertlist[0]->space;
        const auto childSubtype = poly.vertlist[0]->dwgSubtype();
        DrwEntityEncodeTestAccess::seed(poly);
        for (auto& v : poly.vertlist) DrwEntityEncodeTestAccess::seed(*v);
        bool rejected = false;
        if (fault == 0) {
            facade.m_failNextCompoundSeqEndForTest = true;
            rejected = !facade.writePolyline(&poly);
        } else {
            if (fault == 1) drwThrowNextAc1021PolylineForTest();
            else drwThrowNextAc1021PolylineLateForTest();
            try { (void)facade.writePolyline(&poly); }
            catch (const std::bad_alloc&) { rejected = true; }
        }
        DRW::DwgCompoundEntityWriteReceipt receipt;
        if (!rejected || facade.getLastDwgCompoundEntityWriteReceipt(receipt)
            || observed->m_buf.size() != beforeBytes || observed->m_objectMap.size() != beforeMap
            || observed->findUserBlock(block)->entityHandles != beforeOwned
            || observed->m_ac1021PolylineChildPermit != nullptr || poly.handle != 0u
            || poly.parentHandle != parentOwner || poly.space != parentSpace
            || poly.dwgSeqEndHandle() != 0u || !DrwEntityEncodeTestAccess::preserved(poly)) return false;
        for (const auto& v : poly.vertlist)
            if (v->handle != 0u || v->parentHandle != childOwner || v->space != childSpace
                || v->dwgSubtype() != childSubtype || !DrwEntityEncodeTestAccess::preserved(*v)) return false;
        if (!facade.writePolyline(&poly)
            || !facade.getLastDwgCompoundEntityWriteReceipt(receipt)
            || !receipt.valid || receipt.frames.size() != 4u) return false;
        DRW_Line last;
        last.basePoint = {7., 4., 0.}; last.secPoint = {8., 4., 0.};
        if (!facade.writeLine(&last)) return false;
        const auto owned = observed->findUserBlock(block)->entityHandles;
        return owned == std::vector<std::uint32_t>{first.handle, poly.handle, last.handle}
            && observed->m_objectMap.size() == beforeMap + 5u && observed->endBlockContent();
    }
};

class ExceptionAdapter final : public LC3CompatCheck {
public:
    explicit ExceptionAdapter(dwgRW& out) : output(out) {}
    void writeBlocks() override {
        if (!named) return;
        const auto block = output.defineBlock("EXCEPTION_BLOCK", DRW_Coord(0., 0., 0.));
        if (!block || !output.beginBlockContent(block)) return;
        emitPolyline();
        (void)output.endBlockContent();
    }
    void writeEntities() override { if (!named) emitPolyline(); }
    void emitPolyline() {
        DRW_Polyline poly;
        if (paper) poly.space = DRW::PaperSpace;
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
    bool inject {true}, late {false}, threeD {false}, paper {false}, named {false};
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
        poly.addVertex(DRW_Vertex(1., 2., 0., 0.));
        poly.addVertex(DRW_Vertex(3., 4., 0., 0.));
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
    bool targetChild {true}, registered {false}, wrotePolyline {false}, wroteGroup {false};
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
 for (const auto version : {DRW::AC1018,DRW::AC1024}) {
  for (const bool late : {false,true}) for (const bool paper : {false,true})
   if (!DwgBlockRecordXrefWireTestAccess::exceptionRollback(version,late,paper)) { std::cerr << "exception " << int(version) << "\n"; return 1; }
  for (const bool is3d : {false,true}) for (const bool paper : {false,true}) for (const bool explicitOwner : {false,true})
   if (!DwgBlockRecordXrefWireTestAccess::run(version,is3d,paper,explicitOwner)) {std::cerr << "membership " << int(version) << "\n"; return 1;}
  for (int kind : {0,1,2}) if (!DwgBlockRecordXrefWireTestAccess::unchangedUnsupportedScope(version,kind)) {std::cerr << "scope " << int(version) << " kind " << kind << "\n"; return 1;}
  for (const bool is3d : {false,true}) for (const bool explicitOwner : {false,true}) for (int fault=0;fault!=3;++fault)
   if (!DwgBlockRecordXrefWireTestAccess::runNamedBlock(version,is3d,explicitOwner,fault)) {std::cerr << "named " << int(version) << " fault " << fault << "\n"; return 1;}
  for (int flags : {16,64}) if (!DwgBlockRecordXrefWireTestAccess::run(version,false,false,false,flags)) return 1;
 }
 const auto path = std::filesystem::temp_directory_path() / ("poly-owner-cross-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dwg");
 struct Cleanup { std::filesystem::path p; ~Cleanup(){std::error_code e;std::filesystem::remove(p,e);} } cleanup{path};
 dwgRW output(path.string().c_str()); ExceptionAdapter adapter(output);
 for (const auto version : {DRW::AC1018,DRW::AC1024}) for (const bool late : {false,true}) for (const bool threeD : {false,true}) for (int scope : {1,2}) {
  {std::ofstream f(path,std::ios::binary|std::ios::trunc); f << "prior destination";}
  adapter.inject=true;adapter.late=late;adapter.threeD=threeD;adapter.paper=scope==1;adapter.named=scope==2;adapter.caught=adapter.restored=adapter.ok=false;
  if (output.write(&adapter,version,true) || !adapter.caught || !adapter.restored) {std::cerr << "atomic " << int(version) << " scope " << scope << "\n";return 1;}
  std::ifstream f(path,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(f),{})!="prior destination")return 1;f.close();
  adapter.inject=false;if(!output.write(&adapter,version,true)||!adapter.ok) {std::cerr << "retry " << int(version) << " scope " << scope << "\n";return 1;}
 }
 return 0;
}
