// SPDX-License-Identifier: GPL-2.0-or-later
// Authored graph recipes and independent emitted-field checks.
namespace renderPresetGraphOutputControls {
using Wire = std::vector<AttdefWireTag>;
Tags document(unsigned form, bool hard) {
    auto tags = renderPresetControls::exportDocument(form);
    for (auto& tag : tags) if (tag.code == 350) tag.code = hard ? 360 : 350;
    for (auto& tag : tags) if (tag.code == 280) tag.value = hard ? "1" : "0";
    for (auto& tag : tags) if (tag.code == 281) tag.value = hard ? "3" : "1";
    return tags;
}
bool exact(const Wire& actual, const Tags& expected) {
    if (actual.size() != expected.size()) return false;
    for (std::size_t i = 0u; i < actual.size(); ++i) {
        const auto& a = actual[i]; const auto& e = expected[i];
        if (a.code != e.code || a.begin >= a.valueBegin || a.valueBegin >= a.end
            || (i && actual[i - 1u].end != a.begin)) return false;
        if (a.kind == AttdefWireTag::Integer) { if (a.integer != std::stoll(e.value)) return false; }
        else if (a.kind != AttdefWireTag::String || a.text != e.value) return false;
    }
    return true;
}
bool graph(const Wire& wire, unsigned form, bool hard, std::size_t* fields = nullptr) {
    std::map<std::string, Wire> objects;
    bool section = false;
    for (std::size_t i = 0u; i < wire.size(); ++i) {
        if (wire[i].code != 0) continue;
        if (wire[i].text == "SECTION") { section = i + 1u < wire.size() && wire[i + 1u].text == "OBJECTS"; continue; }
        if (wire[i].text == "ENDSEC") { section = false; continue; }
        if (!section) continue;
        auto end = i + 1u;
        while (end < wire.size() && wire[end].code != 0) ++end;
        if (i + 1u >= end || wire[i + 1u].code != 5 || wire[i + 1u].kind != AttdefWireTag::String
            || !objects.emplace(wire[i + 1u].text, Wire(wire.begin() + static_cast<std::ptrdiff_t>(i),
                wire.begin() + static_cast<std::ptrdiff_t>(end))).second) return false;
    }
    if (objects.size() != 7u) return false;
    const int role = hard ? 360 : 350;
    const char* flag = hard ? "1" : "0"; const char* cloning = hard ? "3" : "1";
    const std::map<std::string, Tags> expected{
        {"C", {{0,"DICTIONARY"},{5,"C"},{330,"0"},{100,"AcDbDictionary"},{280,flag},{281,cloning},
            {3,renderPresetControls::root(form)},{role,"EE00"},{3,"ACAD_PLOTSETTINGS"},{role,"EF00"},{3,"ACAD_GROUP"},{350,"D"}}},
        {"D", {{0,"DICTIONARY"},{5,"D"},{330,"C"},{100,"AcDbDictionary"},{281,"1"}}},
        {"EE00", {{0,"DICTIONARY"},{5,"EE00"},{330,"C"},{100,"AcDbDictionary"},{280,flag},{281,cloning},{3,"AuthoredPreset"},{role,"EA02"}}},
        {"EE10", {{0,"DICTIONARY"},{5,"EE10"},{330,"EA02"},{100,"AcDbDictionary"},{280,flag},{281,cloning}}},
        {"EF00", {{0,"DICTIONARY"},{5,"EF00"},{330,"C"},{100,"AcDbDictionary"},{280,flag},{281,cloning},{3,"AuthoredPage"},{role,"EF01"}}}
    };
    std::size_t count = 0u;
    for (const auto& item : expected) {
        const auto found = objects.find(item.first);
        if (found == objects.end() || !exact(found->second, item.second)) return false;
        count += found->second.size();
    }
    const auto preset = objects.find("EA02"), page = objects.find("EF01");
    if (preset == objects.end() || page == objects.end() || preset->second[0].text != renderPresetControls::name(form)
        || page->second[0].text != "PLOTSETTINGS") return false;
    std::vector<AttdefWireTag> body;
    if (!renderPresetOutputControls::selectBody(wire, form, body)
        || !renderPresetOutputControls::bodyMatches(body, renderPresetOutputControls::expectedBody(form, false))) return false;
    const auto one = [](const Wire& record, int code, const char* text, std::int64_t number = 0) {
        unsigned seen = 0u, depth = 0u;
        for (const auto& field : record) {
            if (field.code == 102) {
                if (field.text == "}") { if (!depth) return false;--depth; }
                else if (!field.text.empty() && field.text.front() == '{') ++depth;
                else return false;
            }
            if (field.code != code || (code == 330 && depth)) continue;
            if (++seen != 1u || (text ? field.kind != AttdefWireTag::String || field.text != text
                                      : field.kind != AttdefWireTag::Integer || field.integer != number)) return false;
        }
        return seen == 1u && depth == 0u;
    };
    if (!one(preset->second, 330, "EE00") || !one(preset->second, 360, "EE10")
        || !one(page->second, 330, "EF00") || !one(page->second, 333, "EA02")
        || !one(page->second, 76, nullptr, 5)) return false;
    if (fields) *fields += count + 5u;
    return true;
}

struct EditingWriter : dx_iface {
    unsigned edit = 0u;
    void change() {
        if (edit == 10u) { cData->renderSettings.clear();cData->rawRenderSettingsObjects.clear(); }
        else if (edit == 0u || edit == 7u) cData->renderSettings.front().m_fogEnabled = !cData->renderSettings.front().m_fogEnabled;
        else if (edit == 1u || edit == 5u || edit == 8u) {
            auto& raw = cData->rawRenderSettingsObjects.front();
            for (auto& value : raw.groups) if (value.code() == 290) { value = DRW_Variant(290, value.i_val() ? 0 : 1); break; }
            raw.hasRawValues = false; raw.rawValues.clear();
        } else if (edit == 4u) {
            for (auto& dictionary : cData->dictionaries) if (dictionary.handle == 0xEE00u) {
                auto& role = dictionary.m_entries.front().m_dxfHandleCode;
                using R = DRW_Dictionary::Entry::DxfHandleCode;
                role = role == R::SoftOwner ? R::HardOwner : R::SoftOwner;
            }
        } else {
            const auto handle = edit == 3u ? 0xEE10u : 0xEE00u;
            for (auto& dictionary : cData->dictionaries) if (dictionary.handle == handle) { dictionary.cloning = 2; break; }
        }
    }
    void writeHeader(DRW_Header& header) override {
        dx_iface::writeHeader(header);
        if (edit < 5u) change();
    }
    void writeObjects() override {
        if (edit == 5u || edit == 6u) {
            const auto raw = cData->rawRenderSettingsObjects.front();
            const auto dictionaries = cData->dictionaries;
            change(); dx_iface::writeObjects();
            cData->rawRenderSettingsObjects.front() = raw; cData->dictionaries = dictionaries;
        } else {
            dx_iface::writeObjects();
            if (edit >= 7u && edit < 11u) change();
        }
    }
};

struct BudgetWriter : dx_iface {
    std::size_t bytes = 0u;
    void writeObjects() override {
        bytes = DxIfaceTableSourceTestAccess::presetDictionaryBytes(*this);
        dx_iface::writeObjects();
    }
};
struct ScopedEditingWriter : dx_iface {
    int code = 0;
    void writeObjects() override {
        if (!code) { dx_iface::writeObjects();return; }
        auto& raw = cData->rawRenderSettingsObjects.front();
        const auto original = raw;
        bool scoped = false;
        for (auto& value : raw.groups) {
            if (value.code() == 102 && value.c_str()) scoped = std::string_view(value.c_str()) == "{AUTHORED_GRAPH_SCOPE";
            else if (scoped && value.code() == code) { value = DRW_Variant(code,UTF8STRING("41"));break; }
        }
        raw.hasRawValues=false;raw.rawValues.clear();
        dx_iface::writeObjects();
        raw=original;
    }
};
struct DirectWriter : renderPresetControls::TypedWriter {
    dx_data& original;
    DirectWriter(dxfRW& codec,dx_data& data) : renderPresetControls::TypedWriter(codec,data.renderSettings.front()),original(data) {}
    void writeObjects() override {
        for (auto& dictionary : original.dictionaries) if (!output.writeDictionary(&dictionary)) return;
        if (!output.writeRawDxfObject(&original.rawRenderSettingsObjects.front())) return;
        if (!output.writePlotSettings(&original.plotSettings.front())) return;
        wrote = true;
    }
};

bool tests(const std::filesystem::path& dir) {
    const auto input = dir / "graph-input.dxf", output = dir / "graph-output.dxf";
    std::size_t profiles = 0u, fields = 0u, mutations = 0u, refused = 0u, retries = 0u, quotas = 0u, reached = 0u, unreached = 0u, corrections = 0u, aliases = 0u, scopedRefusals = 0u;
    const auto fail = [&](int line) { std::cerr << "Preset graph check line=" << line << " profiles=" << profiles << " refused=" << refused << '\n'; return false; };
    for (unsigned form = 0u; form < 10u; ++form) for (unsigned encoding : {0u,2u}) for (bool hard : {false,true}) {
        dx_iface reader; dx_data data;
        if (!writeBlockBinding(input, document(form, hard), encoding) || !reader.fileImport(input.string(), &data, false)
            || !reader.fileExport(output.string(), DRW::AC1027, encoding == 2u, &data, false)) return fail(__LINE__);
        const auto wire = attdefWireTags(output, encoding == 2u);
        if (!graph(wire, form, hard, &fields)) return fail(__LINE__);
        for (const auto code : {5,330,280,281,350,360,333,76}) {
            for (std::size_t i = 0u; i < wire.size(); ++i) {
                if (wire[i].code != code) continue;
                // Only selected OBJECTS fields: leave HEADER/TABLES/entities alone.
                bool objects = false;
                for (std::size_t j = 0u; j < i; ++j) {
                    if (wire[j].code == 0 && wire[j].text == "SECTION") objects = j + 1u < wire.size() && wire[j + 1u].text == "OBJECTS";
                    if (wire[j].code == 0 && wire[j].text == "ENDSEC") objects = false;
                }
                if (!objects) continue;
                auto changed = wire;
                if (changed[i].kind == AttdefWireTag::Integer) ++changed[i].integer;
                else changed[i].text = "BAD";
                if (graph(changed, form, hard)) return fail(__LINE__);
                ++mutations;
            }
        }
        ++profiles;
        if (form != 4u && form != 8u) continue;
        for (unsigned edit = 0u; edit < 11u; ++edit) {
            EditingWriter writer; dx_data current; writer.edit = edit;
            if (!writer.fileImport(input.string(), &current, false)) return fail(__LINE__);
            { std::ofstream saved(output, std::ios::binary); saved << "KEEP_GRAPH_DESTINATION"; }
            if (writer.fileExport(output.string(), DRW::AC1027, encoding == 2u, &current, false)) {
                std::cerr << "Callback edit published: " << edit << '\n'; return fail(__LINE__);
            }
            std::ifstream saved(output, std::ios::binary);
            const std::string kept((std::istreambuf_iterator<char>(saved)), {}); saved.close();
            if (kept != "KEEP_GRAPH_DESTINATION") return fail(__LINE__);
            ++refused;
            dx_data corrected; writer.edit = 11u;
            if (!writer.fileImport(input.string(), &corrected, false)
                || !writer.fileExport(output.string(), DRW::AC1027, encoding == 2u, &corrected, false)
                || !graph(attdefWireTags(output, encoding == 2u), form, hard)) return fail(__LINE__);
            ++retries;
        }
    }
    for (unsigned encoding : {0u,2u}) {
        auto tags = document(8u,false);
        const auto at = slot(tags,5,"EA02") + 1u;
        tags.insert(tags.begin() + static_cast<std::ptrdiff_t>(at), {{102,"{AUTHORED_GRAPH_SCOPE"},{320,"40"},{330,"40"},{5,"40"},{102,"}"}});
        dx_iface aliasWriter;dx_data aliasData;
        if (!writeBlockBinding(input,tags,encoding) || !aliasWriter.fileImport(input.string(),&aliasData,false)
            || dx_iface::renderingSourceReport(aliasData).namedTargets != 1u
            || !aliasWriter.fileExport(output.string(),DRW::AC1027,encoding==2u,&aliasData,false)) return fail(__LINE__);
        const auto verifyScoped = [&]() {
            const auto wire = attdefWireTags(output,encoding==2u);
            if (!graph(wire,8u,false)) return false;
            const auto app = std::find_if(wire.begin(),wire.end(),[](const auto& field){return field.code == 102 && field.text == "{AUTHORED_GRAPH_SCOPE";});
            return app != wire.end() && std::distance(app,wire.end()) >= 5
                && (app+1)->code == 320 && (app+1)->text == "40"
                && (app+2)->code == 330 && (app+2)->text == "C"
                && (app+3)->code == 5 && (app+3)->text == "40" && (app+4)->code == 102 && (app+4)->text == "}";
        };
        if (!verifyScoped()) return fail(__LINE__);
        ++aliases;
        const auto atomic = [&](const auto& write) {
            { std::ofstream saved(output,std::ios::binary);saved << "KEEP_GRAPH_DESTINATION"; }
            if (write()) return false;
            std::ifstream saved(output,std::ios::binary);
            return std::string((std::istreambuf_iterator<char>(saved)),{}) == "KEEP_GRAPH_DESTINATION";
        };
        ScopedEditingWriter scoped;
        for (int code : {320,330,5}) {
            scoped.code=code;
            if (!atomic([&](){return scoped.fileExport(output.string(),DRW::AC1027,encoding==2u,&aliasData,false);})) return fail(__LINE__);
            ++scopedRefusals;scoped.code=0;
            if (!scoped.fileExport(output.string(),DRW::AC1027,encoding==2u,&aliasData,false) || !verifyScoped()) return fail(__LINE__);
            ++retries;
        }
        BudgetWriter writer;dx_data data;
        if (!writeBlockBinding(input,document(8u,false),encoding) || !writer.fileImport(input.string(),&data,false)
            || !writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false) || !writer.bytes) return fail(__LINE__);
        const auto bytes = writer.bytes;
        DxIfaceTableSourceTestAccess::presetDictionaryLimit(writer,bytes);
        if (!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)) return fail(__LINE__);
        DxIfaceTableSourceTestAccess::presetDictionaryLimit(writer,bytes-1u);
        if (!atomic([&](){return writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false);})) return fail(__LINE__);
        ++quotas;
        DxIfaceTableSourceTestAccess::presetDictionaryLimit(writer,dx_iface::ImageDefinitionCaptureMaxRetainedBytes);
        if (!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)
            || !graph(attdefWireTags(output,encoding==2u),8u,false)) return fail(__LINE__);
        ++retries;

        dxfRW codec(output.string().c_str());DirectWriter direct(codec,data);
        codec.setHandleRemap({{0x40u,0xCu}});
        codec.setRootDictEntries({{"ACAD_RENDER_SETTINGS","EE00"},{"ACAD_PLOTSETTINGS","EF00"}});
        if (!DxIfaceTableSourceTestAccess::presetDirect(direct,data,codec)) return fail(__LINE__);
        long fault = -1;
        if (!codec.setDxfObjectsWriteInspector([&](const auto& view) {
            SymbolAllocationFailure injection(fault);
            return DxIfaceTableSourceTestAccess::presetOutput(direct,view);
        })) return fail(__LINE__);
        unsigned untouched = 0u;
        for (long offset=0;offset<512;++offset) {
            const auto before = symbolFailedAllocations;fault=offset;
            bool wrote = false;
            const bool rollback = atomic([&](){wrote=codec.write(&direct,DRW::AC1027,encoding==2u);return wrote;});
            if (symbolFailedAllocations != before) {
                if (wrote || !rollback) return fail(__LINE__);
                ++reached;untouched=0u;
            } else { if (!wrote || rollback) return fail(__LINE__);++unreached;++untouched; }
            fault=-1;
            if (!codec.write(&direct,DRW::AC1027,encoding==2u)) return fail(__LINE__);
            std::vector<AttdefWireTag> body;
            if (!renderPresetOutputControls::selectBody(attdefWireTags(output,encoding==2u),8u,body)
                || !renderPresetOutputControls::bodyMatches(body,renderPresetOutputControls::expectedBody(8u,false))) return fail(__LINE__);
            ++corrections;
            if (untouched==2u) break;
            if (offset==511) return fail(__LINE__);
        }
    }
    std::cout << "Render preset emitted graph profiles=" << profiles << "; dictionary/reference fields=" << fields
              << "; observed field refusals=" << mutations << "; callback edit rollbacks=" << refused << "; exact/one-under dictionary limits=" << quotas
              << "; scoped handle profiles=" << aliases << "; scoped handle rollbacks=" << scopedRefusals << "; reached inspector faults=" << reached << "; unreached offsets=" << unreached << "; corrected inspections=" << corrections << "; corrected retries=" << retries << '\n';
    return profiles == 40u && fields == 1760u && mutations == 1240u && refused == 88u && quotas == 2u && aliases == 2u && scopedRefusals == 6u && reached && unreached == 4u && corrections == reached + unreached && retries == 96u;
}
}

// Actual post-preflight consumer edits, with locally authored drawing recipes.
namespace shadeConsumerOutputControls {
Tags document(unsigned form=4u) {
    auto tags=renderPresetControls::exportDocument(form);
    const auto at=slot(tags,2,"OBJECTS")-1u;
    const Tags graph{{0,"SECTION"},{2,"TABLES"},{0,"TABLE"},{2,"BLOCK_RECORD"},{5,"B9"},{70,"2"},
        {0,"BLOCK_RECORD"},{5,"B0"},{100,"AcDbSymbolTableRecord"},{100,"AcDbBlockTableRecord"},{2,"*Model_Space"},{70,"0"},{340,"A0"},
        {0,"BLOCK_RECORD"},{5,"B1"},{100,"AcDbSymbolTableRecord"},{100,"AcDbBlockTableRecord"},{2,"*Paper_Space"},{70,"0"},{340,"A1"},
        {0,"ENDTAB"},{0,"ENDSEC"},{0,"SECTION"},{2,"BLOCKS"},
        {0,"BLOCK"},{5,"F0"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},{2,"*Model_Space"},{70,"0"},{10,"0"},{20,"0"},{30,"0"},{3,"*Model_Space"},
        {0,"ENDBLK"},{5,"F1"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"},
        {0,"BLOCK"},{5,"F2"},{330,"B1"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},{2,"*Paper_Space"},{70,"0"},{10,"0"},{20,"0"},{30,"0"},{3,"*Paper_Space"},
        {0,"ENDBLK"},{5,"F3"},{330,"B1"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"},
        {0,"ENDSEC"},{0,"SECTION"},{2,"ENTITIES"},
        {0,"VIEWPORT"},{5,"E801"},{330,"B1"},{100,"AcDbEntity"},{67,"1"},{410,"Sheet 1"},{8,"0"},{100,"AcDbViewport"},
        {10,"1"},{20,"2"},{40,"20"},{41,"10"},{68,"1"},{69,"2"},{170,"5"},{333,"EA02"},{0,"ENDSEC"}};
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),graph.begin(),graph.end());
    appendBlockFields(tags,"40",{{3,"ACAD_LAYOUT"},{350,"D0"}});
    Tags layouts{{0,"DICTIONARY"},{5,"D0"},{330,"40"},{100,"AcDbDictionary"},{280,"1"},{281,"1"},{3,"Model"},{350,"A0"},{3,"Sheet 1"},{350,"A1"},
        {0,"LAYOUT"},{5,"A0"},{330,"D0"},{100,"AcDbPlotSettings"},{70,"1024"},{76,"0"},{100,"AcDbLayout"},{1,"Model"},{70,"1"},{71,"0"},{76,"0"},{330,"B0"},
        {0,"LAYOUT"},{5,"A1"},{330,"D0"},{100,"AcDbPlotSettings"},{76,"5"},{100,"AcDbLayout"},{1,"Sheet 1"},{70,"1"},{71,"1"},{76,"0"},{330,"B1"},{331,"E801"},{333,"EA02"}};
    const auto end=slot(tags,0,"EOF")-1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(end),layouts.begin(),layouts.end());return tags;
}
using Wire=std::vector<AttdefWireTag>;
void extraViewport(Tags& tags,bool anonymous) {
    const auto table=slot(tags,0,"ENDTAB");
    const Tags record{{0,"BLOCK_RECORD"},{5,"B2"},{100,"AcDbSymbolTableRecord"},{100,"AcDbBlockTableRecord"},{2,"AuthoredBlock"},{70,"0"}};
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(table),record.begin(),record.end());
    auto at=slot(tags,2,"ENTITIES")-2u;
    const Tags block{{0,"BLOCK"},{5,"F4"},{330,"B2"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},{2,"AuthoredBlock"},{70,"0"},{10,"0"},{20,"0"},{30,"0"},{3,"AuthoredBlock"},
        {0,"VIEWPORT"},{5,anonymous?"0":"E802"},{330,"B2"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbViewport"},
        {10,"3"},{20,"4"},{40,"10"},{41,"5"},{68,"1"},{69,"3"},{170,"5"},{333,"EA02"},
        {0,"ENDBLK"},{5,"F5"},{330,"B2"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"}};
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),block.begin(),block.end());
}
DRW_Viewport* viewport(dx_data& data,bool blocks=false) {
    const auto find=[](dx_ifaceBlock* block)->DRW_Viewport* {
        if(block)for(auto* entity:block->ent)if(auto* result=dynamic_cast<DRW_Viewport*>(entity))return result;return nullptr;
    };
    if(!blocks)return find(data.mBlock);
    for(auto* block:data.blocks)if(block&&block->name=="AuthoredBlock")return find(block);
    return nullptr;
}
DRW_Layout& paper(dx_data& data) {for(auto& layout:data.layouts)if(layout.name=="Sheet 1")return layout;throw std::runtime_error("missing paper layout");}
Tags styleDocument() {
    auto tags=document();
    for(auto& tag:tags){if(tag.code==3&&tag.value==renderPresetControls::root(4u))tag.value="ACAD_VISUALSTYLE";
        if(tag.code==3&&tag.value=="AuthoredPreset")tag.value="authored-style";
        if((tag.code==76||tag.code==170)&&tag.value=="5")tag.value="4";}
    auto style=renderingSourceStyle(false);const auto xdata=slot(style,1001,"REGISTERED_APP");style.erase(style.begin()+static_cast<std::ptrdiff_t>(xdata),style.end());
    const auto reactors=slot(style,102,"{ACAD_REACTORS");style.erase(style.begin()+static_cast<std::ptrdiff_t>(reactors),style.begin()+static_cast<std::ptrdiff_t>(reactors+3u));
    for(auto& tag:style){if(tag.value=="EA01")tag.value="EA02";if(tag.value=="EB00")tag.value="EE00";if(tag.value=="EB10")tag.value="EE10";}
    const auto begin=slot(tags,0,"RAPIDRTRENDERSETTINGS");auto end=begin+1u;while(tags[end].code!=0)++end;
    tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(begin),tags.begin()+static_cast<std::ptrdiff_t>(end));tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(begin),style.begin(),style.end());
    viewportBodyAppend(tags,{{348,"EA02"}});return tags;
}
struct EditingWriter:dx_iface {
    unsigned family=0u,phase=0u,edit=99u;long allocation=-1;std::size_t records=0u,bytes=0u;
    void change() {
        if(edit==6u){viewport(*cData)->visualStyleHandle=0xEBADu;return;}
        const auto fields=[&](auto& mode,auto& target,auto& handle,auto& owner) {
            if(edit==0u)target=0xEBADu;
            else if(edit==1u){mode=0;target=0u;}
            else if(edit==2u)handle=0xFE001u;
            else if(edit==3u)owner=0xFE002u;
        };
        if(family==0u){auto& value=cData->plotSettings.front();fields(value.shadePlotMode,value.shadePlotHandle.ref,value.handle,value.parentHandle);if(edit==4u)cData->plotSettings.clear();}
        else if(family==1u){auto& value=paper(*cData);fields(value.shadePlotMode,value.shadePlotHandle.ref,value.handle,value.parentHandle);if(edit==4u)cData->layouts.clear();}
        else {auto* value=viewport(*cData,family==3u);if(!value)throw std::runtime_error("missing viewport");fields(value->shadePlotMode,value->shadePlotHandle,value->handle,value->parentHandle);
            if(edit==4u){auto* block=family==3u?*std::find_if(cData->blocks.begin(),cData->blocks.end(),[](auto* b){return b&&b->name=="AuthoredBlock";}):cData->mBlock;block->ent.remove(value);delete value;}}
    }
    template<class Action>void emission(bool selected,Action action) {
        if(!selected||phase!=1u||(edit>=4u&&edit!=6u)){action();return;}
        if(family==0u){const auto saved=cData->plotSettings.front();change();action();cData->plotSettings.front()=saved;}
        else if(family==1u){const auto saved=paper(*cData);change();action();paper(*cData)=saved;}
        else {auto* value=viewport(*cData,family==3u);const auto saved=*value;change();action();*value=saved;}
    }
    void writeHeader(DRW_Header& header)override {
        dx_iface::writeHeader(header);std::tie(records,bytes)=DxIfaceTableSourceTestAccess::shadeSize(*this);
        if(phase==0u&&(edit<5u||edit==6u))change();
    }
    void writeBlocks()override{emission(family==3u,[&]{dx_iface::writeBlocks();});}
    void writeEntities()override{
        emission(family==2u,[&]{dx_iface::writeEntities();});
        if(edit==5u&&family==2u){auto extra=*viewport(*cData);extra.handle=0u;dxfW->writeViewport(&extra);}
    }
    void writeObjects()override {
        emission(family<2u,[&]{dx_iface::writeObjects();});
        if(phase==2u&&(edit<4u||edit==6u))change();
        if(edit==5u){
            if(family==0u){auto extra=cData->plotSettings.front();extra.handle=0xFE003u;extra.pageSetupName="UnplannedPage";dxfW->writePlotSettings(&extra);}
            else if(family==1u){auto extra=paper(*cData);extra.handle=0xFE004u;extra.name="UnplannedLayout";dxfW->writeLayout(&extra);}
        }
    }
    bool finalizeDxfWrite()override {
        if(phase==3u&&(edit<4u||edit==6u))change();
        if(allocation>=0){SymbolAllocationFailure failure(allocation);if(!DxIfaceTableSourceTestAccess::finalizeShade(*this))return false;}
        return dx_iface::finalizeDxfWrite();
    }
};
bool bindings(const Wire& wire,unsigned form,bool extra,bool edited=false,bool collision=false,std::size_t* fields=nullptr,bool otherPreset=false) {
    struct Row{std::string name,section;Wire fields;};std::vector<Row> rows;std::string section;
    for(std::size_t i=0u;i<wire.size();++i)if(wire[i].code==0){
        if(wire[i].text=="SECTION"){if(i+1u==wire.size())return false;section=wire[i+1u].text;continue;}
        if(wire[i].text=="ENDSEC"){section.clear();continue;}if(section.empty())continue;
        auto end=i+1u;while(end<wire.size()&&wire[end].code!=0)++end;
        rows.push_back({wire[i].text,section,Wire(wire.begin()+static_cast<std::ptrdiff_t>(i+1u),wire.begin()+static_cast<std::ptrdiff_t>(end))});
    }
    const auto scoped=[](const Row& row,const char* wanted,int code)->Wire {
        Wire found;std::string scope;unsigned depth=0u;
        for(const auto& field:row.fields){
            if(field.code==102){if(field.text=="}"){if(!depth)return {};--depth;}else if(!field.text.empty()&&field.text.front()=='{')++depth;else return {};continue;}
            if(depth)continue;if(field.code>=1000)break;if(field.code==100){scope=field.text;continue;}
            if(scope==wanted&&field.code==code)found.push_back(field);
        }return found;
    };
    const auto text=[&](const Row& row,const char* scope,int code)->std::string{const auto values=scoped(row,scope,code);return values.size()==1u&&values.front().kind==AttdefWireTag::String?values.front().text:"";};
    const auto number=[&](const Row& row,const char* scope,int code,int expected){const auto values=scoped(row,scope,code);return values.size()==1u&&values.front().kind==AttdefWireTag::Integer&&values.front().integer==expected;};
    std::string target,paperViewport,blockOwner;unsigned presets=0u,plots=0u,layouts=0u,viewports=0u;std::size_t checked=0u;
    for(const auto& row:rows){
        if(row.name==(form==10u?"VISUALSTYLE":renderPresetControls::name(form))){
            ++presets;if(row.section!="OBJECTS"||text(row,"",330)!="EE00")return false;
            const auto handle=text(row,"",5);if(!otherPreset||handle=="EA03")target=handle;
        }
        if(row.name=="BLOCK_RECORD"&&text(row,"AcDbBlockTableRecord",2)=="AuthoredBlock")blockOwner=text(row,"",5);
        if(row.name=="VIEWPORT"&&row.section=="ENTITIES")paperViewport=text(row,"",5);
    }
    if(presets!=(otherPreset?2u:1u)||target.empty()||(otherPreset&&target!="EA03")||(collision&&target=="1F")||paperViewport.empty())return false;
    for(const auto& row:rows){
        if(row.name!="PLOTSETTINGS"&&row.name!="LAYOUT"&&row.name!="VIEWPORT")continue;
        const bool vp=row.name=="VIEWPORT",layout=row.name=="LAYOUT";
        const auto self=text(row,"",5),owner=text(row,"",330);if(self.empty()||owner.empty())return false;
        const bool model=layout&&text(row,"AcDbLayout",1)=="Model";
        const int mode=model||edited?0:form==10u?4:5;const char* modeScope=vp?"AcDbViewport":"AcDbPlotSettings";const char* targetScope=vp?"AcDbViewport":layout?"AcDbLayout":"AcDbPlotSettings";
        if(!number(row,modeScope,vp?170:76,mode))return false;
        const auto targets=scoped(row,targetScope,333);
        if(mode>3?(targets.size()!=1u||targets.front().kind!=AttdefWireTag::String||targets.front().text!=target):!targets.empty())return false;
        if(vp){const auto styles=scoped(row,"AcDbViewport",348);if(form==10u?(styles.size()!=1u||styles.front().text!=target):!styles.empty())return false;}
        if(vp){if(row.section=="ENTITIES"){if(owner!="1E")return false;}else if(row.section!="BLOCKS"||owner!=blockOwner)return false;++viewports;}
        else if(layout){if(row.section!="OBJECTS"||owner!="D0"||!number(row,"AcDbLayout",76,0))return false;
            if(text(row,"AcDbLayout",330)!=(model?"1F":"1E")||(!model&&text(row,"AcDbLayout",331)!=paperViewport))return false;++layouts;}
        else {if(row.section!="OBJECTS"||owner!="EF00"||text(row,"AcDbPlotSettings",1)!="AuthoredPage"||(collision&&self=="1C"))return false;++plots;}
        checked+=3u+(mode>3?1u:0u)+(vp&&form==10u?1u:0u);
    }
    if(fields)*fields+=checked;return plots==1u&&layouts==2u&&viewports==(extra?2u:1u);
}
bool tests(const std::filesystem::path& dir) {
    const auto input=dir/"shade-input.dxf",output=dir/"shade-output.dxf";
    std::size_t profiles=0u,fields=0u,mutations=0u,edits=0u,rollbacks=0u,retries=0u,limits=0u,reached=0u,unreached=0u,corrections=0u,streaming=0u;
    const auto fail=[&](int line){std::cerr<<"Shade check line="<<line<<" profiles="<<profiles<<" rollbacks="<<rollbacks<<" retries="<<retries<<'\n';return false;};
    const auto atomic=[&](auto action){
        {std::ofstream saved(output,std::ios::binary);saved<<"KEEP_SHADE_DESTINATION";}
        const bool wrote=action();std::ifstream saved(output,std::ios::binary);const std::string actual((std::istreambuf_iterator<char>(saved)),{});saved.close();return !wrote&&actual=="KEEP_SHADE_DESTINATION";
    };
    for(unsigned encoding:{0u,2u}){
        struct InactiveWriter:EditingWriter {
            bool ordinary=false;
            bool finalizeDxfWrite()override{ordinary=DxIfaceTableSourceTestAccess::inactiveShadeOutput(*this);return EditingWriter::finalizeDxfWrite();}
        } writer;
        const Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1027"},{0,"ENDSEC"},{0,"SECTION"},{2,"ENTITIES"},
            {0,"VIEWPORT"},{5,"E801"},{330,"0"},{100,"AcDbEntity"},{67,"1"},{8,"0"},{100,"AcDbViewport"},
            {10,"1"},{20,"2"},{40,"20"},{41,"10"},{68,"1"},{69,"2"},{170,"0"},{0,"ENDSEC"},{0,"EOF"}};
        dx_data data;writer.family=2u;
        if(!writeBlockBinding(input,tags,encoding)||!writer.fileImport(input.string(),&data,false))return fail(__LINE__);
        for(int mode:{0,2}){
            viewport(data)->shadePlotMode=mode;writer.ordinary=false;
            if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)||!writer.ordinary)return fail(__LINE__);
            dx_iface reader;dx_data actual;if(!reader.fileImport(output.string(),&actual,false)||!viewport(actual)||viewport(actual)->shadePlotMode!=mode||viewport(actual)->shadePlotHandle)return fail(__LINE__);
            ++streaming;
        }
        writer.edit=0u;if(!atomic([&]{return writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false);}))return fail(__LINE__);
        ++rollbacks;writer.edit=99u;viewport(data)->shadePlotHandle=0u;
        if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)||!writer.ordinary)return fail(__LINE__);
        ++retries;
    }
    for(unsigned encoding:{0u,2u})for(unsigned form=0u;form<10u;++form){
        dx_iface writer;dx_data data;if(!writeBlockBinding(input,document(form),encoding)||!writer.fileImport(input.string(),&data,false)
            ||!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);
        const auto wire=attdefWireTags(output,encoding==2u);if(!bindings(wire,form,false,false,false,&fields))return fail(__LINE__);
        ++profiles;
        std::string recordName;
        for(std::size_t i=0u;i<wire.size();++i){
            if(wire[i].code==0)recordName=wire[i].text;
            if(recordName!="PLOTSETTINGS"&&recordName!="LAYOUT"&&recordName!="VIEWPORT")continue;
            if(wire[i].code!=333&&wire[i].code!=170&&(wire[i].code!=76||wire[i].integer!=5))continue;
            auto changed=wire;if(changed[i].kind==AttdefWireTag::Integer)++changed[i].integer;else changed[i].text="BAD";
            if(bindings(changed,form,false))return fail(__LINE__);
            ++mutations;
        }
        data.plotSettings.front().shadePlotMode=0;data.plotSettings.front().shadePlotHandle.ref=0u;
        paper(data).shadePlotMode=0;paper(data).shadePlotHandle.ref=0u;viewport(data)->shadePlotMode=0;viewport(data)->shadePlotHandle=0u;
        if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)||!bindings(attdefWireTags(output,encoding==2u),form,false,true))return fail(__LINE__);
        ++edits;
    }
    for(unsigned encoding:{0u,2u})for(unsigned variant=0u;variant<4u;++variant){
        auto tags=document();if(variant<2u)extraViewport(tags,variant==1u);
        else if(variant==2u)for(auto& tag:tags){if(tag.value=="EA02")tag.value="1F";else if(tag.code==5&&tag.value=="EF01")tag.value="1C";else if((tag.code==350||tag.code==360)&&tag.value=="EF01")tag.value="1C";}
        else for(const char* id:{"EF01","A1","E801"}){
            const auto at=slot(tags,5,id)+1u;
            tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{102,"{AUTHORED_SHADE_SCOPE"},{76,"2"},{170,"3"},{333,"EA02"},{100,"AcDbLayout"},{102,"}"}});
        }
        dx_iface writer;dx_data data;if(!writeBlockBinding(input,tags,encoding)||!writer.fileImport(input.string(),&data,false)
            ||!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)||!bindings(attdefWireTags(output,encoding==2u),4u,variant<2u,false,variant==2u,&fields))return fail(__LINE__);
        ++profiles;
    }
    for(unsigned encoding:{0u,2u})for(const auto& version:std::vector<std::pair<const char*,DRW::Version>>{{"AC1021",DRW::AC1021},{"AC1024",DRW::AC1024},{"AC1032",DRW::AC1032}}){
        auto tags=document();tags[slot(tags,1,"AC1027")].value=version.first;dx_iface writer;dx_data data;
        if(!writeBlockBinding(input,tags,encoding)||!writer.fileImport(input.string(),&data,false)
            ||!writer.fileExport(output.string(),version.second,encoding==2u,&data,false)||!bindings(attdefWireTags(output,encoding==2u),4u,false,false,false,&fields))return fail(__LINE__);
        ++profiles;
    }
    for(unsigned encoding:{0u,2u}){
        auto tags=document();appendBlockFields(tags,"EE00",{{3,"OtherPreset"},{350,"EA03"}});
        const auto original=renderPresetControls::exportDocument(4u);auto begin=slot(original,0,renderPresetControls::name(4u));auto end=begin+1u;
        while(end<original.size()&&original[end].code!=0)++end;
        Tags second(original.begin()+static_cast<std::ptrdiff_t>(begin),original.begin()+static_cast<std::ptrdiff_t>(end));
        for(auto& tag:second){if(tag.value=="EA02")tag.value="EA03";if(tag.value=="EE10")tag.value="EE11";if(tag.value=="AuthoredPreset")tag.value="OtherPreset";}
        second.insert(second.end(),{{0,"DICTIONARY"},{5,"EE11"},{330,"EA03"},{100,"AcDbDictionary"},{280,"1"},{281,"1"}});
        const auto at=slot(tags,0,"EOF")-1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),second.begin(),second.end());
        dx_iface writer;dx_data data;if(!writeBlockBinding(input,tags,encoding)||!writer.fileImport(input.string(),&data,false))return fail(__LINE__);
        data.plotSettings.front().shadePlotHandle.ref=0xEA03u;paper(data).shadePlotHandle.ref=0xEA03u;viewport(data)->shadePlotHandle=0xEA03u;
        if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)||!bindings(attdefWireTags(output,encoding==2u),4u,false,false,false,&fields,true))return fail(__LINE__);
        ++profiles;++edits;
    }
    for(unsigned encoding:{0u,2u}){
        dx_iface writer;dx_data data;if(!writeBlockBinding(input,styleDocument(),encoding)||!writer.fileImport(input.string(),&data,false)
            ||!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)||!bindings(attdefWireTags(output,encoding==2u),10u,false,false,false,&fields))return fail(__LINE__);
        ++profiles;
        for(unsigned phase=0u;phase<4u;++phase){EditingWriter editor;editor.family=2u;editor.phase=phase;editor.edit=6u;dx_data current;
            if(!editor.fileImport(input.string(),&current,false)||!atomic([&]{return editor.fileExport(output.string(),DRW::AC1027,encoding==2u,&current,false);}))return fail(__LINE__);
            ++rollbacks;editor.edit=99u;dx_data corrected;if(!editor.fileImport(input.string(),&corrected,false)
                ||!editor.fileExport(output.string(),DRW::AC1027,encoding==2u,&corrected,false)||!bindings(attdefWireTags(output,encoding==2u),10u,false))return fail(__LINE__);
            ++retries;
        }
    }
    for(unsigned encoding:{0u,2u})for(unsigned family=0u;family<4u;++family)for(unsigned phase=0u;phase<4u;++phase)for(unsigned edit=0u;edit<(phase==0u?5u:4u);++edit){
        EditingWriter writer;writer.family=family;writer.phase=phase;writer.edit=edit;dx_data data;auto tags=document();extraViewport(tags,false);
        if(!writeBlockBinding(input,tags,encoding)||!writer.fileImport(input.string(),&data,false)
            ||!atomic([&]{return writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false);}))return fail(__LINE__);
        ++rollbacks;
        writer.edit=99u;dx_data corrected;if(!writer.fileImport(input.string(),&corrected,false)
            ||!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&corrected,false)||!bindings(attdefWireTags(output,encoding==2u),4u,true))return fail(__LINE__);
        ++retries;
    }
    for(unsigned encoding:{0u,2u})for(unsigned family:{0u,1u,2u}){
        EditingWriter writer;writer.family=family;writer.edit=5u;dx_data data;
        if(!writeBlockBinding(input,document(),encoding)||!writer.fileImport(input.string(),&data,false)
            ||!atomic([&]{return writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false);}))return fail(__LINE__);
        ++rollbacks;
        writer.edit=99u;if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)||!bindings(attdefWireTags(output,encoding==2u),4u,false))return fail(__LINE__);
        ++retries;
    }
    for(unsigned encoding:{0u,2u}){
        EditingWriter writer;dx_data data;if(!writeBlockBinding(input,document(),encoding)||!writer.fileImport(input.string(),&data,false)
            ||!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)||!writer.records||!writer.bytes)return fail(__LINE__);
        const auto records=writer.records,bytes=writer.bytes;
        for(bool byteLimit:{false,true}){
            DxIfaceTableSourceTestAccess::shadeLimit(writer,records,bytes);
            if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);
            DxIfaceTableSourceTestAccess::shadeLimit(writer,byteLimit?records:records-1u,byteLimit?bytes-1u:bytes);
            if(!atomic([&]{return writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false);}))return fail(__LINE__);
        ++limits;
            DxIfaceTableSourceTestAccess::shadeLimit(writer,100000u,64u*1024u*1024u);
            if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);
        ++retries;
        }
        unsigned untouched=0u;
        for(long offset=0;offset<256;++offset){const auto before=symbolFailedAllocations;writer.allocation=offset;bool wrote=false;
            const bool preserved=atomic([&]{wrote=writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false);return wrote;});
            if(symbolFailedAllocations!=before){if(wrote||!preserved)return fail(__LINE__);
        ++reached;untouched=0u;}
            else{if(!wrote||preserved)return fail(__LINE__);
        ++unreached;++untouched;}
            writer.allocation=-1;if(!writer.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)||!bindings(attdefWireTags(output,encoding==2u),4u,false))return fail(__LINE__);
        ++corrections;
            if(untouched==2u)break;if(offset==255)return fail(__LINE__);
        }
    }
    std::cout<<"Shade consumer output profiles="<<profiles<<"; independent fields="<<fields<<"; observed mutations="<<mutations<<"; valid preflight edits="<<edits
        <<"; callback/extra-record rollbacks="<<rollbacks<<"; exact/one-under limits="<<limits<<"; reached finalization faults="<<reached<<"; unreached offsets="<<unreached
        <<"; corrected finalizations="<<corrections<<"; corrected retries="<<retries<<"; inactive 2D streaming profiles="<<streaming<<'\n';
    return profiles==38u&&fields==588u&&mutations==120u&&edits==22u&&rollbacks==152u&&limits==4u&&reached&&unreached==4u&&corrections==reached+unreached&&retries==156u&&streaming==4u;
}
}
