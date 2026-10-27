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
