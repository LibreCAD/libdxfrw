// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFBLOCKCHILDREN_H
#define DXFBLOCKCHILDREN_H
#include "drw_interface.h"
#include "dxfreader.h"
#include <limits>

namespace dxfBlockChildren {
// Captures the original scalar stream, including records consumed by compound
// parsers. It never reconstructs identities, children or roles from callbacks.
class Capture final:public DxfReadRecordObserver {
    dxfReader& reader;
    DxfReadRecordObserver* previous{nullptr};
    bool attached{false};
    std::shared_ptr<DxfReadRetainedLease> lease;
    DRW_DxfBlockChildrenSourceLimits limits;
    DRW_DxfBlockChildrenSource result;
    std::size_t nextOrdinal{0},lastOffset{0};
    using Status=DRW_DxfBlockChildrenSource::Status;
    static bool keyword(std::string_view value,std::string_view expected) noexcept {
        if(value.size()!=expected.size())return false;
        for(std::size_t i=0;i<value.size();++i) {
            const char c=value[i]>='a'&&value[i]<='z'?static_cast<char>(value[i]-'a'+'A'):value[i];
            if(c!=expected[i])return false;
        }
        return true;
    }
    static DRW_DxfPhysicalRecordSpan span(const DxfReadRecordView& v) noexcept {
        return {v.ordinal,v.beginOffset,v.endOffset,v.ignoredComment};
    }
    bool charge(std::size_t bytes) {
        if(bytes>(std::numeric_limits<std::size_t>::max)()-64u
            ||result.bytes>limits.bytes||bytes>limits.bytes-result.bytes
            ||result.work>limits.work||bytes+64u>limits.work-result.work) {
            result.status=Status::Quota;return false;
        }
        result.bytes+=bytes;result.work+=bytes+64u;
        if(!retainDxfReadBytes(lease,reader.retainedByteBudget(),bytes)) {result.status=Status::Quota;return false;}
        return true;
    }
    void accept(const DxfReadRecordView& v) {
        if(result.status!=Status::Collecting)return;
        if(v.ordinal!=nextOrdinal||v.beginOffset<lastOffset||v.endOffset<=v.beginOffset
            ||v.ordinal==(std::numeric_limits<std::size_t>::max)()) {result.status=Status::InvalidBoundary;return;}
        nextOrdinal=v.ordinal+1u;lastOffset=v.endOffset;
        if(v.code==0) {
            if(v.kind!=DxfReadRecordView::Text||v.ignoredComment) {result.status=Status::InvalidBoundary;return;}
            if(!result.records.empty()) {
                auto& record=result.records.back();record.endOrdinal=v.ordinal;record.endOffset=v.beginOffset;
            }
            if(keyword(v.text,"ENDBLK")) {result.endMarker=span(v);result.status=Status::Complete;return;}
            if(keyword(v.text,"ENDSEC")||keyword(v.text,"EOF")||keyword(v.text,"SECTION")||keyword(v.text,"BLOCK")
                ||v.text.empty()) {result.status=Status::InvalidBoundary;return;}
            if(result.records.size()>=limits.records||v.text.size()>(std::numeric_limits<std::size_t>::max)()-sizeof(DRW_DxfPhysicalEntitySource)
                ||!charge(sizeof(DRW_DxfPhysicalEntitySource)+v.text.size())) {result.status=Status::Quota;return;}
            DRW_DxfPhysicalEntitySource record;record.name=v.text;record.marker=span(v);
            result.records.push_back(std::move(record));return;
        }
        if(result.records.empty()) {result.status=Status::InvalidBoundary;return;}
        constexpr auto overhead=sizeof(DRW_Variant)+sizeof(UTF8STRING)+sizeof(DRW_DxfPhysicalRecordSpan);
        const auto textBytes=v.kind==DxfReadRecordView::Text||v.kind==DxfReadRecordView::Binary?v.text.size():0u;
        const auto rawBytes=result.binary?0u:v.raw.size();
        const auto max=(std::numeric_limits<std::size_t>::max)();
        if(result.groups>=limits.groups||textBytes>max-overhead||rawBytes>max-overhead-textBytes
            ||!charge(overhead+textBytes+rawBytes)||!reader.chargeTypedVariantNode()) {result.status=Status::Quota;return;}
        ++result.groups;if(v.ignoredComment)++result.ignoredComments;
        auto& record=result.records.back();
        switch(v.kind) {
        case DxfReadRecordView::Integer:record.groups.emplace_back(v.code,v.integer);break;
        case DxfReadRecordView::WideInteger:record.groups.emplace_back(v.code,v.wide);break;
        case DxfReadRecordView::Real:record.groups.emplace_back(v.code,v.real);break;
        default:record.groups.emplace_back(v.code,std::string(v.text));break;
        }
        record.rawValues.emplace_back(result.binary?std::string{}:std::string(v.raw));
        record.spans.push_back(span(v));
    }
public:
    explicit Capture(dxfReader& value) noexcept:reader(value){}
    ~Capture() override {detach();}
    Capture(const Capture&)=delete;
    Capture& operator=(const Capture&)=delete;
    void start(std::uint32_t block,std::uint32_t owner,DRW_DxfBlockChildrenSourceLimits cap) noexcept {
        if(attached||result.status!=Status::NotStarted) {result.status=Status::InvalidBoundary;return;}
        limits=cap;result.version=reader.getSourceVersion();result.binary=reader.isBinary();
        result.blockHandle=block;result.blockRecord=owner;result.status=Status::Collecting;
        previous=reader.exchangeRecordObserver(this);attached=true;
        try {
            DxfReadRecordView first;
            if(!reader.currentRecordView(first)||first.code!=0) {result.status=Status::InvalidBoundary;return;}
            // setEntityBoundary transfers strData into nextentity. The raw
            // code-0 spelling remains owned by the reader in both encodings;
            // seed from that original carrier, never an effective callback.
            first.text=first.raw;
            nextOrdinal=first.ordinal;lastOffset=first.beginOffset;
            if(charge(sizeof(DRW_DxfBlockChildrenSource)+sizeof(DxfReadRetainedLease)+8u*sizeof(void*)))accept(first);
        } catch(...) {result.status=Status::Allocation;}
    }
    void detach() noexcept {if(attached){reader.exchangeRecordObserver(previous);attached=false;}}
    void observe(const DxfReadRecordView& v) noexcept override {
        try {accept(v);}catch(...) {result.status=Status::Allocation;}
        if(previous)previous->observe(v); // first pending marker was already observed
    }
    const DRW_DxfBlockChildrenSource& source() const noexcept {return result;}
};
}
#endif
