#pragma once
// Internal opt-in source observation for decoder clients. Captures ordered
// decoded scalar values, value spellings where available and physical spans;
// this is not a byte-exact file image or a dependency/semantic certificate.
// A read-bound synchronous ticket authenticates the delivered copy against a
// private codec owner. Only a successful whole read can publish a completion.
// Tickets and completion tokens are non-transferable borrows valid only during
// their respective synchronous callback; neither may be retained afterward.
#include "intern/dxfreader.h"
#include <limits>
#include <memory>
#include <new>
#include <cstring>
#include <vector>
#include <string_view>
class dxfRW;
namespace WholeSourceV2 {
inline bool sum(std::size_t a,std::size_t b,std::size_t& out) noexcept {
    if(a>(std::numeric_limits<std::size_t>::max)()-b)return false;out=a+b;return true;
}
inline bool product(std::size_t a,std::size_t b,std::size_t& out) noexcept {
    if(b&&a>(std::numeric_limits<std::size_t>::max)()/b)return false;out=a*b;return true;
}
struct Limits {std::size_t fields=1000000u,bytes=64u*1024u*1024u,work=128u*1024u*1024u;};
enum class State {Collecting,ThroughEof,Quota,Allocation,Invalid};
struct Pool {
    std::size_t bytes=0,peak=0,work=0;Limits cap;bool quota=false;
    std::shared_ptr<DxfReadRetainedLease> lease;
    std::shared_ptr<DxfReadRetainedBudget> sharedBudget;
    explicit Pool(Limits l,std::shared_ptr<DxfReadRetainedBudget> b):cap(l),sharedBudget(std::move(b)){}
    bool take(std::size_t amount) {
        if(quota)return false;
        std::size_t next=0;
        if(!sum(bytes,amount,next)||next>cap.bytes){quota=true;return false;}
        if(!retainDxfReadBytes(lease,sharedBudget,amount)){quota=true;return false;}
        bytes=next;if(peak<bytes)peak=bytes;return true;
    }
    void give(std::size_t amount) noexcept {
        if(amount>bytes)std::terminate();bytes-=amount;
        if(lease && !lease->resizeTo(bytes))std::terminate();
    }
    bool chargeWork(std::size_t amount) noexcept {
        if(quota)return false;
        std::size_t next=0;if(!sum(work,amount,next)||next>cap.work){quota=true;return false;}
        work=next;return true;
    }
};
template<typename T> struct Alloc {
    using value_type=T;
private:
    std::shared_ptr<Pool> pool;template<typename>friend struct Alloc;
public:
    explicit Alloc(std::shared_ptr<Pool> p):pool(std::move(p)){}
    template<typename U> Alloc(const Alloc<U>& other) noexcept:pool(other.pool){}
    T* allocate(std::size_t n) {
        std::size_t bytes=0;if(!product(n,sizeof(T),bytes)||!pool->take(bytes)){pool->quota=true;throw std::bad_alloc();}
        try{return static_cast<T*>(::operator new(bytes));}catch(...){pool->give(bytes);throw;}
    }
    void deallocate(T* p,std::size_t n) noexcept {::operator delete(p);pool->give(n*sizeof(T));}
    template<typename U>bool operator==(const Alloc<U>& o)const noexcept{return pool==o.pool;}
    template<typename U>bool operator!=(const Alloc<U>& o)const noexcept{return !(*this==o);}
};
struct Field {
    DxfReadRecordView::Kind kind;int code;std::size_t ordinal,begin,end,valueOffset,valueSize,rawOffset,rawSize;
    std::int32_t integer;std::int64_t wide;double real;bool ignored;
};
struct Source {
private:
    std::shared_ptr<Pool> pool;
    std::vector<Field,Alloc<Field>> fields;std::vector<char,Alloc<char>> bytes;
    State m_state=State::Collecting;DRW::Version m_version=DRW::UNKNOWNV;bool m_binary=false;
    friend class Capture;friend class Ticket;
public:
    State state()const noexcept{return m_state;}
    DRW::Version version()const noexcept{return m_version;}
    bool binary()const noexcept{return m_binary;}
    explicit Source(std::shared_ptr<Pool> p):pool(std::move(p)),fields(Alloc<Field>(pool)),bytes(Alloc<char>(pool)){}
    std::size_t peakBytes()const noexcept{return pool->peak;}
    std::size_t workUsed()const noexcept{return pool->work;}
    std::size_t fieldCount()const noexcept{return fields.size();}
    Field field(std::size_t index)const{return fields.at(index);}
    bool bounds(const Field& f)const noexcept {
        return f.valueOffset<=bytes.size()&&f.valueSize<=bytes.size()-f.valueOffset
            &&f.rawOffset<=bytes.size()&&f.rawSize<=bytes.size()-f.rawOffset;
    }
    std::string_view value(const Field& f)const noexcept {
        return bounds(f)&&f.valueSize?std::string_view(bytes.data()+f.valueOffset,f.valueSize):std::string_view{};
    }
    std::string_view raw(const Field& f)const noexcept {
        return bounds(f)&&f.rawSize?std::string_view(bytes.data()+f.rawOffset,f.rawSize):std::string_view{};
    }
};
// This separate opt-in interface is experimental and does not alter any
// existing DRW_Interface virtual slot or require existing subclasses to change.
class Ticket;
class FinishToken;
class Handler {
public:
    virtual ~Handler()=default;
    virtual bool wantsWholeSource() const noexcept {return false;}
    virtual Limits wholeSourceLimits() const noexcept {return {};}
    virtual void wholeSourceBegin(const void* reader) noexcept=0;
    virtual bool wholeSourceSelected(const void* reader) const noexcept=0;
    virtual void wholeSourcePending(const void* reader,const Source&,const Ticket&) noexcept=0;
    virtual void wholeSourceFinished(const void* reader,const FinishToken&) noexcept=0;
};
class FinishToken {
    friend class Completion;const void* reader;bool valid;bool active=true;
    FinishToken(const void* r,bool v):reader(r),valid(v){}
public:
    FinishToken(const FinishToken&)=delete;FinishToken& operator=(const FinishToken&)=delete;
    FinishToken(FinishToken&&)=delete;FinishToken& operator=(FinishToken&&)=delete;
    bool validFor(const void* r)const noexcept{return active&&valid&&r==reader;}
};
class Completion final {
    friend class ::dxfRW;
    Handler* handler;const void* reader;bool active;
    Completion(Handler* h,const void* r):handler(h),reader(r),active(h&&h->wantsWholeSource()) {if(active)handler->wholeSourceBegin(reader);}
    Completion(const Completion&)=delete;Completion& operator=(const Completion&)=delete;
    Completion(Completion&&)=delete;Completion& operator=(Completion&&)=delete;
    ~Completion(){finish(false);}
    void finish(bool valid) noexcept {if(active){active=false;FinishToken t(reader,valid);handler->wholeSourceFinished(reader,t);t.active=false;}}
};
class Ticket {
    friend class Capture;const Source* original;std::shared_ptr<Pool> pool;const void* reader;bool active=true;
    Ticket(const Source* s,std::shared_ptr<Pool> p,const void* r):original(s),pool(std::move(p)),reader(r){}
    std::shared_ptr<const Source> clone(const Source& s) const {
        std::size_t fields=0,work=0;
        if(!product(s.fields.size(),sizeof(Field),fields)||!sum(fields,s.bytes.size(),work)
            ||!pool->chargeWork(work)||!pool->take(sizeof(Source)+8u*sizeof(void*)))return {};
        auto copy=std::make_shared<Source>(pool);copy->m_state=s.m_state;copy->m_version=s.m_version;copy->m_binary=s.m_binary;
        copy->fields.assign(s.fields.begin(),s.fields.end());copy->bytes.assign(s.bytes.begin(),s.bytes.end());return copy;
    }
public:
    Ticket(const Ticket&)=delete;Ticket& operator=(const Ticket&)=delete;
    Ticket(Ticket&&)=delete;Ticket& operator=(Ticket&&)=delete;
    // The callback can compare and obtain a fresh, independent private copy
    // only during this event. Any allocation/quota failure invalidates proof.
    bool validFor(const void* r)const noexcept{return active&&reader&&reader==r;}
    std::shared_ptr<const Source> verifiedCopy(const Source& candidate,const void* r)const noexcept {
        try {
            if(!validFor(r)||!original||candidate.m_state!=original->m_state||candidate.m_version!=original->m_version
                ||candidate.m_binary!=original->m_binary||candidate.fields.size()!=original->fields.size()
                ||candidate.bytes.size()!=original->bytes.size())return {};
            std::size_t bytes=0,work=0;
            if(!product(original->fields.size(),sizeof(Field),bytes)||!sum(bytes,original->bytes.size(),work)
                ||!pool->chargeWork(work))return {};
            for(std::size_t i=0;i<original->fields.size();++i) {
                const auto& a=original->fields[i];const auto& b=candidate.fields[i];
                if(a.kind!=b.kind||a.code!=b.code||a.ordinal!=b.ordinal||a.begin!=b.begin||a.end!=b.end
                    ||a.valueOffset!=b.valueOffset||a.valueSize!=b.valueSize||a.rawOffset!=b.rawOffset||a.rawSize!=b.rawSize
                    ||a.integer!=b.integer||a.wide!=b.wide||std::memcmp(&a.real,&b.real,sizeof(double))!=0||a.ignored!=b.ignored)return {};
            }
            if(original->bytes!=candidate.bytes)return {};
            return clone(*original);
        }catch(...){return {};}
    }
};
class Capture final:public DxfReadRecordObserver {
    friend class ::dxfRW;
    dxfReader& reader;DxfReadRecordObserver* previous=nullptr;bool attached=false;
    std::shared_ptr<Source> source;std::shared_ptr<Pool> pool;std::size_t next=1,lastEnd=0;
    template<typename V> bool reserveFor(V& v,std::size_t needed) {
        if(needed<=v.capacity())return true;
        const auto max=(std::numeric_limits<std::size_t>::max)();std::size_t desired=v.capacity()?v.capacity():1;
        while(desired<needed){if(desired>max/2u){desired=needed;break;}desired*=2u;}
        if(desired>v.max_size()){pool->quota=true;return false;}
        std::size_t work=0;if(!product(v.size(),sizeof(typename V::value_type),work)||!pool->chargeWork(work))return false;
        v.reserve(desired);return true;
    }
    explicit Capture(dxfReader& r):reader(r){}
    Capture(const Capture&)=delete;Capture& operator=(const Capture&)=delete;
    Capture(Capture&&)=delete;Capture& operator=(Capture&&)=delete;
    ~Capture() override {if(attached)reader.exchangeIdentityObserver(previous);}
    void start(Limits cap) noexcept {
        try {
            pool=std::make_shared<Pool>(cap,reader.retainedByteBudget());
            // Fixed logical charge for shared control blocks and source owner.
            if(!pool->take(sizeof(Source)+sizeof(Pool)+8u*sizeof(void*)))return;
            source=std::make_shared<Source>(pool);source->m_binary=reader.isBinary();
            previous=reader.exchangeIdentityObserver(this);attached=true;
        }catch(...){source.reset();}
    }
    void observe(const DxfReadRecordView& v) noexcept override {
        try{collect(v);}catch(...){if(source)source->m_state=pool&&pool->quota?State::Quota:State::Allocation;}
        if(previous)previous->observe(v);
    }
    void selfHandle(std::size_t n,Admission a) noexcept override {if(previous)previous->selfHandle(n,a);}
    void finish(Handler& h,const void* identity) noexcept {
        if(!source)return;source->m_version=reader.getSourceVersion();
        if(source->m_state==State::Collecting){
            if(!source->fields.empty()) {const auto& f=source->fields.back();
                source->m_state=f.code==0&&f.kind==DxfReadRecordView::Text&&source->value(f)=="EOF"?State::ThroughEof:State::Invalid;
            }else source->m_state=State::Invalid;
        }
        if(source->m_state!=State::ThroughEof)return;
        try {
            Ticket ticket(source.get(),pool,identity);
            // Public callback receives only a distinct clone; the codec-owned
            // original remains private, including its byte arena.
            auto delivery=ticket.clone(*source);
            if(delivery)h.wholeSourcePending(identity,*delivery,ticket);
            ticket.active=false;
        }catch(...) {source->m_state=pool->quota?State::Quota:State::Allocation;}
    }
private:
    void collect(const DxfReadRecordView& v) {
        if(!source||source->m_state!=State::Collecting)return;
        if(v.ordinal!=next||v.ordinal==(std::numeric_limits<std::size_t>::max)()
            ||v.endOffset<=v.beginOffset||(next!=1&&v.beginOffset!=lastEnd)){source->m_state=State::Invalid;return;}
        ++next;lastEnd=v.endOffset;
        if(source->fields.size()>=pool->cap.fields){pool->quota=true;source->m_state=State::Quota;return;}
        const bool text=v.kind==DxfReadRecordView::Text||v.kind==DxfReadRecordView::Binary;
        const auto value=text?v.text:std::string_view{};
        const auto raw=(!source->m_binary||text)?v.raw:std::string_view{};
        std::size_t amount=0,needed=0,work=0;
        if(!sum(value.size(),raw.size(),amount)||!sum(source->bytes.size(),amount,needed)
            ||!sum(amount,sizeof(Field)+64u,work)||!pool->chargeWork(work)) {pool->quota=true;source->m_state=State::Quota;return;}
        if(!reserveFor(source->fields,source->fields.size()+1u)||!reserveFor(source->bytes,needed)) {source->m_state=State::Quota;return;}
        Field f{v.kind,v.code,v.ordinal,v.beginOffset,v.endOffset,source->bytes.size(),value.size(),
            source->bytes.size()+value.size(),raw.size(),v.kind==DxfReadRecordView::Integer?v.integer:0,
            v.kind==DxfReadRecordView::WideInteger?v.wide:0,v.kind==DxfReadRecordView::Real?v.real:0.,v.ignoredComment};
        source->bytes.insert(source->bytes.end(),value.begin(),value.end());
        source->bytes.insert(source->bytes.end(),raw.begin(),raw.end());source->fields.push_back(f);
    }
};
}
