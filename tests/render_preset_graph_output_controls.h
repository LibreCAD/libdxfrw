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
