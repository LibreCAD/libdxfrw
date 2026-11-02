// SPDX-License-Identifier: GPL-2.0-or-later
// Authored STYLE body/ACAD/custom XDATA; no external drawing fixtures.
Tags styleFontData(unsigned form) {
    constexpr const char* body="5570578";constexpr const char* acad="50331682";
    Tags data;
    if(form==0u||form==1u||form==4u)data.push_back({1071,body});
    if(form==3u)data.push_back({1071,acad});
    if(form==8u)data.push_back({1071,"0"});
    if(form==1u||form==2u||form==5u||form==6u||form==8u)data.insert(data.end(),{{1001,"REGISTERED_APP"},{1071,"-2147483648"},{1071,"2147483647"},{1000,"custom-é"},{1071,"-1234567"}});
    if(form==2u||form==3u||form==4u||form==6u||form==8u) {
        data.insert(data.end(),{{1001,"aCaD"},{1000,"Authored Font-é"},{1071,form==8u?"0":acad}});
        if(form==6u)data.insert(data.end(),{{1071,"-17"},{1071,"-19"}});
    }
    if(form==3u||form==4u)data.insert(data.end(),{{1001,"REGISTERED_APP"},{1071,"-2147483648"},{1071,"2147483647"}});
    if(form==7u)data.insert(data.end(),{{1001,"ACAD"},{1071,"34"},{1071,"-29"}});
    return data;
}
int styleFontExpected(unsigned form) {return form==0u||form==1u?5570578:form==5u||form==8u?0:form==7u?34:50331682;}
Tags styleFontDocument(const std::string& version,bool blocks,unsigned form) {
    auto tags=dependencyDocument(version,blocks,0u);tags[blockField(tags,"D201",2)].value="SCOPED_FONT";tags[blockField(tags,"511",7)].value="SCOPED_FONT";
    auto at=slot(tags,5,"D401");while(tags[at].code!=0)++at;
    const Tags acad{{0,"APPID"},{5,"D402"},{330,"D3"},{100,"AcDbSymbolTableRecord"},{100,"AcDbRegAppTableRecord"},{2,"ACAD"},{70,"0"}};
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),acad.begin(),acad.end());tags[blockField(tags,"D3",70)].value="2";
    appendBlockFields(tags,"D201",styleFontData(form));return tags;
}
bool styleFontXdata(const DRW_Textstyle& style,const Tags& tags) {
    std::size_t first=0u;while(first<tags.size()&&tags[first].code!=1001)++first;
    if(style.extData.size()!=tags.size()-first)return false;
    for(std::size_t i=first;i<tags.size();++i) {
        const auto* v=style.extData[i-first];if(!v||v->code()!=tags[i].code)return false;
        if(tags[i].code==1071){if(v->type()!=DRW_Variant::INTEGER||v->i_val()!=std::stoll(tags[i].value))return false;}
        else if(v->type()!=DRW_Variant::STRING||!v->content.s||*v->content.s!=tags[i].value)return false;
    }
    return true;
}
struct StyleFontProbe:dx_iface {
    bool source=true,throwSource=false,throwTyped=false;std::vector<DRW_Textstyle> seen;
    bool wantsDxfSymbolSource() const override{return source&&dx_iface::wantsDxfSymbolSource();}
    void addDxfSymbolSource(const DRW_TableEntry& model,const DRW_RawDxfObject& raw) override {
        dx_iface::addDxfSymbolSource(model,raw);if(throwSource&&model.name=="SCOPED_FONT")throw std::runtime_error("font source");
    }
    void addTextStyle(const DRW_Textstyle& model) override {
        dx_iface::addTextStyle(model);if(model.name=="SCOPED_FONT"){seen.push_back(model);if(throwTyped)throw std::runtime_error("font typed");}
    }
};
bool styleFontWire(const std::filesystem::path& path,bool binary,int font,const Tags& expected,std::size_t& spans,bool body=true) {
    const auto wire=attdefWireTags(path,binary);unsigned records=0u;
    for(std::size_t first=0u;first<wire.size();++first)if(wire[first].code==0&&wire[first].text=="STYLE") {
        auto end=first+1u;bool selected=false;while(end<wire.size()&&wire[end].code!=0){if(wire[end].code==2&&wire[end].text=="SCOPED_FONT")selected=true;++end;}
        if(!selected)continue;++records;bool xdata=false;unsigned bodies=0u;std::size_t field=0u;
        while(field<expected.size()&&expected[field].code!=1001)++field;
        for(std::size_t at=first+1u;at<end;++at) {
            const auto& scalar=wire[at];if(scalar.code==1001)xdata=true;
            if(!xdata&&scalar.code==1071){if(!body||scalar.integer!=font)return false;++bodies;++spans;}
            if(!xdata)continue;
            if(field>=expected.size()||scalar.code!=expected[field].code)return false;
            if(scalar.code==1071){if(scalar.integer!=std::stoll(expected[field].value))return false;++spans;}
            if(scalar.code==1001&&scalar.text!=expected[field].value)return false;
            ++field;
        }
        if(field!=expected.size()||bodies!=(body&&font!=0?1u:0u))return false;
    }
    return records==1u;
}
struct StyleFontWriter:dx_iface {
    bool armed=false;long offset=-1;
    void writeTextstyles() override {if(armed){SymbolAllocationFailure failure(offset);dx_iface::writeTextstyles();}else dx_iface::writeTextstyles();}
};
bool styleFontTests(const std::filesystem::path& dir) {
    const auto path=dir/"style-font.dxf",output=dir/"style-font-out.dxf";unsigned profiles=0u,copies=0u,origins=0u,withheld=0u,outputs=0u,edits=0u,retries=0u,optouts=0u,throws=0u,escapes=0u,emptyFamilies=0u,integer64Flags=0u,legacy=0u,refusals=0u,versionRefusals=0u,reached=0u,unreached=0u,budgets=0u;std::size_t spans=0u;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true})for(unsigned form=0u;form<9u;++form) {
        StyleFontProbe reader;dx_data data;const auto tags=styleFontDocument(version,blocks,form),fields=styleFontData(form);
        if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||reader.seen.size()!=1u
            ||reader.seen.front().fontFamily!=styleFontExpected(form)||!styleFontXdata(reader.seen.front(),fields)) {
            std::cerr<<"STYLE font "<<version<<'/'<<encoding<<" blocks="<<blocks<<" form="<<form<<" font="<<(reader.seen.empty()?-1:reader.seen.front().fontFamily)
                <<" XDATA="<<(reader.seen.empty()?0u:reader.seen.front().extData.size())<<'\n';return false;
        }
        const auto& style=reader.seen.front();DRW_Textstyle clone(style),assigned;assigned=clone;
        if(!styleFontXdata(clone,fields)||!styleFontXdata(assigned,fields)||clone.fontFamily!=style.fontFamily||assigned.fontFamily!=style.fontFamily)return false;++copies;
        const auto origin=dx_iface::resourceOriginReport(data);const bool qualified=form!=4u&&form!=7u;
        if((origin.state==dx_iface::ResourceOriginState::OriginsBound)!=qualified
            ||dx_iface::dependencyReport(data).originalTargetsMatched!=qualified) {
            std::cerr<<"STYLE source profile form="<<form<<" state="<<static_cast<int>(origin.state)<<'\n';return false;
        }
        if(qualified)++origins;else ++withheld;
        for(bool binary:{false,true}) {
            dx_data clean;clean.textStyles.push_back(style);DRW_AppId app;app.name="REGISTERED_APP";clean.appIds.push_back(app);dx_iface writer;
            if(!writer.fileExport(output.string(),data.sourceVersion,binary,&clean,false))return false;
            StyleFontProbe actual;dx_data readback;if(!actual.fileImport(output.string(),&readback,false)||actual.seen.size()!=1u
                ||actual.seen.front().fontFamily!=style.fontFamily||!styleFontXdata(actual.seen.front(),fields)||!styleFontXdata(clean.textStyles.front(),fields)
                ||!styleFontWire(output,binary,style.fontFamily,fields,spans)){return false;}
            ++outputs;
            clean.textStyles.front().fontFamily=17;auto editedFields=fields;
            for(std::size_t i=0u;i<editedFields.size();++i)if(editedFields[i].code==1001&&(editedFields[i].value=="ACAD"||editedFields[i].value=="aCaD")) {
                auto flag=i+1u;if(flag<editedFields.size()&&editedFields[flag].code==1000)++flag;
                if(flag<editedFields.size()&&editedFields[flag].code==1071)editedFields[flag].value="17";
            }
            if(!writer.fileExport(output.string(),data.sourceVersion,binary,&clean,false))return false;
            StyleFontProbe edited;dx_data editedData;if(!edited.fileImport(output.string(),&editedData,false)||edited.seen.size()!=1u
                ||edited.seen.front().fontFamily!=17||!styleFontXdata(edited.seen.front(),editedFields)||!styleFontXdata(clean.textStyles.front(),fields)
                ||!styleFontWire(output,binary,17,editedFields,spans)){return false;}
            ++edits;
        }
        ++profiles;
    }
    for(unsigned encoding=0u;encoding<3u;++encoding) {
        const auto good=styleFontDocument("AC1032",false,3u);const auto fields=styleFontData(3u);
        {auto tags=good;tags[blockField(tags,"D201",1001)].value="\\U+0041CaD";auto decoded=fields;decoded[1].value="ACaD";
            StyleFontProbe reader;dx_data data;if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||reader.seen.size()!=1u
                ||reader.seen.front().fontFamily!=50331682||!styleFontXdata(reader.seen.front(),decoded)||!dx_iface::dependencyReport(data).originalTargetsMatched){return false;}
            ++escapes;}
        {auto tags=good;tags[blockField(tags,"D201",1000)].value.clear();auto expected=fields;expected[2].value.clear();
            StyleFontProbe reader;dx_data data;if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||reader.seen.size()!=1u
                ||reader.seen.front().fontFamily!=50331682||!styleFontXdata(reader.seen.front(),expected)||!dx_iface::dependencyReport(data).originalTargetsMatched){return false;}
            ++emptyFamilies;}
        if(!writeBlockBinding(path,good,encoding))return false;
        for(bool source:{false,true}){StyleFontProbe reader;reader.source=source;dx_data data;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&data,false)
            ||reader.seen.size()!=1u||!styleFontXdata(reader.seen.front(),fields)||reader.seen.front().fontFamily!=50331682){return false;}
            ++optouts;}
        for(bool source:{false,true}) {
            StyleFontProbe reader;reader.throwSource=source;reader.throwTyped=!source;dx_data data;bool threw=false;
            try{(void)reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
            if(!threw||dx_iface::resourceOriginReport(data).state!=dx_iface::ResourceOriginState::ImportFailed)return false;
            reader.throwSource=reader.throwTyped=false;reader.seen.clear();dx_data next;if(!reader.fileImport(path.string(),&next,false)||reader.seen.size()!=1u||!styleFontXdata(reader.seen.front(),fields))return false;++throws;++retries;
        }
    }
    DRW_Textstyle base;base.name="SCOPED_FONT";base.fontFamily=50331682;bool xdata=false;
    for(const auto& tag:styleFontData(3u)) {
        if(tag.code==1001)xdata=true;
        if(!xdata)continue;
        if(tag.code==1071)base.extData.push_back(new DRW_Variant(1071,static_cast<std::int32_t>(std::stoll(tag.value))));
        else base.extData.push_back(new DRW_Variant(tag.code,tag.value));
    }
    for(const auto version:{DRW::AC1009,DRW::AC1014,DRW::AC1015,DRW::AC1018})for(bool binary:{false,true}) {
        dx_data data;data.textStyles.push_back(base);DRW_AppId app;app.name="REGISTERED_APP";data.appIds.push_back(app);dx_iface writer;
        StyleFontProbe reader;dx_data actual;const bool exported=writer.fileExport(output.string(),version,binary,&data,false);
        const bool imported=exported&&reader.fileImport(output.string(),&actual,false);
        if(!imported||reader.seen.size()!=1u||reader.seen.front().fontFamily!=50331682||!styleFontXdata(reader.seen.front(),styleFontData(3u))) {
            std::cerr<<"Legacy font version="<<static_cast<int>(version)<<" binary="<<binary<<" exported="<<exported<<" imported="<<imported<<" seen="<<reader.seen.size()
                <<" font="<<(reader.seen.empty()?-1:reader.seen.front().fontFamily)<<" XDATA="<<(reader.seen.empty()?0u:reader.seen.front().extData.size())<<'\n';return false;
        }
        ++legacy;
    }
    for(bool binary:{false,true}) {
        StyleFontWriter writer;dx_data data;data.textStyles.push_back(base);data.textStyles.front().fontFamily=17;DRW_AppId app;app.name="REGISTERED_APP";data.appIds.push_back(app);
        auto edited=styleFontData(3u);edited[3].value="17";
        const auto success=[&](){if(!writer.fileExport(output.string(),DRW::AC1032,binary,&data,false))return false;StyleFontProbe reader;dx_data actual;
            return reader.fileImport(output.string(),&actual,false)&&reader.seen.size()==1u&&reader.seen.front().fontFamily==17&&styleFontXdata(reader.seen.front(),edited)
                &&styleFontXdata(data.textStyles.front(),styleFontData(3u))&&styleFontWire(output,binary,17,edited,spans);};
        {auto& flag=*data.textStyles.front().extData[2];flag=DRW_Variant(1071,std::int64_t{50331682});
            if(!writer.fileExport(output.string(),DRW::AC1032,binary,&data,false))return false;
            StyleFontProbe reader;dx_data actual;if(!reader.fileImport(output.string(),&actual,false)||reader.seen.size()!=1u||reader.seen.front().fontFamily!=17
                ||!styleFontXdata(reader.seen.front(),edited)||!styleFontWire(output,binary,17,edited,spans)
                ||flag.type()!=DRW_Variant::INTEGER64||flag.i64_val()!=50331682)return false;
            flag=DRW_Variant(1071,50331682);++integer64Flags;}
        // AC1012 is deliberately not a public writer target. Keep the refusal
        // and destination contract, then retry the same exporter successfully.
        {const std::string sentinel="KEEP_UNSUPPORTED_FONT_TARGET";{std::ofstream out(output,std::ios::binary);out<<sentinel;}
            if(writer.fileExport(output.string(),DRW::AC1012,binary,&data,false))return false;
            {std::ifstream in(output,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(in),{})!=sentinel)return false;}
            if(!success())return false;++versionRefusals;++retries;}
        for(unsigned fault=0u;fault<6u;++fault) {
            auto& row=data.textStyles.front();row=base;row.fontFamily=17;
            if(fault==0u)row.extData[2]->addString(1071,"invalid font flags");
            if(fault==1u)*row.extData[2]=DRW_Variant(1071,std::numeric_limits<std::int64_t>::max());
            if(fault==2u){delete row.extData[2];row.extData[2]=nullptr;}
            if(fault==3u)row.extData.back()->addString(1071,"invalid custom integer");
            if(fault==4u){row.extData.push_back(new DRW_Variant(1001,std::string("ACAD")));row.extData.push_back(new DRW_Variant(1071,7));}
            if(fault==5u)row.extData.push_back(new DRW_Variant(1002,std::string("{")));
            const std::string sentinel="KEEP_STYLE_FONT_DESTINATION";{std::ofstream out(output,std::ios::binary);out<<sentinel;}
            if(writer.fileExport(output.string(),DRW::AC1032,binary,&data,false))return false;
            {std::ifstream in(output,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(in),{})!=sentinel)return false;}
            row=base;row.fontFamily=17;if(!success())return false;++refusals;++retries;
        }
        for(long offset=0;offset<64;++offset) {
            const std::string sentinel="KEEP_FONT_ALLOCATION_DESTINATION";{std::ofstream out(output,std::ios::binary);out<<sentinel;}
            writer.armed=true;writer.offset=offset;const auto before=symbolFailedAllocations;bool ok=false;
            try{ok=writer.fileExport(output.string(),DRW::AC1032,binary,&data,false);}catch(const std::bad_alloc&){}
            writer.armed=false;
            if(symbolFailedAllocations!=before){if(ok)return false;std::ifstream in(output,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(in),{})!=sentinel)return false;++reached;}
            else {if(!ok)return false;++unreached;}
            if(!success())return false;++retries;
        }
    }
    for(bool under:{false,true}) {
        std::istringstream stream("1001\nACAD\n1000\nAuthored Font\n1071\n50331682\n");std::unique_ptr<dxfReader> reader=std::make_unique<dxfReaderAscii>(&stream);
        reader->setVersion("AC1032",true);reader->setTypedVariantNodeBudget(under?2u:3u);DRW_Textstyle style;style.fontFamily=5570578;int code=0;bool ok=true;
        while(reader->readRec(&code))if(!DrwObjectEncodeTestAccess::styleCode(style,code,reader)){ok=false;break;}
        if(ok==under||style.extData.size()!=(under?2u:3u)||style.fontFamily!=(under?5570578:50331682)||reader->typedVariantNodeBudgetExceeded()!=under)return false;
        ++budgets;
    }
    std::cout<<"STYLE scoped font profiles="<<profiles<<"; copy/assignment controls="<<copies<<"; original profiles="<<origins<<"; withheld profiles="<<withheld
        <<"; output/readbacks="<<outputs<<"; edited font/readbacks="<<edits<<"; source opt-out controls="<<optouts<<"; callback throw/retries="<<throws<<"; escaped APPID profiles="<<escapes<<"; empty family profiles="<<emptyFamilies<<"; valid INTEGER64 flags="<<integer64Flags
        <<"; legacy ASCII/Binary profiles="<<legacy<<"; invalid output/retries="<<refusals<<"; unsupported target/retries="<<versionRefusals<<"; reached writer allocation failures="<<reached<<"; unreached offsets="<<unreached
        <<"; integer-node limits="<<budgets<<"; physical integer spans="<<spans<<"; corrected retries="<<retries<<'\n';
    return profiles==216u&&copies==216u&&origins==168u&&withheld==48u&&outputs==432u&&edits==432u&&optouts==6u&&throws==6u&&escapes==3u&&emptyFamilies==3u&&integer64Flags==2u&&legacy==8u&&refusals==12u&&versionRefusals==2u&&reached>0u&&reached+unreached==128u&&budgets==2u&&spans>0u&&retries==148u;
}
