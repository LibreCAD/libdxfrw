// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <new>
#include <vector>

void drwThrowNextLegacyInsertForTest();
void drwThrowNextLegacyInsertLateForTest();
class DrwEntityEncodeTestAccess {
public:
    static void seedRaw(DRW_Entity& entity) {
        entity.dwgEedLayerWriteRefs.push_back({7u,0x1234u,0x123456789ABCDEF0ULL,true});
        entity.dwgEedAppIdWriteRefs.push_back({9u,0x2345u,0xFEDCBA9876543210ULL,true});
    }
    static bool rawPreserved(const DRW_Entity& entity) {
        if(entity.dwgEedLayerWriteRefs.size()!=1u||entity.dwgEedAppIdWriteRefs.size()!=1u)return false;
        const auto& l=entity.dwgEedLayerWriteRefs[0];const auto& a=entity.dwgEedAppIdWriteRefs[0];
        return l.indexInExtData==7u&&l.handleRef==0x1234u&&l.rawHandleRef==0x123456789ABCDEF0ULL&&l.useRawHandleRef
            &&a.indexInExtData==9u&&a.handleRef==0x2345u&&a.rawHandleRef==0xFEDCBA9876543210ULL&&a.useRawHandleRef;
    }
};

class DwgCompoundWriteTestAccess {
public:
    static void failNextSeqEnd(dwgRW& writer) { writer.m_failNextCompoundSeqEndForTest=true; }
};
namespace {
constexpr std::uint32_t kLine=0x130u,kFirst=0x451u,kCircle=0x999u,kSecond=0x777u,kLast=0xB00u;
constexpr std::uint32_t kRecord=0xD00u,kBlock=0xD10u,kEnd=0xD01u;
constexpr std::uint32_t kAttr1=0xA10u,kAttr2=0xA02u,kSeq1=0xA50u,kAttr3=0xC10u,kSeq2=0xC50u;
enum class Mode { Normal, Rotated, ScaleUniform, ScaleXUnit, ScaleNonuniform, ScaleFractional, ScaleNearUnit,
                  MatchingReferences, NoBlock, WrongBlockName, WrongBlockBase,
                  WrongBlockUnits, ExtraBlock, BlockContent, WrongBlockReference,
                  MInsert, WrongSeqend, RetainedFooter, EmptyAttributes, WrongAttribute,
                  DuplicateAttribute, ExtraAttribute, NullAttribute, WrongAttrOwner,
                  WrongAttrSpace, WrongOrder, ExtraInsert, Paper, OtherOwner,
                  MissingBlockPlan, DuplicateModel, WrongPlanParent, WrongPlanBlock,
                  DuplicatePlanAttr, PlanOwnerCollision, PlanSeqCollision, ZeroPlanAttr,
                  ZeroPlanSeq, ReservedPlanAttr, ReservedPlanSeq, InjectedSeqend,
                  InjectedException, InjectedLateException, BadXScale, BadYScale, BadZScale, BadScaleNan,
                  BadExtrusion, ZeroExtrusion, NanExtrusion, BadPosition, BadRotation, BadRowSpacing, BadColSpacing,
                  BadStyle, BadMText, BadAttVersion, BadAttributeType, BadLock,
                  BadXRecord, BadAnnotative, BadAlignmentZ, BadUnicodeText, BadUnicodeTag,
                  DuplicateTag, RetainedAttrHandles, OverBudgetPlan, EarlyNodAlias, EarlyGroupAlias,
                  LateNodAlias, LateGroupAlias, DictionaryAlias, DefaultDictionaryAlias,
                  XrecordOwnerAlias, ImageOwnerAlias, GroupMember, BadAttrExtrusion, BadAttrAlignH,
                  BadAttrAlignV, BadAttrAlignX, BadAttrAlignY, BadAttrWidthScale,
                  BadAttrOblique, BadAttrThickness, BadAttrTextgen, BadAttrFlags,
                  BadAttrFieldLength, BadAttrZ };
using Receipt=DRW::DwgCompoundEntityWriteReceipt;
struct Adapter : LC3CompatCheck, DRW_DwgLegacyModelChainProvider,
                 DRW_DwgLegacyInsertChainProvider, DRW_DwgDefaultPlotStyleProvider {
    explicit Adapter(dwgRW& w):output(w){}
    std::vector<DRW_DwgLegacyModelChainEntry> dwgLegacyModelChain() const override {
        std::vector<DRW_DwgLegacyModelChainEntry> plan{{kLine,DRW::LINE},{kFirst,DRW::INSERT},
            {kCircle,DRW::CIRCLE},{kSecond,DRW::INSERT},{kLast,DRW::LINE}};
        if(mode==Mode::DuplicateModel)plan[3].handle=plan[1].handle;
        return plan;
    }
    DRW_DwgLegacyEmptyBlock dwgLegacyEmptyBlock() const override {
        DRW_DwgLegacyEmptyBlock b;b.name="EMPTY_CHAIN";
        b.blockRecordHandle=kRecord;b.blockHandle=kBlock;b.endBlockHandle=kEnd;
        if(mode==Mode::MissingBlockPlan)b.name.clear();
        return b;
    }
    std::vector<DRW_DwgLegacyInsertChain> dwgLegacyInsertChains() const override {
        DRW_DwgLegacyInsertChain a,b;
        a.parentHandle=kFirst;a.blockRecordHandle=kRecord;a.attributeHandles={kAttr1,kAttr2};a.seqendHandle=kSeq1;
        b.parentHandle=kSecond;b.blockRecordHandle=kRecord;b.attributeHandles={kAttr3};b.seqendHandle=kSeq2;
        if(mode==Mode::OverBudgetPlan) {
            a.attributeHandles.resize(999990u);
            for(std::uint32_t i=0;i<a.attributeHandles.size();++i)a.attributeHandles[i]=0x10000u+i;
        }
        if(mode==Mode::WrongPlanParent)a.parentHandle=0xB500u;
        if(mode==Mode::WrongPlanBlock)a.blockRecordHandle=0xD02u;
        if(mode==Mode::DuplicatePlanAttr)a.attributeHandles[1]=a.attributeHandles[0];
        if(mode==Mode::PlanOwnerCollision)a.attributeHandles[0]=kLine;
        if(mode==Mode::PlanSeqCollision)a.seqendHandle=a.attributeHandles[0];
        if(mode==Mode::ZeroPlanAttr)a.attributeHandles[0]=0;
        if(mode==Mode::ZeroPlanSeq)a.seqendHandle=0;
        if(mode==Mode::ReservedPlanAttr)a.attributeHandles[0]=0x12;
        if(mode==Mode::ReservedPlanSeq)a.seqendHandle=0x12;
        return {a,b};
    }
    static std::array<std::uint32_t,13> identities() {
        return {{kLine,kFirst,kCircle,kSecond,kLast,kRecord,kBlock,kEnd,kAttr1,kAttr2,kSeq1,kAttr3,kSeq2}};
    }
    void writeDwgClasses() override {
        if(mode==Mode::EarlyNodAlias)(void)output.registerDwgNamedObjectDictionaryEntry("ILLEGAL",identities()[aliasSlot]);
        if(mode==Mode::EarlyGroupAlias)(void)output.registerDwgGroupDictionaryEntry("ILLEGAL",identities()[aliasSlot]);
        if(mode==Mode::GroupMember)groupRegistered=output.registerDwgGroupDictionaryEntry("SAFE_GROUP",0xF20u);
        if(mode==Mode::DefaultDictionaryAlias){DRW_DictionaryWithDefault d;d.handle=0xF21u;(void)output.registerDictionaryWithDefaultObjectClass(&d);}
    }
    void writeLTypes() override {
        if(mode==Mode::LateNodAlias)rejectedAlias=!output.registerDwgNamedObjectDictionaryEntry("ILLEGAL",identities()[aliasSlot]);
        if(mode==Mode::LateGroupAlias)rejectedAlias=!output.registerDwgGroupDictionaryEntry("ILLEGAL",identities()[aliasSlot]);
    }
    void writeObjects() override {
        if(mode==Mode::DictionaryAlias){DRW_Dictionary d;d.handle=0xF22u;d.parentHandle=DRW::DwgNamedObjectsDictionaryHandle;
            d.m_entries.push_back({"ILLEGAL",identities()[aliasSlot]});rejectedAlias=!output.writeDictionary(&d);}
        if(mode==Mode::DefaultDictionaryAlias){DRW_DictionaryWithDefault d;d.handle=0xF21u;d.parentHandle=DRW::DwgNamedObjectsDictionaryHandle;
            d.m_entries.push_back({"ILLEGAL",identities()[aliasSlot]});d.m_defaultEntryHandle=identities()[aliasSlot];
            rejectedAlias=!output.writeDictionaryWithDefault(&d);}
        if(mode==Mode::GroupMember){DRW_Group g;g.handle=0xF20u;g.parentHandle=DRW::DwgAcadGroupDictionaryHandle;
            g.name="SAFE_GROUP";g.m_entityHandles={kFirst};groupWritten=output.writeGroup(&g);}
    }
    void writeBlocks() override {
        blockResult=0;
        if(mode==Mode::NoBlock)return;
        DRW_Coord base{};if(mode==Mode::WrongBlockBase)base.x=1.;
        const std::string name=mode==Mode::WrongBlockName?"WRONG_BLOCK":"EMPTY_CHAIN";
        const int units=mode==Mode::WrongBlockUnits?1:0;
        blockResult=output.defineBlock(name,base,units);
        if(mode==Mode::ExtraBlock)(void)output.defineBlock("OTHER",DRW_Coord{},0);
        if(mode==Mode::BlockContent && blockResult!=0) {
            (void)output.beginBlockContent(blockResult);
            DRW_Line line;line.basePoint={0,0,0};line.secPoint={1,0,0};
            (void)output.writeLine(&line);(void)output.endBlockContent();
        }
    }
    static std::shared_ptr<DRW_Attrib> attribute(std::uint32_t handle,
        const char* tag,const char* text,DRW_Coord point,double height,double angle) {
        auto value=std::make_shared<DRW_Attrib>();value->handle=handle;
        value->tag=tag;value->text=text;value->basePoint=point;value->secPoint=point;
        value->height=height;value->angle=angle;return value;
    }
    void writeEntities() override {
        firstReceipt={};secondReceipt={};firstWrite=false;secondWrite=false;mutationPreserved=true;
        if(mode==Mode::XrecordOwnerAlias){DRW_XRecord x;x.handle=0xF23u;x.parentHandle=identities()[aliasSlot];rejectedAlias=!output.writeXRecord(&x);}
        if(mode==Mode::ImageOwnerAlias){DRW_ImageDef image;image.handle=0xF24u;image.parentHandle=identities()[aliasSlot];
            image.name="local-image.png";rejectedAlias=!output.writeImageDef(&image);}
        DRW_Line line;line.handle=kLine;line.basePoint={1,2,3};line.secPoint={4,5,6};
        DRW_Circle circle;circle.handle=kCircle;circle.basePoint={7,8,0};circle.radious=2.5;
        DRW_Line last;last.handle=kLast;last.basePoint={11,12,0};last.secPoint={13,14,0};
        DRW_Insert first,second;first.handle=kFirst;second.handle=kSecond;
        first.name=second.name="EMPTY_CHAIN";first.basePoint={3,4,0};second.basePoint={9,10,0};
        first.attlist={attribute(kAttr1,"FIRST","ONE",{3,4,0},.5,15),
                       attribute(kAttr2,"SECOND","TWO",{4,5,0},.75,25)};
        second.attlist={attribute(kAttr3,"SINGLE","THREE",{9,10,0},.6,0)};
        if(mode==Mode::MatchingReferences) {first.blockRecH.ref=kRecord;first.seqendH.ref=kSeq1;}
        if(mode==Mode::WrongBlockReference)first.blockRecH.ref=0xD02u;
        if(mode==Mode::MInsert)first.colcount=2;
        if(mode==Mode::WrongSeqend)first.seqendH.ref=0xA51u;
        if(mode==Mode::RetainedFooter)first.seqEndEntity=std::make_shared<DRW_SeqEnd>();
        if(mode==Mode::EmptyAttributes)first.attlist.clear();
        if(mode==Mode::WrongAttribute)first.attlist[0]->handle=0xA11u;
        if(mode==Mode::DuplicateAttribute)first.attlist[1]->handle=first.attlist[0]->handle;
        if(mode==Mode::ExtraAttribute)first.attlist.push_back(attribute(0xA30u,"EXTRA","EXTRA",{0,0,0},1,0));
        if(mode==Mode::NullAttribute)first.attlist[0].reset();
        if(mode==Mode::WrongAttrOwner)first.attlist[0]->parentHandle=0xB200u;
        if(mode==Mode::WrongAttrSpace)first.attlist[0]->space=DRW::PaperSpace;
        if(mode==Mode::Paper)first.space=DRW::PaperSpace;
        if(mode==Mode::OtherOwner)first.parentHandle=0xB200u;
        if(mode==Mode::ScaleUniform)first.xscale=first.yscale=first.zscale=2.;
        if(mode==Mode::ScaleXUnit){first.yscale=2.;first.zscale=3.;}
        if(mode==Mode::ScaleNonuniform){first.xscale=2.;first.yscale=3.;first.zscale=4.;}
        if(mode==Mode::ScaleFractional){first.xscale=.5;first.yscale=1.25;first.zscale=2.75;}
        if(mode==Mode::ScaleNearUnit){first.xscale=std::nextafter(1.,2.);first.yscale=1.;first.zscale=std::nextafter(1.,0.);}
        if(mode==Mode::BadXScale)first.xscale=0.;
        if(mode==Mode::BadYScale)first.yscale=-1.;
        if(mode==Mode::BadZScale)first.zscale=std::numeric_limits<double>::infinity();
        if(mode==Mode::BadScaleNan)first.xscale=std::numeric_limits<double>::quiet_NaN();
        if(mode==Mode::BadExtrusion)first.extPoint={1.,0.,0.};
        if(mode==Mode::ZeroExtrusion)first.extPoint={0.,0.,0.};
        if(mode==Mode::NanExtrusion)first.extPoint={0.,std::numeric_limits<double>::quiet_NaN(),0.};
        if(mode==Mode::BadPosition)first.basePoint.x=std::numeric_limits<double>::quiet_NaN();
        if(mode==Mode::Rotated)first.angle=0.5235987755982988;
        if(mode==Mode::BadRotation)first.angle=std::numeric_limits<double>::infinity();
        if(mode==Mode::BadRowSpacing)first.rowspace=.5;
        if(mode==Mode::BadColSpacing)first.colspace=.5;
        if(mode==Mode::BadStyle)first.attlist[0]->style="OTHER_STYLE";
        if(mode==Mode::BadMText)first.attlist[0]->mtext=std::make_unique<DRW_MText>();
        if(mode==Mode::BadAttVersion)first.attlist[0]->attVersion=1;
        if(mode==Mode::BadAttributeType)first.attlist[0]->m_attributeType=2;
        if(mode==Mode::BadLock)first.attlist[0]->lockPosition=true;
        if(mode==Mode::BadXRecord)first.attlist[0]->hasDxfXrecord=true;
        if(mode==Mode::BadAnnotative)first.attlist[0]->m_r2018OuterAnnotativeData={1};
        if(mode==Mode::BadAlignmentZ)first.attlist[0]->secPoint.z=1.;
        if(mode==Mode::BadAttrExtrusion)first.attlist[0]->extPoint={0.,1.,0.};
        if(mode==Mode::BadAttrAlignH)first.attlist[0]->alignH=DRW_Text::HCenter;
        if(mode==Mode::BadAttrAlignV)first.attlist[0]->alignV=DRW_Text::VMiddle;
        if(mode==Mode::BadAttrAlignX)first.attlist[0]->secPoint.x+=1.;
        if(mode==Mode::BadAttrAlignY)first.attlist[0]->secPoint.y+=1.;
        if(mode==Mode::BadAttrWidthScale)first.attlist[0]->widthscale=2.;
        if(mode==Mode::BadAttrOblique)first.attlist[0]->oblique=5.;
        if(mode==Mode::BadAttrThickness)first.attlist[0]->thickness=.3;
        if(mode==Mode::BadAttrTextgen)first.attlist[0]->textgen=2;
        if(mode==Mode::BadAttrFlags)first.attlist[0]->attribFlags=1;
        if(mode==Mode::BadAttrFieldLength)first.attlist[0]->m_fieldLength=1;
        if(mode==Mode::BadAttrZ){first.attlist[0]->basePoint.z=1.;first.attlist[0]->secPoint.z=1.;}
        if(mode==Mode::BadUnicodeText)first.attlist[0]->text="caf\xC3\xA9";
        if(mode==Mode::BadUnicodeTag)first.attlist[0]->tag="caf\xC3\xA9";
        if(mode==Mode::DuplicateTag)first.attlist[1]->tag="first";
        if(mode==Mode::RetainedAttrHandles) {dwgHandle retained;retained.code=4;retained.ref=kAttr1;first.attribHandles.push_back(retained);}
        const bool seedRaw=mode==Mode::InjectedSeqend||mode==Mode::InjectedException||mode==Mode::InjectedLateException;
        if(seedRaw) {DrwEntityEncodeTestAccess::seedRaw(first);for(auto& a:first.attlist)if(a)DrwEntityEncodeTestAccess::seedRaw(*a);}
        const auto oldSeq=first.seqendH;const auto oldBlock=first.blockRecH;
        const auto oldAttribs=first.attribHandles;
        std::vector<std::array<std::uint32_t,3>> initial;
        for(const auto& a:first.attlist)if(a)initial.push_back({a->handle,a->parentHandle,static_cast<std::uint32_t>(a->space)});
        auto emitFirst=[&] {
            if(mode==Mode::InjectedSeqend)DwgCompoundWriteTestAccess::failNextSeqEnd(output);
            if(mode==Mode::InjectedException)drwThrowNextLegacyInsertForTest();
            if(mode==Mode::InjectedLateException)drwThrowNextLegacyInsertLateForTest();
            try {firstWrite=output.writeInsert(&first);}
            catch(const std::bad_alloc&) {
                mutationPreserved=first.handle==kFirst && first.seqendH.ref==oldSeq.ref
                    &&first.blockRecH.ref==oldBlock.ref&&first.attribHandles.size()==oldAttribs.size();
                if(seedRaw) {mutationPreserved &= DrwEntityEncodeTestAccess::rawPreserved(first);
                    for(const auto& a:first.attlist)if(a)mutationPreserved &= DrwEntityEncodeTestAccess::rawPreserved(*a);}
                std::size_t n=0;for(const auto& a:first.attlist)if(a){
                    mutationPreserved &= n<initial.size()&&a->handle==initial[n][0]
                        &&a->parentHandle==initial[n][1]&&static_cast<std::uint32_t>(a->space)==initial[n][2];++n;}
                DRW::DwgCompoundEntityWriteReceipt noReceipt;
                mutationPreserved &= !output.getLastDwgCompoundEntityWriteReceipt(noReceipt)
                    &&output.getLastSuccessfulEntityHandle()==0u;
                exceptionObserved=true;throw;
            }
            if(firstWrite)(void)output.getLastDwgCompoundEntityWriteReceipt(firstReceipt);
            else {
                mutationPreserved=first.handle==kFirst && first.seqendH.ref==oldSeq.ref
                    && first.blockRecH.ref==oldBlock.ref&&first.attribHandles.size()==oldAttribs.size();
                if(seedRaw) {mutationPreserved &= DrwEntityEncodeTestAccess::rawPreserved(first);
                    for(const auto& a:first.attlist)if(a)mutationPreserved &= DrwEntityEncodeTestAccess::rawPreserved(*a);}
                for(std::size_t i=0;i<oldAttribs.size()&&i<first.attribHandles.size();++i)
                    mutationPreserved &= first.attribHandles[i].code==oldAttribs[i].code
                        &&first.attribHandles[i].size==oldAttribs[i].size
                        &&first.attribHandles[i].ref==oldAttribs[i].ref;
                std::size_t n=0;for(const auto& a:first.attlist)if(a) {
                    mutationPreserved &= n<initial.size()&&a->handle==initial[n][0]
                        &&a->parentHandle==initial[n][1]&&static_cast<std::uint32_t>(a->space)==initial[n][2];++n;
                }
            }
        };
        if(mode==Mode::WrongOrder){emitFirst();(void)output.writeLine(&line);}
        else {(void)output.writeLine(&line);emitFirst();}
        (void)output.writeCircle(&circle);
        secondWrite=output.writeInsert(&second);
        if(secondWrite)(void)output.getLastDwgCompoundEntityWriteReceipt(secondReceipt);
        (void)output.writeLine(&last);
        if(mode==Mode::ExtraInsert){DRW_Insert extra;extra.handle=0xB220u;extra.name="EMPTY_CHAIN";(void)output.writeInsert(&extra);}
    }
    static std::vector<std::uint64_t> links(const DRW::DwgObjectFrameReceipt& frame) {
        std::vector<std::uint64_t> result;
        for(const auto& item:frame.occurrences)
            if(item.stream==DRW::DwgObjectHandleStream::Handles
               && item.token.code==DRW::DwgSoftPointer)result.push_back(item.token.reference);
        return result;
    }
    static bool verify(const Receipt& r,std::uint32_t parent,std::uint32_t prev,std::uint32_t next,
                       const std::vector<std::uint32_t>& attrs,std::uint32_t seq) {
        if(!r.valid||r.parentHandle!=parent||r.frames.size()!=attrs.size()+2u
           ||r.frames.front().role!=DRW::DwgCompoundEntityFrameRole::Parent
           ||r.frames.back().role!=DRW::DwgCompoundEntityFrameRole::SeqEnd)return false;
        const auto& p=r.frames.front().frame;const auto pl=links(p);
        if(!p.valid||p.objectHandle!=parent||pl.size()<4u||pl[0]!=prev||pl[1]!=next
           ||pl[pl.size()-2]!=attrs.front()||pl.back()!=attrs.back())return false;
        const auto count=[&p](std::uint8_t code,std::uint32_t target) {
            return std::count_if(p.occurrences.begin(),p.occurrences.end(),[=](const auto& o){
                return o.stream==DRW::DwgObjectHandleStream::Handles&&o.token.code==code&&o.token.reference==target;});};
        if(count(DRW::DwgHardPointer,kRecord)!=1||count(DRW::DwgHardOwnership,seq)!=1)return false;
        for(std::size_t i=0;i<attrs.size();++i){
            const auto& f=r.frames[i+1];const auto l=links(f.frame);
            if(f.role!=DRW::DwgCompoundEntityFrameRole::Attribute||!f.frame.valid||f.frame.objectHandle!=attrs[i]
               ||l.size()<3u||l[0]!=parent||l[1]!=(i?attrs[i-1]:0u)||l[2]!=(i+1<attrs.size()?attrs[i+1]:0u))return false;
        }
        const auto& f=r.frames.back().frame;const auto l=links(f);
        return f.valid&&f.objectHandle==seq&&l.size()>=3u&&l[0]==parent&&l[1]==0u&&l[2]==0u;
    }
    bool receiptValid() const {
        return firstWrite&&secondWrite&&blockResult==kRecord
            &&verify(firstReceipt,kFirst,kLine,kCircle,{kAttr1,kAttr2},kSeq1)
            &&verify(secondReceipt,kSecond,kCircle,kLast,{kAttr3},kSeq2)
            &&firstReceipt.frames[0].frame.generation!=secondReceipt.frames[0].frame.generation;
    }
    dwgRW& output;Mode mode=Mode::Normal;std::uint32_t blockResult=0;
    bool firstWrite=false,secondWrite=false,mutationPreserved=true,exceptionObserved=false;
    bool rejectedAlias=false,groupRegistered=false,groupWritten=false;std::size_t aliasSlot=0;
    Receipt firstReceipt,secondReceipt;
};
struct ScaleReadback : LC3CompatCheck {
    std::vector<std::array<double,3>> values;
    void addInsert(const DRW_Insert& value) override {
        if(value.handle==kFirst)values.push_back({{value.xscale,value.yscale,value.zscale}});
    }
};
}
int main(int argc,char**argv){
    const bool keep=argc==2&&std::string(argv[1])=="--keep";
    if(argc>2||(argc==2&&!keep))return 2;
    const auto dir=std::filesystem::temp_directory_path() /
        ("libdxfrw-legacy-insert-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if(!std::filesystem::create_directory(dir))return 2;
    struct Cleanup{std::filesystem::path p;bool keep;~Cleanup(){if(!keep){std::error_code e;std::filesystem::remove_all(p,e);}}}cleanup{dir,keep};
    const auto path=(dir/"insert.dwg").string();dwgRW output(path.c_str());Adapter adapter(output);
    auto success=[&](Mode mode){adapter.mode=mode;return output.write(&adapter,DRW::AC1015,true)&&adapter.receiptValid();};
    if(!success(Mode::Normal)){std::cerr<<"normal insert/receipts failed\n";return 1;}
    if(keep)std::filesystem::copy_file(path,dir/"two-inserts.dwg");
    if(!success(Mode::Rotated)){std::cerr<<"rotated insert failed\n";return 1;}
    if(keep)std::filesystem::copy_file(path,dir/"rotated-insert.dwg");
    for(const auto& item:{std::make_pair(Mode::ScaleUniform,"scale-uniform.dwg"),
                          std::make_pair(Mode::ScaleXUnit,"scale-xunit.dwg"),
                          std::make_pair(Mode::ScaleNonuniform,"scale-nonuniform.dwg"),
                          std::make_pair(Mode::ScaleFractional,"scale-fractional.dwg"),
                          std::make_pair(Mode::ScaleNearUnit,"scale-nearunit.dwg")}) {
        if(!success(item.first)){std::cerr<<"scaled insert failed\n";return 1;}
        const std::array<double,3> expected = item.first==Mode::ScaleUniform ? std::array<double,3>{{2.,2.,2.}}
            : item.first==Mode::ScaleXUnit ? std::array<double,3>{{1.,2.,3.}}
            : item.first==Mode::ScaleNonuniform ? std::array<double,3>{{2.,3.,4.}}
            : item.first==Mode::ScaleFractional ? std::array<double,3>{{.5,1.25,2.75}}
            : std::array<double,3>{{std::nextafter(1.,2.),1.,std::nextafter(1.,0.)}};
        ScaleReadback readback;dwgR reader(path.c_str());
        if(!reader.read(&readback,false)||readback.values!=std::vector<std::array<double,3>>{expected}) {
            std::cerr<<"scaled INSERT exact typed readback failed\n";return 1;
        }
        if(keep)std::filesystem::copy_file(path,dir/item.second);
    }
    if(!success(Mode::MatchingReferences)){std::cerr<<"matching references failed\n";return 1;}
    adapter.mode=Mode::GroupMember;
    if(!output.write(&adapter,DRW::AC1015,true)||!adapter.receiptValid()
        ||!adapter.groupRegistered||!adapter.groupWritten){std::cerr<<"group membership failed\n";return 1;}
    if(keep)std::filesystem::copy_file(path,dir/"insert-group.dwg");
    for(auto mode:{Mode::NoBlock,Mode::WrongBlockName,Mode::WrongBlockBase,Mode::WrongBlockUnits,
        Mode::ExtraBlock,Mode::BlockContent,Mode::WrongBlockReference,Mode::MInsert,Mode::WrongSeqend,
        Mode::RetainedFooter,Mode::EmptyAttributes,Mode::WrongAttribute,Mode::DuplicateAttribute,
        Mode::ExtraAttribute,Mode::NullAttribute,Mode::WrongAttrOwner,Mode::WrongAttrSpace,
        Mode::WrongOrder,Mode::ExtraInsert,Mode::Paper,Mode::OtherOwner,Mode::MissingBlockPlan,
        Mode::DuplicateModel,Mode::WrongPlanParent,Mode::WrongPlanBlock,Mode::DuplicatePlanAttr,
        Mode::PlanOwnerCollision,Mode::PlanSeqCollision,Mode::ZeroPlanAttr,Mode::ZeroPlanSeq,
        Mode::ReservedPlanAttr,Mode::ReservedPlanSeq,Mode::InjectedSeqend,Mode::InjectedException,
        Mode::InjectedLateException,Mode::BadXScale,Mode::BadYScale,Mode::BadZScale,Mode::BadScaleNan,
        Mode::BadExtrusion,Mode::ZeroExtrusion,Mode::NanExtrusion,Mode::BadPosition,Mode::BadRotation,Mode::BadRowSpacing,Mode::BadColSpacing,
        Mode::BadStyle,Mode::BadMText,Mode::BadAttVersion,Mode::BadAttributeType,Mode::BadLock,
        Mode::BadXRecord,Mode::BadAnnotative,Mode::BadAlignmentZ,Mode::BadUnicodeText,Mode::BadUnicodeTag,
        Mode::DuplicateTag,Mode::RetainedAttrHandles,Mode::OverBudgetPlan,
        Mode::EarlyNodAlias,Mode::EarlyGroupAlias,Mode::LateNodAlias,Mode::LateGroupAlias,
        Mode::DictionaryAlias,Mode::DefaultDictionaryAlias,Mode::XrecordOwnerAlias,Mode::ImageOwnerAlias,
        Mode::BadAttrExtrusion,Mode::BadAttrAlignH,
        Mode::BadAttrAlignV,Mode::BadAttrAlignX,Mode::BadAttrAlignY,Mode::BadAttrWidthScale,
        Mode::BadAttrOblique,Mode::BadAttrThickness,Mode::BadAttrTextgen,Mode::BadAttrFlags,
        Mode::BadAttrFieldLength,Mode::BadAttrZ}) {
        const bool alias=mode==Mode::EarlyNodAlias||mode==Mode::EarlyGroupAlias||mode==Mode::LateNodAlias
            ||mode==Mode::LateGroupAlias||mode==Mode::DictionaryAlias||mode==Mode::DefaultDictionaryAlias
            ||mode==Mode::XrecordOwnerAlias||mode==Mode::ImageOwnerAlias;
        for(std::size_t slot=0;slot<(alias?Adapter::identities().size():1u);++slot){
            {std::ofstream f(path,std::ios::binary);f<<"prior destination";}
            adapter.mode=mode;adapter.aliasSlot=slot;adapter.exceptionObserved=false;adapter.rejectedAlias=false;
            if(output.write(&adapter,DRW::AC1015,true)){std::cerr<<"accepted invalid mode="<<int(mode)<<" slot="<<slot<<"\n";return 1;}
            {std::ifstream f(path,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(f),{})!="prior destination")return 1;}
            if(alias&&mode!=Mode::EarlyNodAlias&&mode!=Mode::EarlyGroupAlias&&!adapter.rejectedAlias){std::cerr<<"alias not rejected mode="<<int(mode)<<" slot="<<slot<<"\n";return 1;}
            if((mode==Mode::InjectedException||mode==Mode::InjectedLateException)&&!adapter.exceptionObserved){std::cerr<<"exception hook did not run\n";return 1;}
            if(!adapter.mutationPreserved){std::cerr<<"caller mutation leaked mode="<<int(mode)<<"\n";return 1;}
            if(!success(Mode::Normal)){std::cerr<<"retry failed mode="<<int(mode)<<"\n";return 1;}
        }
    }
    if(keep)std::cout<<"OUTPUT "<<dir.string()<<'\n';
    std::cout<<"AC1015 bounded attributed INSERT chain PASS\n";return 0;
}
