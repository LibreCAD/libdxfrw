// SPDX-License-Identifier: GPL-2.0-or-later
// Runtime-authored DXF recipes; expectations are literal format roles, not a
// projection of the parser under test. No drawing payload is vendored.
#include "intern/dxfrendersettings.h"
namespace renderingProjectionControls {
struct Tag { int code; std::string text; };
using Tags = std::vector<Tag>;
struct Probe : dx_iface {
    dx_data data;
    std::vector<DRW_RenderSettings> values;
    std::vector<DRW_RawDxfObject> raw;
    std::string order;
    bool throwing = false;
    Probe() { cData = &data; currentBlock = data.mBlock; }
    void addRenderSettings(const DRW_RenderSettings& value) override {
        order += 'T';
        if (throwing) throw std::runtime_error("authored rendering callback");
        values.push_back(value);
    }
    void addRawDxfObject(const DRW_RawDxfObject& value) override {
        order += 'R'; raw.push_back(value);
    }
};
Tags recipe(unsigned kind, unsigned variant, bool foreign) {
    const char* names[] = {"RENDERENVIRONMENT", "RAPIDRTRENDERSETTINGS",
        "MENTALRAYRENDERSETTINGS", "MENTALRAYRENDERSETTINGS", "RENDERSETTINGS", "RENDERGLOBAL", "RENDERENTRY"};
    Tags tags{{0,names[kind]},{5,"E901"},{330,"C"},
        {102,"{VENDOR"},{90,"999"},{1,"application-only"},{290,"1"},{102,"}"},
        {100,"AcDbObject"}};
    if (foreign) tags.insert(tags.end(),{{100,"VendorBefore"},{90,"991"},{1,"foreign-before"},{290,"1"},{280,"88"},{40,"999"}});
    const char* scopes[] = {"AcDbRenderEnvironment", "AcDbRenderSettings", "AcDbRenderSettings",
        "AcDbRenderSettings", "AcDbRenderSettings", "AcDbRenderGlobal", "AcDbRenderEntry"};
    tags.push_back({100,scopes[kind]});
    tags.push_back({90,"1"});
    if (kind == 0u) tags.insert(tags.end(),{{290,"1"},{290,"0"},{280,"17"},{280,"33"},{280,"65"},
        {40,"12.5"},{40,"37.5"},{40,"25"},{40,"75"},{290,"1"},{1,"authored-environment.png"}});
    else if (kind == 5u) tags.insert(tags.end(),{{90,"2"},{90,"1"},{290,"1"},{1,"authored-output.png"},{90,"640"},{90,"480"},{290,"0"},{290,"1"}});
    else if (kind == 6u) tags.insert(tags.end(),{{1,"authored-save"},{1,"authored-preset"},{1,"authored-view"},{90,"3"},{90,"4"},{70,"1"},{40,"2.5"}});
    else {
        tags.insert(tags.end(),{{1,"authored-preset"},{290,"1"}});
        if (kind == 2u) tags.insert(tags.end(),{{90,"7"},{290,"1"},{290,"0"},{1,"authored-preview.png"}});
        else {
            tags.insert(tags.end(),{{290,"0"},{290,"1"},{290,"0"},{1,"authored-preview.png"},{1,"authored-description"},{90,"23"}});
            if (variant != 2u) tags.push_back({290,variant ? "1" : "0"});
        }
        if (kind == 1u) tags.insert(tags.end(),{{100,"AcDbRapidRTRenderSettings"},{90,"2"},{70,"1"},{90,"17"},{90,"19"},{70,"2"},{70,"3"},{40,"1.25"},{40,"2.5"}});
        if (kind == 2u || kind == 3u) tags.insert(tags.end(),{{100,"AcDbMentalRayRenderSettings"},{90,"2"},{90,"-1"},{90,"3"},
            {290,"1"},{290,"1"},{290,"1"},{290,"1"},{1,"authored-mi-statistics.mi"},{40,"3.5"}});
    }
    if (foreign) tags.insert(tags.end(),{{100,"VendorAfter"},{90,"992"},{1,"foreign-after"},{290,"1"},{280,"99"},{40,"888"}});
    tags.insert(tags.end(),{{1001,"AUTHORED_APP"},{1000,"authored-xdata"},{1070,"13"}});
    return tags;
}
std::string wire(const Tags& record, bool binary, const std::string& version) {
    std::ostringstream output(std::ios::out | std::ios::binary);
    if (binary) output.write("AutoCAD Binary DXF\r\n\032\0",22);
    std::unique_ptr<dxfWriter> writer = binary
        ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&output))
        : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&output));
    writer->writeString(0,"SECTION"); writer->writeString(2,"HEADER");
    writer->writeString(9,"$ACADVER"); writer->writeString(1,version);
    writer->writeString(0,"ENDSEC"); writer->writeString(0,"SECTION"); writer->writeString(2,"OBJECTS");
    for (const auto& tag : record) {
        if (tag.code == 40) writer->writeDouble(tag.code,std::stod(tag.text));
        else if (tag.code == 290) writer->writeBool(tag.code,tag.text != "0");
        else if (tag.code == 90) writer->writeInt32(tag.code,std::stoi(tag.text));
        else if (tag.code == 70 || tag.code == 280 || tag.code == 1070) writer->writeInt16(tag.code,std::stoi(tag.text));
        else writer->writeUtf8String(tag.code,tag.text);
    }
    writer->writeString(0,"ENDSEC"); writer->writeString(0,"EOF");
    return output.str();
}
bool matches(const Probe& probe, unsigned kind, unsigned variant) {
    if (probe.order != "TR" || probe.values.size() != 1u || probe.raw.size() != 1u) return false;
    const auto& value = probe.values.front();
    if (value.handle != 0xE901u || value.parentHandle != 0xCu || value.m_classVersion != 1
        || value.appData.size() != 1u || value.extData.size() != 3u) return false;
    if (kind == 0u) return value.m_kind == DRW_RenderSettings::Environment
        && value.m_name == "authored-environment.png" && value.m_environmentImageFilename == "authored-environment.png"
        && value.m_description.empty() && value.m_fogEnabled && !value.m_fogBackgroundEnabled && value.m_environmentImageEnabled
        && value.m_fogColorR == 17 && value.m_fogColorG == 33 && value.m_fogColorB == 65
        && value.m_fogDensityNear == 12.5 && value.m_fogDensityFar == 37.5 && value.m_fogDistanceNear == 25 && value.m_fogDistanceFar == 75;
    if (kind == 5u) return value.m_kind == DRW_RenderSettings::Global && value.m_name == "authored-output.png"
        && value.m_procedure == 2 && value.m_destination == 1 && value.m_environmentImageFilename.empty();
    if (kind == 6u) return value.m_kind == DRW_RenderSettings::Entry && value.m_name == "authored-save"
        && value.m_environmentImageFilename.empty() && value.m_description.empty();
    return value.m_name == "authored-preset" && value.m_environmentImageFilename == "authored-preview.png"
        && value.m_description == (kind == 2u ? "" : "authored-description")
        && value.m_fogEnabled && !value.m_fogBackgroundEnabled && value.m_backfacesEnabled && !value.m_environmentImageEnabled
        && value.m_displayIndex == (kind == 2u ? 0 : 23)
        && value.m_hasPredefinedPresent == (kind != 2u && variant != 2u)
        && value.m_hasPredefined == (kind != 2u && variant == 1u);
}
bool rawMatches(const DRW_RawDxfObject& raw, const Tags& record) {
    if (raw.name != record.front().text || raw.groups.size()+1u != record.size()) return false;
    for (std::size_t i=1u;i<record.size();++i) {
        const auto& tag=record[i]; const auto& group=raw.groups[i-1u];
        if (group.code()!=tag.code) return false;
        if (tag.code==40) { if(group.type()!=DRW_Variant::DOUBLE || group.d_val()!=std::stod(tag.text)) return false; }
        else if(tag.code==90 || tag.code==290 || tag.code==70 || tag.code==280 || tag.code==1070) {
            if(group.type()!=DRW_Variant::INTEGER || group.i_val()!=std::stoll(tag.text)) return false;
        } else if(group.type()!=DRW_Variant::STRING || !group.content.s || *group.content.s!=tag.text) return false;
    }
    return true;
}
std::size_t budgetControls(TestContext& test) {
    const std::string filename(64u,'I');
    std::size_t controls=0u;
    for(bool binary : {false,true}) {
        const auto text=wire({{0,"RENDERENVIRONMENT"},{100,"AcDbRenderEnvironment"},{90,"1"},{1,filename}},binary,"AC1027");
        const auto parse=[&](std::size_t cap,DRW_RenderSettings& output,
                             std::shared_ptr<DxfReadRetainedBudget>& budget) {
            std::istringstream input(binary?text.substr(22u):text,std::ios::in|std::ios::binary);
            std::unique_ptr<dxfReader> reader=binary
                ?std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
                :std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
            budget=std::make_shared<DxfReadRetainedBudget>(cap);reader->setRetainedByteBudget(budget);
            DRW_RenderSettings model;model.m_kind=DRW_RenderSettings::Environment;
            dxfRenderSettings::Decoder decoder(model);bool active=false,good=false;int code=0;
            while(reader->readRec(&code)) {
                if(code==0) {
                    if(active){decoder.finish();good=true;break;}
                    active=reader->getStringView()=="RENDERENVIRONMENT";continue;
                }
                if(active&&!decoder.parseCode(code,reader))break;
            }
            output=model;
            return good&&!budget->exceeded();
        };
        std::size_t low=0u,high=8192u;
        while(low<high) {const auto middle=low+(high-low)/2u;DRW_RenderSettings model;std::shared_ptr<DxfReadRetainedBudget> budget;
            if(parse(middle,model,budget))high=middle;else low=middle+1u;}
        DRW_RenderSettings admitted;std::shared_ptr<DxfReadRetainedBudget> budget;
        test.expect(parse(low,admitted,budget)&&admitted.m_strings.size()==1u&&admitted.m_strings.front()==filename
            &&admitted.m_name==filename&&admitted.m_environmentImageFilename==filename&&budget->used()==3u*filename.size(),
            "environment filename charges vector plus both live named aliases at the exact cap");++controls;
        auto copy=admitted;DRW_RenderSettings assigned;assigned=admitted;admitted.reset();copy.reset();
        test.expect(budget->used()==3u*filename.size()&&assigned.m_environmentImageFilename==filename,
            "environment alias leases survive copying, assignment and source reset");assigned.reset();
        test.expect(budget->used()==0u,"last environment owner releases all alias charges");++controls;
        DRW_RenderSettings refused;std::shared_ptr<DxfReadRetainedBudget> refusedBudget;
        test.expect(low!=0u&&!parse(low-1u,refused,refusedBudget)&&refusedBudget->exceeded(),
            "one-byte-under environment filename limit refuses");refused.reset();
        test.expect(refusedBudget->used()==0u,"failed environment alias admission releases its partial owners");++controls;
    }
    return controls;
}
Tags completePreset(unsigned kind,unsigned variant) {
    auto tags=recipe(kind,variant,false);
    // Strip application/XDATA metadata only for this body-writer control. The
    // source projection matrix above independently preserves that metadata.
    bool application=false;
    tags.erase(std::remove_if(tags.begin(),tags.end(),[&](const Tag& tag) {
        if(tag.code==102){application=tag.text!="}";return true;}
        return application||tag.code>=1000;
    }),tags.end());
    for(auto& tag:tags)if(tag.code==330)tag.text="0";
    if(kind==2u||kind==3u) {
        auto at=std::find_if(tags.begin(),tags.end(),[](const Tag& t){return t.code==100&&t.text=="AcDbMentalRayRenderSettings";});
        tags.erase(at,tags.end());
        tags.insert(tags.end(),{{100,"AcDbMentalRayRenderSettings"},{90,"1"},{90,"-1"},{90,"3"},{70,"2"},
            {40,"1.25"},{40,"2.5"},{40,"0.125"},{40,"0.25"},{40,"0.5"},{40,"0.75"},{70,"1"},
            {290,"1"},{290,"0"},{90,"3"},{90,"4"},{90,"5"},{290,"1"},{90,"101"},{290,"0"},
            {40,"1.5"},{90,"102"},{90,"6"},{90,"7"},{90,"8"},{290,"1"},{90,"103"},
            {290,"1"},{290,"0"},{290,"1"},{40,"2.5"},{40,"3.5"},{40,"4.5"},{70,"1"},{70,"2"},
            {40,"5.5"},{70,"1"},{70,"0"},{290,"0"},{1,"authored-mi-statistics.mi"},{90,"32"},{70,"2"},{90,"512"}});
    }
    return tags;
}
struct Writer : dx_iface {
    dx_data data;dxfRW& output;DRW_RenderSettings value;bool wrote=false;
    Writer(dxfRW& target,const DRW_RenderSettings& model):output(target),value(model){cData=&data;currentBlock=data.mBlock;}
    void writeHeader(DRW_Header& h)override{h.vars.clear();}
    void writeBlocks()override{}void writeBlockRecords()override{}void writeEntities()override{}
    void writeLTypes()override{}void writeLayers()override{}void writeTextstyles()override{}
    void writeVports()override{}void writeDimstyles()override{}void writeAppId()override{}
    void writeObjects()override{wrote=output.writeRenderSettings(&value,value.m_kind==DRW_RenderSettings::RapidRT?"RAPIDRTRENDERSETTINGS":"MENTALRAYRENDERSETTINGS");}
};
bool sameProjection(const DRW_RenderSettings& a,const DRW_RenderSettings& b) {
    return a.m_kind==b.m_kind&&a.m_classVersion==b.m_classVersion&&a.m_name==b.m_name
        &&a.m_environmentImageFilename==b.m_environmentImageFilename&&a.m_description==b.m_description
        &&a.m_displayIndex==b.m_displayIndex&&a.m_hasPredefinedPresent==b.m_hasPredefinedPresent&&a.m_hasPredefined==b.m_hasPredefined
        &&a.m_fogEnabled==b.m_fogEnabled&&a.m_fogBackgroundEnabled==b.m_fogBackgroundEnabled
        &&a.m_backfacesEnabled==b.m_backfacesEnabled&&a.m_environmentImageEnabled==b.m_environmentImageEnabled
        &&a.m_longs==b.m_longs&&a.m_strings==b.m_strings&&a.m_bools==b.m_bools
        &&a.m_shorts==b.m_shorts&&a.m_bytes==b.m_bytes&&a.m_doubles==b.m_doubles;
}
std::size_t writerControls(TestContext& test,const std::filesystem::path& path) {
    std::size_t controls=0u;
    for(unsigned kind:{1u,2u,3u})for(unsigned variant=0u;variant<3u;++variant)for(bool binary:{false,true}) {
        const auto record=completePreset(kind,variant);auto source=wire(record,binary,"AC1027");
        {std::ofstream file(path,std::ios::binary);file.write(source.data(),static_cast<std::streamsize>(source.size()));}
        Probe original;dxfRW input(path.string().c_str());
        const bool parsed=input.read(&original,false)&&original.values.size()==1u;
        test.expect(parsed,"complete authored preset writer input parses");if(!parsed)continue;
        const auto model=original.values.front();dxfRW output(path.string().c_str());Writer writer(output,model);
        const auto checkOutput=[&]() {
            Probe reopened;dxfRW reader(path.string().c_str());
            if(!reader.read(&reopened,false)||reopened.values.size()!=1u||reopened.raw.size()!=1u
                ||!sameProjection(model,reopened.values.front())||!sameProjection(model,writer.value))return false;
            auto begin=std::find_if(record.begin(),record.end(),[](const Tag& t){return t.code==100&&t.text=="AcDbRenderSettings";});
            const auto& groups=reopened.raw.front().groups;
            auto body=std::find_if(groups.begin(),groups.end(),[](const DRW_Variant& v){return v.code()==100&&v.type()==DRW_Variant::STRING&&v.content.s&&*v.content.s=="AcDbRenderSettings";});
            if(static_cast<std::size_t>(std::distance(body,groups.end()))!=static_cast<std::size_t>(std::distance(begin,record.end())))return false;
            DRW_RawDxfObject raw;raw.name="BODY";raw.groups.assign(body,groups.end());Tags expected{{0,"BODY"}};expected.insert(expected.end(),begin,record.end());
            return rawMatches(raw,expected);
        };
        for(int pass=0;pass<2;++pass){test.expect(output.write(&writer,DRW::AC1027,binary)&&writer.wrote&&checkOutput(),
            "typed preset writer preserves canonical base/derived field order and repeated readback");++controls;}
        for(unsigned fault=0u;fault<15u;++fault) {
            writer.value=model;
            if(fault==0u)writer.value.m_longs.pop_back();
            if(fault==1u)writer.value.m_doubles.front()=std::numeric_limits<double>::quiet_NaN();
            if(fault==2u)writer.value.m_shorts.front()=65536;
            if(fault==3u)writer.value.m_name="edited-without-positional-value";
            if(fault==4u)writer.value.m_hasPredefined=!writer.value.m_hasPredefined;
            if(fault==5u)writer.value.m_environmentImageFilename="edited-preview";
            if(fault==6u)test.expect(writer.value.setDwgRawData({0x01u},8u,DRW::AC1027),"authored opaque native tail is reachable");
            if(fault==7u)++writer.value.m_classVersion;
            if(fault==8u)++writer.value.m_displayIndex;
            if(fault==9u)writer.value.m_description="edited-description";
            if(fault==10u)writer.value.m_backfacesEnabled=!writer.value.m_backfacesEnabled;
            if(fault==11u)writer.value.m_fogEnabled=!writer.value.m_fogEnabled;
            if(fault==12u)writer.value.m_fogBackgroundEnabled=!writer.value.m_fogBackgroundEnabled;
            if(fault==13u)writer.value.m_environmentImageEnabled=!writer.value.m_environmentImageEnabled;
            if(fault==14u)writer.value.m_hasPredefinedPresent=!writer.value.m_hasPredefinedPresent;
            {std::ofstream file(path,std::ios::binary);file<<"KEEP_RENDER_DESTINATION";}
            const bool refused=!output.write(&writer,DRW::AC1027,binary)&&!writer.wrote;
            std::ifstream file(path,std::ios::binary);const std::string kept{std::istreambuf_iterator<char>(file),{}};
            test.expect(refused&&kept=="KEEP_RENDER_DESTINATION","ambiguous, unrepresented or edited preset refuses without replacing destination");
            writer.value=model;test.expect(output.write(&writer,DRW::AC1027,binary)&&writer.wrote&&checkOutput(),
                "same writer accepts corrected preset after refusal");++controls;
        }
    }
    return controls;
}
void run(TestContext& test) {
    const auto path=std::filesystem::temp_directory_path()/(
        "libdxfrw-render-projection-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dxf");
    std::size_t profiles=0u, rawFields=0u, corrected=0u, lifetimes=0u, scoped=0u;
    for (const std::string version : {"AC1021","AC1024","AC1027","AC1032"})
    for (unsigned encoding=0u;encoding<3u;++encoding) for(unsigned kind=0u;kind<7u;++kind)
    for(unsigned variant=0u;variant<3u;++variant) for(bool foreign : {false,true}) {
        const auto record=recipe(kind,variant,foreign);const auto text=wire(record,encoding==2u,version);
        {std::ofstream out(path,std::ios::binary);out.write(text.data(),static_cast<std::streamsize>(text.size()));}
        dxfRW reader(path.string().c_str()); Probe probe; std::string memory=text;
        const bool parsed=encoding==0u?reader.readAscii(&probe,false,memory):reader.read(&probe,false);
        const bool good=parsed && matches(probe,kind,variant) && rawMatches(probe.raw.front(),record);
        test.expect(good,"original base-subclass rendering projections and exact raw fields");
        if(!good) {std::cerr<<" render profile="<<version<<'/'<<encoding<<'/'<<kind<<'/'<<variant<<'/'<<foreign<<'\n';continue;}
        ++profiles;rawFields+=record.size()-1u;
        auto copy=probe.values.front(); DRW_RenderSettings assigned; assigned=copy; probe.values.clear(); copy.reset();
        test.expect(assigned.m_classVersion==1 && !assigned.m_name.empty(),"rendering projections survive copy/assignment and original reset");++lifetimes;
    }
    for(unsigned encoding=0u;encoding<3u;++encoding) {
        const Tags partial{{0,"MENTALRAYRENDERSETTINGS"},{5,"E901"},{330,"C"},{100,"AcDbObject"},
            {100,"AcDbRenderSettings"},{90,"1"},{1,"authored-preset"},{290,"1"},
            {100,"AcDbMentalRayRenderSettings"},{90,"99"},{1,"derived-only.mi"},{290,"1"},{290,"1"},{290,"1"},{290,"1"},{290,"1"}};
        const auto text=wire(partial,encoding==2u,"AC1027");
        {std::ofstream out(path,std::ios::binary);out.write(text.data(),static_cast<std::streamsize>(text.size()));}
        dxfRW reader(path.string().c_str());Probe probe;std::string memory=text;
        const bool parsed=encoding==0u?reader.readAscii(&probe,false,memory):reader.read(&probe,false);
        const bool present=parsed&&probe.values.size()==1u;test.expect(present,"partial base render record stays available through tolerant read");
        if(present){const auto& value=probe.values.front();test.expect(value.m_classVersion==1&&value.m_name=="authored-preset"
            &&value.m_environmentImageFilename.empty()&&value.m_description.empty()&&value.m_displayIndex==0
            &&!value.m_hasPredefinedPresent&&!value.m_hasPredefined&&!value.m_backfacesEnabled,
            "derived strings, flags and version never fill missing preset base fields");++scoped;}
        auto markerless=recipe(0u,0u,false);markerless.erase(std::remove_if(markerless.begin(),markerless.end(),[](const Tag& t){return t.code==100;}),markerless.end());
        const auto legacyText=wire(markerless,encoding==2u,"AC1027");
        {std::ofstream out(path,std::ios::binary);out.write(legacyText.data(),static_cast<std::streamsize>(legacyText.size()));}
        Probe legacy;std::string legacyMemory=legacyText;
        test.expect((encoding==0u?reader.readAscii(&legacy,false,legacyMemory):reader.read(&legacy,false))&&matches(legacy,0u,0u),
            "markerless environment compatibility preserves corrected named projections");++scoped;
    }
    const auto record=recipe(1u,0u,true);const auto text=wire(record,false,"AC1027");
    dxfRW reused(path.string().c_str());
    for(const std::string bad : {"not-an-integer","4294967296"}) {
        std::string broken=text; const auto at=broken.find("AcDbRenderSettings\n");
        test.expect(at!=std::string::npos,"authored malformed rendering scalar is reachable");
        if(at==std::string::npos)continue;
        const auto start=broken.find('\n',at+19u)+1u;
        broken.replace(start,broken.find('\n',start)-start,bad);
        Probe failed;test.expect(!reused.readAscii(&failed,false,broken)&&failed.values.empty()&&failed.raw.empty(),"malformed rendering scalar refuses before typed/raw callbacks");
        Probe fixed;std::string correctedText=text;test.expect(reused.readAscii(&fixed,false,correctedText)&&matches(fixed,1u,0u),"same reader accepts corrected rendering input");++corrected;
    }
    Probe throws;throws.throwing=true;std::string thrownText=text;bool caught=false;
    try { (void)reused.readAscii(&throws,false,thrownText); } catch(const std::runtime_error&) {caught=true;}
    test.expect(caught&&throws.order=="T"&&throws.raw.empty(),"throwing typed rendering callback does not publish raw callback");
    Probe fixed;std::string fixedText=text;test.expect(reused.readAscii(&fixed,false,fixedText)&&matches(fixed,1u,0u),"same reader recovers after rendering callback throw");++corrected;
    const auto written=writerControls(test,path);
    std::error_code ignored;std::filesystem::remove(path,ignored);
    const auto budgets=budgetControls(test);
    std::cout<<"Rendering projection profiles="<<profiles<<"; exact raw fields="<<rawFields<<"; scope controls="<<scoped<<"; exact/one-under ownership controls="<<budgets<<"; corrected retries="<<corrected<<"; public lifetimes="<<lifetimes<<"; typed writer/readback/refusal controls="<<written<<'\n';
}
}
