// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFIDENTITYSOURCE_H
#define DXFIDENTITYSOURCE_H
#include "drw_interface.h"
#include "dxfreader.h"

namespace dxfIdentitySource {
// A structural observer of the actual whole-read scalar stream, not a second
// payload parser. It preserves ambiguous candidates instead of guessing roles.
class Capture final:public DxfReadRecordObserver {
    using Source=DRW_DxfIdentitySource;using S=Source::Status;
    static constexpr auto None=Source::None;
    dxfReader& reader;Source result;DRW_DxfIdentitySourceLimits limits;
    DxfReadRecordObserver* previous{nullptr};bool attached{false};
    std::shared_ptr<DxfReadRetainedLease> lease;
    enum class State {Outside,SectionName,Body,Ended} state{State::Outside};
    std::size_t section{None},table{None},record{None},depth{0},nextOrdinal{1},lastOffset{0};
    bool xdata{false},embedded{false},xrecordBody{false};DRW_DxfPhysicalRecordSpan sectionMarker;
    static bool equal(std::string_view a,std::string_view b) noexcept {
        if(a.size()!=b.size())return false;
        for(std::size_t i=0;i<a.size();++i) {
            const char c=a[i]>='a'&&a[i]<='z'?static_cast<char>(a[i]-'a'+'A'):a[i];
            if(c!=b[i])return false;
        }
        return true;
    }
    static DRW_DxfPhysicalRecordSpan span(const DxfReadRecordView& v) noexcept {
        return {v.ordinal,v.beginOffset,v.endOffset,v.ignoredComment};
    }
    bool charge(std::size_t bytes,std::size_t work) {
        if(result.bytes>limits.bytes||bytes>limits.bytes-result.bytes
            ||result.work>limits.work||work>limits.work-result.work) {result.status=S::Quota;return false;}
        result.bytes+=bytes;result.work+=work;
        if(!retainDxfReadBytes(lease,reader.retainedByteBudget(),bytes)) {result.status=S::Quota;return false;}
        return true;
    }
    bool textCharge(std::size_t overhead,std::string_view a,std::string_view b={}) {
        const auto max=(std::numeric_limits<std::size_t>::max)();
        if(a.size()>max-overhead||b.size()>max-overhead-a.size()) {result.status=S::Quota;return false;}
        const auto bytes=overhead+a.size()+b.size();return charge(bytes,bytes);
    }
    void closeRecord(const DxfReadRecordView& v) {
        if(depth) {result.status=S::InvalidBoundary;return;}
        if(record!=None) {
            auto& row=result.records[record];row.endMarker=span(v);
            if(row.kind==Source::Kind::TableControl&&row.tableName.empty())result.status=S::InvalidBoundary;
        }
        record=None;depth=0;xdata=false;embedded=false;xrecordBody=false;
    }
    void marker(const DxfReadRecordView& v) {
        if(v.kind!=DxfReadRecordView::Text||v.text.empty()) {result.status=S::InvalidBoundary;return;}
        if(state==State::Outside) {
            if(equal(v.text,"SECTION")) {sectionMarker=span(v);state=State::SectionName;return;}
            if(equal(v.text,"EOF")) {result.eofMarker=span(v);state=State::Ended;return;}
            result.status=S::InvalidBoundary;return;
        }
        if(state!=State::Body) {result.status=S::InvalidBoundary;return;}
        closeRecord(v);if(result.status!=S::Collecting)return;
        if(equal(v.text,"ENDSEC")) {
            if(table!=None) {result.status=S::InvalidBoundary;return;}
            result.sections[section].endMarker=span(v);section=None;state=State::Outside;return;
        }
        if(equal(v.text,"SECTION")||equal(v.text,"EOF")) {result.status=S::InvalidBoundary;return;}
        const auto kind=result.sections[section].kind;
        if(kind==Source::Section::Header) {result.status=S::InvalidBoundary;return;}
        if(kind==Source::Section::Classes)return; // CLASS is metadata, never a database object.
        Source::Kind recordKind=Source::Kind::Unclassified;
        if(kind==Source::Section::Tables) {
            if(equal(v.text,"ENDTAB")) {
                if(table==None)result.status=S::InvalidBoundary;
                table=None;return;
            }
            if(equal(v.text,"TABLE")) {
                if(table!=None) {result.status=S::InvalidBoundary;return;}
                recordKind=Source::Kind::TableControl;
            } else {
                if(table==None) {result.status=S::InvalidBoundary;return;}
                recordKind=Source::Kind::TableRecord;
            }
        } else if(kind==Source::Section::Blocks||kind==Source::Section::Entities)recordKind=Source::Kind::Entity;
        else if(kind==Source::Section::Objects)recordKind=Source::Kind::Object;
        if(result.records.size()>=limits.records) {result.status=S::Quota;return;}
        if(!textCharge(sizeof(Source::Record),v.text))return;
        Source::Record row;row.kind=recordKind;row.name=v.text;row.section=section;row.table=table;
        row.marker=span(v);row.firstCandidate=result.candidates.size();record=result.records.size();
        if(recordKind==Source::Kind::TableControl) {table=record;row.table=table;}
        result.records.push_back(std::move(row));
    }
    void accept(const DxfReadRecordView& v) {
        if(result.status!=S::Collecting)return;
        if(v.ordinal!=nextOrdinal||v.ordinal==(std::numeric_limits<std::size_t>::max)()
            ||v.beginOffset<lastOffset||v.endOffset<=v.beginOffset) {result.status=S::InvalidBoundary;return;}
        nextOrdinal=v.ordinal+1u;lastOffset=v.endOffset;
        if(!charge(0u,64u)||!charge(0u,v.text.size())||!charge(0u,v.raw.size()))return;
        ++result.scalars;
        if(v.ignoredComment||v.code==999) {++result.ignoredComments;return;}
        if(v.code==0) {marker(v);return;}
        if(state==State::SectionName) {
            if(v.code!=2||v.kind!=DxfReadRecordView::Text||v.text.empty()) {result.status=S::InvalidBoundary;return;}
            if(result.sections.size()>=limits.sections) {result.status=S::Quota;return;}
            if(!textCharge(sizeof(Source::SectionSource),v.text))return;
            Source::SectionSource row;row.name=v.text;row.marker=sectionMarker;
            if(equal(v.text,"HEADER"))row.kind=Source::Section::Header;
            else if(equal(v.text,"CLASSES"))row.kind=Source::Section::Classes;
            else if(equal(v.text,"TABLES"))row.kind=Source::Section::Tables;
            else if(equal(v.text,"BLOCKS"))row.kind=Source::Section::Blocks;
            else if(equal(v.text,"ENTITIES"))row.kind=Source::Section::Entities;
            else if(equal(v.text,"OBJECTS"))row.kind=Source::Section::Objects;
            section=result.sections.size();result.sections.push_back(std::move(row));state=State::Body;return;
        }
        if(state!=State::Body) {result.status=S::InvalidBoundary;return;}
        const auto kind=result.sections[section].kind;
        if(kind==Source::Section::Header||kind==Source::Section::Classes)return;
        if(xrecordBody) {
            // XRECORD 102 and 100 are payload values; they do not open or
            // close outer application/subclass scopes. Its first 1001 starts
            // trailing XDATA, which the typed decoder requires to remain in
            // the XDATA code range until the next record marker.
            if(v.code==1001)xdata=true;
            if(xdata&&v.code<1000) {result.status=S::InvalidBoundary;return;}
            if(v.code!=5&&v.code!=105)return;
        } else if(record!=None&&kind==Source::Section::Objects
            &&equal(result.records[record].name,"XRECORD")&&!depth&&!xdata&&!embedded
            &&v.code==100&&v.kind==DxfReadRecordView::Text&&v.text=="AcDbXrecord") {
            xrecordBody=true;return;
        }
        if(record!=None&&result.records[record].kind==Source::Kind::TableControl&&v.code==2&&depth==0&&!xdata&&!embedded) {
            auto& name=result.records[record].tableName;
            if(!name.empty()||v.text.empty()||!textCharge(0u,v.text)) {if(result.status==S::Collecting)result.status=S::InvalidBoundary;return;}
            name=v.text;
        }
        if(!xrecordBody&&v.code==102) {
            if(v.text=="}") {if(!depth)result.status=S::InvalidBoundary;else --depth;}
            else if(!v.text.empty()&&v.text.front()=='{') {
                if(depth==(std::numeric_limits<std::size_t>::max)())result.status=S::InvalidBoundary;else ++depth;
            }
            return;
        }
        if(!xrecordBody&&!depth&&v.code==1001)xdata=true;
        if(!xrecordBody&&!depth&&v.code==101)embedded=true;
        if(v.code!=5&&v.code!=105)return;
        if(result.candidates.size()>=limits.candidates) {result.status=S::Quota;return;}
        if(!textCharge(sizeof(Source::Candidate),v.text,result.binary?std::string_view{}:v.raw))return;
        Source::Candidate candidate;candidate.record=record;candidate.section=section;candidate.code=v.code;
        candidate.scope=record==None?Source::Scope::Unframed:xrecordBody?Source::Scope::OpaqueBody:
            depth?Source::Scope::Application:embedded?Source::Scope::Embedded:xdata?Source::Scope::Xdata:Source::Scope::Outer;
        candidate.lexeme=v.text;if(!result.binary)candidate.rawValue=v.raw;candidate.span=span(v);
        result.candidates.push_back(std::move(candidate));if(record!=None)++result.records[record].candidates;
    }
public:
    explicit Capture(dxfReader& value) noexcept:reader(value){}
    ~Capture() override {if(attached)reader.exchangeIdentityObserver(previous);}
    Capture(const Capture&)=delete;Capture& operator=(const Capture&)=delete;
    void start(DRW_DxfIdentitySourceLimits cap) noexcept {
        if(attached||result.status!=S::NotStarted) {result.status=S::InvalidBoundary;return;}
        limits=cap;result.binary=reader.isBinary();result.status=S::Collecting;
        previous=reader.exchangeIdentityObserver(this);attached=true;
        try {charge(sizeof(Source)+sizeof(DxfReadRetainedLease)+8u*sizeof(void*),64u);}
        catch(...) {result.status=S::Allocation;}
    }
    void observe(const DxfReadRecordView& v) noexcept override {
        try {accept(v);}catch(...) {result.status=S::Allocation;}
        if(previous)previous->observe(v);
    }
    void selfHandle(std::size_t ordinal,Admission admission) noexcept override {
        if(result.status==S::Collecting) {
            if(result.candidates.empty()||result.candidates.back().span.ordinal!=ordinal)++result.unmatchedAdmissions;
            else {
                auto& c=result.candidates.back();using A=Source::Admission;
                switch(admission) {
                case Admission::Accepted:c.admission=A::Accepted;break;
                case Admission::Null:c.admission=A::Null;break;
                case Admission::Duplicate:c.admission=A::Duplicate;break;
                case Admission::Invalid:c.admission=A::Invalid;break;
                case Admission::Repeated:++c.repeatedAdmissions;break;
                }
            }
        }
        if(previous)previous->selfHandle(ordinal,admission);
    }
    void finish() noexcept {
        result.version=reader.getSourceVersion();
        if(result.status==S::Collecting)result.status=state==State::Ended&&result.eofMarker.ordinal?S::Complete:S::InvalidBoundary;
    }
    const Source& source() const noexcept {return result;}
};
}
#endif
