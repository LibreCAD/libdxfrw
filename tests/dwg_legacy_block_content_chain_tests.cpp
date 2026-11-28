// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <vector>

void drwThrowNextLegacyBlockEntityForTest();
void drwThrowNextLegacyBlockEntityLateForTest();
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
namespace {
constexpr std::uint32_t kRecord=0xD00, kBlock=0xD10, kEnd=0xD01;
constexpr std::uint32_t kLine=0x901, kCircle=0x550, kArc=0xA22;
constexpr std::uint32_t kModelLine=0x130, kInsert=0x451, kModelCircle=0x999;
constexpr std::uint32_t kAttribute=0xA10, kSeqend=0xA50;
enum class Mode { Normal, Singleton, Scaled, Tilted, NegativeZ, MatchingOwner, NoScope, NoEnd, WrongScope, DuplicateScope,
    Missing, Extra, WrongOrder, WrongType, WrongOwner, WrongSpace, DuplicateHandle,
    ModelCollision, BlockCollision, AttributeCollision, Zero, Reserved, EarlyModel, Exception, LateException,
    EarlyNodAlias, EarlyGroupAlias, LateNodAlias, LateGroupAlias, DictionaryAlias, XrecordOwnerAlias };
struct Adapter : LC3CompatCheck, DRW_DwgLegacyModelChainProvider,
                 DRW_DwgLegacyInsertChainProvider, DRW_DwgLegacyBlockContentProvider,
                 DRW_DwgDefaultPlotStyleProvider {
    explicit Adapter(dwgRW& value):output(value) {}
    std::vector<DRW_DwgLegacyModelChainEntry> dwgLegacyModelChain() const override {
        return {{kModelLine,DRW::LINE},{kInsert,DRW::INSERT},{kModelCircle,DRW::CIRCLE}};
    }
    DRW_DwgLegacyEmptyBlock dwgLegacyEmptyBlock() const override {
        DRW_DwgLegacyEmptyBlock block;block.name="CONTENT_CHAIN";
        block.blockRecordHandle=kRecord;block.blockHandle=kBlock;block.endBlockHandle=kEnd;return block;
    }
    std::vector<DRW_DwgLegacyInsertChain> dwgLegacyInsertChains() const override {
        DRW_DwgLegacyInsertChain chain;chain.parentHandle=kInsert;chain.blockRecordHandle=kRecord;
        chain.attributeHandles={kAttribute};chain.seqendHandle=kSeqend;return {chain};
    }
    std::uint32_t aliasHandle() const {return std::array<std::uint32_t,3>{{kLine,kCircle,kArc}}[aliasSlot];}
    void writeDwgClasses() override {
        if(mode==Mode::EarlyNodAlias)(void)output.registerDwgNamedObjectDictionaryEntry("ILLEGAL",aliasHandle());
        if(mode==Mode::EarlyGroupAlias)(void)output.registerDwgGroupDictionaryEntry("ILLEGAL",aliasHandle());
    }
    void writeLTypes() override {
        if(mode==Mode::LateNodAlias)rejectedAlias=!output.registerDwgNamedObjectDictionaryEntry("ILLEGAL",aliasHandle());
        if(mode==Mode::LateGroupAlias)rejectedAlias=!output.registerDwgGroupDictionaryEntry("ILLEGAL",aliasHandle());
    }
    void writeObjects() override {
        if(mode==Mode::DictionaryAlias){DRW_Dictionary d;d.handle=0xF22;d.parentHandle=DRW::DwgNamedObjectsDictionaryHandle;
            d.m_entries.push_back({"ILLEGAL",aliasHandle()});rejectedAlias=!output.writeDictionary(&d);}
    }
    std::vector<DRW_DwgLegacyModelChainEntry> dwgLegacyBlockContent() const override {
        std::vector<DRW_DwgLegacyModelChainEntry> plan{{kLine,DRW::LINE},{kCircle,DRW::CIRCLE},{kArc,DRW::ARC}};
        if(mode==Mode::Singleton)plan.resize(1);
        if(mode==Mode::DuplicateHandle)plan[1].handle=kLine;
        if(mode==Mode::ModelCollision)plan[1].handle=kModelCircle;
        if(mode==Mode::BlockCollision)plan[1].handle=kBlock;
        if(mode==Mode::AttributeCollision)plan[1].handle=kAttribute;
        if(mode==Mode::Zero)plan[1].handle=0;
        if(mode==Mode::Reserved)plan[1].handle=0x12;
        return plan;
    }
    void capture(bool ok) {
        accepted.push_back(ok);
        if(ok) {DRW::DwgObjectFrameReceipt frame;if(output.getLastDwgObjectFrame(frame))frames.push_back(frame);}
    }
    void writeBlocks() override {
        accepted.clear();frames.clear();
        const auto block=output.defineBlock("CONTENT_CHAIN",DRW_Coord{},0);
        if(mode==Mode::EarlyModel) {DRW_Line x;x.handle=kModelLine;(void)output.writeLine(&x);}
        if(mode!=Mode::NoScope)(void)output.beginBlockContent(mode==Mode::WrongScope?kRecord+1:block);
        if(mode==Mode::DuplicateScope)(void)output.beginBlockContent(block);
        DRW_Line line;line.handle=kLine;line.basePoint={1,2,3};line.secPoint={4,5,6};
        DRW_Circle circle;circle.handle=kCircle;circle.basePoint={7,8,9};circle.radious=2.5;
        DRW_Arc arc;arc.handle=kArc;arc.basePoint={21,22,23};arc.radious=4.5;
        arc.staangle=0.3490658503988659;arc.endangle=1.9198621771937625;
        if(mode==Mode::MatchingOwner)line.parentHandle=circle.parentHandle=arc.parentHandle=kRecord;
        if(mode==Mode::WrongOwner)line.parentHandle=kRecord+1;
        if(mode==Mode::WrongSpace)line.space=DRW::PaperSpace;
        const bool injected=mode==Mode::Exception||mode==Mode::LateException;
        exceptionObserved=false;mutationPreserved=true;
        if(injected)DrwEntityEncodeTestAccess::seedRaw(line);
        if(mode==Mode::Exception)drwThrowNextLegacyBlockEntityForTest();
        if(mode==Mode::LateException)drwThrowNextLegacyBlockEntityLateForTest();
        if(mode==Mode::WrongOrder)capture(output.writeCircle(&circle));
        if(mode==Mode::WrongType) {DRW_Circle wrong;wrong.handle=kLine;capture(output.writeCircle(&wrong));}
        else try {capture(output.writeLine(&line));}
        catch(const std::bad_alloc&) {
            exceptionObserved=true;
            mutationPreserved=line.handle==kLine&&line.parentHandle==DRW::NoHandle
                &&line.space==DRW::ModelSpace&&DrwEntityEncodeTestAccess::rawPreserved(line);
            DRW::DwgObjectFrameReceipt noFrame;
            mutationPreserved &= !output.getLastDwgObjectFrame(noFrame)
                &&output.getLastSuccessfulEntityHandle()==0u;
        }
        if(mode!=Mode::Singleton)capture(output.writeCircle(&circle));
        if(mode!=Mode::Missing&&mode!=Mode::Singleton)capture(output.writeArc(&arc));
        if(mode==Mode::Extra){DRW_Line extra;extra.handle=0xB220;capture(output.writeLine(&extra));}
        if(mode!=Mode::NoEnd)(void)output.endBlockContent();
    }
    void writeEntities() override {
        if(mode==Mode::XrecordOwnerAlias){DRW_XRecord x;x.handle=0xF23;x.parentHandle=aliasHandle();
            rejectedAlias=!output.writeXRecord(&x);}
        DRW_Line line;line.handle=kModelLine;line.basePoint={11,12,13};line.secPoint={14,15,16};
        DRW_Insert insert;insert.handle=kInsert;insert.name="CONTENT_CHAIN";insert.basePoint={3,4,0};
        if(mode==Mode::Scaled||mode==Mode::Tilted||mode==Mode::NegativeZ){insert.xscale=2.;insert.yscale=3.;insert.zscale=4.;insert.angle=0.5235987755982988;}
        if(mode==Mode::Tilted)insert.extPoint={0.,1.,0.};
        if(mode==Mode::NegativeZ)insert.extPoint={0.,0.,-1.};
        auto attr=std::make_shared<DRW_Attrib>();attr->handle=kAttribute;attr->tag="FIRST";attr->text="ONE";
        attr->basePoint={3,4,0};attr->secPoint=attr->basePoint;attr->height=.5;insert.attlist={attr};
        DRW_Circle circle;circle.handle=kModelCircle;circle.basePoint={17,18,0};circle.radious=3.5;
        (void)output.writeLine(&line);(void)output.writeInsert(&insert);
        (void)output.getLastDwgCompoundEntityWriteReceipt(insertReceipt);(void)output.writeCircle(&circle);
    }
    bool valid() const {
        const bool singleton=mode==Mode::Singleton;
        if(accepted!=std::vector<bool>(singleton?1u:3u,true)||frames.size()!=(singleton?1u:3u)||!insertReceipt.valid)return false;
        const std::array<std::uint32_t,3> handles{{kLine,kCircle,kArc}};
        for(std::size_t i=0;i<frames.size();++i) {
            const auto& frame=frames[i];if(!frame.valid||frame.objectHandle!=handles[i])return false;
            std::vector<std::uint64_t> links;
            for(const auto& item:frame.occurrences)
                if(item.stream==DRW::DwgObjectHandleStream::Handles&&item.token.code==DRW::DwgSoftPointer)
                    links.push_back(item.token.reference);
            if(links!=std::vector<std::uint64_t>{kRecord,i?handles[i-1]:0u,i+1<frames.size()?handles[i+1]:0u})return false;
        }
        return (singleton||frames[0].generation!=frames[1].generation)&&insertReceipt.parentHandle==kInsert;
    }
    dwgRW& output;Mode mode=Mode::Normal;std::vector<bool> accepted;
    std::vector<DRW::DwgObjectFrameReceipt> frames;DRW::DwgCompoundEntityWriteReceipt insertReceipt;
    bool exceptionObserved=false,mutationPreserved=true,rejectedAlias=false;std::size_t aliasSlot=0;
};
}
int main(int argc,char** argv) {
    const bool keep=argc==2&&std::string(argv[1])=="--keep";
    if(argc>2||(argc==2&&!keep))return 2;
    const auto dir=std::filesystem::temp_directory_path()/
        ("libdxfrw-legacy-block-content-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if(!std::filesystem::create_directory(dir))return 2;
    struct Cleanup{std::filesystem::path p;bool keep;~Cleanup(){if(!keep){std::error_code ec;std::filesystem::remove_all(p,ec);}}} cleanup{dir,keep};
    const auto path=(dir/"content.dwg").string();dwgRW output(path.c_str());Adapter adapter(output);
    auto success=[&](Mode mode){adapter.mode=mode;return output.write(&adapter,DRW::AC1015,true)&&adapter.valid();};
    if(!success(Mode::Normal)){std::cerr<<"normal contents or receipts failed\n";return 1;}
    if(keep)std::filesystem::copy_file(path,dir/"block-contents.dwg");
    if(!success(Mode::Singleton)){std::cerr<<"singleton contents failed\n";return 1;}
    if(keep)std::filesystem::copy_file(path,dir/"singleton-block.dwg");
    if(!success(Mode::Scaled)){std::cerr<<"scaled block contents failed\n";return 1;}
    if(keep)std::filesystem::copy_file(path,dir/"scaled-block.dwg");
    if(!success(Mode::Tilted)){std::cerr<<"tilted block contents failed\n";return 1;}
    if(keep)std::filesystem::copy_file(path,dir/"tilted-block.dwg");
    if(!success(Mode::NegativeZ)){std::cerr<<"negative-Z block contents failed\n";return 1;}
    if(keep)std::filesystem::copy_file(path,dir/"negativez-block.dwg");
    if(!success(Mode::MatchingOwner)){std::cerr<<"matching owner failed\n";return 1;}
    for(auto mode:{Mode::NoScope,Mode::NoEnd,Mode::WrongScope,Mode::DuplicateScope,Mode::Missing,
                   Mode::Extra,Mode::WrongOrder,Mode::WrongType,Mode::WrongOwner,Mode::WrongSpace,
                   Mode::DuplicateHandle,Mode::ModelCollision,Mode::BlockCollision,Mode::AttributeCollision,
                   Mode::Zero,Mode::Reserved,Mode::EarlyModel,Mode::Exception,Mode::LateException,
                   Mode::EarlyNodAlias,Mode::EarlyGroupAlias,Mode::LateNodAlias,Mode::LateGroupAlias,
                   Mode::DictionaryAlias,Mode::XrecordOwnerAlias}) {
      const bool alias=mode==Mode::EarlyNodAlias||mode==Mode::EarlyGroupAlias||mode==Mode::LateNodAlias
          ||mode==Mode::LateGroupAlias||mode==Mode::DictionaryAlias||mode==Mode::XrecordOwnerAlias;
      for(std::size_t slot=0;slot<(alias?3u:1u);++slot) {
        {std::ofstream f(path,std::ios::binary);f<<"prior destination";}
        adapter.mode=mode;adapter.aliasSlot=slot;adapter.rejectedAlias=false;
        if(output.write(&adapter,DRW::AC1015,true)){std::cerr<<"invalid mode accepted "<<int(mode)<<" slot "<<slot<<'\n';return 1;}
        {std::ifstream f(path,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(f),{})!="prior destination")return 1;}
        if(alias&&mode!=Mode::EarlyNodAlias&&mode!=Mode::EarlyGroupAlias&&!adapter.rejectedAlias) {
            std::cerr<<"alias accepted "<<int(mode)<<" slot "<<slot<<'\n';return 1;
        }
        if((mode==Mode::Exception||mode==Mode::LateException)&&(!adapter.exceptionObserved||!adapter.mutationPreserved)) {
            std::cerr<<"exception rollback failed "<<int(mode)<<'\n';return 1;
        }
        if(!success(Mode::Normal)){std::cerr<<"retry failed mode "<<int(mode)<<'\n';return 1;}
      }
    }
    if(keep)std::cout<<"OUTPUT "<<dir.string()<<'\n';
    std::cout<<"AC1015 named-block content chain PASS\n";return 0;
}
