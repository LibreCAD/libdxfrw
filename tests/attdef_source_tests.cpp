// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_iface.h"
#include "dx_standard_output.h"
#include "intern/dxfattdef.h"
#include "intern/dxfblockchildren.h"
#include "intern/dxfblockdelivery.h"
#include "intern/dxfwriter.h"
#include "attdef_source_field_faults.h"
#include "test_scratch.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <new>
#include <sstream>
#include <stdexcept>

namespace {long allocationRemaining=-1;}
void* operator new(std::size_t size) {
    if(allocationRemaining==0)throw std::bad_alloc();
    if(allocationRemaining>0)--allocationRemaining;
    if(void* value=std::malloc(size?size:1u))return value;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size){return ::operator new(size);}
void operator delete(void* value) noexcept {std::free(value);}
void operator delete[](void* value) noexcept {std::free(value);}
void operator delete(void* value,std::size_t) noexcept {std::free(value);}
void operator delete[](void* value,std::size_t) noexcept {std::free(value);}

class DxIfaceStandardDwgCaptureTestAccess {
public:
    static bool estimate(const dx_iface& iface,const DRW_Entity& entity,
                         std::size_t inlineBytes,std::size_t& bytes) {
        return iface.estimateTypedEntityRetainedBytes(entity,inlineBytes,bytes);
    }
    static void attribute(dx_iface& iface,const DRW_Attrib& value) {
        (void)iface.retainTypedEntity<DRW_Attrib>(value);
    }
};

class DxIfaceTableSourceTestAccess {
public:
    using BlockReconciliation=dx_data::TableDxfSourceCapture::BlockSource::Reconciliation;
    static const auto* resource(const dx_data& data) {
        const auto& rows=data.m_tableDxfSource.resources;
        const auto found=rows.find(0xA1u);
        return found==rows.end()?nullptr:&found->second;
    }
    static bool failed(const dx_data& data) {
        const auto& s=data.m_tableDxfSource;
        return s.resourceCaptureFailed||s.resourceProofCaptureFailed||s.contextFailed;
    }
    static std::size_t pending(const dx_data& data) {return data.m_tableDxfSource.attdefPending;}
    static const auto* block(const dx_data& data) {
        const auto& rows=data.m_tableDxfSource.blockSources;const auto at=rows.find(0xB1u);
        return at==rows.end()?nullptr:&at->second;
    }
    static bool blockFailed(const dx_data& data) {return data.m_tableDxfSource.blockSourceCaptureFailed;}
    static bool childrenFailed(const dx_data& data) {return data.m_tableDxfSource.blockChildrenCaptureFailed;}
    static bool deliveryFailed(const dx_data& data) {return data.m_tableDxfSource.blockDeliveryCaptureFailed;}
    static bool membershipMatched(const dx_data& data) {return dx_iface::hasMatchedBlockMembership(data,0xB1u);}
    static bool membershipFailed(const dx_data& data) {return data.m_tableDxfSource.blockMembershipCaptureFailed;}
    using CompoundBody=dx_data::TableDxfSourceCapture::CompoundBody;
    static bool compoundMatched(const dx_data& data,std::size_t dispatch) {return dx_iface::hasMatchedCompoundBody(data,0xB1u,dispatch);}
    static bool compoundSourceMatched(const dx_data& data,std::size_t dispatch) {return dx_iface::hasMatchedCompoundSource(data,0xB1u,dispatch);}
    static const auto* compoundBody(const dx_data& data,std::size_t dispatch) {
        const auto* owner=block(data);if(!owner)return static_cast<const CompoundBody*>(nullptr);
        const auto at=owner->compoundBodies.find(dispatch);return at==owner->compoundBodies.end()?nullptr:at->second.get();
    }
    static bool compoundFailed(const dx_data& data) {return data.m_tableDxfSource.compoundBodyCaptureFailed;}
    static const auto& blockOrigins(const dx_data& data){return data.m_tableDxfSource.blockOriginAudits;}
    static const dxfRW* blockReadInput(const dx_data& data){return data.m_tableDxfSource.entitiesReadInput;}
    static void blockOriginLimit(dx_data& data,bool work,std::size_t value) {
        if(work)data.m_tableDxfSource.maxBlockOriginWork=value;else data.m_tableDxfSource.maxBlockOriginBytes=value;
    }
    static void substituteBlockOrigin(dx_data& data,bool alias) {
        auto& p=data.m_tableDxfSource.blockOriginAudits.at(0xB1u);
        if(alias)p=std::shared_ptr<const dx_data::TableDxfSourceCapture::BlockOriginAudit>(p.get(),[](const auto*){});
        else p=std::make_shared<const dx_data::TableDxfSourceCapture::BlockOriginAudit>(*p);
    }
    static void substituteBlockOriginInput(dx_data& data,unsigned kind,bool alias) {
        auto& row=data.m_tableDxfSource.blockSources.at(0xB1u);
        const auto change=[&](auto& p){using T=typename std::decay_t<decltype(p)>::element_type;
            if(alias)p=std::shared_ptr<T>(p.get(),[](const auto*){});else p=std::make_shared<T>(*p);};
        switch(kind){case 0u:change(row.original);break;case 1u:change(row.effective);break;case 2u:change(row.footer);break;
            case 3u:change(row.beginRaw);break;case 4u:change(row.endRaw);break;case 5u:change(row.children);break;case 6u:change(row.delivery);break;}
    }
    static void transplantBlockOrigin(dx_data& to,const dx_data& from) {
        to.m_tableDxfSource.blockOriginAudits=from.m_tableDxfSource.blockOriginAudits;
    }
    static std::size_t beforeBlockOriginAudit(const dx_data& data) {
        const auto& s=data.m_tableDxfSource;std::size_t final=0u;
        for(const auto& item:s.blockSources)final+=1024u+(item.second.children?item.second.children->bytes+128u*item.second.children->records.size():0u);
        return s.blockOriginWork-final;
    }
    static void repeatBlockOrigins(dx_iface& reader,dx_data& data,std::size_t work) {
        auto& s=data.m_tableDxfSource;s.blockOriginAudits.clear();s.blockOriginOwners.clear();s.blockOriginBytes=0u;s.blockOriginWork=work;s.blockOriginCaptureFailed=false;
        reader.finishDxfBlockOriginAudits(data);
    }
    static void substituteBlockBody(dx_data& data,std::size_t dispatch,bool alias) {
        auto& p=data.m_tableDxfSource.blockSources.at(0xB1u).compoundBodies.at(dispatch);
        if(alias)p=std::shared_ptr<const CompoundBody>(p.get(),[](const auto*){});
        else p=std::make_shared<const CompoundBody>(*p);
    }
    static void substituteBlockContext(dx_data& data,bool alias) {
        auto& p=data.m_tableDxfSource.blockSources.at(0xB1u).context;
        if(alias)p=std::shared_ptr<const dx_data::TableDxfSourceCapture::EntitiesReadContext>(p.get(),[](const auto*){});
        else p=std::make_shared<const dx_data::TableDxfSourceCapture::EntitiesReadContext>(*p);
    }
    static std::array<std::size_t,3> compoundUsage(const dx_data& data) {
        const auto& s=data.m_tableDxfSource;return {s.compoundBodyRecords,s.compoundBodyBytes,s.compoundBodyWork};
    }
    static void compoundLimit(dx_data& data,unsigned which,std::size_t n) {
        auto& s=data.m_tableDxfSource;if(which==0u)s.maxCompoundBodyRecords=n;
        else if(which==1u)s.maxCompoundBodyBytes=n;else s.maxCompoundBodyWork=n;
    }
    static bool recaptureCompound(dx_iface& iface,dx_data& data,std::size_t dispatch,long fault,long& allocations) {
        auto& source=data.m_tableDxfSource;auto& owner=source.blockSources.at(0xB1u);
        const auto saved=owner.compoundBodies.at(dispatch);owner.compoundBodies.erase(dispatch);
        owner.compoundBodyOwners.erase(dispatch);
        source.closed=false;source.successful=false;source.reader=&iface;
        source.compoundBodyRecords=source.compoundBodyBytes=source.compoundBodyWork=0u;source.compoundBodyCaptureFailed=false;
        const auto event=owner.delivery->events[owner.delivery->dispatches[dispatch].firstEvent];
        allocationRemaining=fault;iface.captureDxfCompoundBody(owner,event,saved->model.get());
        allocations=fault-allocationRemaining;allocationRemaining=-1;
        const auto at=owner.compoundBodies.find(dispatch);
        return at!=owner.compoundBodies.end()&&at->second->state==CompoundBody::State::ModeledReplayMatched&&!source.compoundBodyCaptureFailed;
    }
    static void copyMembershipContext(dx_data& target,const dx_data& source) {
        // Copy only evidence/public outcome, not dx_data's owning raw block
        // pointers: this safely isolates a borrowed successful-source context.
        target.m_tableDxfSource=source.m_tableDxfSource;target.importGeneration=source.importGeneration;
        target.sourceVersion=source.sourceVersion;target.sourceFormat=source.sourceFormat;target.dxfReadOutcome=source.dxfReadOutcome;
    }
    static std::array<std::size_t,3> membershipUsage(const dx_data& data) {
        const auto& s=data.m_tableDxfSource;return {s.blockMembershipRecords,s.blockMembershipBytes,s.blockMembershipWork};
    }
    static void membershipLimit(dx_data& data,unsigned which,std::size_t n) {
        auto& s=data.m_tableDxfSource;if(which==0u)s.maxBlockMembershipRecords=n;
        else if(which==1u)s.maxBlockMembershipBytes=n;else s.maxBlockMembershipWork=n;
    }
    static bool recaptureMembership(dx_iface& iface,dx_data& data,long fault,long& allocations) {
        auto& source=data.m_tableDxfSource;auto& owner=source.blockSources.at(0xB1u);
        auto* previousData=iface.m_tableSourceReadData;iface.m_tableSourceReadData=&data;
        source.closed=false;source.successful=false;source.reader=&iface;owner.membership.reset();
        source.blockMembershipRecords=source.blockMembershipBytes=source.blockMembershipWork=0u;source.blockMembershipCaptureFailed=false;
        allocationRemaining=fault;iface.captureDxfBlockMembership(owner);allocations=fault-allocationRemaining;allocationRemaining=-1;
        iface.m_tableSourceReadData=previousData;
        return owner.membership&&owner.membership->complete&&!source.blockMembershipCaptureFailed;
    }
    using OriginState=dx_data::TableDxfSourceCapture::Resource::OriginState;
    using Origin=dx_data::TableDxfSourceCapture::AttdefOrigin;
    static bool originMatched(const dx_iface& iface,const dx_data& data,std::uint32_t handle=0xA1u) {
        return iface.hasMatchedAttdefOrigin(data,handle);
    }
    static bool originFailed(const dx_data& data) {return data.m_tableDxfSource.attdefOriginCaptureFailed;}
    static const auto* originResource(const dx_data& data,std::uint32_t handle=0xA1u) {
        const auto at=data.m_tableDxfSource.resources.find(handle);return at==data.m_tableDxfSource.resources.end()?nullptr:&at->second;
    }
    static auto& mutableOriginResource(dx_data& data,std::uint32_t handle=0xA1u) {return data.m_tableDxfSource.resources.at(handle);}
    static auto& mutableOriginBlock(dx_data& data,std::uint32_t handle=0xB1u) {return data.m_tableDxfSource.blockSources.at(handle);}
    static bool recaptureOrigin(dx_iface& iface,dx_data& data,long fault,long& allocations) {
        auto& source=data.m_tableDxfSource;auto& row=source.resources.at(0xA1u);auto& owner=source.blockSources.at(0xB1u);
        auto* previousData=iface.m_tableSourceReadData;auto* previousBlock=iface.currentBlock;
        iface.m_tableSourceReadData=&data;
        for(auto* block:data.blocks)if(block->handle==0xB1u)iface.currentBlock=block;
        source.closed=false;source.successful=false;source.reader=&iface;source.blockSourceActive=0xB1u;
        owner.closed=false;owner.deliveryFinished=false;owner.deliveryPending=true;owner.deliveryObserved=true;owner.deliveryNext=0u;
        std::vector<std::uint32_t>().swap(owner.attdefOrigins);row.attdefOrigin.reset();row.attdefOriginState=OriginState::NotCaptured;
        source.attdefOriginRecords=source.attdefOriginBytes=source.attdefOriginWork=0u;source.attdefOriginCaptureFailed=false;
        allocationRemaining=fault;iface.captureAttdefOrigin(row);allocations=fault-allocationRemaining;allocationRemaining=-1;
        iface.m_tableSourceReadData=previousData;iface.currentBlock=previousBlock;
        return row.attdefOriginState==OriginState::SourceBound&&!source.attdefOriginCaptureFailed;
    }
    static std::array<std::size_t,3> originUsage(const dx_data& data) {
        const auto& s=data.m_tableDxfSource;return {s.attdefOriginRecords,s.attdefOriginBytes,s.attdefOriginWork};
    }
    static void originLimit(dx_data& data,unsigned which,std::size_t n) {
        auto& s=data.m_tableDxfSource;if(which==0u)s.maxAttdefOriginRecords=n;
        else if(which==1u)s.maxAttdefOriginBytes=n;else s.maxAttdefOriginWork=n;
    }
    static bool deliveryTransport(const dx_data& data) {
        const auto* row=block(data);return childrenTransport(data)&&row->delivery&&row->deliveryExpected
            &&row->deliveryFinished&&!row->deliveryPending&&!deliveryFailed(data)
            &&row->deliveryNext==row->delivery->events.size();
    }
    static std::array<std::size_t,4> deliveryUsage(const dx_data& data) {
        const auto& s=data.m_tableDxfSource;return {s.blockDeliveryDispatches,s.blockDeliveryEvents,s.blockDeliveryBytes,s.blockDeliveryWork};
    }
    static void deliveryLimit(dx_data& data,unsigned which,std::size_t n) {
        auto& s=data.m_tableDxfSource;
        switch(which){case 0:s.maxBlockDeliveryDispatches=n;break;case 1:s.maxBlockDeliveryEvents=n;break;
        case 2:s.maxBlockDeliveryBytes=n;break;case 3:s.maxBlockDeliveryWork=n;break;default:throw std::runtime_error("bad delivery quota");}
    }
    static bool childrenTransport(const dx_data& data) {
        const auto* row=block(data);return blockTransport(data)&&row->children&&!childrenFailed(data)
            &&row->children->status==DRW_DxfBlockChildrenSource::Status::Complete;
    }
    static std::array<std::size_t,4> childrenUsage(const dx_data& data) {
        const auto& s=data.m_tableDxfSource;return {s.blockChildrenRecords,s.blockChildrenBytes,s.blockChildrenGroups,s.blockChildrenWork};
    }
    static void childrenLimit(dx_data& data,unsigned which,std::size_t n) {
        auto& s=data.m_tableDxfSource;
        switch(which){case 0:s.maxBlockChildrenRecords=n;break;case 1:s.maxBlockChildrenBytes=n;break;
        case 2:s.maxBlockChildrenGroups=n;break;case 3:s.maxBlockChildrenWork=n;break;default:throw std::runtime_error("bad children quota");}
    }
    static bool blockTransport(const dx_data& data) {
        const auto* row=block(data);return row&&row->beginPaired&&row->footerPaired&&row->closed
            &&data.m_tableDxfSource.successful&&data.m_tableDxfSource.closed
            &&!data.m_tableDxfSource.contextFailed&&!data.m_tableDxfSource.failed
            &&!blockFailed(data)&&!data.typedEntityCaptureFailed;
    }
    static bool blockEndpointProof(const dx_data& data) {
        const auto* row=block(data);return blockTransport(data)&&row->beginFieldsMatched&&row->footerFieldsMatched
            &&row->beginProfile.complete()&&row->endProfile.complete()
            &&row->reconciliation==decltype(row->reconciliation)::Matched;
    }
    static std::array<std::size_t,4> blockUsage(const dx_data& data) {
        const auto& s=data.m_tableDxfSource;return {s.blockSources.size(),s.blockSourceBytes,s.blockSourceGroups,s.blockSourceWork};
    }
    static void blockLimit(dx_data& data,unsigned which,std::size_t n) {
        auto& s=data.m_tableDxfSource;
        switch(which) {case 0:s.maxBlockSourceRecords=n;break;case 1:s.maxBlockSourceBytes=n;break;
        case 2:s.maxBlockSourceGroups=n;break;case 3:s.maxBlockSourceWork=n;break;default:throw std::runtime_error("bad block quota");}
    }
    static void limits(dx_data& data,unsigned which,std::size_t value) {
        auto& s=data.m_tableDxfSource;
        switch(which) {
        case 0:s.maxResourceRecords=value;break;
        case 1:s.maxResourceItems=value;break;
        case 2:s.maxResourceBytes=value;break;
        case 3:s.maxResourceProofRecords=value;break;
        case 4:s.maxResourceProofGroups=value;break;
        case 5:s.maxResourceProofBytes=value;break;
        case 6:s.maxResourceProofWork=value;break;
        default:throw std::runtime_error("unknown capture limit");
        }
    }
};

namespace {
struct Tag {int code;std::string value;};
using Tags=std::vector<Tag>;
bool check(bool value,const char* label) {
    if(!value)std::cerr<<"ATTDEF source: "<<label<<'\n';
    return value;
}
bool nestedCommonBudgetTests() {
    constexpr std::size_t payload=16384u;
    unsigned cells=0u, boundaries=0u, retries=0u;
    long allocationSchedules=0;
    const auto fill=[](DRW_MText& value,unsigned field) {
        const std::string text(payload,'q');
        switch(field) {
        case 0:value.layer=text;break;
        case 1:value.lineType=text;break;
        case 2:value.proxyGraphics=text;value.numProxyGraph=static_cast<int>(payload);break;
        case 3:value.colorName=text;break;
        case 4:value.dataStorageHandleKey=text;break;
        case 5:value.dataStoragePayloadMarkerSection=text;break;
        case 6:value.dataStorageData.assign(payload,0x5Au);break;
        case 7:value.reactorHandles.assign(payload/sizeof(std::uint32_t),0x12u);break;
        case 8:value.pendingAppIdResolutions.resize(payload/sizeof(DRW_Entity::PendingHandleRef)+1u);break;
        case 9:value.pendingLayerRefResolutions.resize(payload/sizeof(DRW_Entity::PendingHandleRef)+1u);break;
        case 10:value.extData.push_back(std::make_shared<DRW_Variant>(1000,text));break;
        case 11:value.extData.push_back(std::make_shared<DRW_Variant>(1004,std::vector<std::uint8_t>(payload,0xA5u)));break;
        case 12:value.appData.push_back({DRW_Variant(102,std::string("{APP")),DRW_Variant(1,text),DRW_Variant(102,std::string("}"))});break;
        case 13:
            value.numProxyGraph=1;value.proxyGraphics="x";
            if(!value.authorizeProxyGraphicsReplay(DRW::AC1032,text))throw std::runtime_error("authored proxy provenance setup");
            break;
        default:throw std::runtime_error("unknown nested common payload field");
        }
    };
    for(unsigned family=0u;family<4u;++family)for(unsigned field=0u;field<15u;++field) {
        DRW_Attdef definition;DRW_Attrib attribute;DRW_Insert insert;
        std::vector<DRW_MText*> nested;
        DRW_Entity* host=nullptr;std::size_t inlineBytes=0u;
        if(family<2u) {
            DRW_Attrib& value=family==0u?static_cast<DRW_Attrib&>(definition):attribute;
            value.mtext=std::make_unique<DRW_MText>();nested.push_back(value.mtext.get());
            host=&value;inlineBytes=family==0u?sizeof(DRW_Attdef):sizeof(DRW_Attrib);
        } else {
            for(unsigned i=0u;i<(family==2u?1u:2u);++i) {
                auto value=std::make_shared<DRW_Attrib>();value->mtext=std::make_unique<DRW_MText>();
                nested.push_back(value->mtext.get());insert.attlist.push_back(std::move(value));
            }
            host=&insert;inlineBytes=sizeof(DRW_Insert);
        }
        dx_iface estimator;
        std::size_t before=0u,after=0u,expectedGrowth=0u;
        if(!DxIfaceStandardDwgCaptureTestAccess::estimate(estimator,*host,inlineBytes,before))return false;
        for(auto* value:nested) {
            // Slice to LINE to measure only the known common dynamic owners,
            // independently of TEXT/attribute nesting and duplicated text charges.
            DRW_Line commonBefore=static_cast<const DRW_Line&>(*value);
            std::size_t oldCommon=0u,newCommon=0u;
            if(!DxIfaceStandardDwgCaptureTestAccess::estimate(estimator,commonBefore,sizeof(DRW_Line),oldCommon))return false;
            if(field==14u)for(unsigned part=0u;part<14u;++part)fill(*value,part);
            else fill(*value,field);
            DRW_Line commonAfter=static_cast<const DRW_Line&>(*value);
            if(!DxIfaceStandardDwgCaptureTestAccess::estimate(estimator,commonAfter,sizeof(DRW_Line),newCommon)
                ||newCommon<oldCommon||newCommon-oldCommon<payload-32u)return false;
            expectedGrowth+=newCommon-oldCommon;
        }
        if(!check(DxIfaceStandardDwgCaptureTestAccess::estimate(estimator,*host,inlineBytes,after)
            &&after>=before&&after-before==expectedGrowth,
            "nested MTEXT common owner growth must be charged in every attribute container")) {
            std::cerr<<"family="<<family<<" field="<<field<<" before="<<before
                <<" after="<<after<<" expected growth="<<expectedGrowth<<'\n';return false;
        }
        const auto capture=[&](dx_iface& iface) {
            if(family==0u)iface.addAttDef(definition);
            else if(family==1u)DxIfaceStandardDwgCaptureTestAccess::attribute(iface,attribute);
            else iface.addInsert(insert);
        };
        const std::size_t charge=after+dx_iface::TypedEntityListNodeBytes;
        if(charge>=dx_iface::TypedEntityCaptureMaxRetainedBytes)return false;
        for(bool below:{false,true}) {
            dx_data data;dx_ifaceBlock block;dx_iface iface;iface.cData=&data;iface.currentBlock=&block;
            const auto initial=dx_iface::TypedEntityCaptureMaxRetainedBytes-charge+(below?1u:0u);
            data.typedEntityRetainedBytes=initial;capture(iface);
            if(!check(below?(data.typedEntityCaptureFailed&&block.ent.empty()&&data.typedEntityRetainedRecords==0u
                    &&data.typedEntityRetainedBytes==initial)
                :(!data.typedEntityCaptureFailed&&block.ent.size()==1u&&data.typedEntityRetainedRecords==1u
                    &&data.typedEntityRetainedBytes==dx_iface::TypedEntityCaptureMaxRetainedBytes),
                "nested common payload exact/one-byte-short callback boundary"))return false;
            if(!below) {
                std::size_t retained=0u;
                if(!check(DxIfaceStandardDwgCaptureTestAccess::estimate(estimator,*block.ent.front(),inlineBytes,retained)
                    &&retained==after,"retained clone keeps the measured nested payload"))return false;
            } else {
                capture(iface);
                if(!check(data.typedEntityCaptureFailed&&block.ent.empty(),"repeated over-budget callback remains refused"))return false;
                dx_data fresh;dx_ifaceBlock freshBlock;iface.cData=&fresh;iface.currentBlock=&freshBlock;capture(iface);
                if(!check(!fresh.typedEntityCaptureFailed&&freshBlock.ent.size()==1u
                    &&fresh.typedEntityRetainedBytes==charge,"same adapter recovers on a fresh destination after budget refusal"))return false;
                ++retries;
            }
            ++boundaries;
        }
        if(field==14u) {
            dx_data measured;dx_ifaceBlock measuredBlock;dx_iface iface;iface.cData=&measured;iface.currentBlock=&measuredBlock;
            allocationRemaining=(std::numeric_limits<long>::max)();capture(iface);
            const long allocations=(std::numeric_limits<long>::max)()-allocationRemaining;allocationRemaining=-1;
            if(!check(!measured.typedEntityCaptureFailed&&allocations>0&&allocations<5000,"nested common copy allocation calibration"))return false;
            for(long failure=0;failure<allocations;++failure) {
                dx_data rejected;dx_ifaceBlock rejectedBlock;iface.cData=&rejected;iface.currentBlock=&rejectedBlock;
                allocationRemaining=failure;capture(iface);allocationRemaining=-1;
                if(!check(rejected.typedEntityCaptureFailed&&rejectedBlock.ent.empty()
                    &&rejected.typedEntityRetainedBytes==0u&&rejected.typedEntityRetainedRecords==0u,
                    "nested common allocation failure cannot publish or charge a partial clone"))return false;
                dx_data fresh;dx_ifaceBlock freshBlock;iface.cData=&fresh;iface.currentBlock=&freshBlock;capture(iface);
                if(!check(!fresh.typedEntityCaptureFailed&&freshBlock.ent.size()==1u
                    &&fresh.typedEntityRetainedBytes==charge,"same adapter fresh-destination retry after nested allocation failure"))return false;
                ++allocationSchedules;
            }
        }
        ++cells;
    }
    std::cout<<"Nested MTEXT common budget cells="<<cells<<"; exact/below boundaries="<<boundaries
        <<"; budget retries="<<retries<<"; allocation refusals/retries="<<allocationSchedules<<'\n';
    return cells==60u&&boundaries==120u&&retries==60u;
}
Tags body(unsigned mode) {
    Tags result{{5,"A1"},{330,"B0"},{100,"AcDbEntity"},{8,"host-layer"},
        {102,"{HOST"},{5,"DEAD"},{330,"FACE"},{1,"opaque"},{2,"not-a-tag"},
        {3,"not-a-prompt"},{10,"99"},{20,"98"},{40,"97"},{1000,"opaque-\\U+00E9"},{102,"}"},
        {100,"AcDbText"},{10,"1.25"},{20,"2.5"},{30,"3.75"},{40,"4.5"},
        {1,"default-é"},{7,"STANDARD"},{11,"5"},{21,"6"},{31,"7"},
        {50,"15"},{210,"0"},{220,"1"},{230,"0"}};
    if(mode) {
        result.insert(result.end(),{{mode==1u?101:100,"Embedded Object"},
            {10,"10"},{20,"20"},{30,"30"},{40,"2.5"},{41,"12"},{46,"7"},
            {3,"FIRST_"},{1,"LAST-é"},{11,"0"},{21,"1"},{31,"0"},{71,"4"},
            {102,"{NESTED"},{5,"BEEF"},{330,"CAFE"},{1,"not-mtext"},{1000,"nested-\\U+00E9"},{102,"}"}});
    }
    result.insert(result.end(),{{100,"AcDbAttributeDefinition"},{3,"prompt-é"},{2,"TAG"},
        {70,"7"},{73,"5"},{74,"2"},{280,"1"},{1001,"APP"},{1003,"XDATA_LAYER"},
        {1010,"8"},{1020,"9"},{1030,"10"},{1070,"11"}});
    return result;
}
Tags document(const std::string& version,const Tags& row,bool block,bool dropped=false) {
    Tags result{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{0,"ENDSEC"},
        {0,"SECTION"},{2,block?"BLOCKS":"ENTITIES"}};
    if(block)result.insert(result.end(),{{0,"BLOCK"},{5,"B1"},{330,"B0"},{100,"AcDbEntity"},
        {8,"0"},{100,"AcDbBlockBegin"},{2,dropped?"":"GENERATED"},{70,"0"},{10,"0"},{20,"0"},{30,"0"}});
    result.push_back({0,"ATTDEF"});result.insert(result.end(),row.begin(),row.end());
    if(block)result.insert(result.end(),{{0,"ENDBLK"},{5,"B2"},{330,"B0"},
        {100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"}});
    result.insert(result.end(),{{0,"ENDSEC"},{0,"EOF"}});
    return result;
}
bool write(const std::filesystem::path& path,const Tags& tags,bool binary,bool crlf=false) {
    std::ofstream out(path,std::ios::binary);
    const auto word=[&](std::uint64_t value,unsigned bytes) {
        for(unsigned i=0;i<bytes;++i)out.put(static_cast<char>(value>>(8u*i)));
    };
    if(binary)out.write("AutoCAD Binary DXF\r\n\x1a\0",22);
    for(const auto& tag:tags) {
        if(!binary)out<<tag.code<<(crlf?"\r\n":"\n")<<tag.value<<(crlf?"\r\n":"\n");
        else {
            word(static_cast<unsigned>(tag.code),2u);
            if((tag.code>=60&&tag.code<=79)||(tag.code>=170&&tag.code<=179)
                ||(tag.code>=270&&tag.code<=289)||(tag.code>=370&&tag.code<=389)
                ||(tag.code>=400&&tag.code<=409)||tag.code==1070)
                word(static_cast<std::uint16_t>(std::stoi(tag.value)),2u);
            else if((tag.code>=90&&tag.code<=99)||(tag.code>=420&&tag.code<=429)
                ||(tag.code>=440&&tag.code<=459)||tag.code==1071)
                word(static_cast<std::uint32_t>(std::stoll(tag.value)),4u);
            else if(tag.code>=160&&tag.code<=169)word(std::stoull(tag.value),8u);
            else if(tag.code>=290&&tag.code<=299)word(static_cast<unsigned>(std::stoi(tag.value)),1u);
            else if((tag.code>=310&&tag.code<=319)||tag.code==1004) {
                if(tag.value.size()%2u||tag.value.size()/2u>255u)return false;
                word(tag.value.size()/2u,1u);
                for(std::size_t i=0;i<tag.value.size();i+=2u)word(std::stoul(tag.value.substr(i,2u),nullptr,16),1u);
            }
            else if((tag.code>=10&&tag.code<=59)||(tag.code>=210&&tag.code<=239)
                ||(tag.code>=1010&&tag.code<=1059)) {
                const double value=std::stod(tag.value);std::uint64_t bits=0u;
                std::memcpy(&bits,&value,sizeof(bits));word(bits,8u);
            } else {out<<tag.value;out.put('\0');}
        }
    }
    return out.good();
}
bool nestedCommonImportTests(const std::filesystem::path& path) {
    unsigned routes=0u;
    const auto definition=[](const dx_data& data)->const DRW_Attdef* {
        const auto find=[](const dx_ifaceBlock* block)->const DRW_Attdef* {
            if(block)for(const auto* entity:block->ent)
                if(const auto* value=dynamic_cast<const DRW_Attdef*>(entity))return value;
            return nullptr;
        };
        if(const auto* value=find(data.mBlock))return value;
        for(const auto* block:data.blocks)if(const auto* value=find(block))return value;
        return nullptr;
    };
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})
     for(bool binary:{false,true})for(bool block:{false,true})for(unsigned mode:{1u,2u}) {
        auto row=body(mode);
        if(!write(path,document(version,row,block),binary))return false;
        dx_data baseline;dx_iface baselineReader;
        if(!check(baselineReader.fileImport(path.string(),&baseline,false),"baseline actual-import nested owner route"))return false;
        constexpr std::size_t length=16384u;
        bool changed=false;
        for(auto& field:row)if(field.code==1&&field.value=="not-mtext") {
            field.value.assign(length,'q');changed=true;
        }
        if(!changed||!write(path,document(version,row,block),binary))return false;
        dx_data data;dx_iface reader;
        if(!check(reader.fileImport(path.string(),&data,false)&&!data.typedEntityCaptureFailed
            &&data.typedEntityRetainedBytes==baseline.typedEntityRetainedBytes+length-std::string("not-mtext").size(),
            "actual import charges exact added nested application text bytes"))return false;
        const auto* value=definition(data);
        if(!check(value&&value->mtext&&value->mtext->text=="FIRST_LAST-é",
            "opaque nested application text does not replace MTEXT content"))return false;
        bool retained=false;
        for(const auto& group:value->mtext->appData)for(const auto& field:group)
            if(field.code()==1&&field.type()==DRW_Variant::STRING&&field.content.s
                &&*field.content.s==std::string(length,'q'))retained=true;
        if(!check(retained,"actual-import nested application owner retains the complete authored value"))return false;
        ++routes;
    }
    std::cout<<"Nested MTEXT common actual-import routes="<<routes<<'\n';
    return routes==32u;
}

std::string snapshot(const DRW_Attdef& value) {
    std::ostringstream out;
    out.precision(17);
    const auto applicationData=[&](const DRW_Entity& entity) {
        for(const auto& group:entity.appData)for(const auto& field:group) {
            out<<'['<<field.code()<<':'<<field.type()<<':';
            if(field.type()==DRW_Variant::STRING)out<<field.c_str();
            else if(field.type()==DRW_Variant::INTEGER)out<<field.i_val();
            else if(field.type()==DRW_Variant::DOUBLE)out<<field.d_val();
            else throw std::runtime_error("unexpected authored application type");
            out<<']';
        }
    };
    out<<value.handle<<':'<<value.parentHandle<<':'<<value.layer<<':'<<value.text<<':'<<value.tag
        <<':'<<value.prompt<<':'<<unsigned(value.attribFlags)<<':'<<value.m_fieldLength
        <<':'<<value.lockPosition<<':'<<value.basePoint.x<<':'<<value.basePoint.y<<':'<<value.basePoint.z
        <<':'<<value.height<<':'<<value.style<<':'<<value.angle<<':'<<value.extPoint.y
        <<':'<<value.appData.size()<<':'<<value.extData.size()<<':'<<unsigned(value.m_attributeType);
    applicationData(value);
    if(value.mtext)out<<':'<<value.mtext->text<<':'<<value.mtext->angle<<':'<<value.mtext->height
        <<':'<<value.mtext->m_definedHeight<<':'<<value.mtext->appData.size()<<':'<<value.mtext->extData.size();
    if(value.mtext)applicationData(*value.mtext);
    return out.str();
}
class Probe final:public dx_iface {
public:
    bool selected=true,disableOnSource=false,throwSource=false,throwTyped=false;
    mutable unsigned selections=0u;
    long failAllocation=-1;
    unsigned sourceCalls=0u,typedCalls=0u;
    unsigned depth=0u,begins=0u,ends=0u;
    std::vector<std::string> events,typed,sourceTyped;
    std::vector<DRW_RawDxfObject> raw;
    std::vector<DRW_DxfAttdefSourceContext> contexts;
    void addHeader(const DRW_Header*) override {}
    void addBlock(const DRW_Block&) override {++depth;++begins;events.push_back("B");}
    void endBlock() override {--depth;++ends;events.push_back("E");}
    void addBlockRecord(const DRW_Block_Record&) override {}
    bool wantsDxfAttdefSource() const override {
        ++selections;
        if(failAllocation>=0)allocationRemaining=failAllocation;
        return selected;
    }
    void addDxfAttdefSource(const DRW_Attdef& value,const DRW_RawDxfObject& source,
                           const DRW_DxfAttdefSourceContext& context) override {
        if(context.inBlock!=(depth!=0u))throw std::runtime_error("wrong callback block context");
        ++sourceCalls;
        events.push_back("S");raw.push_back(source);sourceTyped.push_back(snapshot(value));contexts.push_back(context);
        if(disableOnSource)selected=false;
        if(throwSource)throw std::runtime_error("injected source failure");
    }
    void addAttDef(const DRW_Attdef& value) override {
        ++typedCalls;
        events.push_back("T");typed.push_back(snapshot(value));
        if(throwTyped)throw std::runtime_error("injected typed failure");
    }
};
bool rawMatches(const DRW_RawDxfObject& raw,const Tags& row,bool binary) {
    if(raw.name!="ATTDEF"||raw.handle!=0xA1u||raw.parentHandle!=0xB0u
        ||raw.groups.size()!=row.size()||raw.rawValues.size()!=row.size()||raw.hasRawValues==binary)return false;
    for(std::size_t i=0;i<row.size();++i) {
        const auto& value=raw.groups[i];
        if(value.code()!=row[i].code||(!binary&&raw.rawValues[i]!=row[i].value))return false;
        if(value.type()==DRW_Variant::STRING) {
            if(!value.content.s||*value.content.s!=row[i].value)return false;
        } else if(value.type()==DRW_Variant::INTEGER) {
            if(value.i_val()!=std::stoi(row[i].value))return false;
        } else if(value.type()==DRW_Variant::DOUBLE) {
            if(value.d_val()!=std::stod(row[i].value))return false;
        } else return false;
        if(binary&&!raw.rawValues[i].empty())return false;
    }
    return true;
}
bool replayMatches(const Tags& row,const std::string& version,const std::string& expected,bool ext) {
    std::ostringstream encoded;
    for(const auto& tag:row)encoded<<tag.code<<'\n'<<tag.value<<'\n';
    std::istringstream input(encoded.str());
    std::unique_ptr<dxfReader> reader=std::make_unique<dxfReaderAscii>(&input);
    reader->setVersion(version,true);
    DRW_Attdef value;dxfAttdef::Decoder decoder(value);int code=0;
    while(reader->readRec(&code))if(!decoder.parseCode(code,reader))return false;
    return decoder.finish(ext)&&snapshot(value)==expected;
}
bool tests(const std::filesystem::path& path) {
    unsigned cells=0u;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})
     for(bool binary:{false,true})for(bool block:{false,true})for(bool ext:{false,true})for(unsigned mode=0u;mode<3u;++mode) {
        const auto row=body(mode);const auto tags=document(version,row,block);
        if(!write(path,tags,binary))return false;
        dxfRW reader(path.string().c_str());Probe optIn;
        if(!check(reader.read(&optIn,ext)&&optIn.raw.size()==1u&&optIn.typed.size()==1u,"positive transport"))return false;
        const auto declared=version=="AC1021"?DRW::AC1021:version=="AC1024"?DRW::AC1024:
            version=="AC1027"?DRW::AC1027:DRW::AC1032;
        if(!check(optIn.typed==optIn.sourceTyped&&rawMatches(optIn.raw.front(),row,binary)
            &&optIn.raw.front().m_version==declared
            &&optIn.contexts.front().applyExtrusion==ext&&optIn.contexts.front().inBlock==block,
            "exact raw/context/finalized typed pair"))return false;
        if(!check(optIn.events==(block?std::vector<std::string>{"B","S","T","E"}
            :std::vector<std::string>{"S","T"}),"callback order"))return false;
        Probe optOut;optOut.selected=false;
        if(!check(reader.read(&optOut,ext)&&optOut.raw.empty()&&optOut.typed==optIn.typed
            &&replayMatches(row,version,optIn.typed.front(),ext),"opt-out and production replay equality"))return false;
        for(unsigned fault=0u;fault<5u;++fault) {
            auto badRow=row;
            // Missing mandatory fields cannot be supplied by opaque appData.
            if(fault<4u) {
                const int removed=fault==0u?1:fault==1u?2:fault==2u?20:40;
                unsigned applications=0u;
                for(auto it=badRow.begin();it!=badRow.end();) {
                    if(it->code==102)applications=it->value=="}"?applications-1u:applications+1u;
                    if(!applications&&it->code==removed)it=badRow.erase(it);else ++it;
                }
            }
            auto bad=document(version,badRow,block);
            if(fault==4u) {
                if(!block)continue;
                bool footer=false;
                for(auto& tag:bad) {
                    if(tag.code==0&&tag.value=="ENDBLK")footer=true;
                    if(footer&&tag.code==330){tag.value="not-a-handle";break;}
                }
            }
            if(!write(path,bad,binary))return false;
            Probe refused;
            const bool accepted=reader.read(&refused,ext);
            if(!check(!accepted&&refused.raw.empty()&&refused.typed.empty()
                &&refused.depth==0u,"malformed record/footer atomic refusal")) {
                std::cerr<<version<<" binary="<<binary<<" block="<<block<<" ext="<<ext<<" mode="<<mode
                    <<" fault="<<fault<<" accepted="<<accepted<<" raw="<<refused.raw.size()
                    <<" typed="<<refused.typed.size()<<" depth="<<refused.depth<<'\n';return false;
            }
            if(!write(path,tags,binary))return false;
            Probe corrected;
            if(!check(reader.read(&corrected,ext)&&corrected.raw.size()==1u,"same-facade corrected read"))return false;
        }
        ++cells;
     }
    // Requests are made before parsing, never re-evaluated at delayed flush.
    auto tags=document("AC1032",body(1u),true);
    auto second=body(2u);second.front().value="A2";
    auto footer=std::find_if(tags.begin(),tags.end(),[](const Tag& tag){return tag.code==0&&tag.value=="ENDBLK";});
    Tags extra{{0,"ATTDEF"}};extra.insert(extra.end(),second.begin(),second.end());
    tags.insert(footer,extra.begin(),extra.end());
    if(!write(path,tags,false))return false;
    dxfRW reader(path.string().c_str());Probe latched;latched.disableOnSource=true;
    const bool latchRead=reader.read(&latched,false);
    if(!check(latchRead&&latched.raw.size()==2u&&latched.typed.size()==2u
        &&latched.selections==2u,"per-record selection survives block flush")) {
        std::cerr<<"latch read="<<latchRead<<" raw="<<latched.raw.size()<<" typed="<<latched.typed.size()
            <<" selections="<<latched.selections<<" error="<<reader.getError()<<'\n';return false;
    }
    for(bool source:{false,true}) {
        Probe throwing;throwing.throwSource=source;throwing.throwTyped=!source;
        bool failed=false;try {failed=!reader.read(&throwing,false);}catch(const std::exception&){failed=true;}
        if(!check(failed&&throwing.depth==0u&&throwing.begins==throwing.ends
            &&throwing.raw.size()==1u&&throwing.typed.size()==(source?0u:1u),"throw closes block without later pairs"))return false;
        Probe retry;if(!check(reader.read(&retry,false)&&retry.raw.size()==2u,"throw corrected retry"))return false;
    }
    auto dropped=document("AC1032",body(1u),true,true);
    if(!write(path,dropped,false))return false;
    Probe drop;if(!check(reader.read(&drop,false)&&drop.events.empty(),"dropped block publishes no source pair"))return false;
    // Exercise aggregate raw node and logical text charges, and corrected reads.
    const auto quotaTags=document("AC1032",body(1u),true);
    if(!write(path,quotaTags,false))return false;
    reader.setDxfReadTypedVariantNodeBudget(12u);Probe nodes;
    if(!check(!reader.read(&nodes,false)&&nodes.raw.empty()&&nodes.typed.empty()&&nodes.depth==0u,"raw node quota refusal"))return false;
    reader.setDxfReadTypedVariantNodeBudget(10000u);
    reader.setDxfReadRetainedByteBudget(64u);Probe bytes;
    if(!check(!reader.read(&bytes,false)&&bytes.raw.empty()&&bytes.typed.empty()&&bytes.depth==0u,"logical byte quota refusal"))return false;
    reader.setDxfReadRetainedByteBudget(1024u*1024u);Probe recovered;
    if(!check(reader.read(&recovered,false)&&recovered.raw.size()==1u,"quota recovery"))return false;

    std::array<std::size_t,2> nodeMinimum{},byteMinimum{};
    for(unsigned selected=0u;selected<2u;++selected)for(bool nodeBudget:{false,true}) {
        std::size_t lower=0u,upper=1024u*1024u;
        const auto attempt=[&](std::size_t limit) {
            reader.setDxfReadRetainedByteBudget(nodeBudget?1024u*1024u:limit);
            reader.setDxfReadTypedVariantNodeBudget(nodeBudget?limit:10000u);
            Probe probe;probe.selected=selected!=0u;
            const bool ok=reader.read(&probe,false);
            if(probe.depth!=0u)throw std::runtime_error("quota left an unbalanced scope");
            return ok;
        };
        while(lower<upper) {
            const auto middle=lower+(upper-lower)/2u;
            if(attempt(middle))upper=middle;else lower=middle+1u;
        }
        if(!check(lower>0u&&attempt(lower)&&!attempt(lower-1u),"exact/below quota boundary"))return false;
        (nodeBudget?nodeMinimum:byteMinimum)[selected]=lower;
    }
    if(!check(nodeMinimum[0]<nodeMinimum[1]&&byteMinimum[0]<byteMinimum[1],
        "opt-out avoids additional source charges"))return false;
    reader.setDxfReadRetainedByteBudget(1024u*1024u);reader.setDxfReadTypedVariantNodeBudget(10000u);

    // Calibrate this one authored record, then refuse every allocation from
    // source selection through staging, callbacks and read completion. A
    // callback failure may deliver a prefix, but must close the public scope.
    Probe calibrated;calibrated.failAllocation=(std::numeric_limits<long>::max)();
    bool measured=false;
    try {measured=reader.read(&calibrated,false);}catch(...){}
    const long allocations=(std::numeric_limits<long>::max)()-allocationRemaining;
    allocationRemaining=-1;
    if(!check(measured&&allocations>0&&allocations<5000,"allocation calibration"))return false;
    for(long failure=0;failure<=allocations;++failure) {
        Probe probe;probe.failAllocation=failure;bool accepted=false;
        try {accepted=reader.read(&probe,false);}catch(...){}
        allocationRemaining=-1;
        if(!check(probe.depth==0u&&probe.begins==probe.ends&&probe.typedCalls<=probe.sourceCalls
            &&(!accepted||(probe.raw.size()==1u&&probe.typed.size()==1u)),"allocation refusal/publication/scope"))return false;
    }
    Probe retry;if(!check(reader.read(&retry,false)&&retry.raw.size()==1u,"allocation corrected retry"))return false;
    std::cout<<"ATTDEF transport routes="<<cells<<"; allocation schedules="<<allocations+1
        <<"; node minima="<<nodeMinimum[0]<<'/'<<nodeMinimum[1]
        <<"; byte minima="<<byteMinimum[0]<<'/'<<byteMinimum[1]
        <<"; malformed/retry, block/throw/opt-out and quotas: PASS\n";
    return true;
}
}

namespace {
class CaptureProbe final:public dx_iface {
public:
    bool selected=true,legacyOptIn=false,skipSource=false,skipTyped=false,duplicateSource=false,duplicateTyped=false,throwSource=false;
    unsigned sourceCalls=0u,typedCalls=0u;
    long failSourceAllocation=-1,sourceAllocations=0,failTypedAllocation=-1,typedAllocations=0;
    std::function<void(DRW_Attdef&)> alterTyped;
    std::function<void(DRW_RawDxfObject&)> alterRaw;
    std::function<void(DRW_DxfAttdefSourceContext&)> alterContext;
    DRW_RawDxfObject mutableRawAlias;
    // Explicit AC1018 opt-in uses the already existing source facet; the
    // concrete adapter's default opt-out remains unchanged.
    bool wantsDxfAttdefSource() const override {return selected&&(legacyOptIn||dx_iface::wantsDxfAttdefSource());}
    void addDxfAttdefSource(const DRW_Attdef& value,const DRW_RawDxfObject& raw,
                           const DRW_DxfAttdefSourceContext& context) override {
        ++sourceCalls;
        if(skipSource)return;
        if(throwSource)throw std::runtime_error("injected ATTDEF capture callback failure");
        mutableRawAlias=raw;if(alterRaw)alterRaw(mutableRawAlias);
        auto actualContext=context;if(alterContext)alterContext(actualContext);
        if(failSourceAllocation>=0)allocationRemaining=failSourceAllocation;
        dx_iface::addDxfAttdefSource(value,mutableRawAlias,actualContext);
        if(failSourceAllocation>=0) {
            sourceAllocations=failSourceAllocation-allocationRemaining;
            allocationRemaining=-1;
        }
        if(duplicateSource)dx_iface::addDxfAttdefSource(value,mutableRawAlias,actualContext);
    }
    void addAttDef(const DRW_Attdef& value) override {
        ++typedCalls;if(skipTyped)return;
        // The parser/staging pair owns a NON-const record; the API borrows it
        // as const. Fault injection here changes exactly one original field,
        // without a model copy resetting unrelated cursor/grammar state.
        if(alterTyped)alterTyped(const_cast<DRW_Attdef&>(value));
        if(failTypedAllocation>=0)allocationRemaining=failTypedAllocation;
        dx_iface::addAttDef(value);
        if(failTypedAllocation>=0) {
            typedAllocations=failTypedAllocation-allocationRemaining;
            allocationRemaining=-1;
        }
        if(duplicateTyped)dx_iface::addAttDef(value);
    }
};

#include "attdef_source_profile_controls.h"
#include "block_source_controls.h"
#include "block_source_proof_controls.h"
#include "block_children_source_controls.h"
#include "raw_application_scope_controls.h"
#include "raw_shell_scope_controls.h"
#include "block_delivery_controls.h"
#include "attdef_origin_controls.h"
#include "block_membership_controls.h"
#include "compound_body_controls.h"
#include "compound_source_profile_controls.h"
#include "application_handle_width_controls.h"
#include "block_primary_controls.h"
#include "block_origin_controls.h"
#include "block_empty_polyline_controls.h"
#include "attdef_equal_alignment_controls.h"
#include "attdef_embedded_dual280_controls.h"

struct LegacyCapturedReplay {
    bool transport=false,decoded=false,state=false,model=false;
};
bool legacyVariantEqual(const DRW_Variant& a,const DRW_Variant& b) noexcept {
    if(a.code()!=b.code()||a.type()!=b.type()||a.isLayerRef()!=b.isLayerRef()
        ||a.rawDwgLayerReference()!=b.rawDwgLayerReference())return false;
    const auto real=[](double x,double y){return std::memcmp(&x,&y,sizeof(x))==0;};
    switch(a.type()) {
    case DRW_Variant::STRING:return a.content.s&&b.content.s&&*a.content.s==*b.content.s;
    case DRW_Variant::INTEGER:return a.i_val()==b.i_val();
    case DRW_Variant::INTEGER64:return a.i64_val()==b.i64_val();
    case DRW_Variant::DOUBLE:return real(a.d_val(),b.d_val());
    case DRW_Variant::COORD:return a.coord()&&b.coord()&&real(a.coord()->x,b.coord()->x)
        &&real(a.coord()->y,b.coord()->y)&&real(a.coord()->z,b.coord()->z);
    case DRW_Variant::BINARY:return a.content.b&&b.content.b&&*a.content.b==*b.content.b;
    default:return false;
    }
}
// Observational reconstruction of the selected known scalar types; this is
// not the adapter's production source-body admission or an output proof.
LegacyCapturedReplay legacyCapturedReplay(const DRW_RawDxfObject& raw,const DRW_Attdef& expected,
        const dxfAttdef::SourceState& expectedState,const std::string& codePage,bool extrusion) {
    LegacyCapturedReplay result;if(raw.groups.size()!=raw.rawValues.size())return result;
    std::ostringstream output(std::ios::out|std::ios::binary);dxfWriterBinary binary(&output);
    for(std::size_t i=0u;i<raw.groups.size();++i) {
        const auto& value=raw.groups[i];const int code=value.code();
        if(raw.hasRawValues) {output<<code<<'\n'<<raw.rawValues[i]<<'\n';continue;}
        if(!raw.rawValues[i].empty())return result;
        if(value.type()==DRW_Variant::STRING&&value.content.s) {if(!binary.writeRawString(code,*value.content.s))return result;}
        else if(value.type()==DRW_Variant::INTEGER) {if(!binary.writeInt16(code,value.i_val()))return result;}
        else if(value.type()==DRW_Variant::DOUBLE) {if(!binary.writeDouble(code,value.d_val()))return result;}
        else return result;
    }
    const auto bytes=output.str();dxfRW::DxfObjectsWriteView view{};view.bytes=bytes;
    view.version=raw.m_version;view.binary=!raw.hasRawValues;view.codePage=codePage;
    result.transport=dx_standard_output::rawScalarTransportEqual(raw.groups,bytes,view,
        [](std::uint32_t source,std::uint32_t& mapped){mapped=source;return true;},false,1024u*1024u,true);
    if(!result.transport)return result;
    dx_standard_output::InputBuffer buffer(bytes);std::istream input(&buffer);
    auto reader=dx_standard_output::readerFor(input,view,std::make_shared<DxfReadRetainedBudget>(1024u*1024u));
    if(!reader)return result;reader->setSemanticStringMode(true);
    DRW_Attdef parsed;dxfAttdef::Decoder decoder(parsed);int code=0;
    while(buffer.position()<bytes.size())
        if(!reader->readRec(&code)||code==0||!decoder.parseCode(code,reader))return result;
    if(!decoder.finish(extrusion)||reader->recordCount()!=raw.groups.size())return result;
    result.decoded=true;const auto observed=dxfAttdef::Decoder::observe(parsed);
    result.state=observed==expectedState&&observed.hasNoPendingXdata();
    result.model=dxfAttdef::Decoder::sameModel(parsed,expected,legacyVariantEqual);
    return result;
}

// Observational transport only: deliberately opts into the reader's existing
// optional facet. It does not change the adapter's AC1018 opt-in or certify
// a profile, owner graph, APPID target, output closure, or native semantics.
bool legacyAttdefObservationTests(const std::filesystem::path& path) {
    struct Observer final:dx_iface {
        dxfRW* reader=nullptr;bool selected=true,live=false;unsigned depth=0u;
        DRW_RawDxfObject source;DRW_Attdef sourceModel,typedModel;
        DRW_DxfAttdefSourceContext context{};dxfAttdef::SourceState sourceState{},typedState{};
        DRW::Version sourceVersion=DRW::UNKNOWNV,codecVersion=DRW::UNKNOWNV;
        std::string page,declaration;bool declared=false,binary=false,extrusion=false;
        unsigned sourceCalls=0u,typedCalls=0u;
        void addHeader(const DRW_Header*)override {}
        void addBlock(const DRW_Block&)override {++depth;}
        void endBlock()override {--depth;}
        void addBlockRecord(const DRW_Block_Record&)override {}
        bool wantsDxfAttdefSource()const override{return selected;}
        void addDxfAttdefSource(const DRW_Attdef& value,const DRW_RawDxfObject& raw,
                                const DRW_DxfAttdefSourceContext& current)override {
            ++sourceCalls;sourceState=dxfAttdef::Decoder::observe(value);source=raw;sourceModel=value;
            context=current;dxfRW::DxfReadContext c;
            live=reader&&reader->getDxfReadContext(c);
            if(live) {sourceVersion=c.sourceVersion;codecVersion=c.codecVersion;page=std::string(c.codecCodePage);
                declaration=std::string(c.declaration);declared=c.hasDeclaration;binary=c.binary;extrusion=c.applyExtrusion;}
        }
        void addAttDef(const DRW_Attdef& value)override{++typedCalls;typedState=dxfAttdef::Decoder::observe(value);typedModel=value;}
    };
    // The two non-ASCII bytes are authored directly in Windows-1252; XDATA
    // includes an APPID name and values, but no APPID target is claimed.
    const Tags row{{5,"A1"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbText"},
        {10,"0"},{20,"0"},{30,"0"},{40,"5"},{1,std::string("caf\xE9",4u)},
        {100,"AcDbAttributeDefinition"},{3,std::string("prom\xE9",5u)},{2,"ATTINFO"},{70,"0"},
        {1001,"AcDbAttr"},{1000,std::string("caf\xE9",4u)},{1070,"0"},{1070,"1"}};
    unsigned originals=0u,spans=0u,optouts=0u,retries=0u,full=0u,mutations=0u,pages=0u;
    for(bool binary:{false,true})for(bool block:{false,true})for(bool extrusion:{false,true}) {
        auto tags=document("AC1018",row,block);tags.insert(tags.begin()+4,{{9,"$DWGCODEPAGE"},{3,"ANSI_1252"}});
        if(!write(path,tags,binary))return false;dxfRW reader(path.string().c_str());Observer selected;selected.reader=&reader;
        if(!check(reader.read(&selected,extrusion)&&selected.depth==0u&&selected.sourceCalls==1u&&selected.typedCalls==1u
            &&selected.live&&selected.sourceVersion==DRW::AC1018&&selected.codecVersion==DRW::AC1015
            &&selected.declared&&selected.declaration=="ANSI_1252"&&selected.page=="ANSI_1252"
            &&selected.binary==binary&&selected.extrusion==extrusion&&selected.context.inBlock==block
            &&selected.context.applyExtrusion==extrusion,"legacy source callback and live codec context"))return false;
        const auto& raw=selected.source;const auto& model=selected.sourceModel;
        if(!check(raw.name=="ATTDEF"&&raw.handle==0xA1u&&raw.parentHandle==0xB0u
            &&raw.m_version==DRW::AC1018&&raw.hasRawValues!=binary&&raw.groups.size()==row.size()
            &&raw.rawValues.size()==row.size()&&snapshot(model)==snapshot(selected.typedModel)
            &&model.text=="caf\xC3\xA9"&&model.prompt=="prom\xC3\xA9"&&model.tag=="ATTINFO"
            &&model.extData.size()==4u&&model.extData[0]&&model.extData[1]&&model.extData[2]&&model.extData[3]
            &&model.extData[0]->code()==1001&&std::string(model.extData[0]->c_str())=="AcDbAttr"
            &&model.extData[1]->code()==1000&&std::string(model.extData[1]->c_str())=="caf\xC3\xA9"
            &&model.extData[2]->code()==1070&&model.extData[2]->i_val()==0
            &&model.extData[3]->code()==1070&&model.extData[3]->i_val()==1,
            "legacy typed capture, source model and XDATA values"))return false;
        const auto wire=attdefWireTags(path,binary);std::size_t first=wire.size();
        for(std::size_t i=0u;i<wire.size();++i)if(wire[i].code==0&&wire[i].text=="ATTDEF")first=i+1u;
        if(first+row.size()>wire.size())return false;
        for(std::size_t i=0u;i<row.size();++i) {
            const auto& field=raw.groups[i];const auto& physical=wire[first+i];
            if(!check(field.code()==row[i].code&&physical.code==row[i].code&&physical.begin<physical.valueBegin
                &&physical.valueBegin<physical.end&&(!i||wire[first+i-1u].end==physical.begin)
                &&(binary?raw.rawValues[i].empty():raw.rawValues[i]==row[i].value),"legacy exact physical field/span"))return false;
            if(physical.kind==AttdefWireTag::String&&(physical.text!=row[i].value
                ||field.type()!=DRW_Variant::STRING||!field.content.s||*field.content.s!=physical.text))return false;
            if(physical.kind==AttdefWireTag::Integer&&(field.type()!=DRW_Variant::INTEGER||field.i_val()!=physical.integer))return false;
            if(physical.kind==AttdefWireTag::Real&&(field.type()!=DRW_Variant::DOUBLE
                ||field.d_val()!=physical.real))return false;
            if(physical.kind==AttdefWireTag::Integer&&physical.integer!=std::stoll(row[i].value))return false;
            if(physical.kind==AttdefWireTag::Real&&physical.real!=std::stod(row[i].value))return false;
            ++spans;
        }
        if(!check(replayMatches(row,"AC1018",snapshot(model),extrusion),
            "legacy authored raw replay matches captured typed model"))return false;
        if(!check(selected.sourceState==selected.typedState&&selected.sourceState.hasNoPendingXdata()
            &&dxfAttdef::Decoder::sameModel(model,selected.typedModel,legacyVariantEqual),"legacy full callback model and state pair"))return false;
        const auto replay=legacyCapturedReplay(raw,model,selected.sourceState,selected.page,extrusion);
        if(!check(replay.transport&&replay.decoded&&replay.state&&replay.model,"legacy captured-raw full model/state replay"))return false;
        ++full;
        const auto wrongPage=legacyCapturedReplay(raw,model,selected.sourceState,"ANSI_1251",extrusion);
        if(!check(wrongPage.transport&&wrongPage.decoded&&wrongPage.state&&!wrongPage.model,"legacy replay requires captured codepage"))return false;
        ++pages;
        for(int altered:{1,1070}) {
            auto corrupted=raw;const auto at=std::find_if(corrupted.groups.begin(),corrupted.groups.end(),
                [=](const DRW_Variant& v){return v.code()==altered;});
            if(at==corrupted.groups.end())return false;
            *at=altered==1?DRW_Variant(1,std::string("changed")):DRW_Variant(1070,7);
            const auto rejected=legacyCapturedReplay(corrupted,model,selected.sourceState,selected.page,extrusion);
            if(!check(!rejected.transport||(rejected.decoded&&!rejected.model),"legacy captured-raw corruption sensitivity"))return false;
            ++mutations;
        }
        const auto gated=dx_attdef_source::audit(raw);
        if(!check(!gated.complete()&&gated.spans.empty()&&gated.reason==dx_attdef_source::Reason::Revision
            &&gated.rejectedGroup==0u,"legacy source audit remains gated"))return false;
        Observer optout;optout.reader=&reader;optout.selected=false;
        if(!check(reader.read(&optout,extrusion)&&optout.sourceCalls==0u&&optout.typedCalls==1u
            &&snapshot(optout.typedModel)==snapshot(selected.typedModel),"legacy source opt-out preserves typed callback"))return false;
        ++optouts;
        auto bad=row;bad.erase(std::find_if(bad.begin(),bad.end(),[](const Tag& t){return t.code==40;}));
        auto badTags=document("AC1018",bad,block);badTags.insert(badTags.begin()+4,{{9,"$DWGCODEPAGE"},{3,"ANSI_1252"}});
        if(!write(path,badTags,binary))return false;Observer rejected;rejected.reader=&reader;
        if(!check(!reader.read(&rejected,extrusion)&&rejected.sourceCalls==0u&&rejected.typedCalls==0u
            &&rejected.depth==0u,"legacy malformed source is not published"))return false;
        if(!write(path,tags,binary))return false;Observer retry;retry.reader=&reader;
        if(!check(reader.read(&retry,extrusion)&&retry.sourceCalls==1u&&retry.typedCalls==1u
            &&snapshot(retry.sourceModel)==snapshot(selected.sourceModel),"legacy corrected read on same facade"))return false;
        ++retries;++originals;
    }
    std::cout<<"AC1018 observational ATTDEF sources="<<originals<<" raw spans="<<spans
        <<" opt-outs="<<optouts<<" malformed/retries="<<retries<<" full-replays="<<full
        <<" wrong-page="<<pages<<" raw-mutations="<<mutations<<'\n';
    return originals==8u&&spans==144u&&optouts==8u&&retries==8u&&full==8u&&pages==8u&&mutations==16u;
}

// Adapter-owned capture/replay is opt-in via the existing optional source
// facet. AC1018 profiles and enclosing graph/output authority stay gated.
bool legacyAttdefAdapterCaptureTests(const std::filesystem::path& path) {
    using A=DxIfaceTableSourceTestAccess;using namespace dx_attdef_source;
    const Tags row{{5,"A1"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbText"},
        {10,"0"},{20,"0"},{30,"0"},{40,"5"},{1,std::string("caf\xE9",4u)},
        {100,"AcDbAttributeDefinition"},{3,std::string("prom\xE9",5u)},{2,"ATTINFO"},{70,"0"},
        {1001,"AcDbAttr"},{1000,std::string("caf\xE9",4u)},{1070,"0"},{1070,"1"}};
    unsigned captures=0u,defaults=0u,mutations=0u,quotas=0u,retries=0u;
    const auto definition=[](const dx_data& data)->const DRW_Attdef* {
        const auto inBlock=[](const dx_ifaceBlock* block)->const DRW_Attdef* {
            if(block)for(const auto* entity:block->ent)
                if(const auto* value=dynamic_cast<const DRW_Attdef*>(entity))if(value->handle==0xA1u)return value;
            return nullptr;
        };
        if(const auto* value=inBlock(data.mBlock))return value;
        for(const auto* block:data.blocks)if(const auto* value=inBlock(block))return value;
        return nullptr;
    };
    for(bool binary:{false,true})for(bool block:{false,true}) {
        auto tags=document("AC1018",row,block);tags.insert(tags.begin()+4,{{9,"$DWGCODEPAGE"},{3,"ANSI_1252"}});
        if(!write(path,tags,binary))return false;
        CaptureProbe defaultReader;dx_data baseline;
        if(!check(defaultReader.fileImport(path.string(),&baseline,false)&&defaultReader.sourceCalls==0u
            &&defaultReader.typedCalls==1u&&definition(baseline),"legacy adapter default opt-out retains 2D import"))return false;
        const auto expected=snapshot(*definition(baseline));++defaults;
        CaptureProbe reader;reader.legacyOptIn=true;dx_data data;
        if(!check(reader.fileImport(path.string(),&data,false)&&reader.sourceCalls==1u&&reader.typedCalls==1u
            &&!A::failed(data)&&definition(data)&&snapshot(*definition(data))==expected,"legacy explicit adapter opt-in"))return false;
        const auto* value=A::resource(data);
        if(!check(value&&value->raw&&value->attdefModel&&value->attdefReadContext
            &&value->attdefReadContext->captured&&value->attdefReadContext->sourceVersion==DRW::AC1018
            &&value->attdefReadContext->codecVersion==DRW::AC1015&&value->attdefReadContext->codecCodePage=="ANSI_1252"
            &&value->attdefReadContext->hasDeclaration&&value->attdefReadContext->declaration=="ANSI_1252"
            &&value->attdefReadContext->binary==binary&&!value->attdefReadContext->applyExtrusion
            &&value->attdefPairMatched&&value->attdefFieldsMatched&&!value->attdefFieldMismatch
            &&!value->rawTypedMatched&&!value->attdefOrigin&&!value->attdefProfile.complete()
            &&value->attdefProfile.reason==Reason::Revision&&value->attdefProfile.rejectedGroup==0u
            &&value->raw->m_version==DRW::AC1018&&value->raw->hasRawValues!=binary
            &&value->raw->groups.size()==row.size()&&value->attdefContext.inBlock==block,
            "legacy adapter owns raw/context and complete replay but no profile or origin"))return false;
        ++captures;
        for(unsigned fault=0u;fault<3u;++fault) {
            CaptureProbe faulty;faulty.legacyOptIn=true;
            if(fault==0u)faulty.alterRaw=[](DRW_RawDxfObject& r) {
                for(auto& field:r.groups)if(field.code()==1){field=DRW_Variant(1,std::string("changed"));return;}
            };
            if(fault==1u)faulty.alterTyped=[](DRW_Attdef& d){d.tag="CHANGED";};
            if(fault==2u)faulty.alterContext=[](DRW_DxfAttdefSourceContext& c){c.applyExtrusion=true;};
            dx_data corrupt;
            if(!faulty.fileImport(path.string(),&corrupt,false))return false;
            const auto* refused=A::resource(corrupt);
            if(!check(A::failed(corrupt)||!refused||!refused->attdefPairMatched||!refused->attdefFieldsMatched,
                "legacy mutated callback cannot certify a captured pair"))return false;
            if(refused&&(!check(!refused->rawTypedMatched&&!refused->attdefProfile.complete()&&!refused->attdefOrigin,
                "legacy failed callback has no qualified profile or origin")))return false;
            faulty.alterRaw={};faulty.alterTyped={};faulty.alterContext={};dx_data recovered;
            if(!check(faulty.fileImport(path.string(),&recovered,false)&&!A::failed(recovered)
                &&A::resource(recovered)&&A::resource(recovered)->attdefFieldsMatched,
                "legacy same-adapter fresh-destination corrected capture"))return false;
            ++mutations;++retries;
        }
        CaptureProbe limited;limited.legacyOptIn=true;dx_data constrained;A::limits(constrained,3u,0u);
        if(!check(limited.fileImport(path.string(),&constrained,false)&&A::failed(constrained)
            &&definition(constrained)&&snapshot(*definition(constrained))==expected,
            "legacy bounded adapter proof failure remains unqualified"))return false;
        const auto* unavailable=A::resource(constrained);
        if(unavailable&&(!check(!unavailable->rawTypedMatched&&!unavailable->attdefProfile.complete(),
            "legacy quota failure has no source authority")))return false;
        ++quotas;
        // A fresh destination under the ordinary default opt-out does not pay
        // the optional capture budget, even if that budget is zero.
        CaptureProbe noCapture;dx_data uncaptured;A::limits(uncaptured,3u,0u);
        if(!check(noCapture.fileImport(path.string(),&uncaptured,false)&&!A::failed(uncaptured)
            &&noCapture.sourceCalls==0u&&definition(uncaptured)&&snapshot(*definition(uncaptured))==expected,
            "legacy default 2D path survives zero optional capture quota"))return false;
        limited.legacyOptIn=false;dx_data restored;
        if(!check(limited.fileImport(path.string(),&restored,false)&&!A::failed(restored)
            &&definition(restored)&&snapshot(*definition(restored))==expected,
            "legacy same adapter can return to default opt-out"))return false;
        ++retries;
    }
    std::cout<<"AC1018 adapter opt-in captures="<<captures<<" default="<<defaults<<" mutations="<<mutations
        <<" proof-quotas="<<quotas<<" retries="<<retries<<'\n';
    return captures==4u&&defaults==4u&&mutations==12u&&quotas==4u&&retries==16u;
}

bool sourceProfileTests(const std::filesystem::path& path) {
    using Access=DxIfaceTableSourceTestAccess;
    if(!attdefDual280ProfileTests(path))return false;
    unsigned tails=0u,attribTails=0u;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})
     for(bool binary:{false,true})for(bool block:{false,true})for(unsigned mode:{0u,1u,2u}) {
        if(!write(path,document(version,body(mode),block),binary))return false;
        CaptureProbe reader;dx_data data;
        if(!check(reader.fileImport(path.string(),&data,false)&&!Access::failed(data),"attribute tail import"))return false;
        const auto* resource=Access::resource(data);
        if(!check(resource&&resource->attdefModel&&resource->attdefModel->m_fieldLength==5
            &&static_cast<int>(resource->attdefModel->alignV)==2
            &&(!resource->attdefModel->mtext||resource->attdefModel->mtext->linespacingStyle==1),
            "ATTDEF field length and vertical alignment belong to the host, not nested MTEXT"))return false;
        ++tails;
        auto attributeRow=body(mode);attributeRow[1].value="A0";
        for(auto& tag:attributeRow) {
            if(tag.code==100&&tag.value=="AcDbAttributeDefinition")tag.value="AcDbAttribute";
        }
        auto attributeTags=document(version,attributeRow,block);
        const auto begin=std::find_if(attributeTags.begin(),attributeTags.end(),[](const Tag& tag){return tag.code==0&&tag.value=="ATTDEF";});
        const auto index=static_cast<std::size_t>(begin-attributeTags.begin());begin->value="INSERT";
        const Tags insertBody{{5,"A0"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockReference"},
            {2,"TARGET"},{66,"1"},{10,"0"},{20,"0"},{30,"0"},{0,"ATTRIB"}};
        attributeTags.insert(attributeTags.begin()+static_cast<std::ptrdiff_t>(index+1u),insertBody.begin(),insertBody.end());
        const Tags footer{{0,"SEQEND"},{5,"A2"},{330,"A0"},{100,"AcDbEntity"},{8,"0"}};
        attributeTags.insert(attributeTags.begin()+static_cast<std::ptrdiff_t>(index+1u+insertBody.size()+attributeRow.size()),footer.begin(),footer.end());
        if(!write(path,attributeTags,binary))return false;
        dx_iface plain;dx_data attributes;
        if(!check(plain.fileImport(path.string(),&attributes,false),"ordinary ATTRIB tail import"))return false;
        const DRW_Attrib* retained=nullptr;
        const auto find=[&](const dx_ifaceBlock* owner) {
            if(owner)for(const auto* entity:owner->ent)
                if(const auto* candidate=dynamic_cast<const DRW_Insert*>(entity))
                    if(!candidate->attlist.empty())retained=candidate->attlist.front().get();
        };
        find(attributes.mBlock);for(const auto* owner:attributes.blocks)find(owner);
        if(!check(retained&&retained->m_fieldLength==5&&static_cast<int>(retained->alignV)==2
            &&(!retained->mtext||retained->mtext->linespacingStyle==1),"ATTRIB shares corrected host tail routing"))return false;
        ++attribTails;
    }
    unsigned qualified=0u,compatibility=0u,observerFaults=0u;
    std::size_t spans=0u;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})
     for(bool binary:{false,true})for(bool block:{false,true})for(unsigned mode:{0u,1u,2u,3u}) {
        const auto ledger=attdefLedger(mode,version=="AC1021");
        const auto tags=document(version,ledger.tags,block);
        if(!write(path,tags,binary))return false;
        CaptureProbe reader;dx_data data;
        if(!check(reader.fileImport(path.string(),&data,false)&&!Access::failed(data),"source-profile actual import"))return false;
        const auto* row=Access::resource(data);
        if(!check(row&&row->attdefFieldsMatched&&!row->rawTypedMatched,"profile cannot substitute for block and target proof"))return false;
        const bool supported=mode==0u||version=="AC1032";
        if(!check(row->attdefProfile.complete()==supported,"known modern classic versus documented multiline revisions")) {
            std::cerr<<"version="<<version<<" mode="<<mode<<" reason="<<static_cast<unsigned>(row->attdefProfile.reason)
                <<" group="<<row->attdefProfile.rejectedGroup<<'\n';return false;
        }
        if(supported) {
            if(!attdefOriginalSpans(path,binary,tags,ledger,row->attdefProfile,spans))return false;
            for(unsigned which=0;which<4u;++which) {
                auto corrupted=row->attdefProfile;
                if(which==0u)corrupted.spans.front().scope=dx_attdef_source::Scope::Nested;
                if(which==1u)corrupted.spans.front().role=dx_attdef_source::Role::Owner;
                if(which==2u)corrupted.spans.front().handle=0xA2u;
                if(which==3u)corrupted.spans.front().groupIndex=1u;
                std::size_t ignored=0u;
                if(!check(!attdefOriginalSpans(path,binary,tags,ledger,corrupted,ignored,true),
                    "independent original-span observer detects scope/role/value/index corruption"))return false;
                ++observerFaults;
            }
            if(!check(row->attdefModel->m_fieldLength==5&&static_cast<int>(row->attdefModel->alignV)==2
                &&(!mode||row->attdefModel->mtext->linespacingStyle==2),"host tail and nested line spacing remain distinct"))return false;
            ++qualified;
        } else {
            if(!check(row->attdefProfile.reason==dx_attdef_source::Reason::Revision&&row->attdefProfile.spans.empty(),
                "pre-R2018 embedded compatibility never creates a partial source certificate"))return false;
            ++compatibility;
        }
    }
    unsigned chunkProfiles=0u,oversizedChunks=0u;std::size_t chunkSpans=0u;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})
     for(bool binary:{false,true})for(bool block:{false,true})for(unsigned mode:{0u,1u,2u,3u}) {
        if(mode&&version!="AC1032")continue;
        auto ledger=attdefLedger(mode,version=="AC1021");
        const std::string chunk(254u,'A');
        for(auto& tag:ledger.tags) {
            if(tag.code==160||tag.code==92)tag.value=std::to_string(2u+chunk.size()/2u);
            if((tag.code==310&&tag.value=="A3")||tag.code==1004)tag.value=chunk;
        }
        const auto tags=document(version,ledger.tags,block);
        if(!write(path,tags,binary))return false;
        CaptureProbe reader;dx_data data;
        if(!check(reader.fileImport(path.string(),&data,false)&&!Access::failed(data)
            &&Access::resource(data)&&Access::resource(data)->attdefProfile.complete(),
            "full 127-byte ASCII/Binary proxy and XDATA chunks qualify without truncation"))return false;
        const auto& row=*Access::resource(data);
        if(!check(row.attdefModel->proxyGraphics.size()==2u+chunk.size()/2u
            &&(!mode||row.attdefModel->mtext->proxyGraphics.size()==2u+chunk.size()/2u),
            "complete maximum chunk bytes retained on host and nested models"))return false;
        if(!attdefOriginalSpans(path,binary,tags,ledger,row.attdefProfile,chunkSpans))return false;
        for(int code:{310,1004})for(std::size_t bytes:{128u,255u}) {
            auto oversized=ledger.tags;
            const auto at=std::find_if(oversized.begin(),oversized.end(),[&](const Tag& tag){return tag.code==code&&tag.value==chunk;});
            if(at==oversized.end())return false;
            at->value=std::string(bytes*2u,'A');
            if(code==310) {
                const auto count=std::find_if(oversized.begin(),oversized.end(),[](const Tag& tag){return tag.code==92||tag.code==160;});
                count->value=std::to_string(bytes+2u);
            }
            if(!write(path,document(version,oversized,block),binary))return false;
            const auto sourceCalls=reader.sourceCalls,typedCalls=reader.typedCalls;
            dx_data rejected;
            if(!check(!reader.fileImport(path.string(),&rejected,false)
                &&reader.sourceCalls==sourceCalls&&reader.typedCalls==typedCalls
                &&(!Access::resource(rejected)||!Access::resource(rejected)->attdefProfile.complete()),
                "oversized physically representable chunk is refused before ATTDEF source/typed publication"))return false;
            if(!write(path,tags,binary))return false;
            dx_data retry;
            if(!check(reader.fileImport(path.string(),&retry,false)&&!Access::failed(retry)
                &&Access::resource(retry)&&Access::resource(retry)->attdefProfile.complete(),
                "fresh-import recovery after 128/255-byte chunk refusal"))return false;
            ++oversizedChunks;
        }
        ++chunkProfiles;
    }
    unsigned refusals=0u,replayAlone=0u;
    const auto faults=attdefProfileFaults();
    for(bool binary:{false,true})for(bool block:{false,true})for(unsigned mode:{0u,1u,2u,3u})
     for(const auto& fault:faults) {
        if(!mode&&fault.nestedOnly)continue;
        const auto clean=attdefLedger(mode,false);auto bad=clean.tags;fault.apply(bad);
        if(!write(path,document("AC1032",bad,block),binary))return false;
        CaptureProbe reader;dx_data data;
        const bool accepted=reader.fileImport(path.string(),&data,false);
        const auto* row=Access::resource(data);
        if(!check(!row||!row->attdefProfile.complete(),"fault cannot publish a complete profile")) {
            std::cerr<<"fault="<<fault.label<<" mode="<<mode<<'\n';return false;
        }
        if(accepted&&row&&row->attdefFieldsMatched&&!Access::failed(data)) {
            ++replayAlone;
            if(!check(row->attdefProfile.reason==fault.reason&&row->attdefProfile.spans.empty(),
                "modeled replay cannot launder a refused physical profile")) {
                std::cerr<<"fault="<<fault.label<<" mode="<<mode<<" reason="<<static_cast<unsigned>(row->attdefProfile.reason)
                    <<" expected="<<static_cast<unsigned>(fault.reason)<<" group="<<row->attdefProfile.rejectedGroup<<'\n';return false;
            }
        } else if(!check(!accepted||Access::failed(data)||!row||!row->attdefFieldsMatched,
            "earlier production/capture refusal is not promoted"))return false;
        if(!write(path,document("AC1032",clean.tags,block),binary))return false;
        dx_data retry;
        if(!check(reader.fileImport(path.string(),&retry,false)&&!Access::failed(retry)
            &&Access::resource(retry)&&Access::resource(retry)->attdefProfile.complete(),
            "same adapter corrected fresh-import profile recovery"))return false;
        ++refusals;
    }
    std::cout<<"ATTDEF source-profile host tail routes="<<tails<<"; ATTRIB host tail routes="<<attribTails
        <<"; qualified profiles="<<qualified
        <<"; earlier multiline compatibility profiles="<<compatibility<<"; original physical spans="<<spans
        <<"; observer corruptions="<<observerFaults<<"; refusal/retry routes="<<refusals
        <<"; replay-alone false positives refused="<<replayAlone
        <<"; maximum chunk profiles="<<chunkProfiles<<"; maximum chunk spans="<<chunkSpans
        <<"; oversized chunk refusals/retries="<<oversizedChunks<<'\n';
    return tails==48u&&attribTails==48u&&qualified==28u&&compatibility==36u&&observerFaults==112u
        &&refusals==548u&&replayAlone>0u&&chunkProfiles==28u&&oversizedChunks==112u;
}

bool immutableCaptureTests(const std::filesystem::path& path) {
    using Access=DxIfaceTableSourceTestAccess;
    unsigned profiles=0u,mutations=0u,callbacks=0u,quotas=0u,rawMutations=0u,richProfiles=0u;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})
     for(bool binary:{false,true})for(bool block:{false,true})for(unsigned mode:{0u,1u,2u}) {
        if(!write(path,document(version,body(mode),block),binary))return false;
        dx_data data;CaptureProbe reader;
        if(!check(reader.fileImport(path.string(),&data,false)&&!Access::failed(data),"actual ATTDEF capture import"))return false;
        const auto* row=Access::resource(data);
        if(!check(row&&row->attdefModel&&row->raw&&row->attdefPairMatched&&!row->awaitingAttdef
            &&row->attdefFieldsMatched&&!row->attdefFieldMismatch
            &&row->attdefProfile.complete()==(mode==0u||version=="AC1032")
            &&!row->rawTypedMatched&&!row->model&&Access::pending(data)==0u
            &&reader.sourceCalls==1u&&reader.typedCalls==1u&&row->attdefContext.inBlock==block
            &&!row->attdefContext.applyExtrusion&&row->attdefState.hasNested==(mode!=0u),
            "frozen source/typed pair is captured but not promoted to raw/target proof"))return false;
        if(block&&!check(row->attdefBlockHandle==0xB1u&&row->attdefBlockRecord==0xB0u
            &&row->attdefBlockName=="GENERATED","actual enclosing block context is retained"))return false;
        if(!check(row->attdefState.attributeSubclass
            &&!dxfAttdef::Decoder::observe(*row->attdefModel).attributeSubclass,
            "original subclass state survives separately from the sanitized model copy"))return false;
        const auto rawSize=row->raw->groups.size();const auto frozenText=row->attdefModel->text;
        reader.mutableRawAlias.groups.clear();reader.mutableRawAlias.rawValues.clear();
        if(!check(row->raw->groups.size()==rawSize&&row->raw->groups.front().type()==DRW_Variant::STRING
            &&row->raw->groups.front().content.s&&*row->raw->groups.front().content.s=="A1"
            &&row->attdefModel->text==frozenText,
            "mutable callback aliases cannot change frozen carriers"))return false;
        DRW_Attdef* editable=nullptr;
        const auto findEditable=[&](dx_ifaceBlock* owner) {
            if(owner)for(auto* entity:owner->ent)
                if(auto* candidate=dynamic_cast<DRW_Attdef*>(entity))editable=candidate;
        };
        findEditable(data.mBlock);for(auto* owner:data.blocks)findEditable(owner);
        if(!check(editable&&editable!=row->attdefModel.get()
            &&editable->extData.front().get()!=row->attdefModel->extData.front().get(),
            "editable entity and XDATA owners are separate from source evidence"))return false;
        editable->text="edited";editable->extData.front()->addString(1001,"EDITED_APP");
        if(!check(row->attdefModel->text==frozenText
            &&std::string(row->attdefModel->extData.front()->c_str())=="APP",
            "editable text/XDATA mutation leaves source evidence unchanged"))return false;
        CaptureProbe optOut;optOut.selected=false;dx_data legacy;
        if(!check(optOut.fileImport(path.string(),&legacy,false)&&!Access::failed(legacy)
            &&Access::resource(legacy)&&!Access::resource(legacy)->attdefModel&&optOut.sourceCalls==0u,
            "opt-out retains identity-only admission without frozen owners"))return false;
        ++profiles;
    }
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})
     for(bool binary:{false,true})for(bool block:{false,true}) {
        auto row=body(1u);
        const Tags common{{6,"DASHED"},{347,"123"},{62,"3"},{370,"35"},{48,"1.5"},{60,"1"},
            {420,"1193046"},{430,"common-color"},{440,"1234"},{390,"456"},{284,"2"},
            {version=="AC1021"?92:160,"3"},{310,"0102A3"}};
        auto marker=std::find_if(row.begin(),row.end(),[](const Tag& t){return t.code==101;});
        row.insert(marker+1,common.begin(),common.end());
        auto nestedText=std::find_if(row.begin(),row.end(),[](const Tag& t){return t.code==1&&t.value=="LAST-é";});
        row.insert(nestedText+1,{{90,"1"},{45,"1.5"},{63,"3"},{421,"1193046"},{431,"book$swatch"},{441,"1234"}});
        row.insert(row.begin()+4,common.begin(),common.end());
        if(!write(path,document(version,row,block),binary))return false;
        dx_data data;CaptureProbe reader;
        if(!check(reader.fileImport(path.string(),&data,false)&&!Access::failed(data)
            &&Access::resource(data)&&Access::resource(data)->attdefPairMatched&&Access::resource(data)->attdefFieldsMatched,
            "rich common/proxy/background modeled replay")) {
            std::cerr<<"version="<<version<<" binary="<<binary<<" block="<<block<<'\n';return false;
        }
        ++richProfiles;
    }
    for(bool binary:{false,true}) {
        if(!write(path,document("AC1015",body(0u),true),binary))return false;
        dx_data data;CaptureProbe reader;
        if(!check(reader.fileImport(path.string(),&data,false)&&reader.sourceCalls==0u,
            "legacy declared revisions remain on the existing opt-out path"))return false;
    }
    if(!write(path,document("AC1032",body(1u),true),false))return false;
    const auto faults=attdefSourceFieldFaults();
    for(const auto& fault:faults) {
        dx_data data;CaptureProbe reader;reader.alterTyped=fault.second;
        if(!reader.fileImport(path.string(),&data,false)||!Access::failed(data)
            ||(Access::resource(data)&&Access::resource(data)->attdefPairMatched)) {
            std::cerr<<"ATTDEF callback field escaped pairing: "<<fault.first<<'\n';return false;
        }
        reader.alterTyped={};dx_data fresh;
        if(!check(reader.fileImport(path.string(),&fresh,false)&&!Access::failed(fresh)
            &&Access::resource(fresh)->attdefFieldsMatched,"same adapter fresh-import recovery after field mutation"))return false;
        ++mutations;
    }
    for(unsigned action=0u;action<10u;++action) {
        dx_data data;CaptureProbe reader;
        switch(action) {
        case 0:reader.duplicateSource=true;break;
        case 1:reader.duplicateTyped=true;break;
        case 2:reader.skipTyped=true;break;
        case 3:reader.alterRaw=[](auto& raw){raw.name="LINE";};break;
        case 4:reader.alterRaw=[](auto& raw){++raw.handle;};break;
        case 5:reader.alterRaw=[](auto& raw){++raw.parentHandle;};break;
        case 6:reader.alterRaw=[](auto& raw){raw.rawValues.pop_back();};break;
        case 7:reader.alterRaw=[](auto& raw){raw.groups[0]=DRW_Variant(5,DRW_Coord{});};break;
        case 8:reader.alterContext=[](auto& context){context.inBlock=false;};break;
        case 9:reader.alterContext=[](auto& context){context.applyExtrusion=true;};break;
        }
        if(!check(reader.fileImport(path.string(),&data,false)&&Access::failed(data),
            "duplicate/missing/wrong-family/context capture cannot succeed as evidence"))return false;
        reader.duplicateSource=false;reader.duplicateTyped=false;reader.skipTyped=false;reader.alterRaw={};reader.alterContext={};
        dx_data fresh;
        if(!check(reader.fileImport(path.string(),&fresh,false)&&!Access::failed(fresh)
            &&Access::resource(fresh)->attdefFieldsMatched,"fresh-import callback recovery"))return false;
        ++callbacks;
    }
    const std::vector<std::pair<int,std::string>> rawFaults{
        {1,"default-é"},{2,"TAG"},{3,"prompt-é"},{1,"LAST-é"},
        {1,"not-mtext"},{1000,"opaque-\\U+00E9"},{1070,"11"},{10,"1.25"}};
    for(const auto& fault:rawFaults) {
        dx_data data;CaptureProbe reader;
        reader.alterRaw=[&](auto& raw) {
            bool changed=false;
            for(std::size_t i=0;i<raw.groups.size();++i) {
                auto& value=raw.groups[i];
                if(value.code()!=fault.first)continue;
                if(value.type()==DRW_Variant::STRING&&value.content.s&&*value.content.s==fault.second) {
                    value=DRW_Variant(value.code(),std::string("changed-source"));raw.rawValues[i]="changed-source";changed=true;break;
                }
                if(value.type()==DRW_Variant::INTEGER&&fault.second=="11"&&value.i_val()==11) {
                    value=DRW_Variant(value.code(),12);raw.rawValues[i]="12";changed=true;break;
                }
                if(value.type()==DRW_Variant::DOUBLE&&fault.second=="1.25"&&value.d_val()==1.25) {
                    value=DRW_Variant(value.code(),2.25);raw.rawValues[i]="2.25";changed=true;break;
                }
            }
            if(!changed)throw std::runtime_error("missing raw mutation target");
        };
        if(!check(reader.fileImport(path.string(),&data,false)&&Access::resource(data)->attdefPairMatched
            &&!Access::resource(data)->attdefFieldsMatched&&Access::resource(data)->attdefFieldMismatch,
            "consistent raw spelling/scalar mutation cannot pass modeled field replay"))return false;
        reader.alterRaw={};dx_data fresh;
        if(!check(reader.fileImport(path.string(),&fresh,false)&&Access::resource(fresh)->attdefFieldsMatched,
            "corrected raw-field replay on a fresh import"))return false;
        ++rawMutations;
    }
    { // The tolerant entity parser retains partial XDATA tuples. A copy
      // clears their cursor, which must never turn them into qualified input.
        auto partial=body(1u);
        partial.erase(std::remove_if(partial.begin(),partial.end(),[](const Tag& t) {
            return t.code==1030||t.code==1070;
        }),partial.end());
        if(!write(path,document("AC1032",partial,true),false))return false;
        dx_data data;CaptureProbe reader;
        if(!check(reader.fileImport(path.string(),&data,false)&&Access::resource(data)->attdefPairMatched
            &&!Access::resource(data)->attdefState.hasNoPendingXdata()
            &&dxfAttdef::Decoder::observe(*Access::resource(data)->attdefModel).hasNoPendingXdata()
            &&!Access::resource(data)->attdefFieldsMatched,
            "original partial-XDATA state is retained and cannot be laundered by a model copy"))return false;
        if(!write(path,document("AC1032",body(1u),true),false))return false;
    }
    { // Missing source remains identity-only; it cannot acquire pairing proof.
        dx_data data;CaptureProbe reader;reader.skipSource=true;
        if(!check(reader.fileImport(path.string(),&data,false)&&Access::resource(data)
            &&!Access::resource(data)->attdefPairMatched&&!Access::resource(data)->attdefModel,
            "missing source cannot be replaced with a typed-only frozen model"))return false;
    }
    { // A raw-only spelling edit is NOT validated by source/typed agreement.
        dx_data data;CaptureProbe reader;reader.alterRaw=[](auto& raw){raw.rawValues[0]="FFFF";};
        if(!check(reader.fileImport(path.string(),&data,false)&&Access::resource(data)->attdefPairMatched
            &&!Access::resource(data)->attdefFieldsMatched&&Access::failed(data)
            &&!Access::resource(data)->rawTypedMatched,"pairing cannot launder inconsistent raw spellings into replay proof"))return false;
    }
    { // A delivered source callback may throw, and later calls are out of session.
        dx_data data;CaptureProbe reader;reader.throwSource=true;
        bool accepted=false,threw=false;
        try {accepted=reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
        if(!check(!accepted&&threw&&data.dxfReadOutcome.readThrew,
            "throwing capture callback propagates while recording the failed import"))return false;
        reader.throwSource=false;dx_data fresh;
        if(!check(reader.fileImport(path.string(),&fresh,false)&&!Access::failed(fresh),"throw recovery on fresh import"))return false;
        const auto* row=Access::resource(fresh);
        reader.dx_iface::addDxfAttdefSource(*row->attdefModel,*row->raw,row->attdefContext);
        if(!check(Access::failed(fresh),"late source callback is sticky context failure"))return false;
    }
    for(unsigned kind=0u;kind<7u;++kind) {
        const auto attempt=[&](std::size_t limit) {
            dx_data data;Access::limits(data,kind,limit);CaptureProbe reader;
            return reader.fileImport(path.string(),&data,false)&&!Access::failed(data)
                &&Access::resource(data)&&Access::resource(data)->attdefFieldsMatched
                &&Access::resource(data)->attdefProfile.complete();
        };
        std::size_t high=1000000u,low=0u;
        if(!check(attempt(high)&&!attempt(low),"capture quota refuses and recovers"))return false;
        while(low+1u<high){const auto middle=low+(high-low)/2u;if(attempt(middle))high=middle;else low=middle;}
        if(!check(attempt(high)&&!attempt(high-1u),"exact/below source snapshot quota"))return false;
        ++quotas;
    }
    dx_data measured;CaptureProbe calibrate;calibrate.failSourceAllocation=(std::numeric_limits<long>::max)();
    if(!check(calibrate.fileImport(path.string(),&measured,false)&&!Access::failed(measured)
        &&calibrate.sourceAllocations>0&&calibrate.sourceAllocations<2000,"capture allocation calibration"))return false;
    for(long failure=0;failure<calibrate.sourceAllocations;++failure) {
        dx_data data;CaptureProbe reader;reader.failSourceAllocation=failure;
        if(!check(reader.fileImport(path.string(),&data,false)&&Access::failed(data),"snapshot allocation cannot create successful evidence"))return false;
        reader.failSourceAllocation=-1;dx_data fresh;
        if(!check(reader.fileImport(path.string(),&fresh,false)&&!Access::failed(fresh),"snapshot allocation fresh-import recovery"))return false;
    }
    dx_data replayMeasured;CaptureProbe replayCalibrate;
    replayCalibrate.failTypedAllocation=(std::numeric_limits<long>::max)();
    if(!check(replayCalibrate.fileImport(path.string(),&replayMeasured,false)&&!Access::failed(replayMeasured)
        &&!replayMeasured.typedEntityCaptureFailed&&Access::resource(replayMeasured)->attdefFieldsMatched
        &&Access::resource(replayMeasured)->attdefProfile.complete()
        &&replayCalibrate.typedAllocations>0&&replayCalibrate.typedAllocations<2000,
        "typed pairing/replay/consumer allocation calibration"))return false;
    for(long failure=0;failure<replayCalibrate.typedAllocations;++failure) {
        dx_data data;CaptureProbe reader;reader.failTypedAllocation=failure;
        if(!check(reader.fileImport(path.string(),&data,false)
            &&(Access::failed(data)||data.typedEntityCaptureFailed||!Access::resource(data)->attdefFieldsMatched
                ||!Access::resource(data)->attdefProfile.complete()),
            "pairing/replay/consumer allocation failure cannot yield complete evidence and retained consumer"))return false;
        reader.failTypedAllocation=-1;dx_data fresh;
        if(!check(reader.fileImport(path.string(),&fresh,false)&&!Access::failed(fresh)
            &&!fresh.typedEntityCaptureFailed&&Access::resource(fresh)->attdefFieldsMatched
            &&Access::resource(fresh)->attdefProfile.complete(),
            "typed/replay allocation fresh-import recovery"))return false;
    }
    std::cout<<"ATTDEF immutable capture profiles="<<profiles<<"; rich profiles="<<richProfiles<<"; field faults/retries="<<mutations
        <<"; raw field faults/retries="<<rawMutations<<"; callback faults/retries="<<callbacks<<"; quota boundaries="<<quotas
        <<"; source allocation faults/retries="<<calibrate.sourceAllocations
        <<"; typed/replay allocation faults/retries="<<replayCalibrate.typedAllocations<<'\n';
    return profiles==48u&&richProfiles==16u&&mutations==faults.size()&&callbacks==10u&&quotas==7u;
}
}
int main(int argc,char** argv) {
    const bool budget=argc==2&&std::string(argv[1])=="--nested-common-budget";
    const bool capture=argc==2&&std::string(argv[1])=="--immutable-capture";
    const bool profile=argc==2&&std::string(argv[1])=="--source-profile";
    const bool legacyObservation=argc==2&&std::string(argv[1])=="--legacy-observation";
    const bool legacyAdapterCapture=argc==2&&std::string(argv[1])=="--legacy-adapter-capture";
    const bool equalAlignment=argc==2&&std::string(argv[1])=="--equal-alignment";
    const bool dualEmbedded=argc==2&&std::string(argv[1])=="--embedded-dual280";
    const bool blocks=argc==2&&std::string(argv[1])=="--block-source";
    const bool blockProof=argc==2&&std::string(argv[1])=="--block-proof";
    const bool blockChildren=argc==2&&std::string(argv[1])=="--block-children";
    const bool rawIdentity=argc==2&&std::string(argv[1])=="--raw-identity";
    const bool rawMissing=argc==2&&std::string(argv[1])=="--raw-identity-missing";
    const bool proxyScope=argc==2&&std::string(argv[1])=="--proxy-scope";
    const bool rawWriter=argc==2&&std::string(argv[1])=="--raw-writer";
    const bool rawShell=argc==2&&std::string(argv[1])=="--raw-shell";
    const bool acshScope=argc==2&&std::string(argv[1])=="--acsh-scope";
    const bool blockDelivery=argc==2&&std::string(argv[1])=="--block-delivery";
    const bool polymorphic=argc==2&&std::string(argv[1])=="--block-polymorphic";
    const bool attdefOrigin=argc==2&&std::string(argv[1])=="--attdef-origin";
    const bool blockMembership=argc==2&&std::string(argv[1])=="--block-membership";
    const bool compoundBody=argc==2&&std::string(argv[1])=="--compound-body";
    const bool compoundProfile=argc==2&&std::string(argv[1])=="--compound-profile";
    const bool applicationWidth=argc==2&&std::string(argv[1])=="--application-width";
    const bool blockPrimary=argc==2&&std::string(argv[1])=="--block-primary";
    const bool blockOrigins=argc==2&&std::string(argv[1])=="--block-origins";
    const bool emptyBlockPolyline=argc==2&&std::string(argv[1])=="--empty-block-polyline";
    if(argc!=1&&!budget&&!capture&&!profile&&!legacyObservation&&!legacyAdapterCapture&&!equalAlignment&&!dualEmbedded&&!blocks&&!blockProof&&!blockChildren&&!rawIdentity&&!rawMissing&&!proxyScope&&!rawWriter&&!rawShell&&!acshScope&&!blockDelivery&&!polymorphic&&!attdefOrigin&&!blockMembership&&!compoundBody&&!compoundProfile&&!applicationWidth&&!blockPrimary&&!blockOrigins&&!emptyBlockPolyline)return 2;
    std::error_code error;const auto parent=std::filesystem::temp_directory_path();
    const auto stem="libdxfrw-attdef-source-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto scratch=libdxfrw_test::reserveTestScratch(parent,stem,error);
    if(scratch.empty()){std::cerr<<"Cannot reserve test scratch "<<(parent/stem)<<": "<<error.message()<<'\n';return 1;}
    struct Cleanup {std::filesystem::path path;bool success=false;~Cleanup(){if(!success){std::cerr<<"Evidence: "<<path<<'\n';return;}std::error_code ignored;std::filesystem::remove_all(path,ignored);}} cleanup{scratch};
    unsigned polymorphicProfiles=0u;
    try {const bool ok=(emptyBlockPolyline?emptyBlockPolylineTests(scratch/"generated.dxf"):
        blockOrigins?blockOriginTests(scratch/"generated.dxf"):
        blockPrimary?blockPrimaryTests(scratch/"generated.dxf"):
        polymorphic?blockPolymorphicDeliveryTests(scratch/"generated.dxf",polymorphicProfiles):
        attdefOrigin?attdefOriginTests(scratch/"generated.dxf"):
        blockMembership?blockMembershipTests(scratch/"generated.dxf"):
        compoundBody?compoundBodyTests(scratch/"generated.dxf"):
        compoundProfile?compoundSourceProfileTests(scratch/"generated.dxf"):
        applicationWidth?applicationHandleWidthTests(scratch/"generated.dxf"):
        (rawIdentity||rawMissing)?rawApplicationIdentityTests(scratch/"generated.dxf",rawMissing):
        blockDelivery?blockDeliveryTests(scratch/"generated.dxf"):
        rawShell?rawShellScopeTests(scratch/"generated.dxf"):
        acshScope?acshProjectionScopeTests(scratch/"generated.dxf"):
        rawWriter?rawApplicationWriterTests(scratch/"generated.dxf"):
        proxyScope?rawProxyScopeTests(scratch/"generated.dxf"):
        blockChildren?blockChildrenSourceTests(scratch/"generated.dxf"):
        blockProof?blockSourceProofTests(scratch/"generated.dxf"):
        blocks?blockSourceTests(scratch/"generated.dxf"):
        dualEmbedded?embeddedDual280Tests(scratch/"generated.dxf"):
        equalAlignment?equalAlignmentTests(scratch/"generated.dxf"):
        legacyAdapterCapture?legacyAttdefAdapterCaptureTests(scratch/"generated.dxf"):
        legacyObservation?legacyAttdefObservationTests(scratch/"generated.dxf"):
        profile?sourceProfileTests(scratch/"generated.dxf"):
        capture?immutableCaptureTests(scratch/"generated.dxf"):
        budget?(nestedCommonBudgetTests()&&nestedCommonImportTests(scratch/"generated.dxf"))
        :tests(scratch/"generated.dxf"));cleanup.success=ok;return ok?0:1;}
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
