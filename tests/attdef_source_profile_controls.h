// SPDX-License-Identifier: GPL-2.0-or-later
// Included inside the owning probe's anonymous namespace. This independent
// original-file observer uses neither dxfReader nor the production audit.
struct AttdefWireTag {
    int code=0;
    std::size_t begin=0,valueBegin=0,end=0;
    std::string text;
    std::int64_t integer=0;
    double real=0;
    enum Kind {String,Integer,Real,Binary} kind=String;
};
std::vector<AttdefWireTag> attdefWireTags(const std::filesystem::path& path,bool binary) {
    std::ifstream input(path,std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(input)),{});
    std::vector<AttdefWireTag> out;std::size_t at=binary?22u:0u;
    const auto word=[&](unsigned count) {
        if(count>bytes.size()-at)throw std::runtime_error("independent wire observer: truncated number");
        std::uint64_t result=0u;
        for(unsigned n=0;n<count;++n)result|=static_cast<std::uint64_t>(static_cast<unsigned char>(bytes[at++]))<<(8u*n);
        return result;
    };
    const auto line=[&]() {
        const auto end=bytes.find('\n',at);
        if(end==std::string::npos)throw std::runtime_error("independent wire observer: truncated line");
        auto result=bytes.substr(at,end-at);at=end+1u;return result;
    };
    while(at<bytes.size()) {
        AttdefWireTag tag;tag.begin=at;
        tag.code=binary?static_cast<int>(word(2u)):std::stoi(line());tag.valueBegin=at;
        unsigned width=0u;
        if((tag.code>=60&&tag.code<=79)||(tag.code>=170&&tag.code<=179)
            ||(tag.code>=270&&tag.code<=289)||(tag.code>=370&&tag.code<=389)
            ||(tag.code>=400&&tag.code<=409)||tag.code==1070)width=2u;
        else if((tag.code>=90&&tag.code<=99)||(tag.code>=420&&tag.code<=429)
            ||(tag.code>=440&&tag.code<=459)||tag.code==1071)width=4u;
        else if(tag.code>=160&&tag.code<=169)width=8u;
        else if(tag.code>=290&&tag.code<=299)width=1u;
        if(width) {
            tag.kind=AttdefWireTag::Integer;
            if(!binary)tag.integer=std::stoll(line());
            else {
                const auto value=word(width);
                tag.integer=width==1u?static_cast<std::int64_t>(value):width==2u?static_cast<std::int16_t>(value)
                    :width==4u?static_cast<std::int32_t>(value):static_cast<std::int64_t>(value);
            }
        } else if((tag.code>=10&&tag.code<=59)||(tag.code>=210&&tag.code<=239)
            ||(tag.code>=1010&&tag.code<=1059)) {
            tag.kind=AttdefWireTag::Real;
            if(!binary)tag.real=std::stod(line());
            else {const auto bits=word(8u);std::memcpy(&tag.real,&bits,sizeof(bits));}
        } else if(binary&&((tag.code>=310&&tag.code<=319)||tag.code==1004)) {
            tag.kind=AttdefWireTag::Binary;const auto count=word(1u);
            constexpr char digits[]="0123456789ABCDEF";
            for(std::uint64_t n=0;n<count;++n) {const auto value=word(1u);tag.text+=digits[value>>4u];tag.text+=digits[value&15u];}
        } else if(!binary)tag.text=line();
        else {
            const auto end=bytes.find('\0',at);
            if(end==std::string::npos)throw std::runtime_error("independent wire observer: truncated string");
            tag.text=bytes.substr(at,end-at);at=end+1u;
        }
        tag.end=at;out.push_back(std::move(tag));
    }
    return out;
}
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
