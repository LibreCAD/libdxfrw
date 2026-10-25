// SPDX-License-Identifier: GPL-2.0-or-later
#include "intern/dxffield.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>
#include <sstream>
#include <type_traits>

namespace {
long allocationsBeforeFailure=-1;
struct AllocationFailure {
    explicit AllocationFailure(long count){allocationsBeforeFailure=count;}
    ~AllocationFailure(){allocationsBeforeFailure=-1;}
};
}
void* operator new(std::size_t size) {
    if(allocationsBeforeFailure==0)throw std::bad_alloc();
    if(allocationsBeforeFailure>0)--allocationsBeforeFailure;
    if(void* value=std::malloc(size?size:1u))return value;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size){return ::operator new(size);}
void operator delete(void* value) noexcept {std::free(value);}
void operator delete[](void* value) noexcept {std::free(value);}
void operator delete(void* value,std::size_t) noexcept {std::free(value);}
void operator delete[](void* value,std::size_t) noexcept {std::free(value);}

class DrwObjectEncodeTestAccess {
public:
    static std::size_t retained(const DRW_TableEntry& value) {
        return value.m_dxfRetainedStringLease?value.m_dxfRetainedStringLease->bytes():0u;
    }
};

namespace {
struct Tag {int code;std::string value;};
using Tags=std::vector<Tag>;
struct Field:DRW_Field {
    using DRW_Field::parseCode;
    std::size_t retained() const {return DrwObjectEncodeTestAccess::retained(*this);}
};
static_assert(sizeof(Field)==sizeof(DRW_Field),"probe has no extra model state");
static_assert(std::is_nothrow_move_constructible<DRW_Field::ChildValue>::value,"child payloads can transfer without copies");

std::size_t dynamicValue(const DRW_CadValue& value) {
    std::size_t bytes=value.m_formatString.size()+value.m_valueString.size()+value.m_rawData.size();
    if(value.m_value.type()==DRW_Variant::STRING)bytes+=value.m_value.content.s->size();
    if(value.m_value.type()==DRW_Variant::BINARY)bytes+=value.m_value.binary()->size();
    return bytes;
}
std::size_t payload(const DRW_Field& field) {
    std::size_t bytes=field.m_evaluatorId.size()+field.m_fieldCode.size()+field.m_formatString.size()
        +field.m_evaluationErrorMessage.size()+field.m_valueString.size()+dynamicValue(field.m_value)
        +(field.m_childHandles.size()+field.m_objectHandles.size())*sizeof(std::uint32_t)
        +field.m_childValues.size()*sizeof(DRW_Field::ChildValue);
    for(const auto& child:field.m_childValues)bytes+=child.m_key.size()+dynamicValue(child.m_value);
    return bytes;
}
Tags body() {
    return {{5,"A"},{330,"0"},{100,"AcDbField"},{1,"eval"},{2,"part"},{3,"-two"},
        {4,"outer-format"},{300,"old"},{300,"replacement-error"},{90,"1"},{360,"B"},{97,"1"},{331,"C"},{93,"1"},
        {6,"child-key"},{93,"0"},{90,"4"},{1,"first"},{2,"-second"},{94,"0"},{300,"child-format"},
        {302,"child-cache"},{304,"ACVALUE_END"},{7,"ACFD_FIELD_VALUE"},{93,"0"},{90,"8"},
        {92,"4"},{310,"00FF"},{310,"1080"},{94,"0"},{300,"main-format"},{302,"main-cache"},
        {304,"ACVALUE_END"},{301,"display"},{9,"-tail"},{98,"12"}};
}
std::string wire(const Tags& tags,bool binary) {
    std::ostringstream out(std::ios::binary);
    const auto word=[&](std::uint64_t value,unsigned size){for(unsigned i=0;i<size;++i)out.put(static_cast<char>(value>>(8u*i)));};
    for(const auto& tag:tags) {
        if(!binary)out<<tag.code<<'\n'<<tag.value<<'\n';
        else {
            word(static_cast<unsigned>(tag.code),2u);
            if(tag.code>=90&&tag.code<=99)word(static_cast<std::uint32_t>(std::stoll(tag.value)),4u);
            else if(tag.code==310) {
                out.put(static_cast<char>(tag.value.size()/2u));
                for(std::size_t i=0;i<tag.value.size();i+=2u)word(std::stoul(tag.value.substr(i,2u),nullptr,16),1u);
            } else if((tag.code>=10&&tag.code<=59)||(tag.code>=140&&tag.code<=149)) {
                const double value=std::stod(tag.value);std::uint64_t bits=0u;std::memcpy(&bits,&value,sizeof bits);word(bits,8u);
            } else {out<<tag.value;out.put('\0');}
        }
    }
    return out.str();
}
bool decode(Field& field,const std::string& bytes,bool binary,const char* revision,
            const std::shared_ptr<DxfReadRetainedBudget>& budget) {
    std::istringstream input(bytes,std::ios::binary);
    std::unique_ptr<dxfReader> reader;
    if(binary)reader=std::make_unique<dxfReaderBinary>(&input);else reader=std::make_unique<dxfReaderAscii>(&input);
    reader->setVersion(revision,true);reader->setRetainedByteBudget(budget);
    dxfField::Decoder decoder(field,static_cast<DRW::Version>(reader->getVersion()));
    int code=0;
    while(reader->readRec(&code)) {
        if(!decoder.beforeRecord(code))return false;
        if(code==102||(code>=1000&&code<=1071)) {if(!field.parseCode(code,reader))return false;}
        else {dxfField::ReferenceKind reference;if(!decoder.parseCode(code,reader,reference))return false;}
    }
    return (!budget||!budget->exceeded())&&decoder.finish();
}
bool parse(Field& field,dxfField::Decoder& decoder,const std::unique_ptr<dxfReader>& reader,int code) {
    if(!decoder.beforeRecord(code))return false;
    if(code==102||(code>=1000&&code<=1071))return field.parseCode(code,reader);
    dxfField::ReferenceKind reference;
    return decoder.parseCode(code,reader,reference);
}
struct Operation {const char* name;Tags prefix;Tag target;std::size_t extra;};
std::vector<Operation> operations() {
    const std::string text(512u,'x');
    const Tags scalar={{5,"A"},{100,"AcDbField"},{1,std::string(256u,'e')}};
    Tags child=scalar;child.insert(child.end(),{{93,"1"},{6,"key"},{93,"0"},{90,"4"},{1,"old"}});
    Tags binary=scalar;binary.insert(binary.end(),{{93,"0"},{7,"ACFD_FIELD_VALUE"},{93,"0"},{90,"8"},{92,"32"}});
    Tags raw=binary;raw.push_back({310,std::string(64u,'A')});
    Tags string=scalar;string.insert(string.end(),{{93,"0"},{7,"ACFD_FIELD_VALUE"},{93,"0"},{90,"4"},{1,"old"}});
    Tags childStart=scalar;childStart.push_back({93,"1"});
    Tags emptyChild=childStart;emptyChild.insert(emptyChild.end(),{{6,""},{93,"0"},{90,"0"},{91,"0"}});
    const Tags firstChild={{5,"A"},{100,"AcDbField"},{93,"1"},{6,""},{93,"0"},{90,"0"},{91,"0"}};
    return {{"evaluator replacement",scalar,{1,text},text.size()},
        {"field code append",scalar,{2,text},text.size()},
        {"field code overflow",scalar,{3,text},text.size()},
        {"outer format",scalar,{4,text},text.size()},
        {"error message",scalar,{300,text},text.size()},
        {"display text",scalar,{301,text},text.size()},
        {"display continuation",scalar,{9,text},text.size()},
        {"child handle",scalar,{360,"B"},sizeof(std::uint32_t)},
        {"object handle",scalar,{331,"C"},sizeof(std::uint32_t)},
        {"child key",childStart,{6,text},text.size()},
        {"pending value string",child,{2,text},text.size()},
        {"pending format",child,{300,text},text.size()},
        {"pending cache",child,{302,text},text.size()},
        {"child slot",child,{304,"ACVALUE_END"},sizeof(DRW_Field::ChildValue)},
        {"empty child slot",emptyChild,{304,"ACVALUE_END"},sizeof(DRW_Field::ChildValue)},
        {"first owner from child slot",firstChild,{304,"ACVALUE_END"},sizeof(DRW_Field::ChildValue)},
        {"primary value string",string,{2,text},text.size()},
        {"primary format",string,{300,text},text.size()},
        {"primary cache",string,{302,text},text.size()},
        {"raw binary",binary,{310,std::string(64u,'A')},32u},
        {"binary variant copy",raw,{304,"ACVALUE_END"},32u}};
}
// The current record has already been admitted. Hold unrelated budget pressure
// so only the operation's exact destination growth is available; this isolates
// owner admission from transport/getter reservations. No RSS/capacity claim.
bool operation(const Operation& item,bool binary,bool below,long allocation=-1) {
    auto budget=std::make_shared<DxfReadRetainedBudget>(1024u*1024u);
    Field field;bool accepted=false;std::size_t before=0u;
    {
        Tags tags=item.prefix;tags.push_back(item.target);
        std::istringstream input(wire(tags,binary),std::ios::binary);
        std::unique_ptr<dxfReader> reader;
        if(binary)reader=std::make_unique<dxfReaderBinary>(&input);else reader=std::make_unique<dxfReaderAscii>(&input);
        reader->setVersion("AC1032",true);reader->setRetainedByteBudget(budget);
        dxfField::Decoder decoder(field,DRW::AC1032);int code=0;
        for(std::size_t i=0;i<item.prefix.size();++i)
            if(!reader->readRec(&code)||!parse(field,decoder,reader,code))return false;
        if(!reader->readRec(&code))return false;
        const auto available=item.extra-(below?1u:0u);
        const auto pressure=allocation<0?budget->limit()-budget->used()-available:0u;
        if(!budget->reserve(pressure))return false;
        before=field.retained();
        try {
            // Only the parse operation is armed, never input/assertion setup.
            AllocationFailure failure(allocation);
            accepted=parse(field,decoder,reader,code);
        } catch(const std::bad_alloc&) {accepted=false;}
        budget->release(pressure);
        if(allocation<0&&(accepted==below||budget->exceeded()!=below)) {
            std::cerr<<"admission failed: "<<item.name<<" binary="<<binary<<" below="<<below<<'\n';return false;
        }
        if(!accepted&&field.retained()!=before) {
            std::cerr<<"failed operation changed owner: "<<item.name<<'\n';return false;
        }
    }
    if(field.retained()!=payload(field)||budget->used()!=field.retained()) {
        std::cerr<<"teardown owner mismatch: "<<item.name<<" binary="<<binary<<" allocation="<<allocation<<'\n';return false;
    }
    field.reset();return budget->used()==0u;
}
bool boundaries() {
    unsigned exact=0u,failures=0u;
    for(const auto& item:operations())for(bool binary:{false,true}) {
        if(!operation(item,binary,false)||!operation(item,binary,true))return false;
        ++exact;
        for(long allocation=0;allocation<5;++allocation) {
            if(!operation(item,binary,false,allocation))return false;
            ++failures;
        }
    }
    std::cout<<"FIELD admission: "<<exact<<" exact/one-below pairs; "<<failures
        <<" scoped allocation schedules; pending-child cleanup and model-prefix ownership\n";
    return exact==42u&&failures==210u;
}
bool pendingBinaryAndContexts() {
    // A binary pending child can acquire its second view before child-slot
    // admission fails. That lexical payload must not leak into a closed sibling
    // or survive decoder teardown, including allocation-failure schedules.
    Tags tags={{5,"A"},{100,"AcDbField"},{1,"eval"},{90,"0"},{97,"0"},{93,"2"},
        {6,"closed"},{93,"0"},{90,"4"},{1,"retained sibling"},{304,"ACVALUE_END"},
        {6,"pending"},{93,"0"},{90,"8"},{92,"32"},{310,std::string(64u,'A')}};
    for(bool binary:{false,true})for(long allocation=-2;allocation<6;++allocation) {
        auto budget=std::make_shared<DxfReadRetainedBudget>(1024u*1024u);Field field;
        {
            Tags inputTags=tags;inputTags.push_back({304,"ACVALUE_END"});
            std::istringstream input(wire(inputTags,binary),std::ios::binary);
            std::unique_ptr<dxfReader> reader;
            if(binary)reader=std::make_unique<dxfReaderBinary>(&input);else reader=std::make_unique<dxfReaderAscii>(&input);
            reader->setVersion("AC1032",true);reader->setRetainedByteBudget(budget);
            dxfField::Decoder decoder(field,DRW::AC1032);int code=0;
            for(std::size_t i=0;i<tags.size();++i)if(!reader->readRec(&code)||!parse(field,decoder,reader,code))return false;
            if(!reader->readRec(&code))return false;
            const auto pressure=allocation==-2?budget->limit()-budget->used()-32u-sizeof(DRW_Field::ChildValue)+1u:0u;
            if(!budget->reserve(pressure))return false;
            bool accepted=false;
            try {AllocationFailure failure(allocation);accepted=parse(field,decoder,reader,code);}
            catch(const std::bad_alloc&) {accepted=false;}
            budget->release(pressure);
            if(allocation==-2&&(accepted||!budget->exceeded()))return false;
        }
        if(field.retained()!=payload(field)||budget->used()!=field.retained()||field.m_childValues.empty())return false;
        field.reset();if(budget->used())return false;
    }
    // An active decoder must never migrate or steal a sibling copy's lease
    // when presented with an unrelated reader/budget context.
    Field field;auto original=std::make_shared<DxfReadRetainedBudget>(1024u*1024u);
    auto incoming=std::make_shared<DxfReadRetainedBudget>(1024u*1024u);
    {
        std::istringstream input("100\nAcDbField\n1\noriginal\n1\nreplacement\n");
        std::unique_ptr<dxfReader> reader=std::make_unique<dxfReaderAscii>(&input);
        reader->setVersion("AC1032",true);reader->setRetainedByteBudget(original);
        dxfField::Decoder decoder(field,DRW::AC1032);int code=0;
        for(unsigned i=0;i<2u;++i)if(!reader->readRec(&code)||!parse(field,decoder,reader,code))return false;
        const auto owned=field.retained();reader->setRetainedByteBudget(incoming);
        if(!reader->readRec(&code)||parse(field,decoder,reader,code)||!incoming->exceeded()
            ||field.retained()!=owned||field.m_evaluatorId!="original")return false;
    }
    if(original->used()!=payload(field)||incoming->used())return false;
    field.reset();if(original->used())return false;
    std::cout<<"FIELD cleanup: 16 pending-binary schedules plus budget-context isolation\n";
    return true;
}
bool tests() {
    unsigned routes=0u;
    for(const char* version:{"AC1015","AC1018","AC1021","AC1024","AC1027","AC1032"})for(bool binary:{false,true}) {
        auto budget=std::make_shared<DxfReadRetainedBudget>(1024u*1024u);
        Field field;
        if(!decode(field,wire(body(),binary),binary,version,budget))return false;
        const auto expected=payload(field);
        if(field.retained()!=expected||budget->used()!=expected) {
            std::cerr<<"FIELD owner undercharge: "<<version<<" binary="<<binary<<" retained="<<field.retained()
                <<" used="<<budget->used()<<" expected="<<expected<<'\n';return false;
        }
        Field copy(field);Field assigned;assigned=field;field.reset();
        if(budget->used()!=expected||copy.m_childValues[0].m_value.m_value.type()!=DRW_Variant::STRING)return false;
        copy.reset();Field moved(std::move(assigned));assigned.reset();
        if(budget->used()!=expected)return false;
        moved.reset();if(budget->used())return false;
        ++routes;
    }
    std::cout<<"FIELD ownership: "<<routes<<" revision/encoding routes; exact model payload and copy/reset lifetime\n";
    // Disabled accounting preserves decoding. A shared budget aggregates distinct
    // siblings, whereas copies share only their originating logical lease.
    Field disabled;if(!decode(disabled,wire(body(),false),false,"AC1032",{}))return false;
    if(disabled.retained())return false;
    auto shared=std::make_shared<DxfReadRetainedBudget>(1024u*1024u);
    Field first,second;
    if(!decode(first,wire(body(),false),false,"AC1032",shared)
        ||!decode(second,wire(body(),true),true,"AC1032",shared))return false;
    if(shared->used()!=payload(first)+payload(second))return false;
    first.reset();if(shared->used()!=payload(second))return false;
    second.reset();if(shared->used())return false;
    return routes==12u&&boundaries()&&pendingBinaryAndContexts();
}
}
int main(){return tests()?0:1;}
