// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <vector>

void drwThrowNextLegacyPolylineForTest();
void drwThrowNextLegacyPolylineLateForTest();

class DrwEntityEncodeTestAccess {
public:
    static void seedRawLayer(DRW_Entity& entity) {
        entity.dwgEedLayerWriteRefs.push_back({7u,0x1234u,0x123456789ABCDEF0ULL,true});
        entity.dwgEedAppIdWriteRefs.push_back({9u,0x2345u,0xFEDCBA9876543210ULL,true});
    }
    static bool rawLayerPreserved(const DRW_Entity& entity) {
        if(entity.dwgEedLayerWriteRefs.size()!=1u || entity.dwgEedAppIdWriteRefs.size()!=1u) return false;
        const auto& ref=entity.dwgEedLayerWriteRefs[0];
        const auto& app=entity.dwgEedAppIdWriteRefs[0];
        return ref.indexInExtData==7u && ref.handleRef==0x1234u
            && ref.rawHandleRef==0x123456789ABCDEF0ULL && ref.useRawHandleRef
            && app.indexInExtData==9u && app.handleRef==0x2345u
            && app.rawHandleRef==0xFEDCBA9876543210ULL && app.useRawHandleRef;
    }
};

class DwgCompoundWriteTestAccess {
public:
    static void failNextSeqEnd(dwgRW& writer) { writer.m_failNextCompoundSeqEndForTest=true; }
};

namespace {
constexpr std::uint32_t kLine=0x130u, kPoly=0x451u, kCircle=0x999u;
constexpr std::array<std::uint32_t,3> kVertices{{0xA10u,0xA02u,0xA20u}};
constexpr std::uint32_t kSeqend=0xA50u;
enum class Mode { Normal, ThreeD, Singleton, TwoVertex, TwoVertex3D, ExplicitSeqend, WrongSeqend, InjectedSeqend, InjectedException, InjectedLateException,
                  MissingCompoundPlan, DuplicateCompoundPlan, WrongPlanParent,
                  EmptyPlan, OverBudgetPlan, RetainedFooter, DuplicatePlanVertex, PlanOwnerCollision, PlanSeqCollision,
                  ZeroPlanChild, ZeroPlanSeqend, ReservedPlanChild, ReservedPlanSeqend,
                  WrongVertex, DuplicateVertex, MissingVertex, ExtraVertex, NullVertex,
                  VertexOtherOwner, VertexPaper, VertexExplicitOwner,
                  AliasNod, AliasGroup, AliasDictionary, AliasXrecord, AliasImage,
                  Rich2D, Explicit3DFlags, BadPolyFlags, BadCurve, BadBaseX, BadBaseY,
                  BadVertexCount, BadFaceCount, BadSmoothM, BadSmoothN, Bad2DVertexZ,
                  Bad3DBaseZ, Bad3DExtrusion, Bad3DStartWidth, Bad3DEndWidth, Bad3DThickness,
                  Bad3DVertexStart, Bad3DVertexEnd, Bad3DVertexBulge, Bad3DVertexTangent,
                  BadVertexIdentifier, BadVertexIndex1, BadVertexIndex2, BadVertexIndex3,
                  BadVertexIndex4, Bad2DVertexFlags, Bad3DVertexFlags, Bad2DNegativeWidth,
                  Mesh, Polyface, WrongOrder, ExtraPolyline, Paper, OtherOwner };
using Receipt=DRW::DwgCompoundEntityWriteReceipt;

struct TwoPolylineAdapter : LC3CompatCheck, DRW_DwgLegacyModelChainProvider,
        DRW_DwgLegacyPolylineChainProvider, DRW_DwgDefaultPlotStyleProvider {
    explicit TwoPolylineAdapter(dwgRW& writer) : output(writer) {}
    std::vector<DRW_DwgLegacyModelChainEntry> dwgLegacyModelChain() const override {
        return {{0x451u,DRW::POLYLINE},{0x777u,DRW::POLYLINE}};
    }
    std::vector<DRW_DwgLegacyPolylineChain> dwgLegacyPolylineChains() const override {
        DRW_DwgLegacyPolylineChain first,second;
        first.parentHandle=0x451u;first.vertexHandles={0xA10u,0xA02u};first.seqendHandle=0xA50u;
        second.parentHandle=0x777u;second.vertexHandles={0xC11u,0xC00u};second.seqendHandle=0xC50u;
        return {first,second};
    }
    void writeEntities() override {
        DRW_Polyline first,second;first.handle=0x451u;second.handle=0x777u;second.flags=8;
        for(std::size_t i=0;i<2;++i) {
            DRW_Vertex a,b;a.handle=i?0xA02u:0xA10u;b.handle=i?0xC00u:0xC11u;
            a.basePoint={double(i),double(i*2u),0.};b.basePoint={double(i),double(i*2u),double(i+1u)};
            first.addVertex(a);second.addVertex(b);
        }
        DRW::DwgCompoundEntityWriteReceipt saved,secondReceipt;
        if(!output.writePolyline(&first)||!output.getLastDwgCompoundEntityWriteReceipt(saved))return;
        if(!output.writePolyline(&second)||!output.getLastDwgCompoundEntityWriteReceipt(secondReceipt))return;
        const auto check=[](const DRW::DwgCompoundEntityWriteReceipt& r,std::uint32_t parent,
                std::uint32_t v1,std::uint32_t v2,std::uint32_t seq) {
            return r.valid && r.parentHandle==parent && r.frames.size()==4u
                && r.frames[0].role==DRW::DwgCompoundEntityFrameRole::Parent
                && r.frames[0].frame.objectHandle==parent
                && r.frames[1].role==DRW::DwgCompoundEntityFrameRole::Vertex
                && r.frames[1].frame.objectHandle==v1
                && r.frames[2].role==DRW::DwgCompoundEntityFrameRole::Vertex
                && r.frames[2].frame.objectHandle==v2
                && r.frames[3].role==DRW::DwgCompoundEntityFrameRole::SeqEnd
                && r.frames[3].frame.objectHandle==seq;
        };
        valid=check(saved,0x451u,0xA10u,0xA02u,0xA50u)
            && check(secondReceipt,0x777u,0xC11u,0xC00u,0xC50u)
            && saved.frames[0].frame.generation!=secondReceipt.frames[0].frame.generation;
    }
    dwgRW& output; bool valid=false;
};

struct OrphanProvider : LC3CompatCheck, DRW_DwgLegacyPolylineChainProvider {
    std::vector<DRW_DwgLegacyPolylineChain> dwgLegacyPolylineChains() const override {
        DRW_DwgLegacyPolylineChain chain;
        chain.parentHandle=kPoly;chain.vertexHandles={kVertices[0]};chain.seqendHandle=kSeqend;
        return {chain};
    }
};

struct Adapter : LC3CompatCheck, DRW_DwgLegacyModelChainProvider,
                 DRW_DwgLegacyPolylineChainProvider, DRW_DwgDefaultPlotStyleProvider {
    explicit Adapter(dwgRW& w) : output(w) {}
    std::vector<DRW_DwgLegacyModelChainEntry> dwgLegacyModelChain() const override {
        return {{kLine,DRW::LINE},{kPoly,DRW::POLYLINE},{kCircle,DRW::CIRCLE}};
    }
    std::vector<DRW_DwgLegacyPolylineChain> dwgLegacyPolylineChains() const override {
        DRW_DwgLegacyPolylineChain chain;
        chain.parentHandle=kPoly;
        chain.vertexHandles.assign(kVertices.begin(),kVertices.end());
        chain.seqendHandle=kSeqend;
        if(mode==Mode::OverBudgetPlan) {
            chain.vertexHandles.resize(999997u);
            for(std::uint32_t i=0;i<chain.vertexHandles.size();++i)
                chain.vertexHandles[i]=0x10000u+i;
        }
        if(mode==Mode::Singleton) chain.vertexHandles.resize(1);
        if(mode==Mode::TwoVertex||mode==Mode::TwoVertex3D) chain.vertexHandles.resize(2);
        if(mode==Mode::WrongPlanParent) chain.parentHandle=0xB500u;
        if(mode==Mode::EmptyPlan) chain.vertexHandles.clear();
        if(mode==Mode::DuplicatePlanVertex) chain.vertexHandles[1]=chain.vertexHandles[0];
        if(mode==Mode::PlanOwnerCollision) chain.vertexHandles[1]=kLine;
        if(mode==Mode::PlanSeqCollision) chain.seqendHandle=chain.vertexHandles[0];
        if(mode==Mode::ZeroPlanChild) chain.vertexHandles[0]=0u;
        if(mode==Mode::ZeroPlanSeqend) chain.seqendHandle=0u;
        if(mode==Mode::ReservedPlanChild) chain.vertexHandles[0]=0x12u;
        if(mode==Mode::ReservedPlanSeqend) chain.seqendHandle=0x12u;
        if(mode==Mode::MissingCompoundPlan) return {};
        if(mode==Mode::DuplicateCompoundPlan) return {chain,chain};
        return {chain};
    }
    void writeLTypes() override {
        const std::array<std::uint32_t,5> ids{{kPoly,kVertices[0],kVertices[1],kVertices[2],kSeqend}};
        if(mode==Mode::AliasNod) rejectedAlias=!output.registerDwgNamedObjectDictionaryEntry("ILLEGAL",ids[aliasSlot]);
        if(mode==Mode::AliasGroup) rejectedAlias=!output.registerDwgGroupDictionaryEntry("ILLEGAL",ids[aliasSlot]);
    }
    void writeObjects() override {
        if(mode==Mode::AliasDictionary) {
            const std::array<std::uint32_t,5> ids{{kPoly,kVertices[0],kVertices[1],kVertices[2],kSeqend}};
            DRW_Dictionary dictionary; dictionary.handle=0xB600u;
            dictionary.parentHandle=DRW::DwgNamedObjectsDictionaryHandle;
            dictionary.m_entries.push_back({"ILLEGAL",ids[aliasSlot]});
            rejectedAlias=!output.writeDictionary(&dictionary);
        }
    }
    void writeEntities() override {
        receipt={}; preMutationPreserved=true; polyWrite=false;
        DRW_Line line; line.handle=kLine; line.basePoint={1.,2.,3.}; line.secPoint={4.,5.,6.};
        DRW_Circle circle; circle.handle=kCircle; circle.basePoint={7.,8.,0.}; circle.radious=2.5;
        DRW_Polyline poly; poly.handle=kPoly;
        const bool special3D=mode==Mode::Explicit3DFlags||mode==Mode::Bad3DBaseZ
            ||mode==Mode::Bad3DExtrusion||mode==Mode::Bad3DStartWidth||mode==Mode::Bad3DEndWidth
            ||mode==Mode::Bad3DThickness||mode==Mode::Bad3DVertexStart||mode==Mode::Bad3DVertexEnd
            ||mode==Mode::Bad3DVertexBulge||mode==Mode::Bad3DVertexTangent||mode==Mode::Bad3DVertexFlags;
        if(mode==Mode::ThreeD||mode==Mode::TwoVertex3D||special3D) poly.flags=8;
        if(mode==Mode::BadPolyFlags) poly.flags=2;
        if(mode==Mode::BadCurve) poly.curvetype=5;
        if(mode==Mode::BadBaseX) poly.basePoint.x=1;
        if(mode==Mode::BadBaseY) poly.basePoint.y=1;
        if(mode==Mode::BadVertexCount) poly.vertexcount=1;
        if(mode==Mode::BadFaceCount) poly.facecount=1;
        if(mode==Mode::BadSmoothM) poly.smoothM=1;
        if(mode==Mode::BadSmoothN) poly.smoothN=1;
        if(mode==Mode::Bad3DBaseZ) poly.basePoint.z=1;
        if(mode==Mode::Bad3DExtrusion) poly.extPoint={0.,1.,0.};
        if(mode==Mode::Bad3DStartWidth) poly.defstawidth=.2;
        if(mode==Mode::Bad3DEndWidth) poly.defendwidth=.2;
        if(mode==Mode::Bad3DThickness) poly.thickness=.2;
        if(mode==Mode::Rich2D) { poly.defstawidth=.5;poly.defendwidth=.75;poly.thickness=.25; }
        if(mode==Mode::Mesh) poly.flags=16;
        if(mode==Mode::Polyface) poly.flags=64;
        if(mode==Mode::Paper) poly.space=DRW::PaperSpace;
        if(mode==Mode::OtherOwner) poly.parentHandle=0xB200u;
        std::size_t count=mode==Mode::Singleton?1u:
            (mode==Mode::TwoVertex||mode==Mode::TwoVertex3D)?2u:3u;
        if(mode==Mode::MissingVertex) count=2;
        if(mode==Mode::ExtraVertex) count=4;
        for(std::size_t i=0;i<count;++i) {
            DRW_Vertex v; v.handle=i<kVertices.size()?kVertices[i]:0xA30u;
            v.basePoint={double(i),double(i*2u),(mode==Mode::ThreeD||mode==Mode::TwoVertex3D||special3D)?double(i+1u):0.};
            poly.addVertex(v);
        }
        if(mode==Mode::Rich2D) {
            poly.vertlist[0]->stawidth=.2;poly.vertlist[0]->endwidth=.4;
            poly.vertlist[0]->bulge=.25;poly.vertlist[0]->tgdir=.75;
        }
        if(mode==Mode::Explicit3DFlags) for(auto& v:poly.vertlist) v->flags=32;
        if(mode==Mode::Bad2DVertexZ) poly.vertlist[0]->basePoint.z=1;
        if(mode==Mode::Bad3DVertexStart) poly.vertlist[0]->stawidth=.2;
        if(mode==Mode::Bad3DVertexEnd) poly.vertlist[0]->endwidth=.2;
        if(mode==Mode::Bad3DVertexBulge) poly.vertlist[0]->bulge=.2;
        if(mode==Mode::Bad3DVertexTangent) poly.vertlist[0]->tgdir=.2;
        if(mode==Mode::BadVertexIdentifier) poly.vertlist[0]->identifier=1;
        if(mode==Mode::BadVertexIndex1) poly.vertlist[0]->vindex1=1;
        if(mode==Mode::BadVertexIndex2) poly.vertlist[0]->vindex2=1;
        if(mode==Mode::BadVertexIndex3) poly.vertlist[0]->vindex3=1;
        if(mode==Mode::BadVertexIndex4) poly.vertlist[0]->vindex4=1;
        if(mode==Mode::Bad2DVertexFlags) poly.vertlist[0]->flags=1;
        if(mode==Mode::Bad3DVertexFlags) poly.vertlist[0]->flags=64;
        if(mode==Mode::Bad2DNegativeWidth) poly.vertlist[0]->stawidth=-.2;
        if(mode==Mode::WrongVertex) poly.vertlist[1]->handle=0xA03u;
        if(mode==Mode::DuplicateVertex) poly.vertlist[1]->handle=poly.vertlist[0]->handle;
        if(mode==Mode::NullVertex) poly.vertlist[1].reset();
        if(mode==Mode::VertexOtherOwner) poly.vertlist[1]->parentHandle=0xB200u;
        if(mode==Mode::VertexPaper) poly.vertlist[1]->space=DRW::PaperSpace;
        if(mode==Mode::VertexExplicitOwner) for(auto& v:poly.vertlist) v->parentHandle=kPoly;
        const bool seedRaw=mode==Mode::InjectedSeqend||mode==Mode::InjectedException
            ||mode==Mode::InjectedLateException;
        if(seedRaw) {
            DrwEntityEncodeTestAccess::seedRawLayer(poly);
            for(auto& v:poly.vertlist) if(v)DrwEntityEncodeTestAccess::seedRawLayer(*v);
        }
        if(mode==Mode::RetainedFooter) poly.seqEndEntity=std::make_shared<DRW_SeqEnd>();
        if(mode==Mode::ExplicitSeqend) poly.setDwgSeqEndHandle(kSeqend);
        if(mode==Mode::WrongSeqend) poly.setDwgSeqEndHandle(0xA51u);
        const auto originalSeqend=poly.dwgSeqEndHandle();
        const auto originalParent=poly.parentHandle;
        const auto originalSpace=poly.space;
        const auto originalFlags=poly.flags;
        std::vector<std::array<std::uint32_t,4>> before;
        for(const auto& v:poly.vertlist) if(v)before.push_back({v->handle,v->parentHandle,
            static_cast<std::uint32_t>(v->space),static_cast<std::uint32_t>(v->dwgSubtype())});
        if(mode==Mode::AliasXrecord||mode==Mode::AliasImage) {
            const std::array<std::uint32_t,5> ids{{kPoly,kVertices[0],kVertices[1],kVertices[2],kSeqend}};
            if(mode==Mode::AliasXrecord) {
                DRW_XRecord x; x.handle=0xB601u;x.parentHandle=ids[aliasSlot];
                rejectedAlias=!output.writeXRecord(&x);
            } else {
                DRW_ImageDef image; image.handle=0xB602u;image.parentHandle=ids[aliasSlot];
                image.name="local-image.png";rejectedAlias=!output.writeImageDef(&image);
            }
        }
        auto emitLine=[&] { (void)output.writeLine(&line); };
        auto emitPolyline=[&] {
            if(mode==Mode::InjectedSeqend) DwgCompoundWriteTestAccess::failNextSeqEnd(output);
            if(mode==Mode::InjectedException) drwThrowNextLegacyPolylineForTest();
            if(mode==Mode::InjectedLateException) drwThrowNextLegacyPolylineLateForTest();
            try { polyWrite=output.writePolyline(&poly); }
            catch(const std::bad_alloc&) {
                preMutationPreserved=poly.dwgSeqEndHandle()==originalSeqend
                    && poly.handle==kPoly && poly.parentHandle==originalParent
                    && poly.space==originalSpace && poly.flags==originalFlags;
                std::size_t n=0;
                for(const auto& v:poly.vertlist) if(v) {
                    preMutationPreserved &= n<before.size()
                        && v->handle==before[n][0] && v->parentHandle==before[n][1]
                        && static_cast<std::uint32_t>(v->space)==before[n][2]
                        && static_cast<std::uint32_t>(v->dwgSubtype())==before[n][3];
                    ++n;
                }
                if(seedRaw) {
                    preMutationPreserved &= DrwEntityEncodeTestAccess::rawLayerPreserved(poly);
                    for(const auto& v:poly.vertlist) if(v)
                        preMutationPreserved &= DrwEntityEncodeTestAccess::rawLayerPreserved(*v);
                }
                DRW::DwgCompoundEntityWriteReceipt noReceipt;
                preMutationPreserved &= !output.getLastDwgCompoundEntityWriteReceipt(noReceipt)
                    && output.getLastSuccessfulEntityHandle()==0u;
                exceptionObserved=true;
                throw;
            }
            if(polyWrite) {
                (void)output.getLastDwgCompoundEntityWriteReceipt(receipt);
                seqendAfter=poly.dwgSeqEndHandle();
            } else {
                preMutationPreserved=poly.dwgSeqEndHandle()==originalSeqend
                    && poly.handle==kPoly && poly.parentHandle==originalParent
                    && poly.space==originalSpace && poly.flags==originalFlags;
                std::size_t n=0;
                for(const auto& v:poly.vertlist) if(v) {
                    preMutationPreserved &= n<before.size()
                        && v->handle==before[n][0] && v->parentHandle==before[n][1]
                        && static_cast<std::uint32_t>(v->space)==before[n][2]
                        && static_cast<std::uint32_t>(v->dwgSubtype())==before[n][3];
                    ++n;
                }
                if(seedRaw) {
                    preMutationPreserved &= DrwEntityEncodeTestAccess::rawLayerPreserved(poly);
                    for(const auto& v:poly.vertlist) if(v)
                        preMutationPreserved &= DrwEntityEncodeTestAccess::rawLayerPreserved(*v);
                }
            }
        };
        if(mode==Mode::WrongOrder) { emitPolyline();emitLine(); }
        else { emitLine();emitPolyline(); }
        (void)output.writeCircle(&circle);
        if(mode==Mode::ExtraPolyline) { DRW_Polyline extra; extra.handle=0xB220u; (void)output.writePolyline(&extra); }
    }
    static std::vector<std::uint64_t> links(const DRW::DwgObjectFrameReceipt& frame) {
        std::vector<std::uint64_t> result;
        for(const auto& item:frame.occurrences)
            if(item.stream==DRW::DwgObjectHandleStream::Handles
               && item.token.code==DRW::DwgSoftPointer) result.push_back(item.token.reference);
        return result;
    }
    bool receiptValid() const {
        const auto count=mode==Mode::Singleton?1u:
            (mode==Mode::TwoVertex||mode==Mode::TwoVertex3D)?2u:3u;
        if(!polyWrite || !receipt.valid || receipt.parentHandle!=kPoly
           || receipt.frames.size()!=count+2u || seqendAfter!=kSeqend
           || receipt.frames.front().role!=DRW::DwgCompoundEntityFrameRole::Parent
           || receipt.frames.back().role!=DRW::DwgCompoundEntityFrameRole::SeqEnd) return false;
        const auto& parentFrame=receipt.frames.front().frame;
        const auto parentLinks=links(parentFrame);
        const auto seqReferences=std::count_if(parentFrame.occurrences.begin(),parentFrame.occurrences.end(),
            [](const auto& occurrence) { return occurrence.stream==DRW::DwgObjectHandleStream::Handles
                && occurrence.token.code==DRW::DwgHardOwnership && occurrence.token.reference==kSeqend; });
        if(seqReferences!=1) return false;
        if(parentLinks.size()<4u || parentLinks[0]!=kLine || parentLinks[1]!=kCircle
           || parentLinks[parentLinks.size()-2]!=kVertices[0]
           || parentLinks.back()!=kVertices[count-1]) return false;
        for(std::size_t i=0;i<count;++i) {
            const auto& frame=receipt.frames[i+1];
            if(frame.role!=DRW::DwgCompoundEntityFrameRole::Vertex
               || !frame.frame.valid || frame.frame.objectHandle!=kVertices[i])return false;
            const auto child=links(frame.frame);
            const std::uint32_t prev=i?kVertices[i-1]:0u;
            const std::uint32_t next=i+1<count?kVertices[i+1]:0u;
            if(child.size()<3u || child[0]!=kPoly || child[1]!=prev || child[2]!=next) return false;
        }
        const auto& last=receipt.frames.back().frame;
        if(!last.valid || last.objectHandle!=kSeqend) return false;
        const auto tail=links(last);
        return tail.size()>=3u && tail[0]==kPoly && tail[1]==0u && tail[2]==0u;
    }
    dwgRW& output; Mode mode=Mode::Normal; bool polyWrite=false,preMutationPreserved=true;
    bool rejectedAlias=false,exceptionObserved=false; std::size_t aliasSlot=0;
    std::uint32_t seqendAfter=0; Receipt receipt;
};
}
int main(int argc,char**argv) {
    const bool keep=argc==2 && std::string(argv[1])=="--keep";
    if(argc>2 || (argc==2&&!keep)) return 2;
    const auto dir=std::filesystem::temp_directory_path() /
        ("libdxfrw-legacy-polyline-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if(!std::filesystem::create_directory(dir))return 2;
    struct Cleanup {std::filesystem::path p;bool keep;~Cleanup(){if(!keep){std::error_code e;std::filesystem::remove_all(p,e);}}} cleanup{dir,keep};
    const auto path=(dir/"polyline.dwg").string(); dwgRW output(path.c_str()); Adapter adapter(output);
    auto success=[&](Mode mode) {
        adapter.mode=mode;
        return output.write(&adapter,DRW::AC1015,true) && adapter.receiptValid();
    };
    if(!success(Mode::Normal)) {std::cerr<<"normal polyline/receipt failed\n";return 1;}
    if(keep)std::filesystem::copy_file(path,dir/"polyline2d.dwg");
    if(!success(Mode::ThreeD)) {std::cerr<<"3d polyline failed\n";return 1;}
    if(keep)std::filesystem::copy_file(path,dir/"polyline3d.dwg");
    if(!success(Mode::TwoVertex)) {std::cerr<<"two vertex 2d polyline failed\n";return 1;}
    if(keep)std::filesystem::copy_file(path,dir/"polyline2d-two.dwg");
    if(!success(Mode::TwoVertex3D)) {std::cerr<<"two vertex 3d polyline failed\n";return 1;}
    if(keep)std::filesystem::copy_file(path,dir/"polyline3d-two.dwg");
    if(!success(Mode::Rich2D)) {std::cerr<<"rich 2d polyline failed\n";return 1;}
    if(keep)std::filesystem::copy_file(path,dir/"polyline2d-rich.dwg");
    if(!success(Mode::Explicit3DFlags)) {std::cerr<<"explicit 3d vertex flags failed\n";return 1;}
    if(!success(Mode::ExplicitSeqend)) {std::cerr<<"explicit seqend failed\n";return 1;}
    if(!success(Mode::VertexExplicitOwner)) {std::cerr<<"explicit vertex owner failed\n";return 1;}
    {std::ofstream f(path,std::ios::binary);f<<"prior destination";}
    OrphanProvider orphan;
    if(output.write(&orphan,DRW::AC1015,true)) return 1;
    {std::ifstream f(path,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(f),{})!="prior destination")return 1;}
    if(!success(Mode::Normal))return 1;
    for (auto mode : {Mode::Singleton,Mode::WrongSeqend,Mode::InjectedSeqend,
        Mode::InjectedException,Mode::InjectedLateException,Mode::MissingCompoundPlan,Mode::DuplicateCompoundPlan,
        Mode::WrongPlanParent,Mode::EmptyPlan,Mode::OverBudgetPlan,Mode::RetainedFooter,Mode::DuplicatePlanVertex,Mode::PlanOwnerCollision,
        Mode::PlanSeqCollision,Mode::ZeroPlanChild,Mode::ZeroPlanSeqend,
        Mode::ReservedPlanChild,Mode::ReservedPlanSeqend,Mode::WrongVertex,Mode::DuplicateVertex,Mode::MissingVertex,
        Mode::ExtraVertex,Mode::NullVertex,Mode::VertexOtherOwner,Mode::VertexPaper,
        Mode::AliasNod,Mode::AliasGroup,Mode::AliasDictionary,Mode::AliasXrecord,Mode::AliasImage,
        Mode::BadPolyFlags,Mode::BadCurve,Mode::BadBaseX,Mode::BadBaseY,
        Mode::BadVertexCount,Mode::BadFaceCount,Mode::BadSmoothM,Mode::BadSmoothN,Mode::Bad2DVertexZ,
        Mode::Bad3DBaseZ,Mode::Bad3DExtrusion,Mode::Bad3DStartWidth,Mode::Bad3DEndWidth,Mode::Bad3DThickness,
        Mode::Bad3DVertexStart,Mode::Bad3DVertexEnd,Mode::Bad3DVertexBulge,Mode::Bad3DVertexTangent,
        Mode::BadVertexIdentifier,Mode::BadVertexIndex1,Mode::BadVertexIndex2,Mode::BadVertexIndex3,
        Mode::BadVertexIndex4,Mode::Bad2DVertexFlags,Mode::Bad3DVertexFlags,Mode::Bad2DNegativeWidth,
        Mode::Mesh,Mode::Polyface,Mode::WrongOrder,
        Mode::ExtraPolyline,Mode::Paper,Mode::OtherOwner}) {
        const bool alias=mode==Mode::AliasNod||mode==Mode::AliasGroup||mode==Mode::AliasDictionary
            ||mode==Mode::AliasXrecord||mode==Mode::AliasImage;
        for(std::size_t slot=0;slot<(alias?5u:1u);++slot) {
            {std::ofstream f(path,std::ios::binary);f<<"prior destination";}
            adapter.mode=mode;adapter.aliasSlot=slot;adapter.rejectedAlias=false;adapter.exceptionObserved=false;
            if(output.write(&adapter,DRW::AC1015,true)) {std::cerr<<"accepted invalid mode="<<int(mode)<<" slot="<<slot<<"\n";return 1;}
            {std::ifstream f(path,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(f),{})!="prior destination")return 1;}
            if(alias && !adapter.rejectedAlias) {std::cerr<<"alias not rejected mode="<<int(mode)<<" slot="<<slot<<"\n";return 1;}
            if((mode==Mode::InjectedException||mode==Mode::InjectedLateException) && !adapter.exceptionObserved) {std::cerr<<"exception seam did not run\n";return 1;}
            if(!adapter.preMutationPreserved) {std::cerr<<"caller mutation leaked mode="<<int(mode)<<"\n";return 1;}
            if(!success(Mode::Normal)) {std::cerr<<"retry failed mode="<<int(mode)<<"\n";return 1;}
        }
    }
    const auto multiplePath=(dir/"polyline-multiple.dwg").string();
    dwgRW multipleOutput(multiplePath.c_str());TwoPolylineAdapter multiple(multipleOutput);
    if(!multipleOutput.write(&multiple,DRW::AC1015,true)||!multiple.valid) {
        std::cerr<<"multiple independent polylines failed\n";return 1;
    }
    if(keep)std::cout<<"OUTPUT "<<dir.string()<<'\n';
    std::cout<<"AC1015 bounded polyline chain PASS\n";return 0;
}
