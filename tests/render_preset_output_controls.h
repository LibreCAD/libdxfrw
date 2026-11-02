// SPDX-License-Identifier: GPL-2.0-or-later
// Independent output checks against locally authored literals. No production
// decoder, preset-profile selector or writer code table is an oracle here.
namespace renderPresetOutputControls {
Tags expectedBody(unsigned form, bool typed) {
    auto tags = renderPresetControls::record(form);
    const auto first = slot(tags, 100, "AcDbRenderSettings");
    const auto last = slot(tags, 1001, "REGISTERED_APP");
    Tags result(tags.begin() + static_cast<std::ptrdiff_t>(first),
                tags.begin() + static_cast<std::ptrdiff_t>(last));
    if (typed && form == 4u) {
        // The independently documented canonical writer form puts the optional
        // false flag in the common body, before the RapidRT subclass marker.
        const auto flag = result.back();
        result.pop_back();
        const auto derived = slot(result, 100, "AcDbRapidRTRenderSettings");
        result.insert(result.begin() + static_cast<std::ptrdiff_t>(derived), flag);
    }
    return result;
}

bool bodyMatches(const std::vector<AttdefWireTag>& actual, const Tags& expected) {
    if (actual.size() != expected.size()) return false;
    for (std::size_t i = 0; i < actual.size(); ++i) {
        const auto& field = actual[i];
        const auto& value = expected[i];
        if (field.code != value.code || field.begin >= field.valueBegin
            || field.valueBegin >= field.end
            || (i && field.begin != actual[i - 1u].end)) return false;
        if (value.code == 1 || value.code == 100) {
            if (field.kind != AttdefWireTag::String || field.text != value.value) return false;
        } else if (value.code == 40) {
            const auto number = std::stod(value.value);
            if (field.kind != AttdefWireTag::Real
                || std::memcmp(&number, &field.real, sizeof(number))) return false;
        } else if (value.code == 70 || value.code == 90 || value.code == 290) {
            if (field.kind != AttdefWireTag::Integer || field.integer != std::stoll(value.value)) return false;
        } else return false;
    }
    return true;
}

bool selectBody(const std::vector<AttdefWireTag>& wire, unsigned form,
                std::vector<AttdefWireTag>& body) {
    std::size_t first = wire.size(), end = wire.size();
    for (std::size_t i = 0; i < wire.size(); ++i) {
        if (wire[i].code == 0 && wire[i].text == renderPresetControls::name(form)) {
            if (first != wire.size()) return false;
            first = i;
        }
    }
    if (first == wire.size()) return false;
    for (auto i = first + 1u; i < wire.size(); ++i) {
        if (wire[i].code == 0) { end = i; break; }
    }
    unsigned depth = 0u, self = 0u, owner = 0u;
    auto start = end;
    for (auto i = first + 1u; i < end; ++i) {
        const auto& field = wire[i];
        if (field.code == 102) {
            if (field.text == "}") { if (!depth) return false; --depth; }
            else if (!field.text.empty() && field.text.front() == '{') ++depth;
            else return false;
        } else if (!depth && field.code == 100 && field.text == "AcDbRenderSettings") {
            start = i;
            break;
        } else if (!depth && (field.code == 5 || field.code == 330)) {
            if (field.kind != AttdefWireTag::String) return false;
            const auto handle = std::stoull(field.text, nullptr, 16);
            if (field.code == 5) { if (self++ || handle != 0xEA02u) return false; }
            else if (owner++ || handle != 0xEE00u) return false;
        }
    }
    if (depth || self != 1u || owner != 1u || start == end || end == wire.size()) return false;
    body.assign(wire.begin() + static_cast<std::ptrdiff_t>(start),
                wire.begin() + static_cast<std::ptrdiff_t>(end));
    return true;
}

bool tests(const std::filesystem::path& dir) {
    const auto source = dir / "preset-source.dxf", output = dir / "preset-output.dxf";
    std::size_t profiles = 0u, fields = 0u, mutations = 0u, identities = 0u, refused = 0u, retries = 0u;
    const auto fail = [&](int line) {
        std::cerr << "Preset output check line=" << line << " profiles=" << profiles << '\n';
        return false;
    };
    for (unsigned form = 0u; form < 10u; ++form) for (unsigned encoding : {0u, 2u}) {
        dx_iface reader;
        dx_data data;
        if (!writeBlockBinding(source, renderPresetControls::exportDocument(form), encoding)
            || !reader.fileImport(source.string(), &data, false) || data.renderSettings.size() != 1u
            || data.rawRenderSettingsObjects.size() != 1u) return fail(__LINE__);
        for (bool typed : {false, true}) {
            if (typed) {
                dxfRW writer(output.string().c_str());
                renderPresetControls::TypedWriter target(writer, data.renderSettings.front());
                if (!writer.write(&target, DRW::AC1027, encoding == 2u) || !target.wrote) return fail(__LINE__);
            } else if (!reader.fileExport(output.string(), DRW::AC1027, encoding == 2u, &data, false)) return fail(__LINE__);
            const auto wire = attdefWireTags(output, encoding == 2u);
            const auto expected = expectedBody(form, typed);
            std::vector<AttdefWireTag> actual;
            if (!selectBody(wire, form, actual) || !bodyMatches(actual, expected)) return fail(__LINE__);
            identities += 2u;
            fields += actual.size();
            for (std::size_t i = 0; i < actual.size(); ++i) {
                auto changed = actual;
                changed[i].code = 999;
                if (bodyMatches(changed, expected)) return fail(__LINE__);
                ++mutations;
                changed = actual;
                if (changed[i].kind == AttdefWireTag::Integer) ++changed[i].integer;
                else if (changed[i].kind == AttdefWireTag::Real) changed[i].real += 0.125;
                else changed[i].text = "edited authored output field";
                if (bodyMatches(changed, expected)) return fail(__LINE__);
                ++mutations;
            }
            ++profiles;
        }
        const auto original = data.rawRenderSettingsObjects.front();
        for (bool text : {false, true}) {
            auto& raw = data.rawRenderSettingsObjects.front();
            const auto field = std::find_if(raw.groups.begin(), raw.groups.end(), [&](const auto& v) { return v.code() == (text ? 1 : 290); });
            if (field == raw.groups.end()) return fail(__LINE__);
            *field = text ? DRW_Variant(1, UTF8STRING("edited raw preset")) : DRW_Variant(290, field->i_val() ? 0 : 1);
            { std::ofstream sentinel(output, std::ios::binary); sentinel << "KEEP_RAW_PRESET_DESTINATION"; }
            if (reader.fileExport(output.string(), DRW::AC1027, encoding == 2u, &data, false)) return fail(__LINE__);
            std::ifstream saved(output, std::ios::binary);
            const std::string kept((std::istreambuf_iterator<char>(saved)), {});
            if (kept != "KEEP_RAW_PRESET_DESTINATION") return fail(__LINE__);
            saved.close();
            ++refused;
            raw = original;
            if (!reader.fileExport(output.string(), DRW::AC1027, encoding == 2u, &data, false)) return fail(__LINE__);
            std::vector<AttdefWireTag> actual;
            if (!selectBody(attdefWireTags(output, encoding == 2u), form, actual)
                || !bodyMatches(actual, expectedBody(form, false))) return fail(__LINE__);
            ++retries;
        }
    }
    std::cout << "Render preset emitted profiles=" << profiles << "; independent body fields=" << fields
              << "; handle/owner checks=" << identities << "; field/code refusals=" << mutations
              << "; raw edit rollbacks=" << refused << "; corrected retries=" << retries << '\n';
    return profiles == 40u && fields == 1396u && identities == 80u && mutations == 2792u && refused == 40u && retries == 40u;
}
}

// Authored typed edit controls; expected fields come from literal source tags.
namespace renderPresetEditControls {
using A=DxIfaceTableSourceTestAccess;
void aliases(DRW_RenderSettings& value,unsigned form) {
    value.m_environmentImageFilename=value.m_strings[1];
    value.m_fogEnabled=value.m_bools[0];value.m_backfacesEnabled=value.m_bools[form==5u?1u:2u];
    value.m_environmentImageEnabled=value.m_bools[form==5u?2u:3u];
    if(form!=5u){value.m_description=value.m_strings[2];value.m_displayIndex=value.m_longs[1];value.m_fogBackgroundEnabled=value.m_bools[1];}
    if(value.m_hasPredefinedPresent)value.m_hasPredefined=value.m_bools[4];
}
Tags edit(DRW_RenderSettings& value,unsigned form,unsigned mode) {
    auto body=renderPresetOutputControls::expectedBody(form,false);
    std::map<int,std::size_t> ordinal;
    bool derived=false;
    for(auto& field:body){
        if(field.code==100){derived=field.value!="AcDbRenderSettings";continue;}
        const auto at=ordinal[field.code]++;
        if((field.code==1&&at==0u)||(field.code==90&&(at==0u||(form>=2u&&at==2u))))continue;
        if(mode==1u&&((derived&&!(form==4u&&field.code==290))||(field.code==90&&form==5u)))continue;
        if(field.code==1){field.value+=" edited";if(mode!=1u)value.m_strings[at]=field.value;}
        else if(field.code==90){field.value=std::to_string(std::stol(field.value)+2);if(mode!=1u)value.m_longs[at]=std::stoi(field.value);}
        else if(field.code==70){field.value=field.value=="1"?"0":"1";value.m_shorts[at]=std::stoi(field.value);}
        else if(field.code==290){field.value=field.value=="1"?"0":"1";if(mode!=1u)value.m_bools[at]=field.value=="1";}
        else if(field.code==40){field.value=std::to_string(std::stod(field.value)+0.125);value.m_doubles[at]=std::stod(field.value);}
    }
    if(mode==1u){value.m_environmentImageFilename+=" edited";value.m_fogEnabled=!value.m_fogEnabled;
        value.m_backfacesEnabled=!value.m_backfacesEnabled;value.m_environmentImageEnabled=!value.m_environmentImageEnabled;
        if(form!=5u){value.m_description+=" edited";value.m_displayIndex+=2;value.m_fogBackgroundEnabled=!value.m_fogBackgroundEnabled;}
        if(value.m_hasPredefinedPresent)value.m_hasPredefined=!value.m_hasPredefined;}
    else if(mode==2u)aliases(value,form);
    return body;
}
bool preserved(const std::filesystem::path& path){std::ifstream file(path,std::ios::binary);return std::string((std::istreambuf_iterator<char>(file)),{})=="KEEP_EDIT_DESTINATION";}
void sentinel(const std::filesystem::path& path){std::ofstream file(path,std::ios::binary);file<<"KEEP_EDIT_DESTINATION";}
struct EditingWriter:dx_iface {
    unsigned when=0u;bool enabled=true;
    void change(){cData->renderSettings.front().m_description+=" callback";}
    void writeHeader(DRW_Header& header)override{dx_iface::writeHeader(header);if(enabled&&when==0u)change();}
    void writeObjects()override{
        if(enabled&&when==1u){const auto original=cData->renderSettings.front();change();dx_iface::writeObjects();cData->renderSettings.front()=original;}
        else{dx_iface::writeObjects();if(enabled&&when==2u)change();}
    }
    bool finalizeDxfWrite()override{if(enabled&&when==3u)change();return dx_iface::finalizeDxfWrite();}
};
struct BudgetWriter:dx_iface{std::pair<std::size_t,std::size_t> size;void writeObjects()override{size=A::presetEditSize(*this);dx_iface::writeObjects();}};
struct FaultWriter:dx_iface {
    long offset=-1;bool refused=false;
    bool finalizeDxfWrite()override{
        if(offset>=0){SymbolAllocationFailure fault(offset);refused=!A::presetEditsUnchanged(*this);if(refused)return false;}
        return dx_iface::finalizeDxfWrite();
    }
};
bool tests(const std::filesystem::path& dir) {
    const auto input=dir/"edit-input.dxf",output=dir/"edit-output.dxf";
    std::size_t profiles=0u,fields=0u,refusals=0u,retries=0u,callbacks=0u,limits=0u,reached=0u,unreached=0u,corrections=0u,metadata=0u,versions=0u,textProfiles=0u,textRollbacks=0u,observedRefusals=0u,prepareFaults=0u,prepareUnreached=0u,preparedRetries=0u;
    const auto fail=[&](int line){std::cerr<<"Preset edits line="<<line<<" profiles="<<profiles<<" fields="<<fields<<" refusals="<<refusals<<'\n';return false;};
    for(unsigned form=0u;form<10u;++form)for(unsigned encoding:{0u,2u})for(unsigned mode=0u;mode<3u;++mode){
        dx_iface writer;dx_data data;
        if(!writeBlockBinding(input,renderPresetControls::exportDocument(form),encoding)||!writer.fileImport(input.string(),&data,false))return fail(__LINE__);
        const auto original=data.renderSettings.front();const auto sourceReport=dx_iface::renderingSourceReport(data);
        const auto expected=edit(data.renderSettings.front(),form,mode);const auto edited=data.renderSettings.front();
        if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);
        std::vector<AttdefWireTag> body;
        if(!renderPresetOutputControls::selectBody(attdefWireTags(output,encoding==2u),form,body)
            ||!renderPresetOutputControls::bodyMatches(body,expected))return fail(__LINE__);
        fields+=body.size();
        if(mode==2u){
            // Rebuild the facade's frozen plan, then feed real emitted section
            // bytes and single-byte corruptions into the actual inspector.
            dx_iface proof;dxfRW codec(output.string().c_str());codec.setHandleRemap({{0x40u,0xCu}});
            if(!A::presetDirect(proof,data,codec)||!A::preparePresetEdits(proof,data,encoding==2u))return fail(__LINE__);
            const auto wire=attdefWireTags(output,encoding==2u);std::size_t first=0u,last=0u;
            for(std::size_t i=0u;i+1u<wire.size();++i)if(wire[i].code==0&&wire[i].text=="SECTION"&&wire[i+1u].text=="OBJECTS"){
                first=wire[i+1u].end;for(auto j=i+2u;j<wire.size();++j)if(wire[j].code==0&&wire[j].text=="ENDSEC"){last=wire[j].begin;break;}break;}
            std::ifstream file(output,std::ios::binary);const std::string bytes((std::istreambuf_iterator<char>(file)),{});
            if(!last||last<=first)return fail(__LINE__);
            const auto section=bytes.substr(first,last-first);
            dxfRW::DxfObjectsWriteView view{section,DRW::AC1027,encoding==2u,"UTF-8",dxfRW::DxfCompatibilityProfile::StandaloneSafe};
            if(!A::presetOutput(proof,view))return fail(__LINE__);
            for(const auto& field:body){auto changed=section;auto index=field.valueBegin-first;
                if(encoding==2u)changed.at(index)^=1;
                else{while(index<changed.size()&&(changed[index]==' '||changed[index]=='\t'))++index;
                    if(index==changed.size())return fail(__LINE__);
                    auto& byte=changed[index];byte=byte=='1'?'2':byte=='0'?'1':byte>='2'&&byte<='9'?'1':'X';}
                view.bytes=changed;if(A::presetOutput(proof,view))return fail(__LINE__);++observedRefusals;
            }
            view.bytes=section;if(!A::presetOutput(proof,view))return fail(__LINE__);
        }
        dx_iface reader;dx_data reopened;
        if(!reader.fileImport(output.string(),&reopened,false)||reopened.renderSettings.size()!=1u)return fail(__LINE__);
        // Compare against independent literal arrays, with aliases supplied by
        // the reader; do not use the production normalizer as an oracle.
        auto normalized=edited;
        if(mode!=1u)aliases(normalized,form);
        else{normalized.m_strings[1]=normalized.m_environmentImageFilename;normalized.m_bools[0]=normalized.m_fogEnabled;
            normalized.m_bools[form==5u?1u:2u]=normalized.m_backfacesEnabled;normalized.m_bools[form==5u?2u:3u]=normalized.m_environmentImageEnabled;
            if(form!=5u){normalized.m_strings[2]=normalized.m_description;normalized.m_longs[1]=normalized.m_displayIndex;normalized.m_bools[1]=normalized.m_fogBackgroundEnabled;}
            if(normalized.m_hasPredefinedPresent)normalized.m_bools[4]=normalized.m_hasPredefined;}
        if(!dxfRendering::Observation::render(normalized,reopened.renderSettings.front())
            ||!dxfRendering::Observation::render(edited,data.renderSettings.front()))return fail(__LINE__);
        const auto after=dx_iface::renderingSourceReport(data);
        if(after.originalSourcesMatched!=sourceReport.originalSourcesMatched||after.outputEligible||after.recursiveClosureMatched||after.ownerTargetsResolved)return fail(__LINE__);
        data.renderSettings.front()=original;
        if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);
        ++retries;++profiles;
    }
    for(unsigned encoding:{0u,2u})for(unsigned fault=0u;fault<13u;++fault){
        dx_iface writer;dx_data data;
        if(!writeBlockBinding(input,renderPresetControls::exportDocument(3u),encoding)||!writer.fileImport(input.string(),&data,false))return fail(__LINE__);
        auto& value=data.renderSettings.front();const auto original=value;
        if(fault==0u){value.m_description="alias change";value.m_strings[2]="conflicting vector";}
        if(fault==1u)value.m_fogColorR=1;
        if(fault==2u)value.m_shorts[0]=32768;
        if(fault==3u)value.m_doubles[0]=std::numeric_limits<double>::infinity();
        if(fault==4u)value.m_doubles[0]=std::numeric_limits<double>::quiet_NaN();
        if(fault==5u)value.m_name=value.m_strings[0]="unqualified rename";
        if(fault==6u)value.m_procedure=17;
        if(fault==7u)value.m_bools.push_back(true);
        if(fault==8u)value.m_longs[2]=99;
        if(fault==9u)value.xDictHandle=0;
        if(fault==10u)value.reactorHandles.push_back(0xEF01u);
        if(fault==11u)value.appData.push_back({DRW_Variant(102,UTF8STRING("{NEW_METADATA")),DRW_Variant(102,UTF8STRING("}"))});
        if(fault==12u)value.m_environmentImageFilename=std::string("bad\0text",8u);
        sentinel(output);
        if(writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)||!preserved(output))return fail(__LINE__);
        ++refusals;value=original;value.m_description="corrected edit";
        if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);++retries;
    }
    for(unsigned encoding:{0u,2u})for(unsigned when=0u;when<4u;++when){
        EditingWriter writer;writer.when=when;dx_data data;
        if(!writeBlockBinding(input,renderPresetControls::exportDocument(3u),encoding)||!writer.fileImport(input.string(),&data,false))return fail(__LINE__);
        data.renderSettings.front().m_description="valid initial edit";const auto original=data.renderSettings.front();sentinel(output);
        if(writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)||!preserved(output))return fail(__LINE__);
        ++callbacks;writer.enabled=false;data.renderSettings.front()=original;
        if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);++retries;
    }
    for(unsigned encoding:{0u,2u}){
        BudgetWriter writer;dx_data data;
        if(!writeBlockBinding(input,renderPresetControls::exportDocument(9u),encoding)||!writer.fileImport(input.string(),&data,false))return fail(__LINE__);
        data.renderSettings.front().m_description="bounded edit";
        if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)||writer.size.first!=1u||!writer.size.second)return fail(__LINE__);
        const auto exact=writer.size;
        A::presetEditLimit(writer,exact.first,exact.second);
        if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);
        for(bool records:{false,true}){A::presetEditLimit(writer,exact.first-(records?1u:0u),exact.second-(records?0u:1u));sentinel(output);
            if(writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)||!preserved(output))return fail(__LINE__);++limits;
            A::presetEditLimit(writer,exact.first,exact.second);
            if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);++retries;}
        FaultWriter faults;unsigned untouched=0u;
        for(long offset=0;offset<256&&untouched<2u;++offset){faults.offset=offset;faults.refused=false;const auto before=symbolFailedAllocations;sentinel(output);
            const bool ok=faults.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false);
            if(symbolFailedAllocations!=before){if(ok||!faults.refused||!preserved(output))return fail(__LINE__);++reached;untouched=0u;}
            else{if(!ok||faults.refused)return fail(__LINE__);++unreached;++untouched;}
            faults.offset=-1;if(!faults.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);++corrections;
        }
        if(untouched!=2u)return fail(__LINE__);
    }
    for(unsigned encoding:{0u,2u})for(bool named:{false,true}){
        dx_iface writer;dx_data data;
        if(!writeBlockBinding(input,renderPresetControls::exportDocument(3u),encoding)||!writer.fileImport(input.string(),&data,false))return fail(__LINE__);
        std::string text;for(unsigned i=0u;i<4096u;++i)text+="\\U+";
        if(named)data.renderSettings.front().m_description=text;else data.renderSettings.front().m_strings[2]=text;
        A::presetEditLimit(writer,1u,32768u);sentinel(output);
        if(writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)||!preserved(output))return fail(__LINE__);++limits;
        A::presetEditLimit(writer,1u,64u*1024u*1024u);
        if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);++retries;
        dx_iface reader;dx_data reopened;if(!reader.fileImport(output.string(),&reopened,false)||reopened.renderSettings.front().m_description!=text)return fail(__LINE__);
    }
    for(const std::string version:{"AC1021","AC1024","AC1032"})for(unsigned encoding:{0u,2u}){
        auto tags=renderPresetControls::exportDocument(4u);tags[slot(tags,1,"AC1027")].value=version;
        dx_iface writer;dx_data data;if(!writeBlockBinding(input,tags,encoding)||!writer.fileImport(input.string(),&data,false))return fail(__LINE__);
        data.renderSettings.front().m_description="Unicode \xCE\xA9 and literal \\U+0041";
        if(!writer.fileExport(output.string(),data.sourceVersion,encoding==2u,&data,false))return fail(__LINE__);
        dx_iface reader;dx_data reopened;if(!reader.fileImport(output.string(),&reopened,false)
            ||reopened.renderSettings.front().m_description!=data.renderSettings.front().m_description)return fail(__LINE__);
        ++versions;
    }
    for(unsigned encoding:{0u,2u})for(bool binary:{false,true})for(bool strict:{false,true}){
        dx_iface writer;dx_data data;
        if(!writeBlockBinding(input,renderPresetControls::exportDocument(3u),encoding)||!writer.fileImport(input.string(),&data,false))return fail(__LINE__);
        const std::string text="Unicode \xCE\xA9 literal \\U+0041 \\M+5B2A caret ^J";
        data.renderSettings.front().m_description=text;
        const auto policy=strict?DRW::DxfTextWritePolicy::RequireLossless:DRW::DxfTextWritePolicy::Compatible;
        if(!writer.fileExport(output.string(),DRW::AC1027,binary,&data,false,false,dx_dxfSourceTextPolicy::Compatible,policy))return fail(__LINE__);
        dx_iface reader;dx_data reopened;if(!reader.fileImport(output.string(),&reopened,false)||reopened.renderSettings.front().m_description!=text)return fail(__LINE__);
        ++textProfiles;
        if(!binary){data.renderSettings.front().m_description="unsafe\nASCII record";sentinel(output);
            if(writer.fileExport(output.string(),DRW::AC1027,false,&data,false)||!preserved(output))return fail(__LINE__);++textRollbacks;
            data.renderSettings.front().m_description=text;
            if(!writer.fileExport(output.string(),DRW::AC1027,false,&data,false))return fail(__LINE__);++retries;}
    }
    for(unsigned encoding:{0u,2u}){
        auto tags=renderPresetControls::exportDocument(3u);
        auto at=slot(tags,5,"EA02")+1u;
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{102,"{AUTHORED_EDIT_SCOPE"},{5,"40"},{330,"40"},{340,"40"},{360,"40"},{100,"OPAQUE_SUBCLASS"},{1,"opaque payload"},{102,"}"},
            {102,"{ACAD_REACTORS"},{330,"EF01"},{102,"}"}});
        // Exercise a real emitted-identity collision. Application code5 is
        // literal; translated reference codes retain the existing raw policy.
        for(auto& tag:tags)if(tag.value=="EA02")tag.value="1F";
        auto objects=slot(tags,2,"OBJECTS")-1u;
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(objects),{{0,"SECTION"},{2,"TABLES"},{0,"TABLE"},{2,"APPID"},{5,"E100"},{330,"0"},{100,"AcDbSymbolTable"},{70,"1"},
            {0,"APPID"},{5,"E101"},{330,"E100"},{100,"AcDbSymbolTableRecord"},{100,"AcDbRegAppTableRecord"},{2,"EDIT_APP"},{70,"0"},{0,"ENDTAB"},{0,"ENDSEC"}});
        auto end=slot(tags,5,"1F");while(end<tags.size()&&tags[end].code!=0)++end;
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(end),{{1001,"EDIT_APP"},{1000,"authored xdata payload"},{1003,"0"},{1005,"EF01"},{1070,"17"}});
        dx_iface writer;dx_data data;if(!writeBlockBinding(input,tags,encoding)||!writer.fileImport(input.string(),&data,false))return fail(__LINE__);
        data.renderSettings.front().m_description="metadata retained edit";
        if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);
        const auto wire=attdefWireTags(output,encoding==2u);bool record=false,app=false,reactor=false;unsigned checked=0u;std::string self;
        for(const auto& tag:wire){if(tag.code==0){record=tag.text=="RAPIDRTRENDERSETTINGS";continue;}if(!record)continue;
            if(tag.code==102){app=tag.text=="{AUTHORED_EDIT_SCOPE";reactor=tag.text=="{ACAD_REACTORS";continue;}
            if(app&&(tag.code==5||tag.code==330||tag.code==340||tag.code==360)){if(tag.text!=(tag.code==5?"40":"C"))return fail(__LINE__);++checked;}
            if(app&&tag.code==1){if(tag.text!="opaque payload")return fail(__LINE__);++checked;}
            if(reactor&&tag.code==330){if(tag.text!="EF01")return fail(__LINE__);++checked;}
            if(!app&&!reactor&&tag.code==5)self=tag.text;
            if(tag.code==360&&!app){if(tag.text!="EE10")return fail(__LINE__);++checked;}
            if(tag.code==1001){if(tag.text!="EDIT_APP")return fail(__LINE__);++checked;}
            if(tag.code==1000){if(tag.text!="authored xdata payload")return fail(__LINE__);++checked;}
            if(tag.code==1003){if(tag.text!="0")return fail(__LINE__);++checked;}
            if(tag.code==1005){if(tag.text!="EF01")return fail(__LINE__);++checked;}
            if(tag.code==1070){if(tag.integer!=17)return fail(__LINE__);++checked;}
        }
        if(checked!=12u||self.empty()||self=="1F")return fail(__LINE__);++metadata;
    }
    for(unsigned encoding:{0u,2u}){
        dx_iface writer;dx_data data;
        if(!writeBlockBinding(input,renderPresetControls::exportDocument(0u),encoding)||!writer.fileImport(input.string(),&data,false))return fail(__LINE__);
        data.renderSettings.front().m_description="allocation control";unsigned untouched=0u;
        for(long offset=0;offset<1024&&untouched<2u;++offset){const auto before=symbolFailedAllocations;bool ok=false;
            {SymbolAllocationFailure fault(offset);ok=A::preparePresetEdits(writer,data,encoding==2u);}
            if(symbolFailedAllocations!=before){if(ok)return fail(__LINE__);++prepareFaults;untouched=0u;}
            else{if(!ok)return fail(__LINE__);++prepareUnreached;++untouched;}
            if(!A::preparePresetEdits(writer,data,encoding==2u)||!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);
            ++preparedRetries;
        }
        if(untouched!=2u)return fail(__LINE__);
    }
    std::cout<<"Render preset edited profiles="<<profiles<<"; independent fields="<<fields<<"; invalid/conflicting rollbacks="<<refusals
        <<"; callback rollbacks="<<callbacks<<"; exact/one-under limits="<<limits<<"; reached finalization faults="<<reached
        <<"; unreached offsets="<<unreached<<"; corrected finalizations="<<corrections<<"; corrected retries="<<retries
        <<"; observed edited-field refusals="<<observedRefusals<<"; preparation faults="<<prepareFaults<<"; preparation unreached offsets="<<prepareUnreached<<"; prepared export retries="<<preparedRetries
        <<"; text/policy profiles="<<textProfiles<<"; text rollbacks="<<textRollbacks
        <<"; other-version text profiles="<<versions<<"; scoped metadata/collision profiles="<<metadata<<'\n';
    return profiles==60u&&refusals==26u&&callbacks==8u&&limits==8u&&reached&&unreached==4u
        &&corrections==reached+unreached&&retries==106u&&versions==6u&&textProfiles==8u&&textRollbacks==4u&&metadata==2u
        &&observedRefusals==698u&&prepareFaults&&prepareUnreached==4u&&preparedRetries==prepareFaults+prepareUnreached;
}
}

namespace renderPresetTransactionControls {
using A=DxIfaceTableSourceTestAccess;
bool present(unsigned form){return form==1u||form==3u||form==4u||form==7u||form==9u;}
std::string changedName(unsigned form){return std::string(48u,'P')+" renamed 'Profile' "+std::to_string(form)+" \xCE\xA9";}
void dictionaryName(dx_data& data,std::uint32_t target,const std::string& name){
    for(auto& dictionary:data.dictionaries)if(dictionary.handle==0xEE00u)
        for(auto& entry:dictionary.m_entries)if(entry.m_handle==target)entry.m_name=name;
}
Tags edit(dx_data& data,unsigned form,unsigned mode,unsigned operation){
    auto& value=data.renderSettings.front();auto body=renderPresetOutputControls::expectedBody(form,false);
    if(operation!=1u){const auto name=changedName(form);dictionaryName(data,value.handle,name);
        if(mode!=1u)value.m_name=name;if(mode!=0u)value.m_strings[0]=name;
        body[slot(body,1,"AuthoredPreset")].value=name;}
    if(operation!=0u){const bool before=present(form);
        if(mode!=1u){value.m_hasPredefinedPresent=!before;value.m_hasPredefined=!before;}
        if(mode!=0u){if(before)value.m_bools.erase(value.m_bools.begin()+4);else value.m_bools.insert(value.m_bools.begin()+4,true);}
        if(before){if(form==4u)body.pop_back();else body.erase(body.begin()+10);}
        else body.insert(body.begin()+10,{290,"1"});
    }
    return body;
}
using Wire=std::vector<AttdefWireTag>;
bool dictionaryMatches(const Wire& wire,const std::string& name,std::uint32_t target){
    bool dictionary=false,selected=false;std::string pending;unsigned count=0u;
    for(const auto& tag:wire){if(tag.code==0){dictionary=tag.text=="DICTIONARY";selected=false;pending.clear();}
        else if(dictionary&&tag.code==5)selected=tag.text=="EE00";
        else if(selected&&tag.code==3)pending=tag.text;
        else if(selected&&(tag.code==350||tag.code==360)&&std::stoul(tag.text,nullptr,16)==target){if(pending!=name)return false;++count;}}
    return count==1u;
}
struct EditingWriter:dx_iface{
    unsigned when=0u,what=0u;bool enabled=true;
    void change(){auto& value=cData->renderSettings.front();
        if(what==0u||what==3u){value.m_name+=" callback";value.m_strings[0]=value.m_name;}
        if(what==1u||what==3u)dictionaryName(*cData,value.handle,changedName(3u)+" callback");
        if(what==2u||what==3u)value.m_hasPredefinedPresent=!value.m_hasPredefinedPresent;
    }
    void writeHeader(DRW_Header& header)override{dx_iface::writeHeader(header);if(enabled&&when==0u)change();}
    void writeObjects()override{
        if(enabled&&when==1u){const auto value=cData->renderSettings.front();const auto dictionaries=cData->dictionaries;
            change();dx_iface::writeObjects();cData->renderSettings.front()=value;cData->dictionaries=dictionaries;}
        else{dx_iface::writeObjects();if(enabled&&when==2u)change();}
    }
    bool finalizeDxfWrite()override{if(enabled&&when==3u)change();return dx_iface::finalizeDxfWrite();}
};
struct BudgetWriter:dx_iface{std::pair<std::size_t,std::size_t> size;void writeObjects()override{size=A::presetEditSize(*this);dx_iface::writeObjects();}};
Tags twoPresets(){
    auto tags=renderPresetControls::exportDocument(3u);
    const auto entry=slot(tags,3,"AuthoredPreset")+2u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(entry),{{3,"SecondPreset"},{350,"EA03"}});
    auto second=renderPresetControls::record(3u);const auto xdata=slot(second,1001,"REGISTERED_APP");second.erase(second.begin()+static_cast<std::ptrdiff_t>(xdata),second.end());
    const auto reactor=slot(second,102,"{ACAD_REACTORS");second.erase(second.begin()+static_cast<std::ptrdiff_t>(reactor),second.begin()+static_cast<std::ptrdiff_t>(reactor+3u));
    for(auto& tag:second){if(tag.value=="EA02")tag.value="EA03";if(tag.value=="EE10")tag.value="EE11";if(tag.value=="AuthoredPreset")tag.value="SecondPreset";}
    second.insert(second.end(),{{0,"DICTIONARY"},{5,"EE11"},{330,"EA03"},{100,"AcDbDictionary"},{280,"1"},{281,"1"}});
    auto end=tags.size()-2u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(end),second.begin(),second.end());return tags;
}
bool tests(const std::filesystem::path& dir){
    const auto input=dir/"transaction-input.dxf",output=dir/"transaction-output.dxf";
    std::size_t profiles=0u,fields=0u,readbacks=0u,validNames=0u,retries=0u,refusals=0u,callbacks=0u,mutations=0u,quotas=0u,multi=0u,collisions=0u,metadata=0u,reached=0u,unreached=0u,corrections=0u;
    const auto fail=[&](int line){std::cerr<<"Preset transaction line="<<line<<" profiles="<<profiles<<" refusals="<<refusals<<" callbacks="<<callbacks<<'\n';return false;};
    for(unsigned form=0u;form<10u;++form)for(unsigned encoding:{0u,2u})for(bool binary:{false,true})for(unsigned mode=0u;mode<3u;++mode)for(unsigned operation=0u;operation<3u;++operation){
        if(form==5u&&operation!=0u)continue;
        dx_iface writer;dx_data data;if(!writeBlockBinding(input,renderPresetControls::exportDocument(form),encoding)||!writer.fileImport(input.string(),&data,false))return fail(__LINE__);
        const auto original=data.renderSettings.front();const auto dictionaries=data.dictionaries;
        const auto expected=edit(data,form,mode,operation);const auto live=data.renderSettings.front();
        if(!writer.fileExport(output.string(),DRW::AC1027,binary,&data,false))return fail(__LINE__);
        const auto wire=attdefWireTags(output,binary);Wire body;
        if(!renderPresetOutputControls::selectBody(wire,form,body)||!renderPresetOutputControls::bodyMatches(body,expected)
            ||!dictionaryMatches(wire,operation==1u?"AuthoredPreset":changedName(form),0xEA02u))return fail(__LINE__);
        fields+=body.size();
        dx_iface reader;dx_data reopened;if(!reader.fileImport(output.string(),&reopened,false)||reopened.renderSettings.size()!=1u)return fail(__LINE__);
        const auto& actual=reopened.renderSettings.front();const bool after=operation==0u?present(form):!present(form);
        if(actual.m_name!=(operation==1u?"AuthoredPreset":changedName(form))||actual.m_strings[0]!=actual.m_name
            ||actual.m_hasPredefinedPresent!=after||actual.m_hasPredefined!=(after&&(operation!=0u||form!=4u))
            ||!dxfRendering::Observation::render(live,data.renderSettings.front()))return fail(__LINE__);
        ++readbacks;
        if(mode==2u&&operation==2u&&((!binary&&encoding==0u)||(binary&&encoding==2u))){
            dx_iface proof;dxfRW codec(output.string().c_str());codec.setHandleRemap({{0x40u,0xCu}});
            if(!A::presetDirect(proof,data,codec)||!A::preparePresetEdits(proof,data,binary))return fail(__LINE__);
            std::size_t first=0u,last=0u;for(std::size_t i=0u;i+1u<wire.size();++i)if(wire[i].text=="SECTION"&&wire[i+1u].text=="OBJECTS"){
                first=wire[i+1u].end;for(auto j=i+2u;j<wire.size();++j)if(wire[j].code==0&&wire[j].text=="ENDSEC"){last=wire[j].begin;break;}break;}
            std::ifstream file(output,std::ios::binary);const std::string bytes((std::istreambuf_iterator<char>(file)),{});
            if(last<=first)return fail(__LINE__);const auto section=bytes.substr(first,last-first);
            dxfRW::DxfObjectsWriteView view{section,DRW::AC1027,binary,"UTF-8",dxfRW::DxfCompatibilityProfile::StandaloneSafe};
            if(!A::presetOutput(proof,view))return fail(__LINE__);
            Wire selected=body;bool selectedDictionary=false;
            for(const auto& field:wire){if(field.code==0)selectedDictionary=false;
                else if(field.code==5)selectedDictionary=field.text=="EE00"||field.text=="EF00";
                else if(selectedDictionary&&field.code==3)selected.push_back(field);}
            for(const auto& field:selected){auto changed=section;auto at=field.valueBegin-first;
                if(binary)changed.at(at)^=1;
                else{while(at<changed.size()&&(changed[at]==' '||changed[at]=='\t'))++at;
                    if(at==changed.size())return fail(__LINE__);auto& byte=changed[at];byte=byte=='1'?'2':byte=='0'?'1':byte>='2'&&byte<='9'?'1':'X';}
                view.bytes=changed;if(A::presetOutput(proof,view))return fail(__LINE__);++mutations;}
            view.bytes=section;if(!A::presetOutput(proof,view))return fail(__LINE__);
        }
        data.renderSettings.front()=original;data.dictionaries=dictionaries;
        if(!writer.fileExport(output.string(),DRW::AC1027,binary,&data,false))return fail(__LINE__);++retries;++profiles;
    }
    for(unsigned encoding:{0u,2u})for(unsigned fault=0u;fault<17u;++fault){
        const auto form=fault==15u?4u:3u;dx_iface writer;dx_data data;
        if(!writeBlockBinding(input,renderPresetControls::exportDocument(form),encoding)||!writer.fileImport(input.string(),&data,false))return fail(__LINE__);
        const auto original=data.renderSettings.front();const auto dictionaries=data.dictionaries;auto& value=data.renderSettings.front();
        if(fault==0u)value.m_name=value.m_strings[0]="MissingKey";
        if(fault==1u)dictionaryName(data,value.handle,"UnmatchedKey");
        if(fault==2u){value.m_name="NamedConflict";value.m_strings[0]="VectorConflict";dictionaryName(data,value.handle,"NamedConflict");}
        if(fault==3u)for(auto& dictionary:data.dictionaries)if(dictionary.handle==0xEE00u)dictionary.m_entries.front().m_handle=0xEA03u;
        if(fault==4u)value.parentHandle=0xEF00u;
        if(fault==5u){value.m_name=value.m_strings[0]="";dictionaryName(data,value.handle,"");}
        if(fault==6u){value.m_name=value.m_strings[0]="*Invalid";dictionaryName(data,value.handle,value.m_name);}
        if(fault==7u){value.m_name=value.m_strings[0]="Bad\\Name";dictionaryName(data,value.handle,value.m_name);}
        if(fault==8u){value.m_name=value.m_strings[0]=std::string("bad\0name",8u);dictionaryName(data,value.handle,value.m_name);}
        if(fault==9u){value.m_name=value.m_strings[0]=std::string("bad\xED\xA0\x80",6u);dictionaryName(data,value.handle,value.m_name);}
        if(fault==10u)value.m_bools.insert(value.m_bools.begin()+4,2u,true);
        if(fault==11u)value.m_kind=DRW_RenderSettings::MentalRay;
        if(fault==12u)value.m_classVersion=value.m_longs[0]=99;
        if(fault==13u)for(auto& dictionary:data.dictionaries)if(dictionary.handle==0xEE00u)dictionary.m_entries.push_back(dictionary.m_entries.front());
        if(fault==14u){value.m_name="CarrierEdit";dictionaryName(data,value.handle,value.m_name);data.rawRenderSettingsObjects.front().groups.push_back(DRW_Variant(999,UTF8STRING("raw edit")));}
        if(fault==15u){value.m_hasPredefinedPresent=false;value.m_hasPredefined=true;}
        if(fault==16u){value.m_name=value.m_strings[0]="CaseChanged";dictionaryName(data,value.handle,"CASECHANGED");}
        renderPresetEditControls::sentinel(output);
        if(writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)||!renderPresetEditControls::preserved(output))return fail(__LINE__);++refusals;
        value=original;data.dictionaries=dictionaries;
        if(fault==14u){auto& groups=data.rawRenderSettingsObjects.front().groups;groups.pop_back();}
        edit(data,form,2u,2u);
        if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);++retries;
    }
    for(unsigned encoding:{0u,2u})for(char illegal:std::string("|*\\:;<>?\"")){
        dx_iface writer;dx_data data;if(!writeBlockBinding(input,renderPresetControls::exportDocument(3u),encoding)||!writer.fileImport(input.string(),&data,false))return fail(__LINE__);
        const auto original=data.renderSettings.front();const auto dictionaries=data.dictionaries;const auto name=std::string("Invalid")+illegal+"Name";
        data.renderSettings.front().m_name=data.renderSettings.front().m_strings[0]=name;dictionaryName(data,0xEA02u,name);renderPresetEditControls::sentinel(output);
        if(writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)||!renderPresetEditControls::preserved(output))return fail(__LINE__);++refusals;
        data.renderSettings.front()=original;data.dictionaries=dictionaries;edit(data,3u,2u,2u);
        if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);++retries;
    }
    for(unsigned encoding:{0u,2u})for(unsigned when=0u;when<4u;++when)for(unsigned what=0u;what<4u;++what){
        EditingWriter writer;writer.when=when;writer.what=what;dx_data data;
        if(!writeBlockBinding(input,renderPresetControls::exportDocument(3u),encoding)||!writer.fileImport(input.string(),&data,false))return fail(__LINE__);
        edit(data,3u,2u,2u);const auto original=data.renderSettings.front();const auto dictionaries=data.dictionaries;renderPresetEditControls::sentinel(output);
        if(writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)||!renderPresetEditControls::preserved(output))return fail(__LINE__);++callbacks;
        writer.enabled=false;data.renderSettings.front()=original;data.dictionaries=dictionaries;
        if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);++retries;
    }
    for(unsigned encoding:{0u,2u}){
        BudgetWriter writer;dx_data data;if(!writeBlockBinding(input,renderPresetControls::exportDocument(0u),encoding)||!writer.fileImport(input.string(),&data,false))return fail(__LINE__);
        edit(data,0u,2u,2u);
        if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);const auto exact=writer.size;
        A::presetEditLimit(writer,exact.first,exact.second);
        if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);
        for(bool records:{false,true}){A::presetEditLimit(writer,exact.first-(records?1u:0u),exact.second-(records?0u:1u));renderPresetEditControls::sentinel(output);
            if(writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)||!renderPresetEditControls::preserved(output))return fail(__LINE__);++quotas;
            A::presetEditLimit(writer,exact.first,exact.second);if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);++retries;}
        A::presetEditLimit(writer,100000u,64u*1024u*1024u);unsigned untouched=0u;
        for(long offset=0;offset<1024&&untouched<2u;++offset){const auto before=symbolFailedAllocations;bool ok=false;
            {SymbolAllocationFailure injection(offset);ok=A::preparePresetEdits(writer,data,encoding==2u);}
            if(symbolFailedAllocations!=before){if(ok)return fail(__LINE__);++reached;untouched=0u;}
            else{if(!ok)return fail(__LINE__);++unreached;++untouched;}
            if(!A::preparePresetEdits(writer,data,encoding==2u)||!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);++corrections;
        }
        if(untouched!=2u)return fail(__LINE__);
        dx_data pair;if(!writeBlockBinding(input,twoPresets(),encoding)||!writer.fileImport(input.string(),&pair,false)||pair.renderSettings.size()!=2u)return fail(__LINE__);
        for(auto& value:pair.renderSettings){const auto name=value.handle==0xEA02u?"SecondPreset":"AuthoredPreset";value.m_name=value.m_strings[0]=name;dictionaryName(pair,value.handle,name);}
        if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&pair,false))return fail(__LINE__);
        dx_iface pairReader;dx_data pairReadback;
        if(!pairReader.fileImport(output.string(),&pairReadback,false)||pairReadback.renderSettings.size()!=2u)return fail(__LINE__);
        const auto pairWire=attdefWireTags(output,encoding==2u);
        for(const auto& value:pairReadback.renderSettings){const auto name=value.handle==0xEA02u?"SecondPreset":"AuthoredPreset";
            if(value.m_name!=name||!dictionaryMatches(pairWire,name,value.handle))return fail(__LINE__);}
        ++multi;
        const auto originals=pair.renderSettings;const auto dictionaries=pair.dictionaries;
        for(unsigned duplicate=0u;duplicate<2u;++duplicate){unsigned index=0u;for(auto& value:pair.renderSettings){const auto name=duplicate&&index++?"duplicate":"DUPLICATE";value.m_name=value.m_strings[0]=name;dictionaryName(pair,value.handle,name);}
            renderPresetEditControls::sentinel(output);if(writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&pair,false)||!renderPresetEditControls::preserved(output))return fail(__LINE__);++refusals;
            pair.renderSettings=originals;pair.dictionaries=dictionaries;if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&pair,false))return fail(__LINE__);++retries;}
        auto tags=renderPresetControls::exportDocument(4u);for(auto& tag:tags)if(tag.value=="EA02")tag.value="1F";
        auto at=slot(tags,5,"1F")+1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{102,"{TRANSACTION_SCOPE"},{290,"1"},{100,"OpaqueSubclass"},{1,"opaque name"},{102,"}"}});
        dx_data current;if(!writeBlockBinding(input,tags,encoding)||!writer.fileImport(input.string(),&current,false))return fail(__LINE__);
        edit(current,4u,2u,2u);
        if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&current,false))return fail(__LINE__);
        dx_iface reader;dx_data reopened;if(!reader.fileImport(output.string(),&reopened,false)||reopened.renderSettings.size()!=1u)return fail(__LINE__);
        const auto& value=reopened.renderSettings.front();if(value.handle==0x1Fu||value.m_name!=changedName(4u)||value.m_hasPredefinedPresent
            ||reopened.plotSettings.front().shadePlotHandle.ref!=value.handle)return fail(__LINE__);
        ++collisions;
        const auto wire=attdefWireTags(output,encoding==2u);bool app=false;unsigned scoped=0u;
        for(const auto& field:wire){if(field.code==102){app=field.text=="{TRANSACTION_SCOPE";continue;}if(!app)continue;
            if(field.code==290){if(field.integer!=1)return fail(__LINE__);++scoped;}
            if(field.code==100){if(field.text!="OpaqueSubclass")return fail(__LINE__);++scoped;}
            if(field.code==1){if(field.text!="opaque name")return fail(__LINE__);++scoped;}}
        if(scoped!=3u)return fail(__LINE__);++metadata;
    }
    for(unsigned encoding:{0u,2u})for(unsigned form:{2u,3u,4u}){
        auto tags=renderPresetControls::exportDocument(form);
        const auto boundary=form==4u?slot(tags,40,"2.875")+1u:slot(tags,90,"19")+(form==3u?2u:1u);
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(boundary),{{102,"{SLOT_BOUNDARY_SCOPE"},{290,"0"},{100,"OpaqueAtSlot"},{1,"authored slot metadata"},{102,"}"}});
        dx_iface writer;dx_data data;if(!writeBlockBinding(input,tags,encoding)||!writer.fileImport(input.string(),&data,false))return fail(__LINE__);
        const auto& source=*A::renderingSources(data)->records.at(0xEA02u);
        if(!source.preset.complete()||!source.bodyMatched||!source.namedTarget)return fail(__LINE__);
        edit(data,form,2u,2u);
        if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);
        dx_iface reader;dx_data reopened;if(!reader.fileImport(output.string(),&reopened,false)||reopened.renderSettings.front().m_hasPredefinedPresent==present(form)
            ||reopened.renderSettings.front().m_name!=changedName(form))return fail(__LINE__);
        bool app=false;unsigned checked=0u;
        for(const auto& field:attdefWireTags(output,encoding==2u)){
            if(field.code==102){app=field.text=="{SLOT_BOUNDARY_SCOPE";continue;}if(!app)continue;
            if(field.code==290){if(field.integer!=0)return fail(__LINE__);++checked;}
            if(field.code==100){if(field.text!="OpaqueAtSlot")return fail(__LINE__);++checked;}
            if(field.code==1){if(field.text!="authored slot metadata")return fail(__LINE__);++checked;}
        }
        if(checked!=3u)return fail(__LINE__);++metadata;
    }
    for(unsigned encoding:{0u,2u})for(bool binary:{false,true}){
        dx_iface writer;dx_data data;if(!writeBlockBinding(input,renderPresetControls::exportDocument(0u),encoding)||!writer.fileImport(input.string(),&data,false))return fail(__LINE__);
        const std::string name="Caret ^J \xCE\xA9";
        data.renderSettings.front().m_name=data.renderSettings.front().m_strings[0]=name;dictionaryName(data,0xEA02u,name);
        if(!writer.fileExport(output.string(),DRW::AC1027,binary,&data,false))return fail(__LINE__);
        dx_iface reader;dx_data reopened;if(!reader.fileImport(output.string(),&reopened,false)||reopened.renderSettings.front().m_name!=name)return fail(__LINE__);
        bool exact=false;for(const auto& dictionary:reopened.dictionaries)if(dictionary.handle==0xEE00u)
            exact=dictionary.m_entries.size()==1u&&dictionary.m_entries.front().m_name==name;
        if(!exact)return fail(__LINE__);++validNames;
    }
    std::cout<<"Render preset transactions="<<profiles<<"; independent fields="<<fields<<"; readbacks="<<readbacks<<"; observed-field refusals="<<mutations
        <<"; invalid/name collision rollbacks="<<refusals<<"; callback rollbacks="<<callbacks<<"; exact/one-under limits="<<quotas
        <<"; preparation faults="<<reached<<"; unreached offsets="<<unreached<<"; corrected prepared exports="<<corrections
        <<"; corrected retries="<<retries<<"; literal caret name profiles="<<validNames<<"; simultaneous name swaps="<<multi<<"; mapped identity profiles="<<collisions<<"; scoped metadata profiles="<<metadata<<'\n';
    return profiles==336u&&readbacks==profiles&&refusals==56u&&callbacks==32u&&quotas==4u&&reached&&unreached==4u&&corrections==reached+unreached
        &&retries==428u&&validNames==4u&&multi==2u&&collisions==2u&&metadata==8u&&mutations==630u;
}
}
