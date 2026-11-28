// SPDX-License-Identifier: GPL-2.0-or-later
#include "independent_dxf_scalar_walker.h"
struct AttdefLedger {
    Tags tags;
    std::vector<dx_attdef_source::Span> expected;
};
AttdefLedger attdefLedger(unsigned mode,bool legacyCount) {
    using S=dx_attdef_source::Scope;using F=dx_attdef_source::Field;using R=dx_attdef_source::Role;
    AttdefLedger result;
    const auto add=[&](int code,const char* value,S scope,F field,R role=R::None,std::uint64_t handle=0u) {
        result.expected.push_back({result.tags.size(),code,scope,field,role,handle});result.tags.push_back({code,value});
    };
    add(5,"A1",S::Host,F::Identity,R::Self,0xA1u);add(330,"B0",S::Host,F::Identity,R::Owner,0xB0u);
    const auto applications=[&](S scope) {
        add(102,"{CUSTOM",scope,F::Application);add(100,"AcDbText",scope,F::Application);
        add(10,"99.5",scope,F::Application);add(73,"99",scope,F::Application);add(1,"not-text",scope,F::Application);
        add(330,"FACE",scope,F::Application,R::OpaqueHandle,0xFACEu);
        add(102,"{INNER",scope,F::Application);add(1001,"NOT_AN_APPID",scope,F::Application);
        add(1005,"BEEF",scope,F::Application,R::OpaqueHandle,0xBEEFu);
        add(102,"}",scope,F::Application);add(102,"}",scope,F::Application);
        add(102,"{ACAD_REACTORS",scope,F::Application);add(330,"10",scope,F::Application,R::Reactor,0x10u);
        add(330,"11",scope,F::Application,R::Reactor,0x11u);add(102,"}",scope,F::Application);
        add(102,"{ACAD_XDICTIONARY",scope,F::Application);
        add(360,"C1",scope,F::Application,R::ExtensionDictionary,0xC1u);add(102,"}",scope,F::Application);
    };
    applications(S::Host);
    add(100,"AcDbEntity",S::Host,F::Subclass);
    const auto common=[&](S scope) {
        add(8,"DRAW_LAYER",scope,F::Common,R::LayerName);add(6,"DASHED",scope,F::Common,R::LineTypeName);
        add(347,"123",scope,F::Common,R::Material,0x123u);add(348,"124",scope,F::Common,R::FullVisualStyle,0x124u);
        add(390,"125",scope,F::Common,R::PlotStyle,0x125u);add(62,"3",scope,F::Common);
        add(370,"35",scope,F::Common);add(48,"1.25",scope,F::Common);add(60,"0",scope,F::Common);
        add(420,"1193046",scope,F::Common);add(430,"book$host",scope,F::Common);add(440,"1234",scope,F::Common);
        add(67,"0",scope,F::Common);add(284,"2",scope,F::Common);
        add(legacyCount?92:160,"3",scope,F::Common);add(310,"0102",scope,F::Common);add(310,"A3",scope,F::Common);
    };
    common(S::Host);add(100,"AcDbText",S::Host,F::Subclass);
    for(const auto& tag:Tags{{10,"1.25"},{20,"2.5"},{30,"3.75"},{40,"4.5"},{1,"default-é"},
        {39,"0.5"},{50,"15"},{41,"1.5"},{51,"2"},{71,"2"},{72,"0"},{210,"0"},{220,"0"},{230,"1"}})
        add(tag.code,tag.value.c_str(),S::Host,F::Text);
    add(7,"STANDARD",S::Host,F::Text,R::TextStyleName);
    const auto attribute=[&]() {
        add(100,"AcDbAttributeDefinition",S::Host,F::Subclass);
        for(const auto& tag:Tags{{3,"prompt-é"},{2,"TAG"},{70,"7"},{73,"5"},{74,"2"},{280,"1"}})
            add(tag.code,tag.value.c_str(),S::Host,F::Attribute);
    };
    if(mode==3u) {
        attribute();add(71,"4",S::Host,F::Attribute);add(72,"0",S::Host,F::ReservedDefault);
        add(11,"5",S::Host,F::Attribute);add(21,"6",S::Host,F::Attribute);add(31,"7",S::Host,F::Attribute);
    }
    if(mode) {
        add(mode==2u?100:101,"Embedded Object",S::Nested,F::EmbeddedMarker);
        applications(S::Nested);common(S::Nested);
        for(const auto& tag:Tags{{10,"10"},{20,"20"},{30,"30"},{40,"2.5"},{41,"12"},{46,"7"},
            {3,"FIRST_"},{3,"SECOND_"},{1,"LAST-é"},{50,"15"},{11,"0"},{21,"1"},{31,"0"},
            {71,"4"},{72,"1"},{73,"2"},{44,"1.25"},{90,"1"},{45,"1.5"},{63,"3"},
            {421,"1193046"},{431,"book$fill"},{441,"1234"}})
            add(tag.code,tag.value.c_str(),S::Nested,F::MText);
        add(7,"NESTED_STYLE",S::Nested,F::MText,R::TextStyleName);
        add(42,"5",S::Nested,F::ComputedIgnored);add(43,"6",S::Nested,F::ComputedIgnored);
    }
    if(mode!=3u)attribute();
    const S applicationOwner=mode?S::Nested:S::Host;
    add(102,"{AFTER_ATTRIBUTE",applicationOwner,F::Application);
    add(1003,"NOT_AN_XDATA_LAYER",applicationOwner,F::Application);
    add(340,"ABCD",applicationOwner,F::Application,R::OpaqueHandle,0xABCDu);
    add(102,"}",applicationOwner,F::Application);
    add(1001,"APP",S::Host,F::Xdata,R::AppIdName);add(1002,"{",S::Host,F::Xdata);
    add(1003,"XDATA_LAYER",S::Host,F::Xdata,R::XdataLayerName);
    add(1005,"F1",S::Host,F::Xdata,R::XdataObjectId,0xF1u);
    for(const auto& tag:Tags{{1000,"payload"},{1004,"0102"},{1010,"1"},{1020,"2"},{1030,"3"},
        {1011,"4"},{1021,"5"},{1031,"6"},{1040,"7"},{1070,"8"},{1071,"9"},{1002,"}"}})
        add(tag.code,tag.value.c_str(),S::Host,F::Xdata);
    return result;
}
bool attdefOriginalSpans(const std::filesystem::path& path,bool binary,const Tags& documentTags,
                         const AttdefLedger& ledger,const dx_attdef_source::Audit& audit,std::size_t& spans,bool quiet=false) {
    const auto verify=[&](bool value,const char* label){return value||(!quiet&&check(false,label));};
    const auto wire=attdefWireTags(path,binary);
    if(!verify(wire.size()==documentTags.size(),"independent observer sees every original file tag"))return false;
    std::size_t first=wire.size();
    for(std::size_t i=0;i<wire.size();++i) {
        const auto& tag=wire[i];const auto& expected=documentTags[i];
        if(!verify(tag.code==expected.code&&tag.begin<tag.valueBegin&&tag.valueBegin<tag.end
            &&(!i||wire[i-1u].end==tag.begin),"independent original byte bounds and code"))return false;
        if(tag.kind==AttdefWireTag::Integer) {if(tag.integer!=std::stoll(expected.value))return false;}
        else if(tag.kind==AttdefWireTag::Real) {if(tag.real!=std::stod(expected.value))return false;}
        else if(tag.text!=expected.value)return false;
        if(tag.code==0&&tag.text=="ATTDEF")first=i+1u;
    }
    if(!verify(first<wire.size()&&audit.spans.size()==ledger.expected.size(),"complete original ATTDEF span inventory"))return false;
    for(std::size_t i=0;i<ledger.expected.size();++i) {
        const auto& a=audit.spans[i];const auto& b=ledger.expected[i];
        if(!verify(a.groupIndex==i&&a.groupIndex==b.groupIndex&&a.code==wire[first+i].code
            &&a.code==b.code&&a.scope==b.scope&&a.field==b.field&&a.role==b.role&&a.handle==b.handle,
            "manually authored physical role/scope ledger agrees at original wire span")) {
            if(!quiet)std::cerr<<"group="<<i<<" code="<<a.code<<'\n';return false;
        }
        ++spans;
    }
    return true;
}

struct AttdefProfileFault {
    const char* label;
    std::function<void(Tags&)> apply;
    dx_attdef_source::Reason reason;
    bool nestedOnly=false;
};
std::vector<AttdefProfileFault> attdefProfileFaults() {
    using R=dx_attdef_source::Reason;
    const auto locate=[](Tags& tags,int code,const char* value) {
        unsigned depth=0u;
        for(auto at=tags.begin();at!=tags.end();++at) {
            if(at->code==102)depth=at->value=="}"?depth-1u:depth+1u;
            if(at->code==code&&at->value==value&&(code!=100||!depth))return at;
        }
        throw std::runtime_error("missing authored fault location");
    };
    std::vector<AttdefProfileFault> faults;
    const auto insert=[&](const char* label,int code,const char* value,Tags extra,R reason) {
        faults.push_back({label,[=](Tags& tags){const auto at=locate(tags,code,value);tags.insert(at,extra.begin(),extra.end());},reason});
    };
    const auto change=[&](const char* label,int code,const char* value,Tag replacement,R reason) {
        faults.push_back({label,[=](Tags& tags){*locate(tags,code,value)=replacement;},reason});
    };
    insert("ignored unknown host field",100,"AcDbAttributeDefinition",{{300,"lost"}},R::UnknownField);
    insert("ignored unknown attribute field",1001,"APP",{{340,"ABCD"}},R::UnknownField);
    insert("duplicate same-valued TEXT",1,"default-é",{{1,"default-é"}},R::DuplicateField);
    insert("duplicate layer",8,"DRAW_LAYER",{{8,"DRAW_LAYER"}},R::DuplicateField);
    insert("duplicate changing color",62,"3",{{62,"4"}},R::DuplicateField);
    insert("ambiguous version and lock",280,"1",{{280,"0"}},R::AmbiguousVersion);
    insert("secondary attribute section",1001,"APP",{{100,"AcDbXrecord"},{280,"1"},{70,"4"},{70,"1"},{70,"1"},{340,"ABCD"}},R::UnmodeledSecondary);
    insert("unknown subclass",1001,"APP",{{100,"AcDbUnknown"}},R::Subclass);
    insert("repeated subclass",100,"AcDbText",{{100,"AcDbText"}},R::Subclass);
    change("wrong text subclass",100,"AcDbText",{100,"AcDbWrongText"},R::Subclass);
    insert("secondary self outside header",1001,"APP",{{105,"A1"}},R::UnknownField);
    insert("reserved dictionary duplicate",360,"C1",{{360,"C2"}},R::Application);
    insert("reserved group with foreign field",360,"C1",{{1,"not-a-handle"}},R::Application);
    insert("reserved group with nested group",330,"10",{{102,"{INNER"},{102,"}"}},R::Application);
    insert("repeated reserved group",100,"AcDbEntity",{{102,"{ACAD_REACTORS"},{330,"12"},{102,"}"}},R::DuplicateField);
    insert("direct dictionary overwrites application",100,"AcDbText",{{360,"C2"}},R::DuplicateField);
    change("opaque overflowing handle",330,"FACE",{330,"10000000000000000"},R::InvalidValue);
    change("orphan XDATA",1001,"APP",{1003,"ORPHAN"},R::Xdata);
    insert("interrupted coordinate sanitized later",1020,"2",{{1070,"12"}},R::IncompletePoint);
    change("wrong coordinate role",1020,"2",{1021,"2"},R::IncompletePoint);
    change("unclosed XDATA list",1002,"}",{1000,"not-close"},R::Xdata);
    change("unopened XDATA list",1002,"{",{1002,"}"},R::Xdata);
    insert("APPID inside unclosed list",1000,"payload",{{1001,"OTHER"}},R::Xdata);
    insert("unknown XDATA code",1002,"}",{{1006,"lost"}},R::UnknownField);
    insert("ordinary field after XDATA",1002,"}",{{40,"10"}},R::Xdata);
    change("short proxy count",310,"A3",{310,""},R::ProxyGraphics);
    change("tag containing whitespace",2,"TAG",{2,"BAD TAG"},R::InvalidValue);
    change("negative text height",40,"4.5",{40,"-1"},R::InvalidValue);
    change("lossy unknown line weight",370,"35",{370,"17"},R::InvalidValue);
    const auto nestedInsert=[&](const char* label,Tag extra,R reason) {
        faults.push_back({label,[=](Tags& tags){tags.insert(locate(tags,40,"2.5"),extra);},reason,true});
    };
    nestedInsert("unmodeled nested columns",{75,"1"},R::UnknownField);
    nestedInsert("unmodeled nested gutter",{49,"2"},R::UnknownField);
    nestedInsert("unmodeled nested subclass",{100,"AcDbMText"},R::Subclass);
    faults.push_back({"duplicate nested final text",[=](Tags& tags){tags.insert(locate(tags,1,"LAST-é"),{1,"LAST-é"});},R::DuplicateField,true});
    faults.push_back({"invalid nested flow",[=](Tags& tags){*locate(tags,72,"1")={72,"2"};},R::InvalidValue,true});
    faults.push_back({"missing nested final text",[=](Tags& tags){tags.erase(locate(tags,1,"LAST-é"));},R::IncompletePoint,true});
    faults.push_back({"missing nested insertion Y",[=](Tags& tags){tags.erase(locate(tags,20,"20"));},R::IncompletePoint,true});
    return faults;
}

AttdefLedger attdefDual280Ledger(bool lock,bool application=true,unsigned mode=0u) {
    using S=dx_attdef_source::Scope;using F=dx_attdef_source::Field;using R=dx_attdef_source::Role;
    auto ledger=attdefLedger(mode,false);
    const auto at=std::find_if(ledger.tags.begin(),ledger.tags.end(),[](const Tag& t){return t.code==100&&t.value=="AcDbAttributeDefinition";});
    if(at==ledger.tags.end())throw std::runtime_error("dual280 authored attribute subclass");
    const auto first=static_cast<std::size_t>(at-ledger.tags.begin())+1u;
    ledger.tags.insert(ledger.tags.begin()+static_cast<std::ptrdiff_t>(first),{280,"0"});
    ledger.expected.insert(ledger.expected.begin()+static_cast<std::ptrdiff_t>(first),{first,280,S::Host,F::ReservedDefault,R::None,0u});
    if(application) {
        const Tags fields{{102,"{VERSION_META"},{280,"7"},{3,"not-prompt"},{2,"NOT_TAG"},{70,"15"},{102,"}"}};
        ledger.tags.insert(ledger.tags.begin()+static_cast<std::ptrdiff_t>(first+1u),fields.begin(),fields.end());
        std::vector<dx_attdef_source::Span> expected;for(const auto& field:fields)expected.push_back({0u,field.code,S::Host,F::Application,R::None,0u});
        ledger.expected.insert(ledger.expected.begin()+static_cast<std::ptrdiff_t>(first+1u),expected.begin(),expected.end());
    }
    for(std::size_t i=0u;i<ledger.expected.size();++i) {
        ledger.expected[i].groupIndex=i;
        if(ledger.expected[i].field==F::Attribute&&ledger.expected[i].code==280)ledger.tags[i].value=lock?"1":"0";
    }
    return ledger;
}
bool attdefDual280ProfileTests(const std::filesystem::path& path) {
    using A=DxIfaceTableSourceTestAccess;using namespace dx_attdef_source;
    unsigned profiles=0u,roleCorruptions=0u,refusals=0u,retries=0u,singles=0u,versionRefusals=0u,attributeProfiles=0u,embeddedRefusals=0u;std::size_t spans=0u;
    const auto positions=[](const AttdefLedger& ledger){std::array<std::size_t,2> out{{ledger.tags.size(),ledger.tags.size()}};
        for(std::size_t i=0u;i<ledger.expected.size();++i){const auto& s=ledger.expected[i];if(s.code==280&&s.field==Field::ReservedDefault)out[0]=i;if(s.code==280&&s.field==Field::Attribute)out[1]=i;}
        if(out[0]>=out[1]||out[1]>=ledger.tags.size())throw std::runtime_error("dual280 authored roles");return out;};
    const auto positive=[&](const AttdefLedger& ledger,const std::string& version,bool binary,bool block,CaptureProbe& reader,dx_data& data){
        const auto tags=document(version,ledger.tags,block);
        if(!write(path,tags,binary)||!reader.fileImport(path.string(),&data,false)||A::failed(data))return false;
        const auto* row=A::resource(data);if(!row||!row->attdefModel||!row->attdefPairMatched||!row->attdefFieldsMatched||row->attdefFieldMismatch
            ||row->rawTypedMatched||!row->attdefProfile.complete()||row->attdefModel->attVersion!=0||row->attdefModel->mtext
            ||row->attdefContext.inBlock!=block||!row->attdefReadContext||row->attdefReadContext->binary!=binary)return false;
        if(!attdefOriginalSpans(path,binary,tags,ledger,row->attdefProfile,spans)
            ||!replayMatches(ledger.tags,version,snapshot(*row->attdefModel),false))return false;
        const auto* editable=static_cast<const DRW_Attdef*>(nullptr);const auto find=[&](const dx_ifaceBlock* owner){if(owner)for(const auto* entity:owner->ent)
            if(const auto* value=dynamic_cast<const DRW_Attdef*>(entity)){if(editable)throw std::runtime_error("dual280 editable alias");editable=value;}};
        find(data.mBlock);for(const auto* owner:data.blocks)find(owner);
        return editable&&editable->lockPosition==row->attdefModel->lockPosition&&editable->attVersion==0&&editable->prompt=="prompt-é"&&editable->tag=="TAG";
    };
    for(const std::string version:{"AC1024","AC1027","AC1032"})for(bool binary:{false,true})for(bool block:{false,true})for(bool lock:{false,true}) {
        const auto ledger=attdefDual280Ledger(lock);CaptureProbe reader;dx_data data;
        if(!check(positive(ledger,version,binary,block,reader,data),"dual280 original capture and independently positioned field roles"))return false;
        const auto* row=A::resource(data);const auto role=positions(ledger);
        if(!check(row->attdefModel->lockPosition==lock&&row->raw->groups[role[0]].i_val()==0&&row->raw->groups[role[1]].i_val()==int(lock)
            &&row->attdefProfile.spans[role[0]].field==Field::ReservedDefault&&row->attdefProfile.spans[role[1]].field==Field::Attribute,
            "zero version prefix and boolean lock retain distinct original roles"))return false;
        for(unsigned which=0u;which<4u;++which) {
            auto corrupted=row->attdefProfile;
            if(which==0u)corrupted.spans[role[0]].field=Field::Attribute;
            if(which==1u)corrupted.spans[role[1]].field=Field::ReservedDefault;
            if(which==2u)std::swap(corrupted.spans[role[0]].groupIndex,corrupted.spans[role[1]].groupIndex);
            if(which==3u)corrupted.spans[role[0]].scope=Scope::Nested;
            std::size_t ignored=0u;if(!check(!attdefOriginalSpans(path,binary,document(version,ledger.tags,block),ledger,corrupted,ignored,true),
                "independent dual280 span observer detects same-code role/index/scope corruption"))return false;
            ++roleCorruptions;
        }
        // The same scalar geometry/text has no revision-specific proxy count
        // when testing unsupported revisions independently of proxy spelling.
        for(const auto refusedVersion:{DRW::AC1021,DRW::UNKNOWNV}) {
            auto raw=*row->raw;raw.m_version=refusedVersion;
            raw.groups.erase(std::remove_if(raw.groups.begin(),raw.groups.end(),[](const DRW_Variant& v){return v.code()==160||v.code()==310;}),raw.groups.end());
            const auto audit=dx_attdef_source::audit(raw);if(!check(!audit.complete()&&audit.spans.empty(),"unsupported revisions do not inherit classic dual280 qualification"))return false;++versionRefusals;
        }
        auto raw=*row->raw;raw.name="ATTRIB";
        for(auto& value:raw.groups)if(value.code()==100&&value.type()==DRW_Variant::STRING&&*value.content.s=="AcDbAttributeDefinition")value=DRW_Variant(100,UTF8STRING("AcDbAttribute"));
        // Remove only the owning prompt; opaque application prompt shadows stay opaque.
        raw.groups.erase(raw.groups.begin()+static_cast<std::ptrdiff_t>(role[0]+1u+(ledger.tags[role[0]+1u].code==102?6u:0u)));
        const auto audit=dx_attdef_source::audit(raw,false);
        if(!check(audit.complete()&&audit.form==Form::Classic&&audit.spans[role[0]].field==Field::ReservedDefault
            &&audit.spans[role[1]-1u].field==Field::Attribute,"classic ATTRIB has version/lock roles without an owning prompt"))return false;
        ++attributeProfiles;
        ++profiles;
    }
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(bool binary:{false,true})for(bool block:{false,true})for(bool lock:{false,true}) {
        auto single=attdefDual280Ledger(false,false);
        if(version=="AC1021")for(std::size_t i=0u;i<single.tags.size();++i)if(single.tags[i].code==160){single.tags[i].code=92;single.expected[i].code=92;}
        const auto role=positions(single);
        single.tags.erase(single.tags.begin()+static_cast<std::ptrdiff_t>(role[1]));single.expected.erase(single.expected.begin()+static_cast<std::ptrdiff_t>(role[1]));
        single.expected[role[0]].field=Field::Attribute;single.tags[role[0]].value=lock?"1":"0";
        for(std::size_t i=0u;i<single.expected.size();++i)single.expected[i].groupIndex=i;
        CaptureProbe reader;dx_data data;if(!check(positive(single,version,binary,block,reader,data)&&A::resource(data)->attdefModel->lockPosition==lock,
            "existing single leading boolean stays an attribute lock, not a version certificate"))return false;
        ++singles;
    }
    for(const std::string version:{"AC1024","AC1027","AC1032"})for(bool binary:{false,true})for(bool block:{false,true}) {
        for(unsigned kind=0u;kind<14u;++kind) {
            auto ledger=attdefDual280Ledger(true,false);const auto role=positions(ledger);auto bad=ledger.tags;
            if(kind==0u)bad[role[0]].value="1";if(kind==1u)bad[role[0]].value="-1";if(kind==2u)bad[role[1]].value="2";
            if(kind==3u){bad[role[0]].value="1";bad[role[1]].value="0";}
            if(kind==4u){const auto version=bad[role[0]];bad.erase(bad.begin()+static_cast<std::ptrdiff_t>(role[0]));bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(role[0]+1u),version);}
            if(kind==5u)bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(role[1]+1u),{280,"1"});
            if(kind==6u)bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(role[0]+1u),{280,"0"});
            if(kind==7u){const auto at=std::find_if(bad.begin()+static_cast<std::ptrdiff_t>(role[0]+1u),bad.end(),[](const Tag& t){return t.code==70;});
                const auto flags=*at;bad.erase(at);bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(role[1]),flags);}
            if(kind==8u){bad.erase(bad.begin()+static_cast<std::ptrdiff_t>(role[0]+1u));bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(role[0]+1u),{{102,"{SHADOW"},{3,"not-prompt"},{2,"NOT_TAG"},{70,"7"},{102,"}"}});}
            if(kind==9u)bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(role[1]),{{100,"AcDbXrecord"},{280,"0"}});
            if(kind>=10u) {
                // Start with otherwise admitted complete MTEXT, either before
                // the attribute subclass or after its dual pair.
                const unsigned mode=kind<12u?(kind==10u?1u:2u):3u;
                auto embedded=attdefDual280Ledger(true,false,mode);
                if(kind>=12u)for(std::size_t i=embedded.tags.size();i-->0u;) {
                    const auto& span=embedded.expected[i];
                    if(span.scope==Scope::Host&&((span.field==Field::Attribute&&(span.code==71||span.code==11||span.code==21||span.code==31))
                        ||(span.field==Field::ReservedDefault&&span.code==72)))embedded.tags.erase(embedded.tags.begin()+static_cast<std::ptrdiff_t>(i));
                }
                bad=std::move(embedded.tags);
                if(kind==13u)for(auto& tag:bad)if(tag.code==101&&tag.value=="Embedded Object")tag.code=100;
            }
            if(!write(path,document(version,bad,block),binary))return false;CaptureProbe failed;dx_data rejected;(void)failed.fileImport(path.string(),&rejected,false);
            const auto* row=A::resource(rejected);if(!check(!row||(!row->attdefProfile.complete()&&row->attdefProfile.spans.empty()),"malformed dual280 source cannot publish a partial or complete profile"))return false;
            if(kind>=10u) {
                if(!check(row&&row->raw,"embedded source is captured before its profile is refused"))return false;
                const auto audit=dx_attdef_source::audit(*row->raw);
                const auto expected=kind<12u&&version!="AC1032"?Reason::Revision:Reason::AmbiguousVersion;
                if(!check(audit.reason==expected,"embedded dual280 refusal occurs at the explicit source grammar boundary"))return false;
                ++embeddedRefusals;
            }
            ++refusals;
            failed.skipSource=failed.skipTyped=false;dx_data corrected;if(!positive(ledger,version,binary,block,failed,corrected)||!A::resource(corrected)->attdefModel->lockPosition)return false;++retries;
        }
    }
    std::cout<<"ATTDEF dual280 profiles="<<profiles<<"; independent spans="<<spans<<"; role corruptions="<<roleCorruptions<<"; malformed/retries="<<refusals<<'/'<<retries
        <<"; single compatibility="<<singles<<"; revision refusals="<<versionRefusals<<"; ATTRIB direct profiles="<<attributeProfiles<<"; embedded refusals="<<embeddedRefusals<<'\n';
    return profiles==24u&&roleCorruptions==96u&&refusals==168u&&retries==168u&&singles==32u&&versionRefusals==48u&&attributeProfiles==24u&&embeddedRefusals==48u;
}
